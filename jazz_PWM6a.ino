#include "jazz_logic.h"

// MIDI instance is created here and used by jazz_logic.cpp
MIDI_CREATE_DEFAULT_INSTANCE();

const int ANALOG_PIN_ERROR = A0;
const int ANALOG_PIN_BASE_FREQ = A1;
const int LED_PIN = 13;

void setup() {
  MIDI.begin(MIDI_CHANNEL_OMNI);
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  Serial.println("Setup complete. Starting MIDI generation...");
}

void loop() {
  int sensorValue = analogRead(ANALOG_PIN_ERROR);
  int currentError = map(sensorValue, 0, 1023, 0, 127);

  int rawBaseFrequency = analogRead(ANALOG_PIN_BASE_FREQ);
  int baseNote = map(rawBaseFrequency, 0, 1023, 48, 72);

  Serial.print("Base Note (Tonic): ");
  Serial.println(baseNote);
  Serial.print("Error Value (0-127): ");
  Serial.println(currentError);

  playChordProgression(currentError, baseNote);

  int predictedError = predictError(currentError);
  Serial.print("Predicted Error: ");
  Serial.println(predictedError);

  delay(500);
}
