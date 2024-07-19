#include <MIDI.h>
#include <math.h>

// Initialize MIDI library
MIDI_CREATE_DEFAULT_INSTANCE();

// Constants
const int analogPin = A0;  // Analog input pin
const int midiChannel = 1; // MIDI channel

// Function prototypes
int readErrorValue();
int errorToMIDINote(int errorValue);
void sendMIDINoteOn(int note);
void sendMIDINoteOff(int note);
int predictError(int currentError);

void setup() {
  // Start MIDI communication
  MIDI.begin(MIDI_CHANNEL_OMNI);
  Serial.begin(9600);
}

void loop() {
  // Read the current error value
  int currentError = readErrorValue();
  
  // Convert error to a MIDI note
  int midiNote = errorToMIDINote(currentError);

  // Send MIDI note on command
  sendMIDINoteOn(midiNote);

  // Predict the error value
  int predictedError = predictError(currentError);

  // Convert predicted error to MIDI note
  int predictedMIDINote = errorToMIDINote(predictedError);

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

// Function to convert an error value to a MIDI note
int errorToMIDINote(int errorValue) {
  // Convert error to frequency using inverse logarithmic relationship
  float frequency = pow(2, (127.0 - errorValue) / 12.0) * 440.0;
  int midiNote = 69 + 12 * log2(frequency / 440.0);
  midiNote = constrain(midiNote, 0, 127); // Ensure note is within MIDI range
  Serial.print("MIDI Note: ");
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

// Function to predict the next error value (simple prediction algorithm)
int predictError(int currentError) {
  // Simple prediction: assume the error will decrease by a fixed amount
  int predictedError = currentError - 1;
  predictedError = max(predictedError, 0); // Ensure error does not go below 0
  Serial.print("Predicted Error: ");
  Serial.println(predictedError);
  return predictedError;
}
