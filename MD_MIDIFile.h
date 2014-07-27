/**
\mainpage Arduino Standard MIDI File (SMF) Player

This library allows Standard MIDI Files (SMF) to be read from an SD card and played through a MIDI interface. 
SMF can be opened and processed, with MIDI and SYSEX events passed to the calling program through callback functions.
This allows the calling application to manage sending to a MIDI synthesizer through serial interface or other 
output device, like a MIDI shield. SMF playing may be controlled through the library using methods to start, pause and restart 
playback.

External Dependencies
---------------------
The MD_MIDIFile library uses the following additional libraries and may need additional hardware components

- **SdFat library** at http://code.google.com/p/sdfatlib/. Used by the library to manage the SD card file system.

- **MIDI interface hardware** like http://www.stephenhobley.com/blog/2011/03/14/the-last-darned-midi-interface-ill-ever-build/.
The interface can be stand alone or built onto an Arduino shield, as required.

Topics
------
- \subpage pageSmfDefinition
- \subpage pageTiming
- \subpage pageHardware
- \subpage pageLibrary

Revision History
----------------
##xx xxxx 2014 - version 2.0##
- Renamed MD_MIDIFile Library for consistency with other MajicDesigns libraries.
- Documentation extensively revised and now in Doxygen format.
- Revised debug output macros.
- Fixed some minor errors in code.
- Added looping() method.

##20 January 2013 - version 1.2##
- Cleaned up examples and added additional comments.
- Removed dependency on the MIDI library. 
- This version has no major changes to the core library.

##6 January 2013 - version 1.1##
- Minor fixes and changes.
- More robust handling of file errors on MIDIFile.load().

##5 January 2013 - version 1.0##
- Initial release as MIDIFile Library.

Copyright
---------
Copyright (C) 2013-14 Marco Colli
All rights reserved.

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

\page pageSmfDefinition SMF Format Definition
MIDI
----
MIDI (Musical Instrument Digital Interface) is a technical standard that describes a protocol, digital interface
and connectors and allows a wide variety of electronic musical instruments, computers and other related devices
to connect and communicate with one another. A single MIDI link can carry up to sixteen channels of information,
each of which can be routed to a separate device.

MIDI carries event messages that specify notation, pitch and velocity, control signals for parameters such as
volume, vibrato, audio panning, cues, and clock signals that set and synchronize tempo between multiple devices.
These messages are sent to other devices where they control sound generation and other features. This data can
also be recorded into a hardware or software device called a sequencer, which can be used to edit the data and
to play it back at a later time. These recordings are usually in Standard MIDI File (SMF) format.

Advantages of MIDI include compactness (an entire song can be coded in a few kilobytes), ease of modification
and manipulation and choice of instruments.

Standard MIDI File Format
-------------------------
The Standard MIDI File (SMF) is a file format that provides a standardized way for
sequences to be saved, transported, and opened in other systems. The compact size
of these files has led to their widespread use in computers, mobile phone ring tones,
web page authoring and greeting cards. They are intended for universal use, and include
such information as note values, timing and track names. Lyrics may be included as
metadata, and can be displayed by karaoke machines. The SMF specification was developed
and is maintained by the MIDI Manufacturer's Association (MMA).

SMFs are created as an export format of software sequencers or hardware workstations.
They organize MIDI messages into one or more parallel tracks, and timestamp the events
so that they can be played back in sequence. A SMF file is arranged into "chunks". It
starts with a header chunk and is followed by one or more track chunks.

The header chunk contains the arrangement's setup data, which may include such things
as tempo and instrumentation, and information such as the song's composer. The header
also specifies which of three SMF formats applies to the file
- A __type 0__ file contains the entire performance, merged onto a single track
- A __type 1__ files may contain any number of tracks, running synchronously.
- A __type 2__ files may contain any number of tracks, running asynchronously. This type
is rarely used and not supported by this library.

Each track chunk defines a logical track and contains events to be
processed at specific time intervals. Events can be one of three types:

- __MIDI Events (status bytes 0x8n - 0xE0n)__

These correspond to the standard Channel MIDI messages. In this case '_n_' is the MIDI channel
(0 - 15). This status byte will be followed by 1 or 2 data bytes, as is usual for the
particular MIDI message. Any valid Channel MIDI message can be included in a MIDI file.

If the first (status) byte is less than 128 (hex 80), this implies that running status is in
effect, and that this byte is actually the first data byte (the status carrying over from the
previous MIDI event). This can only be the case if the immediately previous event was also a
MIDI event, ie SysEx and Meta events interrupt (clear) running status.

MIDI events may be processed by the calling program through a callback.

- __SYSEX events (status bytes 0xF0 and 0xF7)__

There are a couple of ways in which system exclusive messages can be encoded - as a single
message (using the 0xF0 status), or split into packets (using the 0xF7 status). The 0xF7
status is also used for sending escape sequences.

SYSEX events may be processed by the calling program through a callback.

- __META events (status byte 0xFF)__

These contain additional information which would not be in the MIDI data stream itself.
TimeSig, KeySig, Tempo, TrackName, Text, Marker, Special, EOT (End of Track) are the most
common such events being some of the most common.

Relevant META events are processed by the library code, but thid id only a subset of all
the available events.

Note that the status bytes associated with System Common messages (0xF1 to 0xF6 inclusive)
and System Real Time messages (0xF8 to 0xFE inclusive) are not valid within a MIDI file.
For the rare occasion when you do need to include one of these messages, it should be embedded
within a SysEx escape sequence.

SMF Format Grammar
------------------

This Grammar is from http://www.ccarh.org/courses/253/handout/smf/ and is useful to understand
the structure of the file in a programmer-friendly format.

     <descriptor:length> means 'length' bytes, MSB first
     <descriptor:v> means variable length argument (special format)

     SMF := <header_chunk> + <track_chunk> [ + <track_chunk> ... ]
     header chunk := "MThd" + <header_length:4> + <format:2> + <num_tracks:2> + <time_division:2>
     track_chunk := "MTrk" + <length:4> + <track_event> [ + <track_event> ... ]
     track_event := <time:v> + [ <midi_event> | <meta_event> | <sysex_event> ]
     midi_event := any MIDI channel message, including running status
     meta_event := 0xFF + <meta_type:1> + <length:v> + <event_data_bytes>
     sysex_event := 0xF0 + <len:1> + <data_bytes> + 0xF7
     sysex_event := 0xF7 + <len:1> + <data_bytes> + 0xF7

___

\page pageTiming MIDI Beat Time Considerations

Timing in music is very important. So it stands to reason that MIDI files include a number of 
parameters related to keeping time, and the MIDI standard also includes time synchronization
messages to ensure that all the instruments keep to the same musical beat.

What is a Beat?
---------------
The fundamental time unit of music is the __beat__. Beats can be slower or faster depending
on the kind of music, and the __tempo__ (speed of the beats) can change even in a single
piece. Tempo in standard music notation are typically given in __beats per minute (BPM)__.

In music a __bar__ is a segment of time defined by a given number of beats of a given duration.
The values which define a bar, are called the __Time Signature__.

Notes come in different power-of-two lengths. A MIDI quarter note normally is one beat long. 
A half note is two beats, and a whole note is four beats (as it takes up a whole measure, 
if you're in 4). An eighth note is half a quarter note, so there are two eighth notes per 
beat, a sixteenth note is half an eighth so there are 4 sixteenths per beat, and so on.

A __Time Signature__ is two numbers, one on top of the other. The numerator describes the number 
of beats in a Bar, while the denominator describes of what note value a beat is. So 

- 4/4 would be four quarter-notes per bar,
- 4/2 would be four half-notes per bar (or 8 quarter notes),
- 4/8 would be four eighth-notes per bar (or 2 quarter notes), and 
- 2/4 would be two quarter-notes per Bar.

The default MIDI tempo is 120 BPM, and the default Time Signature is 4/4. 

However the _Set Tempo_ meta event can change these defaults. As MIDI only deals in quarter notes, 
the _Set Tempo_ meta event also only deals in quarter notes but also gives the time signature. If 
the time signature is 4/8, a quarter-note is not a beat since its described as an eighth-note, so
using it to calculate beats per minute on its own is incorrect.

MIDI Beat Time
--------------
Musical timing is defined in fractions of a musical beat, so it makes sense to create a timebase 
that measures time as fractions of a beat. A quarter note is always one fourth of a whole note - 
regardless of the tempo. Similarly, a sixteenth note is always the same fraction of a beat. The 
rate at which the notes occur can change as the tempo changes, but the relative durations are 
always the same. So ideal timebase divides a musical beat into many small bits that occur at a 
rate determined by the current tempo. Each of these tiny fractions of a beat is called a __tick__,
and the number of ticks per beat is independent of the tempo.

The SMF header chunk contains a 16-bit value that gives the number of ticks per quarter note. If 
it is not specified the MIDI default is 48 ticks per quarter note. This value is a constant over the 
whole file. Within the MIDI data stream are tempo meta-events, which contain a 24-bit value that 
give the number of microseconds per quarter note. Divide this one by the first one, and you get 
the number of microseconds per tick.

SMF Time Specification
----------------------
Events in a SMF are defined in terms of __Delta Time__.  Delta Time determines when an event should 
be played relative to the track's last event, in ticks. A delta time of 0 ticks means that it should 
play simultaneously with the last event. A track's first event delta time defines the amount of time
(number of ticks) to wait before playing this first event. Events unaffected by time are still 
preceded by a delta time, but should always use a value of 0 and come first in the stream of track
events.

Time Calculations
-----------------
Delta times are stored as ticks, so what we need to know now is how many ticks make up a quarter-note.
This is given in the header of the SMF as _Ticks Per Quarter Note_.

The number of microseconds per quarter note is given in the _Set tempo_ meta event and is by default 
500,000 if not specified. So

     microseconds per tick = microseconds per quarter note / ticks per quarter note
     
Delta times are cumulative, and the next event�s delta time needs to be added onto this one after it has 
been calculated. If the MIDI time division is 60 ticks per beat and if the microseconds per beat
is 500,000, then 1 tick = 500,000 / 60 = 8333.33 microseconds. The fractional number of microseconds must 
be properly accounted for or the MIDI playback will drift away from the correctly synchronisaed time.

____

\page pageLibrary Notes on the Library

Conditional Compilation Switches
--------------------------------
The library allows the run time code to be tailored through the use of compilation
switches. The compile options are documented in the section related to the main header
file MD_MIDIFile.h.

_NOTE_: Compile switches must be edited in the library header file. Arduino header file
'mashing' during compilation makes the setting of these switches from user code
completely unreliable.

\page pageHardware Hardware Interface

The MIDI communications hardware is a opt-isolated byte-based serial interface configured 
at 31,250 baud (bps). The standard connector is a 5 pin DIN, however other types of 
connections (like MIDI over USB) have become increasingly common as other interfaces 
that had been used for MIDI connections (serial, joystick, etc.) disappeared as standard 
features of personal computers.

Standard MIDI IN, OUT and THRU Ports
------------------------------------
Standard MIDI cables terminate in a 180 degree five-pin DIN connector. Standard applications 
use only three of the five conductors: a ground wire, and a balanced pair of conductors 
that carry a +5 volt signal. This connector configuration can only carry messages in one 
direction, so a second cable is necessary for two-way communication. These are usually 
labeled IN and OUT designating the messages going into and coming out of the device, 
respectively.

Most devices do not copy messages from their IN to their OUT port. A third type of 
port, the THRU port, emits a copy of everything received at the input port, allowing 
data to be forwarded to another instrument in a daisy-chain arrangement. Not all devices 
contain THRU ports, and devices that lack the ability to generate MIDI data, such as 
effects units and sound modules, may not include OUT ports.

Opto-isolators keep MIDI devices electrically separated from their connectors, which 
prevents the occurrence of ground loops and protects equipment from voltage spikes. There 
is no error detection capability in MIDI, so the maximum cable length is set at 15 meters 
(50 feet) in order to limit interference.

A circuit using the Arduino Serial interface is shown below. This can be built as a stand-alone 
interface or incorporated into an Arduino shield. This is based on the device described at 
http://www.stephenhobley.com/blog/2011/03/14/the-last-darned-midi-interface-ill-ever-build/.

![MIDI Interface Circuit] (MIDI_Interface.jpg "MIDI Interface Circuit")

*/

#ifndef _MDMIDIFILE_H
#define _MDMIDIFILE_H

#include <Arduino.h>
#include <SdFat.h>

/**
 * \file
 * \brief Main header file for the MD_MIDIFile library
 */

// ------------- Configuration Section - START

/**
 \def DUMP_DATA
 Set to 1 to to dump the file data instead of processing callback.
 Neither of DUMP_DATA or SHOW_UNUSED_META should be 1 when MIDI messages are being 
 transmitted as the Serial.print() functions are called to print information.
 */
#define	DUMP_DATA			    0

/**
 \def SHOW_UNUSED_META
 Set to 1 to display unused META messages. DUMP_DATA must also be enabled.
 Neither of DUMP_DATA or SHOW_UNUSED_META should be 1 when MIDI messages are being 
 transmitted as the Serial.print() functions are called to print information.
 */
#define	SHOW_UNUSED_META	0

/**
 \def MIDI_MAX_TRACKS
 Max number of MIDI tracks. This may be reduced or increased depending on memory requirements.
 16 tracks is the maximum available to any MIDI device. Fewer tracks may not allow many MIDI
 files to be played, while a minority of SMF may require more tracks.
 */
#define MIDI_MAX_TRACKS 16

/**
 \def TRACK_PRIORITY
 Events may be processed in 2 different ways. One way is to give priority to all 
 events on one track before moving on to the next track (TRACK_PRIORITY) or to process 
 one event from each track and cycling through all tracks round robin fashion until 
 no events are left to be processed (EVENT_PRIORITY). This macro definition enables
 the mode of operation implemented in getNextEvent().
 */
#define	TRACK_PRIORITY	1

// ------------- Configuration Section - END

#if DUMP_DATA
#define	DUMPS(s)    Serial.print(F(s))                            ///< Print a string
#define	DUMP(s, v)	{ Serial.print(F(s)); Serial.print(v); }      ///< Print a value (decimal)
#define	DUMPX(s, x)	{ Serial.print(F(s)); Serial.print(x,HEX); }  ///< Print a value (hex)
#else
#define	DUMPS(s)      ///< Print a string
#define	DUMP(s, v)    ///< Print a value (decimal)
#define	DUMPX(s, x)   ///< Print a value (hex)
#endif // DUMP_DATA

/**
 * MIDI event definition structure
 *
 * Structure defining a MIDI event and its related data.
 * A pointer to this structure type is passed the the related callback function.
 */
typedef struct
{
	uint8_t	track;		///< the track this was on
	uint8_t	channel;	///< the midi channel
	uint8_t	size;		  ///< the number of data bytes
	uint8_t	data[4];	///< the data. Only 'size' bytes are valid
} midi_event;

/**
 * SYSEX event definition structure
 *
 * Structure defining a SYSEX event and its related data.
 * A pointer to this structure type is passed the the related callback function.
 */
typedef struct
{
	uint8_t	track;		///< the track this was on
	uint8_t	size;		  ///< the number of data bytes
	uint8_t	data[50];	///< the data. Only 'size' bytes are valid
} sysex_event;


class MD_MIDIFile;

/**
 * Object definition for a SMF MIDI track. 
 * This object is not invoked by user code.
 */
class MD_MFTrack 
{
public:
  /** 
   * Class Constructor
   *
   * Instantiate a new instance of the class.
   *
   * \return No return data.
   */
  MD_MFTrack(void);
  
  /** 
   * Class Destructor
   *
   * Released allocated memory and does the necessary to clean up once the object is
   * no longer required.
   *
   * \return No return data.
   */
  ~MD_MFTrack(void);

  //--------------------------------------------------------------
  /** \name Methods for track file data
   * @{
   */
  /** 
   * Get end of track status
   *
   * The method is used to test whether a track has reached the end.
   * Once a track reached the end it stops being processed for MIDI events.
   * 
   * \return true if end of track has been reached.
   */
  bool getEndOfTrack(void);
 
  /** 
   * Get the length of the track in bytes
   *
   * The length of a track is specified in the track header. This method returns 
   * the total length of the track in bytes.
   * 
   * \return the total track length (bytes).
   */
	uint32_t  getLength(void);
  /** @} */

  //--------------------------------------------------------------
  /** \name Methods for data handling
   * @{
   */
  /** 
   * Close the track
   *
   * Stop processing the track and reset it to start conditions.
   *
   * \return No return data.
   */
	void close(void);
  
  /** 
   * Get and process the next sequential MIDI or SYSEX event
   *
   * Each track is made up of a sequence of MIDI and SYSEX events that are processed 
   * in sequential order using this method. An event will not be processed if the track
   * is at end of track or insufficient time has elapsed for the next event to be 
   * triggered. Once processed the track position is advanced to the next event to be 
   * processed.
   * 
   * \param mf          pointer to the MIDI file object calling this track.
   * \param elapsedTime the time in milliseconds since this method was last called for this track.
   * \return true if an event was found and processed.
   */
	bool getNextEvent(MD_MIDIFile *mf, uint32_t elapsedTime);
  
  /** 
   * Load the definition of a track
   *
   * Before it can be processed, each track must be initialise to its start conditions by 
   * invoking this method.
   * 
   * \param trackId the identifying number for the track [0..MIDI_MAX_TRACKS-1].
   * \param mf      pointer to the MIDI file object calling this track.
   * \return Error code with one of these values 
   * - -1 if successful 
   * - 0 if the track header is not in the correct format 
   * - 1 if the track chunk is past the end of file
   */
	int load(uint8_t trackId, MD_MIDIFile *mf);
  
  /** 
   * Reset the track to the start of the data in the file
   *
   * Sets up the track to start playing from the beginning again.
   *
   * \return No return data.
   */
	void restart(void);
  
  /** 
   * Reset the start time for this track
   *
   * This is used to resynchronize a track when it has been paused.
   *
   * \return No return data.
   */
	void syncTime(void);
  /** @} */

  //--------------------------------------------------------------
  /** \name Methods for debugging
   * @{
   */
  /** 
   * Dump the track header data to the debug stream
   *
   * During debugging, this method provides a formatted dump of data to the debug
   * output stream.
   *
   * The DUMP_DATA macro define must be set to 1 to enable this method.
   *
   * \return No return data.
   */
	void	dump(void);
  /** @} */

protected:
	void		parseEvent(MD_MIDIFile *mf);	///< process the event from the physical file
	void		reset(void);	      ///< initialize class variables all in one place

	uint8_t		_trackId;		      ///< the id for this track
  uint32_t	_length;          ///< length of track in bytes
  uint32_t	_startOffset;     ///< start of the track in bytes from start of file
	uint32_t	_currOffset;	    ///< offset from start of the track for the next read of SD data
	bool		  _endOfTrack;	    ///< true when we have reached end of track or we have encountered an undefined event
	uint32_t	_elapsedTimeTotal;///< the total time elapsed in microseconds since events were checked
	midi_event  _mev;			      ///< data for MIDI callback function - persists between calls for run-on messages
};

/**
 * Core object for the MD_MIDIFile library
 *
 * This is the class to handle a MIDI file, including all tracks, and is the only one 
 * available to user programs.
 */
class MD_MIDIFile 
{
public:
	friend class MD_MFTrack;

  /** 
   * Class Constructor
   *
   * Instantiate a new instance of the class.
   *
   * \return No return data.
   */
  MD_MIDIFile(void);
  
  /** 
   * Class Destructor
   *
   * Released allocated memory and does the necessary to clean up once the object is
   * no longer required.
   *
   * \return No return data.
   */
  ~MD_MIDIFile(void);
 
  /** 
   * Initialize the object
   *
   * Initialise the object data. This needs to be called during setup() to initialise new 
   * data for the class that cannot be done during the object creation.
   * 
   * The main purpose if to pass in a pointer to the SDFat object will be used for file 
   * operations on the SMF.  A copy of this pointer is retained by the library for the life 
   * of the MD_MIDIFile object. THge SDFat object should be initialized before calling this 
   * method.
   *
   * \param psd Pointer to the SDFat object.
   * \return No return data.
   */
  void begin(SdFat *psd);

  //--------------------------------------------------------------
  /** \name Methods for MIDI time base
   * @{
   */
  /** 
   * Get the internally calculated tick time
   *
   * Changes to tempo, TPQN and time signature all affect the tick time. This returns the
   * number of microseconds for each tick.
   * 
   * \return the tick time in microseconds
   */
  uint32_t getTickTime(void);
  
  /** 
   * Get the overall tempo
   *
   * Retrieve the overall tempo for the MIDI playback. Tempo is set by the SMF and the
   * setTempo() method. 
   * 
   * \return the tempo.
   */
	uint16_t getTempo(void);
  
  /** 
   * Get the number of ticks per quarter note
   *
   * Retrieve the number of ticks per quarter note for the MIDI playback. TPQN is set by 
   * the SMF and the setTicksPerQuarterNote() method. 
   * 
   * \return the number of TPQN.
   */
  uint16_t getTicksPerQuarterNote(void);
  
  /** 
   * Get the Time Signature
   *
   * Retrieve the time signature for the MIDI playback. Time Signature is set by the SMF 
   * and the setTimeSignature() method. 
   * 
   * \return the time signature (numerator in the top byte and the denominator in the lower byte).
   */
	uint16_t getTimeSignature(void);
  
  /** 
   * Set the internal tick time
   *
   * Set the number of microseconds per quarter note directly. Normally this is calculated 
   * from the tempo and ticks per quarter note.
   *
   * \param m the number of microseconds per quarter note.
   * \return No return data.
   */
	void setMicrosecondPerQuarterNote(uint32_t m);

  /** 
   * Set the overall tempo
   *
   * Set the overall tempo for the MIDI playback. Tempo is set by the SMF but can be 
   * changed through this method. 
   * 
   * The default MIDI tempo if not specified is 120 beats/minute.
   *
   * \param t the tempo required.
   * \return No return data.
   */
	void setTempo(uint16_t t);
  
  /** 
   * Set number of ticks per quarter note (TPQN)
   *
   * Set the overall ticks per quarter note tempo for the MIDI playback. TPQN is set by the SMF 
   * but can be changed through this method. 
   *
   * The default if not specified is 48 TPQN.
   *
   * \param ticks the number of ticks per quarter note.
   * \return No return data.
   */
  void setTicksPerQuarterNote(uint16_t ticks);
  
  /** 
   * Set Time Signature for the SMF
   *
   * Set the time signature for the MIDI playback. Time signature is specified in 2 parts defined
   * by the numerator and the denominator of the normal notation.
   * 
   * The default if not specified is 4/4.
   *
   * \param n numerator of the time signature.
   * \param d denominator of the time signature.
   * \return No return data.
   */
	void setTimeSignature(uint8_t n, uint8_t d);
  /** @} */

  //--------------------------------------------------------------
  /** \name Methods for SMF file handling
   * @{
   */
  /** 
   * Close the SMF
   *
   * Before a new SMF is processed the current SMF must be closed. All tracks are stopped and 
   * the file name is reset. A new file can then be started using load().
   *
   * \return No return data.
   */
  void close(void);
  
  /**
   * Check if SMF is and end of file
   *
   * An SMF is at EOF when there is nothing left to play (ie, all tracks have finished).
   * The method will return EOF when this condition is reached.
   *
   * \return true if the SMF is at EOF, false otherwise.
   */
	bool isEOF(void);

  /** 
   * Get the name of the SMF
   *
   * Gets a pointer to the buffer containing the name of the SMF.
   * 
   * \return character pointer to the name string
   */
  const char* getFilename(void);

  /** 
   * Set the name of the SMF
   *
   * Copies the supplied name to the class file name buffer.
   * The name is expected to be in 8.3 format.
   * 
   * \param aname pointer to a string with the file name.
   * \return No return data.
   */
  void setFilename(const char* aname);

  /** 
   * Load the SMF
   *
   * Before it can be processed, a file must be opened, loaded and the MIDI playback 
   * initialized by invoking this method. The file name must be set using the 
   * setfilename() method before calling load().
   * 
   * \return Error code with one of these values
   * - -1 = no errors
   * - 0 = Blank file name
   * - 2 = Can't open file specified
	 * - 3 = File is not MIDI format
   * - 4 = MIDI header size incorrect
   * - 5 = File format type not 0 or 1
   * - 6 = File format 0 but more than 1 track
   * - 7 = More than MIDI_MAX_TRACKS required
   * - n0 = Track n track chunk not found
   * - n1 = Track n chunk size past end of file
   */
  int load(void);
  /** @} */

  //--------------------------------------------------------------
  /** \name Methods for SMF header data
   * @{
   */
  /** 
   * Get the format of the SMF
   *
   *  The SMF header specifies which of three SMF formats applies to the file
   * - Type 0 file contains the entire performance, merged onto a single track
   * - Type 1 files may contain any number of tracks, running synchronously.
   * - Type 2 files may contain any number of tracks, running asynchronously. This type
   * is rarely used and not supported by this library..
   *
   * The load() method must be invoked to read the SMF header information.
   * 
   * \return number [0..2] representing the format.
   */
  uint8_t getFormat(void);
  
  /** 
   * Get the number of tracks in the file
   *
   * The SMF header specifies the number of MIDI tracks in the file. This must be
   * between [0..MIDI_MAX_TRACKS-1] for the SMF to be successfully processed.
   * 
   * The load() method must be invoked to read the SMF header information.
   * 
   * \return the number of tracks in the file
   */
  uint8_t getTrackCount(void);
  /** @} */

  //--------------------------------------------------------------
  /** \name Methods for MIDI playback control
   * @{
   */
  /** 
   * Set the looping mode for SMF playback
   *
   * A SMF can be set to automatically loop to the start once it has completed.
   * For SMF file type 0 (1 track only) the single track is looped. 
   * For file type 1, all tracks except track 0 (the first) is looped. Track 0 contains
   * global setup information that does not need to be repeated and would delay the restart
   * of the looped tracks.
   * 
   * \param bMode Set true to enable mode, false to disable.
   * \return No return data.
   */
  void looping(bool bMode);
  
  /** 
   * Pause or un-pause SMF playback
   *
   * SMF playback can be paused (true) or un-paused (false) using this method. Whilst in pause 
   * mode, calls to methods to play the SMF will not work. 
   * 
   * \param bMode Set true to enable mode, false to disable.
   *
   * \return No return data.
   */
	void pause(bool bMode);
  
  /** 
   * Force the SMF to be restarted
   *
   * SMF playback can be reset back to the beginning of all tracks using this method. The method
   * can be invoked at any time during playback and has immediate effect.
   *
   * \return No return data.
   */
  void restart(void);
  /** @} */

  //--------------------------------------------------------------
  /** \name Methods for data handling
   * @{
   */
  /** 
   * Read and process the next event from the SMF
   *
   * Once a SMF is ready for processing, this method is called as frequently as possible to
   * process the next MIDI, SYSEX or META from the tracks in the SMF. 
   * 
   * Events in the SMF are processed in sequential order in one of 2 different ways:
   * - process all events on one track before moving on to the next track (TRACK_PRIORITY)
   * - process one event from each track and cycling through all tracks round robin fashion 
   * until no events are left to be processed (EVENT_PRIORITY).
   *
   * The TRACK_PRIORITY define selects which is used. In practice there is little or no
   * difference between the two methods.
   *
   * Each MIDI and SYSEX event is passed back to the calling program for processing though the 
   * callback functions set up by setMidiHandler() and setSysexHandler().
   * 
   * \return true if a 'tick' has passed since the last call.
   */
	boolean	  getNextEvent(void);

  /** 
   * Set the MIDI callback function
   *
   * The callback function is called from the library when a MIDI events read from a track 
   * needs to be processed.
   *  
   * The callback function has one parameter of type midi_event. 
   * The pointer passed to the callback will be initialized with the event data for the 
   * callback to process. Once the function returns from the callback the pointer 
   * may no longer be valid (ie, don't rely on it!).
   * 
   * \param mh	the address of the function to be called from the library.
   * \return No return data
   */
	void	    setMidiHandler(void (*mh)(midi_event *pev));		// set the data handling callback for MIDI messages

  /** 
   * Set the SYSEX callback function
   *
   * The callback function is called from the library when a SYSEX events read from a track 
   * needs to be processed.
   *  
   * The callback function has one parameter of type sysex_event.
   * The pointer passed to the callback will be initialized with the event data for the 
   * callback to process. Once the function returns from the callback the pointer
   * may no longer be valid (ie, don't rely on it!).
   * 
   * \param sh	the address of the function to be called from the library.
   * \return No return data
   */
	void	    setSysexHandler(void (*sh)(sysex_event *pev));	// set the data handling callback for SYSEX messages
  /** @} */

  //--------------------------------------------------------------
  /** \name Methods for debugging
   * @{
   */
  /** 
   * Dump the SMF header data to the debug stream
   *
   * During debugging, this method provides a formatted dump of data to the debug
   * output stream.
   *
   * The DUMP_DATA macro define must be set to 1 to enable this method.
   */
	void	dump(void);
  /** @} */

protected:
  void    calcMicrosecondDelta(void);	///< called internally to update the time per tick
	void	  initialise(void);						///< initialize class variables all in one place
	void	  synchTracks(void);					///< synchronize the start of all tracks

	void	  (*_midiHandler)(midi_event *pev);		///< callback into user code to process MIDI stream
	void	  (*_sysexHandler)(sysex_event *pev);	///< callback into user code to process SYSEX stream

	char      _fileName[13];        ///< MIDI file name - should be 8.3 format

  uint8_t   _format;              ///< file format - 0: single track, 1: multiple track, 2: multiple song
  uint8_t   _trackCount;          ///< number of tracks in file
 
	uint16_t  _ticksPerQuarterNote; ///< time base of file
	uint32_t  _microsecondDelta;	  ///< calculated per tick based on other data for MIDI file
	uint32_t  _lastEventCheckTime;	///< the last time an event check was performed
	bool	  _syncAtStart;			      ///< sync up at the start of all tracks
	bool	  _paused;				        ///< if true we are currently paused
  bool    _looping;               ///< if true we are currently looping

 	uint16_t  _tempo;				        ///< tempo for this file in beats per minute

	uint8_t   _timeSignature[2];	  ///< time signature [0] = numerator, [1] = denominator

	// file handling
  uint8_t   _selectSD;          ///< SDFat select line
  SdFat     *_sd;		            ///< SDFat library descriptor supplied by calling program
  SdFile    _fd;                ///< SDFat file descriptor
  MD_MFTrack   _track[MIDI_MAX_TRACKS]; ///< the track data for this file
};

#endif /* _MDMIDIFILE_H */
