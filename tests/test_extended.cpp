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

    // G7 chord: G(67), B(71), D(74), F(77)
    int G7[] = {67, 71, 74, 77};
    sendChord(G7, 4, 0);

    // Check how many notes were played
    int notesOn = 0;
    for (const auto& e : MIDI.events) {
        if (e.on) notesOn++;
    }

    std::cout << "Notes played for G7: " << notesOn << std::endl;
    // Now, all 4 notes should be played because we relaxed isDissonant
    assert(notesOn == 4);

    std::cout << "Chord filtering tests passed!" << std::endl;
}

int main() {
    test_isDissonant_extended();
    test_chord_filtering();
    std::cout << "Extended tests passed!" << std::endl;
    return 0;
}
