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


#define DEBUG_ON 1

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
#define SERIAL_RATE 31250

#endif


// Library objects -------------

// Playlist handling -----------
const uint8_t FNAME_SIZE = 16;                // file names 8.3 to fit onto LCD display
const char *PLAYLIST_FILE = "/PLAYLIST.TXT";  // file of file names
const char *MIDI_EXT = ".mid";                // MIDI file extension
uint16_t plCount = 0;
char fname[FNAME_SIZE + 1];

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
  }
  File dir = SPIFFS.open("/");  // directory folder
  while (mFile = dir.openNextFile()) {
    mFile.name();
    count++;
    DEBUG("\n", count);
    DEBUG(" ", mFile.name());

    mFile.close();
  }
  // close the control files
  plFile.close();
  dir.close();

  // close the control files
  DEBUGS("\nList completed");
}


void setup(void) {
  // initialize MIDI output stream
  Serial.begin(SERIAL_RATE);
  delay(1500);
  DEBUGS("\n[MIDI PLAY LCD]");

  // initialize SPIFFS
  if (!SPIFFS.begin())
    DEBUGS("SD init fail!");

  plCount = createPlaylistFile();
  if (plCount == 0)
    DEBUGS("No files");
}

void loop(void) {
  delay(10000);
}