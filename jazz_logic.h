#ifndef JAZZ_LOGIC_H
#define JAZZ_LOGIC_H

// If we are in the Arduino environment, we use its types and MIDI instance.
// If we are in the test environment, we use the mocks.
#ifdef MOCK_TESTING
#include "tests/mock_arduino.h"
extern MockMIDI MIDI;
extern MockSerial Serial;
#else
#include <Arduino.h>
#include <MIDI.h>
extern midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> MIDI;
#endif

// Constants
extern const int ERROR_THRESHOLD_1;
extern const int ERROR_THRESHOLD_2;
extern const int ERROR_THRESHOLD_3;
extern const int ERROR_THRESHOLD_4;
extern const int ERROR_THRESHOLD_5;
extern const int MAX_NOTES_PER_CHORD;

// Chord Definitions
extern const int iiChord_abs[];
extern const int VChord_abs[];
extern const int IChord_abs[];
extern const int IVChord_abs[];
extern const int viChord_abs[];
extern const int iiiChord_abs[];

// Functions
bool isDissonant(int note, const int* contextNotes, int contextNotesCount);
int predictError(int currentError);
void sendChord(const int* chordDefinition, int chordDefSize, int transpositionOffset, int velocity = 100);
void playChordProgression(int currentErrorValue, int currentBaseNote);
void sendMIDINoteOnWrapper(int note, int velocity = 127);
void sendMIDINoteOffWrapper(int note);
void visualFeedback(int intensity);

#endif
