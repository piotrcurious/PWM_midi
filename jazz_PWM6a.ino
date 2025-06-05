#include <MIDI.h>
// #include <math.h> // abs() is usually in stdlib.h or implicitly available for integers. For floats, cmath/math.h is needed.

// Initialize MIDI library
MIDI_CREATE_DEFAULT_INSTANCE();

// --- Constants ---
const int ANALOG_PIN_ERROR = A0;     // Analog input pin for error value
const int ANALOG_PIN_BASE_FREQ = A1; // Analog input pin for base frequency
const int MIDI_CHANNEL = 1;          // MIDI channel
const int LED_PIN = 13;              // Built-in LED for visual feedback

// Chord Progression Error Thresholds
const int ERROR_THRESHOLD_1 = 21;
const int ERROR_THRESHOLD_2 = 42;
const int ERROR_THRESHOLD_3 = 63;
const int ERROR_THRESHOLD_4 = 84;
const int ERROR_THRESHOLD_5 = 105;

// Dissonance check buffer
const int MAX_NOTES_PER_CHORD = 4; // Maximum notes we might expect in any chord (triads have 3, 7ths have 4)

// --- State variables ---
// For AI-inspired prediction
int previousError = 0;
int trend = 0; // -1 (decreasing), 0 (stable), 1 (increasing)
int predictionState = 0; // State machine variable for predictError()

// For tracking played notes (primarily for dissonance and turning notes off)
int lastPlayedNotes[MAX_NOTES_PER_CHORD];
int lastPlayedNotesCount = 0;

// Note: The original 'currentNote' global variable was for single notes and not robust for chords.
// The MIDI library handles individual note states. We primarily need to track notes to turn them off.

// --- Chord Definitions ---
// These are currently TRIADS. Names suggest 7ths, but definitions are for 3-note chords.
// Example: Cmaj7 usually is C-E-G-B. Here it's C-E-G.
// These chords are defined as if in the key of C (MIDI note 60 for C4).
// They will be transposed based on 'baseNote'.
const int iiChord_abs[] = {62, 65, 69}; // Dm triad: D, F, A (if C=60 is tonic)
const int VChord_abs[] =  {67, 71, 74}; // G major triad: G, B, D
const int IChord_abs[] =  {60, 64, 67}; // C major triad: C, E, G
const int IVChord_abs[] = {65, 69, 72}; // F major triad: F, A, C
const int viChord_abs[] = {69, 72, 76}; // Am triad: A, C, E
const int iiiChord_abs[] ={64, 67, 71}; // Em triad: E, G, B

// --- Function Prototypes ---
int readErrorValue();
int readBaseFrequency();
void sendMIDINoteOnWrapper(int note);
void sendMIDINoteOffWrapper(int note);
int predictError(int currentError);
void playChordProgression(int currentErrorValue, int baseNote);
void visualFeedback(int intensity);
void sendChord(const int* chordDefinition, int chordDefSize, int transpositionOffset);
bool isDissonant(int note, const int* contextNotes, int contextNotesCount);
// int mapErrorToNoteInChord(int errorValue, const int* chord, int chordSize, int baseNote); // Unused
// int evaluateNoteForDissonance(int note, const int* contextNotes, int numContextNotes); // Unused

void setup() {
  MIDI.begin(MIDI_CHANNEL_OMNI); // Listen on all channels for input (if any), output on MIDI_CHANNEL
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);

  // Initialize lastPlayedNotes buffer
  for (int i = 0; i < MAX_NOTES_PER_CHORD; ++i) {
    lastPlayedNotes[i] = -1; // -1 indicates no note
  }
  lastPlayedNotesCount = 0;
  Serial.println("Setup complete. Starting MIDI generation...");
}

void loop() {
  int currentError = readErrorValue();
  int rawBaseFrequency = readBaseFrequency();

  // Map the raw base frequency reading to a MIDI note (e.g., C3 to C5 for tonic)
  int baseNote = map(rawBaseFrequency, 0, 1023, 48, 72);
  Serial.print("Base Note (Tonic): ");
  Serial.println(baseNote);

  playChordProgression(currentError, baseNote);

  int predictedError = predictError(currentError); // Prediction is made but not used to alter music in this loop
  Serial.print("Predicted Error: ");
  Serial.println(predictedError);

  delay(500); // Main loop delay, affects overall tempo of progression changes
}

int readErrorValue() {
  int sensorValue = analogRead(ANALOG_PIN_ERROR);
  int errorValue = map(sensorValue, 0, 1023, 0, 127); // Map to 0-127 range
  Serial.print("Error Value (0-127): ");
  Serial.println(errorValue);
  return errorValue;
}

int readBaseFrequency() {
  int sensorValue = analogRead(ANALOG_PIN_BASE_FREQ);
  // Serial.print("Base Frequency Sensor (0-1023): "); // Optional: print raw value
  // Serial.println(sensorValue);
  return sensorValue;
}

void sendMIDINoteOnWrapper(int note) {
  if (note >= 0 && note <= 127) { // Basic validation
    MIDI.sendNoteOn(note, 127, MIDI_CHANNEL); // Max velocity
    Serial.print("MIDI Note On: ");
    Serial.println(note);
  }
}

void sendMIDINoteOffWrapper(int note) {
  if (note >= 0 && note <= 127) { // Basic validation
    MIDI.sendNoteOff(note, 0, MIDI_CHANNEL); // Velocity 0
    Serial.print("MIDI Note Off: ");
    Serial.println(note);
  }
}

// Function to send a chord, applying transposition and checking for dissonance.
// Updates lastPlayedNotes with the notes of the chord it sends.
void sendChord(const int* chordDefinition, int chordDefSize, int transpositionOffset) {
  // Turn off notes from the *previously* played chord that are stored in lastPlayedNotes
  for (int i = 0; i < lastPlayedNotesCount; ++i) {
    if (lastPlayedNotes[i] != -1) {
      sendMIDINoteOffWrapper(lastPlayedNotes[i]);
    }
    lastPlayedNotes[i] = -1; // Clear the slot
  }
  lastPlayedNotesCount = 0; // Reset count for the new chord

  int currentChordNotes[MAX_NOTES_PER_CHORD]; // Temp storage for notes of current chord being built
  int currentChordNotesCount = 0;

  for (int i = 0; i < chordDefSize && currentChordNotesCount < MAX_NOTES_PER_CHORD; ++i) {
    int noteToPlay = chordDefinition[i] + transpositionOffset;

    // Ensure note is within valid MIDI range after transposition
    if (noteToPlay < 0 || noteToPlay > 127) {
        Serial.print("Warning: Transposed note out of MIDI range: ");
        Serial.println(noteToPlay);
        continue; // Skip this note
    }

    // Check for dissonance:
    // 1. Against notes of the *previous* chord (not applicable here as they are turned off above)
    // 2. Against notes *already added to the current chord* (intra-chord dissonance)
    if (!isDissonant(noteToPlay, currentChordNotes, currentChordNotesCount)) {
      sendMIDINoteOnWrapper(noteToPlay);
      currentChordNotes[currentChordNotesCount] = noteToPlay; // Add to current chord's notes
      lastPlayedNotes[currentChordNotesCount] = noteToPlay;   // Store for next cycle's turn-off and potential inter-chord dissonance check
      currentChordNotesCount++;
      lastPlayedNotesCount++;
    } else {
      Serial.print("Skipping dissonant note: ");
      Serial.println(noteToPlay);
    }
  }
}


// Function to check if a note is dissonant with a given set of context notes
bool isDissonant(int note, const int* contextNotes, int contextNotesCount) {
  for (int i = 0; i < contextNotesCount; ++i) {
    if (contextNotes[i] == -1) continue; // Skip uninitialized notes
    int interval = abs(note - contextNotes[i]) % 12; // Mod 12 for octave equivalence in interval type
    // Dissonant intervals: minor second (1), major second (2), tritone (6)
    // Minor seventh (10) can also be dissonant in some contexts, included from original code.
    if (interval == 1 || interval == 2 || interval == 6 || interval == 10) {
      Serial.print("Dissonance detected: note ");
      Serial.print(note);
      Serial.print(" with context note ");
      Serial.print(contextNotes[i]);
      Serial.print(", interval ");
      Serial.println(interval);
      return true;
    }
  }
  return false;
}

void playChordProgression(int currentErrorValue, int currentBaseNote) {
  const int* chord1Def;
  const int* chord2Def;
  int chord1Size;
  int chord2Size;

  // Determine transposition offset based on currentBaseNote
  // Assuming the defined chords (IChord_abs, etc.) have their root as if C4 (MIDI 60) is the tonic.
  int transpositionOffset = currentBaseNote - 60; // 60 is MIDI for C4

  // Choose chord progression based on errorValue
  if (currentErrorValue < ERROR_THRESHOLD_1) { // 0-20
    chord1Def = iiChord_abs; chord1Size = sizeof(iiChord_abs) / sizeof(iiChord_abs[0]);
    chord2Def = VChord_abs;  chord2Size = sizeof(VChord_abs) / sizeof(VChord_abs[0]);
  } else if (currentErrorValue < ERROR_THRESHOLD_2) { // 21-41
    chord1Def = iiiChord_abs; chord1Size = sizeof(iiiChord_abs) / sizeof(iiiChord_abs[0]);
    chord2Def = viChord_abs;  chord2Size = sizeof(viChord_abs) / sizeof(viChord_abs[0]);
  } else if (currentErrorValue < ERROR_THRESHOLD_3) { // 42-62
    chord1Def = viChord_abs; chord1Size = sizeof(viChord_abs) / sizeof(viChord_abs[0]);
    chord2Def = iiChord_abs; chord2Size = sizeof(iiChord_abs) / sizeof(iiChord_abs[0]);
  } else if (currentErrorValue < ERROR_THRESHOLD_4) { // 63-83
    chord1Def = IVChord_abs; chord1Size = sizeof(IVChord_abs) / sizeof(IVChord_abs[0]);
    chord2Def = VChord_abs;  chord2Size = sizeof(VChord_abs) / sizeof(VChord_abs[0]);
  } else if (currentErrorValue < ERROR_THRESHOLD_5) { // 84-104
    chord1Def = VChord_abs;  chord1Size = sizeof(VChord_abs) / sizeof(VChord_abs[0]);
    chord2Def = IChord_abs;  chord2Size = sizeof(IChord_abs) / sizeof(IChord_abs[0]);
  } else { // 105-127
    chord1Def = IChord_abs;  chord1Size = sizeof(IChord_abs) / sizeof(IChord_abs[0]);
    chord2Def = iiChord_abs; chord2Size = sizeof(iiChord_abs) / sizeof(iiChord_abs[0]);
  }

  Serial.println("Playing Chord 1:");
  sendChord(chord1Def, chord1Size, transpositionOffset);
  visualFeedback(255); // Bright LED
  delay(250);          // Hold chord 1

  Serial.println("Playing Chord 2:");
  sendChord(chord2Def, chord2Size, transpositionOffset); // sendChord will turn off chord1 notes
  visualFeedback(128); // Dimmer LED
  delay(250);          // Hold chord 2

  // Turn off notes of the last played chord (chord2), which are now in lastPlayedNotes
  Serial.println("Turning off Chord 2 notes.");
  for (int i = 0; i < lastPlayedNotesCount; ++i) {
    if (lastPlayedNotes[i] != -1) {
      sendMIDINoteOffWrapper(lastPlayedNotes[i]);
      lastPlayedNotes[i] = -1;
    }
  }
  lastPlayedNotesCount = 0;
  visualFeedback(0); // Turn off LED
}

int predictError(int currentError) {
  // State machine-based prediction
  switch (predictionState) {
    case 0: // Initial or stable state
      if (currentError > previousError) {
        trend = 1; // Error increasing
        predictionState = 1;
      } else if (currentError < previousError) {
        trend = -1; // Error decreasing
        predictionState = 2;
      } else {
        trend = 0; // No change
      }
      break;
    case 1: // Error was increasing
      if (currentError < previousError) { // Trend reversed
        trend = -1;
        predictionState = 2; // Switch to decreasing state
      } else if (currentError == previousError) {
        // trend remains 1 (or could be set to 0 if plateauing)
        predictionState = 0; // Go to stable to re-evaluate
      }
      // else: still increasing, trend and state remain
      break;
    case 2: // Error was decreasing
      if (currentError > previousError) { // Trend reversed
        trend = 1;
        predictionState = 1; // Switch to increasing state
      } else if (currentError == previousError) {
        // trend remains -1 (or could be set to 0 if plateauing)
        predictionState = 0; // Go to stable to re-evaluate
      }
      // else: still decreasing, trend and state remain
      break;
  }

  // Add a random element to the prediction
  int randomFactor = random(-2, 3); // Random value between -2 and 2 (exclusive of 3)
  int predictedError = currentError + trend * 2 + randomFactor; // Simple linear prediction with trend and randomness
  predictedError = constrain(predictedError, 0, 127);      // Ensure within MIDI range

  previousError = currentError; // Update previous error for the next iteration
  return predictedError;
}

void visualFeedback(int intensity) {
  // On many Arduinos, pin 13 is not true PWM.
  // analogWrite to non-PWM pins acts like digitalWrite (HIGH if value > ~127, else LOW)
  if (intensity > 0) {
    digitalWrite(LED_PIN, HIGH);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
  // If LED_PIN is a PWM pin, analogWrite(LED_PIN, intensity); would give variable brightness.
}


/*
// --- UNUSED FUNCTIONS --- (Commented out as they are not called in the main logic)

// Function to map an error value to a single note within a chord, harmonized to the base note.
// This could be used for generating a monophonic melody line based on error.
int mapErrorToNoteInChord(int errorValue, const int* chord, int chordSize, int currentBaseNote) {
  if (chordSize <= 0) return -1; // Invalid chord
  int index = errorValue % chordSize; // Pick a note from the chord based on error

  // Assuming 'chord' contains absolute MIDI notes for C-based voicings (like IChord_abs)
  // We need the same transposition logic as in sendChord
  int transpositionOffset = currentBaseNote - 60; // 60 is C4
  int midiNote = chord[index] + transpositionOffset;

  midiNote = constrain(midiNote, 0, 127); // Ensure valid MIDI note

  Serial.print("Mapped Error to Single MIDI Note (from chord): ");
  Serial.println(midiNote);
  return midiNote;
}

// Function to evaluate a note for dissonance, providing a score.
// Positive score = more consonant, Negative score = more dissonant.
// This offers a more nuanced approach than the boolean isDissonant().
int evaluateNoteForDissonance(int note, const int* contextNotes, int numContextNotes) {
  int score = 0;
  for (int i = 0; i < numContextNotes; ++i) {
    if (contextNotes[i] == -1) continue;
    int interval = abs(note - contextNotes[i]) % 12; // Mod 12 for octave equivalence

    switch (interval) {
      case 0: // Unison (if checking against different notes, this implies an octave)
        score += 2; break;
      case 1: // Minor second
      case 2: // Major second
        score -= 3; break; // Highly dissonant
      case 3: // Minor third
      case 4: // Major third
        score += 2; break; // Consonant
      case 5: // Perfect fourth
        score += 1; break; // Mildly consonant (can be dissonant against bass)
      case 6: // Tritone
        score -= 4; break; // Highly dissonant
      case 7: // Perfect fifth
        score += 3; break; // Highly consonant
      case 8: // Minor sixth
      case 9: // Major sixth
        score += 1; break; // Consonant
      case 10: // Minor seventh
        score -= 2; break; // Dissonant (though common in 7th chords)
      case 11: // Major seventh
        score -= 1; break; // Can be dissonant or coloristic
      default: break;
    }
  }
  return score;
}
*/
