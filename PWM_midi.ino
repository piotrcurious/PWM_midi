#include <MIDI.h>

// Define the analog pin for reading the error value
const int analogPin = A0;

// MIDI channel to use
const int midiChannel = 1;

// MIDI note range (typically 21 to 108 for piano)
const int minMIDINote = 21;
const int maxMIDINote = 108;

// Initialize the MIDI library
MIDI_CREATE_DEFAULT_INSTANCE();

void setup() {
  // Start serial communication for debugging purposes
  Serial.begin(115200);
  
  // Initialize MIDI communication
  MIDI.begin(MIDI_CHANNEL_OMNI);
}

void loop() {
  // Read the analog value (0-1023)
  int analogValue = analogRead(analogPin);
  
  // Map the analog value to the logarithmic scale for MIDI notes
  int midiNote = mapAnalogToMIDINoteLog(analogValue);
  
  // Send the MIDI note on command
  MIDI.sendNoteOn(midiNote, 127, midiChannel);
  
  // Wait for a short period to simulate note duration
  delay(100);
  
  // Send the MIDI note off command
  MIDI.sendNoteOff(midiNote, 0, midiChannel);
  
  // Wait before reading the next value
  delay(100);
}

// Function to map analog value to MIDI note logarithmically
int mapAnalogToMIDINoteLog(int analogValue) {
  // Calculate the logarithmic scale factor
  float logScaleFactor = log(analogValue + 1) / log(1024);
  
  // Map the logarithmic value to the MIDI note range
  int midiNote = minMIDINote + (maxMIDINote - minMIDINote) * logScaleFactor;
  
  // Constrain the MIDI note to the valid range
  midiNote = constrain(midiNote, minMIDINote, maxMIDINote);
  
  return midiNote;
}
