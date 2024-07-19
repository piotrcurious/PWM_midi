#include <MIDI.h>
#include <math.h>

// Initialize MIDI library
MIDI_CREATE_DEFAULT_INSTANCE();

// Constants
const int analogPin = A0;  // Analog input pin
const int midiChannel = 1; // MIDI channel
const int ledPin = 13;     // Built-in LED for visual feedback

// State variables for AI-inspired prediction
int previousError = 0;
int trend = 0;
int state = 0; // State machine variable
int currentNote = -1; // To keep track of the current note being played

// Function prototypes
int readErrorValue();
int mapErrorToNoteInChord(int errorValue, int* chord, int chordSize);
void sendMIDINoteOn(int note);
void sendMIDINoteOff(int note);
int predictError(int currentError);
void playChordProgression(int errorValue);
void visualFeedback(int intensity);

// Funky jazz chord progressions (ii-V-I in C major with variations)
const int iiChord[] = {62, 65, 69, 72};  // Dm7: D, F, A, C
const int VChord[] = {67, 71, 74, 77};   // G7: G, B, D, F
const int IChord[] = {60, 64, 67, 71};   // Cmaj7: C, E, G, B
const int IVChord[] = {65, 69, 72, 76};  // Fmaj7: F, A, C, E

void setup() {
  // Start MIDI communication
  MIDI.begin(MIDI_CHANNEL_OMNI);
  Serial.begin(9600);
  pinMode(ledPin, OUTPUT);
}

void loop() {
  // Read the current error value
  int currentError = readErrorValue();

  // Play a chord progression based on the current error
  playChordProgression(currentError);

  // Predict the error value
  int predictedError = predictError(currentError);

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
  if (note != currentNote) {  // Only send the note if it's different from the current one
    MIDI.sendNoteOn(note, 127, midiChannel);
    Serial.print("Note On: ");
    Serial.println(note);
    currentNote = note;
  }
}

// Function to send a MIDI note off command
void sendMIDINoteOff(int note) {
  MIDI.sendNoteOff(note, 0, midiChannel);
  Serial.print("Note Off: ");
  Serial.println(note);
  currentNote = -1;
}

// Function to predict the next error value using a more advanced AI-inspired algorithm
int predictError(int currentError) {
  // State machine-based prediction
  switch (state) {
    case 0: // Initial state
      if (currentError > previousError) {
        trend = 1; // Error increasing
        state = 1;
      } else if (currentError < previousError) {
        trend = -1; // Error decreasing
        state = 2;
      } else {
        trend = 0; // No change
      }
      break;

    case 1: // Error increasing
      if (currentError < previousError) {
        state = 3; // Switch to decreasing state
      }
      break;

    case 2: // Error decreasing
      if (currentError > previousError) {
        state = 4; // Switch to increasing state
      }
      break;

    case 3: // Adjusting from increase to decrease
      trend = -1;
      state = 2;
      break;

    case 4: // Adjusting from decrease to increase
      trend = 1;
      state = 1;
      break;
  }

  // Add a random element to the prediction to emulate human-like behavior
  int randomFactor = random(-2, 3); // Random value between -2 and 2
  int predictedError = currentError + trend * 2 + randomFactor; // Adjust factor as needed
  predictedError = constrain(predictedError, 0, 127); // Ensure within range

  // Update previous error for next iteration
  previousError = currentError;

  Serial.print("Predicted Error: ");
  Serial.println(predictedError);
  return predictedError;
}

// Function to play a chord progression based on the error value
void playChordProgression(int errorValue) {
  // Choose a chord based on error range and create a progression
  int* chord1;
  int* chord2;
  int chordSize = 4;
  
  if (errorValue < 32) {
    chord1 = (int*)iiChord;
    chord2 = (int*)VChord;
  } else if (errorValue < 64) {
    chord1 = (int*)VChord;
    chord2 = (int*)IChord;
  } else if (errorValue < 96) {
    chord1 = (int*)IVChord;
    chord2 = (int*)IChord;
  } else {
    chord1 = (int*)IChord;
    chord2 = (int*)iiChord;
  }

  // Play the first chord
  int note1 = mapErrorToNoteInChord(errorValue, chord1, chordSize);
  sendMIDINoteOn(note1);
  visualFeedback(255); // Bright LED for visual feedback
  delay(250); // Hold the note for 250 ms

  // Play the second chord
  int note2 = mapErrorToNoteInChord(errorValue, chord2, chordSize);
  sendMIDINoteOff(note1);
  sendMIDINoteOn(note2);
  visualFeedback(128); // Dimmer LED for visual feedback
  delay(250); // Hold the note for 250 ms

  // Turn off the second note
  sendMIDINoteOff(note2);
  visualFeedback(0); // Turn off LED
}

// Function to provide visual feedback using the built-in LED
void visualFeedback(int intensity) {
  analogWrite(ledPin, intensity);
}