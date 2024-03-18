// User selects a file from the SD card list on the LCD display and plays
// the music selected.
// Example program to demonstrate the use of the MIDIFile library.
//
// Hardware required:
//  SD card interface - change SD_SELECT for SPI comms.
//  LCD interface - assumed to be 2 rows 16 chars. Change LCD
//    pin definitions for hardware setup. Uses the MD_UISwitch library
//    (found at https://github.com/MajicDesigns/MD_UISwitch) to read and manage
//    the LCD display switches.
//

#include <FS.h>
#include <SPIFFS.h>
#include <MD_MIDIFileSPIFF.h>
#include <MD_UISwitch.h>
#include <LiquidCrystal_I2C.h>

#define DEBUG_ON 0

#if DEBUG_ON

#define DEBUG(s, x) \
  do { \
    Serial.print(F(s)); \
    Serial.print(x); \
  } while (false)
#define DEBUGX(s, x) \
  do { \
    Serial.print(F(s)); \
    Serial.print(F("0x")); \
    Serial.print(x, HEX); \
  } while (false)
#define DEBUGS(s) \
  do { Serial.print(F(s)); } while (false)
#define SERIAL_RATE 57600

#else

#define DEBUG(s, x)
#define DEBUGX(s, x)
#define DEBUGS(s)
#define MIDI_RATE 31250

#endif

// LCD display defines ---------
const uint8_t LCD_ROWS = 4;
const uint8_t LCD_COLS = 20;

// LCD user defined characters
char PAUSE = '\1';
uint8_t cPause[8] = { 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x12, 0x00 };

// Define the key table for the analog keys
MD_UISwitch_Analog::uiAnalogKeys_t kt0[] = {
  { 10, 10, 'L' },    // Left
  { 4085, 10, 'R' },  // Right
};
MD_UISwitch_Analog::uiAnalogKeys_t kt1[] = {
  { 10, 10, 'U' },    // Up
  { 4085, 10, 'D' },  // Down
};
MD_UISwitch_Analog::uiAnalogKeys_t kt2[] = {
  { 5, 5, 'PUSH' },  // Push
};

float tempo_adjust = 0;

// Library objects -------------
LiquidCrystal_I2C LCD(0x27, LCD_COLS, LCD_ROWS);  // I2C address 0x27, column and rows
MD_MIDIFile SMF;
const uint8_t ANALOG_LR_PIN = A0;
const uint8_t ANALOG_UD_PIN = A1;
const uint8_t ANALOG_PUSH_PIN = A2;

MD_UISwitch_Analog LR(ANALOG_LR_PIN, kt0, ARRAY_SIZE(kt0));
MD_UISwitch_Analog UD(ANALOG_UD_PIN, kt1, ARRAY_SIZE(kt1));
MD_UISwitch_Analog PUSH(ANALOG_PUSH_PIN, kt2, ARRAY_SIZE(kt2));


// Playlist handling -----------
const uint8_t FNAME_SIZE = 20;                // file names 8.3 to fit onto LCD display
const char *PLAYLIST_FILE = "/PLAYLIST.TXT";  // file of file names
const char *MIDI_EXT = ".mid";                // MIDI file extension
uint16_t plCount = 0;
char fname[FNAME_SIZE + 1];

// Enumerated types for the FSM(s)
enum lcd_state { LSBegin,
                 LSSelect,
                 LSShowFile };
enum midi_state { MSBegin,
                  MSLoad,
                  MSOpen,
                  MSProcess,
                  MSClose };
enum seq_state { LCDSeq,
                 MIDISeq };

// MIDI callback functions for MIDIFile library ---------------

void midiCallback(midi_event *pev)
// Called by the MIDIFile library when a file event needs to be processed
// thru the midi communications interface.
// This callback is set up in the setup() function.
{
#if !DEBUG_ON
  if ((pev->data[0] >= 0x80) && (pev->data[0] <= 0xe0)) {
    Serial0.write(pev->data[0] | pev->channel);
    Serial0.write(&pev->data[1], pev->size - 1);
  } else
    Serial.write(pev->data, pev->size);
#endif
  DEBUG("\nM T", pev->track);
  DEBUG(":  Ch ", pev->channel + 1);
  DEBUGS(" Data");
  for (uint8_t i = 0; i <= pev->size; i++)
    DEBUGX(" ", pev->data[i]);
}

void sysexCallback(sysex_event *pev)
// Called by the MIDIFile library when a System Exclusive (sysex) file event needs
// to be processed thru the midi communications interface. Most sysex events cannot
// really be processed, so we just ignore it here.
// This callback is set up in the setup() function.
{
  DEBUG("\nS T", pev->track);
  DEBUGS(": Data");
  for (uint8_t i = 0; i < pev->size; i++)
    DEBUGX(" ", pev->data[i]);
}

void midiSilence(void)
// Turn everything off on every channel.
// Some midi files are badly behaved and leave notes hanging, so between songs turn
// off all the notes and sound
{
  midi_event ev;

  // All sound off
  // When All Sound Off is received all oscillators will turn off, and their volume
  // envelopes are set to zero as soon as possible.
  ev.size = 0;
  ev.data[ev.size++] = 0xb0;
  ev.data[ev.size++] = 120;
  ev.data[ev.size++] = 0;

  for (ev.channel = 0; ev.channel < 16; ev.channel++)
    midiCallback(&ev);
}

// LCD Message Helper functions -----------------
void LCDMessage(uint8_t r, uint8_t c, const char *msg, bool clrEol = false)
// Display a message on the LCD screen with optional spaces padding the end
{
  LCD.setCursor(c, r);
  LCD.print(msg);
  if (clrEol) {
    c += strlen(msg);
    while (c++ < LCD_COLS)
      LCD.write(' ');
  }
}

void LCDErrMessage(const char *msg, bool fStop) {
  LCDMessage(1, 0, msg, true);
  DEBUG("\nLCDErr: ", msg);
  while (fStop) { yield(); };  // stop here (busy loop) if told to
  delay(2000);                 // if not stop, pause to show message
}

// Create list of files for menu --------------

uint16_t createPlaylistFile(void)
// create a play list file on the SD card with the names of the files.
// This will then be used in the menu.
{
  SDFILE plFile;       // play list file
  SDFILE mFile;        // MIDI file
  uint16_t count = 0;  // count of files
  char fname[FNAME_SIZE + 1];

  // Open/create the display list and directory files
  // Errors will stop execution...
  plFile = SPIFFS.open(PLAYLIST_FILE, "w+");
  if (!plFile) {
    DEBUGS("\nPL Create fail, err ");
    LCDErrMessage("PL create fail", true);
  }

  File dir = SPIFFS.open("/");  // directory folder
  while (mFile = dir.openNextFile()) {
    for (int i = 0; i < FNAME_SIZE; i++) {
      fname[i] = mFile.name()[i];
    }
    DEBUG("\n", count);
    DEBUG(" ", fname);

    // only include files with MIDI extension
    if (strcasecmp(MIDI_EXT, &fname[strlen(fname) - strlen(MIDI_EXT)]) == 0) {
      DEBUGS(" -> W");
      for (int i = 0; i < FNAME_SIZE; i++) {
        (plFile.print(fname[i]) == 0);
      }
      DEBUGS(" OK");
      count++;
    } else DEBUGS(" -> not MIDI");

    mFile.close();
  }
  // close the control files
  plFile.close();
  dir.close();
  DEBUGS("\nList completed");

  return (count);
}

// FINITE STATE MACHINES -----------------------------

seq_state lcdFSM(seq_state curSS)
// Handle selecting a file name from the list (user input)
{
  static lcd_state s = LSBegin;
  static uint8_t plIndex = 0;
  static SDFILE plFile;  // play list file

  // LCD state machine
  switch (s) {
    case LSBegin:
      LCDMessage(0, 0, "Select song: #", true);
      LCD.setCursor(14, 0);
      LCD.print(plIndex);
      LCD.print(" ");
      plFile = SPIFFS.open(PLAYLIST_FILE, "r");
      if (!plFile)
        LCDErrMessage("PL file no read open", true);
      s = LSShowFile;
      break;

    case LSShowFile:
      plFile.seek(FNAME_SIZE * plIndex, SeekSet);
      plFile.readBytes(fname, FNAME_SIZE);
      LCDMessage(1, 0, fname, true);

      // Draw Arrows to show Direction
      LCD.setCursor(LCD_COLS - 1, 1);
      LCD.print("X");
      if (plIndex == 0) {
        LCD.setCursor(LCD_COLS - 1, 1);
        LCD.print('V');
      } else if (plIndex == plCount - 1) {
        LCD.setCursor(LCD_COLS - 1, 1);
        LCD.print('^');
      }
      LCD.setCursor(14, 0);
      LCD.print(plIndex);
      LCD.print(" ");
      s = LSSelect;
      break;

    case LSSelect:
      if (UD.read() == MD_UISwitch::KEY_PRESS) {
        switch (UD.getKey())
        // Keys are mapped as follows:
        // Up:    use the previous file name (move back one file name)
        // Down:   use the next file name (move forward one file name)
        {
          case 'U':  // Left
            DEBUGS("\n>Previous");
            if (plIndex != 0)
              plIndex--;
            s = LSShowFile;
            break;

          case 'D':  // Right
            DEBUGS("\n>Next");
            if (plIndex != plCount - 1)
              plIndex++;
            s = LSShowFile;
            break;
        }
      } else if (PUSH.read() == MD_UISwitch::KEY_PRESS) {
        switch (PUSH.getKey())
          // Keys are mapped as follows:
          // PUSH:    use the previous file name (move back one file name)

        case 'PUSH':  // Select
          DEBUGS("\n>Play");
        curSS = MIDISeq;  // switch mode to playing MIDI in main loop
        s = LSBegin;      // reset for next time
        break;

        break;
      }
    default:
      s = LSBegin;  // reset for next time
      break;
  }

  return (curSS);
}

seq_state midiFSM(seq_state curSS)
// Handle playing the selected MIDI file
{
  static midi_state s = MSBegin;

  switch (s) {
    case MSBegin:
      // Set up the LCD
      LCDMessage(0, 0, "       \1", true);
      LCDMessage(1, 0, "SLOW  \xdb  FAST", true);  // string of user defined characters
      LCDMessage(3, 0, "        >", true);

      s = MSLoad;
      break;

    case MSLoad:
      // Load the current file in preparation for playing
      {
        int err;

        // Attempt to load the file
        for (int i = FNAME_SIZE - 1; i >= 1; i--) {
          fname[i] = fname[i - 1];
        }
        fname[0] = '/';



        if ((err = SMF.load(fname)) == MD_MIDIFile::E_OK) {
          s = MSProcess;
        } else {
          DEBUG("FNAME OPENED: ", fname);
          char aErr[16];
          sprintf(aErr, "SMF error %03d", err);
          LCDErrMessage(aErr, false);
          s = MSClose;
        }
      }
      break;

    case MSProcess:
      // Play the MIDI file
      if (!SMF.isEOF()) {
        if (SMF.getNextEvent()) {
          char sBuf[10];

          sprintf(sBuf, "bpm%5d", SMF.getTempo() + SMF.getTempoAdjust());
          LCDMessage(0, LCD_COLS - strlen(sBuf), sBuf, true);
          sprintf(sBuf, "In:%5d", SMF.getTimeSignature() >> 8, SMF.getTimeSignature() & 0xf);
          LCDMessage(1, LCD_COLS - strlen(sBuf), sBuf, true);
        };
      } else
        s = MSClose;

      // check the keys
      if (LR.read() == MD_UISwitch::KEY_PRESS) {
        switch (LR.getKey()) {
          case 'L':
            tempo_adjust -= .05;
            SMF.setTempoAdjust(SMF.getTempo() * tempo_adjust);
            break;  // ADjust tempo down 2%
          case 'R':
            tempo_adjust += 0.05;
            SMF.setTempoAdjust(SMF.getTempo() * tempo_adjust);
            break;  // ADjust tempo up 2%
        }
      }
      if (UD.read() == MD_UISwitch::KEY_PRESS) {
        switch (UD.getKey()) {
          case 'U':
            SMF.pause(true);
            midiSilence();
            break;                            // Pause
          case 'D': SMF.pause(false); break;  // Start
        }
      }
      if (PUSH.read() == MD_UISwitch::KEY_PRESS) {
        switch (PUSH.getKey()) {
          case 'PUSH': s = MSClose; break;  // Stop
        }
      }
      break;

    case MSClose:
      // close the file and switch mode to user input
      SMF.close();
      midiSilence();
      tempo_adjust = 0;
      curSS = LCDSeq;
      // fall through to default state

    default:
      s = MSBegin;
      break;
  }

  return (curSS);
}

void setup(void) {
  // initialize MIDI output stream
  Serial0.begin(MIDI_RATE);
  DEBUGS("\n[MIDI PLAY LCD]");

  // initialize LCD keys
  LR.begin();
  LR.enableDoublePress(false);
  LR.enableLongPress(false);
  LR.enableRepeat(false);
  LR.setPressTime(50);
  UD.begin();
  UD.enableDoublePress(false);
  UD.enableLongPress(false);
  UD.enableRepeat(false);
  UD.setPressTime(50);
  PUSH.begin();
  PUSH.enableDoublePress(false);
  PUSH.enableLongPress(false);
  PUSH.enableRepeat(false);
  PUSH.setPressTime(50);


  // initialize LCD display
  LCD.init();
  LCD.backlight();
  LCD.clear();
  LCD.noCursor();
  LCDMessage(0, 0, "   DURANGO STREET   ", false);
  LCDMessage(1, 0, "  PIANOS ~~ONLINE~~ ", false);

  // Load characters to the LCD
  LCD.createChar(PAUSE, cPause);

  // initialize SPIFFS
  if (!SPIFFS.begin())
    LCDErrMessage("SPIFFS init fail!", true);

  plCount = createPlaylistFile();
  if (plCount == 0)
    LCDErrMessage("No files", true);

  // initialize MIDIFile
  SMF.begin(&SPIFFS);
  SMF.setMidiHandler(midiCallback);
  SMF.setSysexHandler(sysexCallback);

  delay(750);  // allow the welcome to be read on the LCD
}

void loop(void)
// only need to look after 2 things - the user interface (LCD_FSM)
// and the MIDI playing (MIDI_FSM). While playing we have a different
// mode from choosing the file, so the FSM will run alternately, depending
// on which state we are currently in.
{
  static seq_state s = LCDSeq;

  switch (s) {
    case LCDSeq: s = lcdFSM(s); break;
    case MIDISeq: s = midiFSM(s); break;
    default: s = LCDSeq;
  }
}
