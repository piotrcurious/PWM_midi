#include "jazz_logic.h"
#include <cassert>
#include <iostream>
#include <vector>

void test_isDissonant_extended() {
    std::cout << "Testing isDissonant extended..." << std::endl;

    // Test basic intervals
    int context[] = {60, -1, -1, -1};
    assert(isDissonant(61, context, 1) == true);  // Minor 2nd
    assert(isDissonant(62, context, 1) == false); // Major 2nd - OK now
    assert(isDissonant(63, context, 1) == false); // Minor 3rd
    assert(isDissonant(64, context, 1) == false); // Major 3rd
    assert(isDissonant(65, context, 1) == false); // Perfect 4th
    assert(isDissonant(66, context, 1) == false); // Tritone - OK now
    assert(isDissonant(67, context, 1) == false); // Perfect 5th
    assert(isDissonant(68, context, 1) == false); // Minor 6th
    assert(isDissonant(69, context, 1) == false); // Major 6th
    assert(isDissonant(70, context, 1) == false); // Minor 7th - OK now
    assert(isDissonant(71, context, 1) == false); // Major 7th

    std::cout << "isDissonant extended tests passed!" << std::endl;
}

void test_chord_filtering() {
    std::cout << "Testing chord filtering..." << std::endl;

    // Mock MIDI should clear events
    MIDI.events.clear();

    // G7 chord: G, B, D, F
    int G7[] = {7, 11, 2, 5}; // Using base notes to test voice leading
    sendChord(G7, 4, 60); // Base at 60 (Middle C)

    // Check how many notes were played
    int notesOn = 0;
    for (const auto& e : MIDI.events) {
        if (e.on) notesOn++;
    }

    std::cout << "Notes played for G7: " << notesOn << std::endl;
    // All 4 notes should be played because we relaxed isDissonant
    assert(notesOn == 4);

    std::cout << "Chord filtering tests passed!" << std::endl;
}

void test_velocity_and_timing() {
    std::cout << "Testing velocity and timing..." << std::endl;

    MIDI.events.clear();
    // Low error = Low velocity
    playChordProgression(10, 60);
    // After playChordProgression, stopLastPlayedNotes is called, sending Note Offs with velocity 0.
    // We need to find the Note On events.
    int lowVelocity = 0;
    for (auto it = MIDI.events.rbegin(); it != MIDI.events.rend(); ++it) {
        if (it->on) {
            lowVelocity = it->velocity;
            break;
        }
    }

    MIDI.events.clear();
    // High error = High velocity
    playChordProgression(110, 60);
    int highVelocity = 0;
    for (auto it = MIDI.events.rbegin(); it != MIDI.events.rend(); ++it) {
        if (it->on) {
            highVelocity = it->velocity;
            break;
        }
    }

    std::cout << "Low Error Velocity: " << lowVelocity << ", High Error Velocity: " << highVelocity << std::endl;
    assert(highVelocity > lowVelocity);

    std::cout << "Velocity and timing tests passed!" << std::endl;
}

void test_voice_leading() {
    std::cout << "Testing voice leading..." << std::endl;

    MIDI.events.clear();
    int CMaj7[] = {0, 4, 7, 11};
    sendChord(CMaj7, 4, 48, 100); // Low octave

    int firstChordAvg = 0;
    int notesCount = 0;
    for (const auto& e : MIDI.events) {
        if (e.on) {
            firstChordAvg += e.note;
            notesCount++;
        }
    }
    firstChordAvg /= notesCount;

    MIDI.events.clear();
    int FMaj7[] = {5, 9, 0, 4};
    sendChord(FMaj7, 4, 72, 100); // High octave transposition

    int secondChordAvg = 0;
    notesCount = 0;
    for (const auto& e : MIDI.events) {
        if (e.on) {
            secondChordAvg += e.note;
            notesCount++;
        }
    }
    secondChordAvg /= notesCount;

    std::cout << "First chord avg: " << firstChordAvg << ", Second chord avg: " << secondChordAvg << std::endl;
    // Even with high transposition, the second chord should stay close to the first one due to voice leading octave selection
    assert(abs(secondChordAvg - firstChordAvg) < 12);

    std::cout << "Voice leading tests passed!" << std::endl;
}

int main() {
    test_isDissonant_extended();
    test_chord_filtering();
    test_velocity_and_timing();
    test_voice_leading();
    std::cout << "Extended tests passed!" << std::endl;
    return 0;
}
