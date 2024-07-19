#include <MIDI.h>
#include <math.h>

// Initialize MIDI library
MIDI_CREATE_DEFAULT_INSTANCE();

// Constants
const int analogPin = A0;  // Analog input pin
const int midiChannel = 1; // MIDI channel

// State variables for AI-inspired prediction
int previousError = 0;
int trend = 0;

// Function prototypes
int readErrorValue();
int mapErrorToNoteInChord(int errorValue, int* chord, int chordSize);
void sendMIDINoteOn(int note);
void sendMIDINoteOff(int note);
int predictError(int currentError);

// Funky jazz chord progressions (ii-V-I in C major)
const int iiChord[] = {62, 65, 69}; // Dm7: D, F, A
const int VChord[] = {67, 71, 74};  // G7: G, B, D
const int IChord[] = {60, 64, 67};  // Cmaj7: C, E, G

void setup() {
  // Start MIDI communication
  MIDI.begin(MIDI_CHANNEL_OMNI);
  Serial.begin(9600);
}

void loop() {
  // Read the current error value
  int currentError = readErrorValue();

  // Choose a chord based on error range
  int* chord;
  int chordSize = 3;
  if (currentError < 42) {
    chord = (int*)iiChord;
  } else if (currentError < 84) {
    chord = (int*)VChord;
  } else {
    chord = (int*)IChord;
  }

  // Map error to a note within the chosen chord
  int midiNote = mapErrorToNoteInChord(currentError, chord, chordSize);

  // Send MIDI note on command
  sendMIDINoteOn(midiNote);

  // Predict the error value
  int predictedError = predictError(currentError);

  // Send MIDI note off command for the previous note
  sendMIDINoteOff(midiNote);

  // Delay for a short period before the next iteration
  delay(500);
}

// Function to read the analog input and return the error value
int readErrorValue() {
  int sensorValue = analogRead(analogPin);
  int errorValue = map(sensorValue, 0, 1023, 0, 127); // Map to MIDI note range
  Serial.print("Error Value: ");
  Serial.println(errorValue);
  return errorValue;
}

// Function to map an error value to a note within a chord
int mapErrorToNoteInChord(int errorValue, int* chord, int chordSize) {
  int index = errorValue % chordSize;
  int midiNote = chord[index];
  Serial.print("MIDI Note (Chord): ");
  Serial.println(midiNote);
  return midiNote;
}

// Function to send a MIDI note on command
void sendMIDINoteOn(int note) {
  MIDI.sendNoteOn(note, 127, midiChannel);
  Serial.print("Note On: ");
  Serial.println(note);
}

// Function to send a MIDI note off command
void sendMIDINoteOff(int note) {
  MIDI.sendNoteOff(note, 0, midiChannel);
  Serial.print("Note Off: ");
  Serial.println(note);
}

// Function to predict the next error value using a simple AI-inspired algorithm
int predictError(int currentError) {
  // Simple AI-inspired prediction: detect trend and adjust
  if (currentError > previousError) {
    trend = 1; // Error increasing
  } else if (currentError < previousError) {
    trend = -1; // Error decreasing
  } else {
    trend = 0; // No change
  }

  // Predict next error based on current trend
  int predictedError = currentError + trend * 2; // Adjust factor as needed
  predictedError = constrain(predictedError, 0, 127); // Ensure within range

  // Update previous error for next iteration
  previousError = currentError;

  Serial.print("Predicted Error: ");
  Serial.println(predictedError);
  return predictedError;
}
