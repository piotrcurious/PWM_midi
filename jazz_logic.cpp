#include "jazz_logic.h"

#ifdef MOCK_TESTING
MockMIDI MIDI;
MockSerial Serial;
#endif

const int ERROR_THRESHOLD_1 = 21;
const int ERROR_THRESHOLD_2 = 42;
const int ERROR_THRESHOLD_3 = 63;
const int ERROR_THRESHOLD_4 = 84;
const int ERROR_THRESHOLD_5 = 105;
const int MAX_NOTES_PER_CHORD = 4;

static int previousError = 0;
static int trend = 0;
static int predictionState = 0;

static int lastPlayedNotes[MAX_NOTES_PER_CHORD] = {-1, -1, -1, -1};
static int lastPlayedNotesCount = 0;
static int lastTargetNotes[MAX_NOTES_PER_CHORD] = {-1, -1, -1, -1};
static int lastTargetNotesCount = 0;

const int iiChord_abs[] = {62, 65, 69, 72}; // Dm7
const int VChord_abs[] =  {67, 71, 74, 77}; // G7
const int IChord_abs[] =  {60, 64, 67, 71}; // Cmaj7
const int IVChord_abs[] = {65, 69, 72, 76}; // Fmaj7
const int viChord_abs[] = {69, 72, 76, 79}; // Am7
const int iiiChord_abs[] ={64, 67, 71, 74}; // Em7

bool isDissonant(int note, const int* contextNotes, int contextNotesCount) {
  for (int i = 0; i < contextNotesCount; ++i) {
    if (contextNotes[i] == -1) continue;
    int interval = abs(note - contextNotes[i]) % 12;
    // In jazz, major 2nds (2) and minor 7ths (10) are often acceptable.
    // Tritones (6) are essential in dominant chords.
    // Minor 2nd (1) is usually the most dissonant.
    if (interval == 1) {
      return true;
    }
  }
  return false;
}

int predictError(int currentError) {
  // Simple trend analysis: if error is increasing, assume it will continue.
  // We use predictionState to add some hysteresis to the trend detection.
  switch (predictionState) {
    case 0: // Stable
      if (currentError > previousError) {
        trend = 1;
        predictionState = 1; // Increasing
      } else if (currentError < previousError) {
        trend = -1;
        predictionState = 2; // Decreasing
      }
      break;
    case 1: // Increasing
      if (currentError < previousError) {
        predictionState = 0; // Became unstable, reset to stable and see next time
        trend = 0;
      }
      break;
    case 2: // Decreasing
      if (currentError > previousError) {
        predictionState = 0; // Became unstable
        trend = 0;
      }
      break;
  }

  int randomFactor = random(-2, 3);
  // Add some "momentum" to the prediction based on the trend
  int predictedError = currentError + trend * 3 + randomFactor;
  predictedError = constrain(predictedError, 0, 127);

  previousError = currentError;
  return predictedError;
}

void sendMIDINoteOnWrapper(int note, int velocity) {
  if (note >= 0 && note <= 127) {
    #ifdef MOCK_TESTING
    MIDI.sendNoteOn(note, velocity, 1);
    #else
    MIDI.sendNoteOn(note, velocity, 1);
    Serial.print("MIDI Note On: ");
    Serial.print(note);
    Serial.print(" Velocity: ");
    Serial.println(velocity);
    #endif
  }
}

void sendMIDINoteOffWrapper(int note) {
  if (note >= 0 && note <= 127) {
    #ifdef MOCK_TESTING
    MIDI.sendNoteOff(note, 0, 1);
    #else
    MIDI.sendNoteOff(note, 0, 1);
    Serial.print("MIDI Note Off: ");
    Serial.println(note);
    #endif
  }
}

void stopLastPlayedNotes() {
  for (int i = 0; i < lastPlayedNotesCount; ++i) {
    if (lastPlayedNotes[i] != -1) {
      sendMIDINoteOffWrapper(lastPlayedNotes[i]);
      lastPlayedNotes[i] = -1;
    }
  }
  lastPlayedNotesCount = 0;
}

void sendChord(const int* chordDefinition, int chordDefSize, int transpositionOffset, int velocity) {
  stopLastPlayedNotes();

  int currentChordNotes[MAX_NOTES_PER_CHORD] = {-1, -1, -1, -1};
  int currentChordNotesCount = 0;

  // Simple voice leading: find the best octave for each note in the new chord
  // to be close to the average of the last played notes.
  int targetCenter = 64; // Default middle C area
  if (lastTargetNotesCount > 0) {
    long sum = 0;
    for (int i = 0; i < lastTargetNotesCount; ++i) sum += lastTargetNotes[i];
    targetCenter = sum / lastTargetNotesCount;
  }

  for (int i = 0; i < chordDefSize && currentChordNotesCount < MAX_NOTES_PER_CHORD; ++i) {
    int noteBase = (chordDefinition[i] + transpositionOffset) % 12;

    // Find octave that brings noteBase closest to targetCenter
    int bestNote = -1;
    int minDiff = 128;
    for (int octave = 3; octave <= 6; ++octave) {
      int candidate = noteBase + octave * 12;
      int diff = abs(candidate - targetCenter);
      if (diff < minDiff) {
        minDiff = diff;
        bestNote = candidate;
      }
    }

    if (bestNote >= 0 && bestNote <= 127) {
      if (!isDissonant(bestNote, currentChordNotes, currentChordNotesCount)) {
        sendMIDINoteOnWrapper(bestNote, velocity);
        currentChordNotes[currentChordNotesCount] = bestNote;
        lastPlayedNotes[currentChordNotesCount] = bestNote;
        currentChordNotesCount++;
        lastPlayedNotesCount++;
      }
    }
  }

  // Update lastTargetNotes for next call
  for (int i = 0; i < currentChordNotesCount; ++i) {
    lastTargetNotes[i] = currentChordNotes[i];
  }
  lastTargetNotesCount = currentChordNotesCount;
}

void playChordProgression(const EVContext& context, int currentBaseNote) {
  const int* chord1Def;
  const int* chord2Def;
  int chord1Size = 4;
  int chord2Size = 4;

  // Base transposition offset
  int transpositionOffset = currentBaseNote - 60;

  // Speed influences the register (octave)
  int speedOffset = map(context.speed, 0, 127, -12, 12);
  transpositionOffset += speedOffset;

  // Error selects the harmonic complexity/progression
  if (context.error < ERROR_THRESHOLD_1) {
    chord1Def = iiChord_abs; chord2Def = VChord_abs;
  } else if (context.error < ERROR_THRESHOLD_2) {
    chord1Def = iiiChord_abs; chord2Def = viChord_abs;
  } else if (context.error < ERROR_THRESHOLD_3) {
    chord1Def = viChord_abs; chord2Def = iiChord_abs;
  } else if (context.error < ERROR_THRESHOLD_4) {
    chord1Def = IVChord_abs; chord2Def = VChord_abs;
  } else if (context.error < ERROR_THRESHOLD_5) {
    chord1Def = VChord_abs; chord2Def = IChord_abs;
  } else {
    chord1Def = IChord_abs; chord2Def = iiChord_abs;
  }

  // Velocity influenced by throttle and error, but dampened by brake
  int baseVelocity = map(context.error, 0, 127, 40, 80) + map(context.throttle, 0, 127, 0, 40);
  int velocity = constrain(baseVelocity - map(context.brake, 0, 127, 0, 50), 30, 120);

  // Rhythm (delay) influenced by speed and error, slowed down by brake
  int baseDelay = map(context.error, 0, 127, 500, 200) - map(context.speed, 0, 127, 0, 100);
  int actualDelay = constrain(baseDelay + map(context.brake, 0, 127, 0, 400), 100, 1000);

  sendChord(chord1Def, chord1Size, transpositionOffset, velocity);
  visualFeedback(255);
  delay(actualDelay);

  // If braking hard, maybe don't play the second chord or play it very softly
  if (context.brake < 100) {
    int velocity2 = constrain(velocity + trend * 10, 30, 127);
    sendChord(chord2Def, chord2Size, transpositionOffset, velocity2);
    visualFeedback(128);
    delay(actualDelay);
  }

  stopLastPlayedNotes();
  visualFeedback(0);
}

void visualFeedback(int intensity) {
#ifndef MOCK_TESTING
  if (intensity > 0) {
    digitalWrite(13, HIGH);
  } else {
    digitalWrite(13, LOW);
  }
#endif
}
