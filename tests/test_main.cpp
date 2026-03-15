#include "jazz_logic.h"
#include <cassert>
#include <iostream>

void test_isDissonant() {
    std::cout << "Testing isDissonant..." << std::endl;
    int context[] = {60, -1, -1, -1}; // C

    assert(isDissonant(61, context, 1) == true);
    assert(isDissonant(62, context, 1) == true);
    assert(isDissonant(64, context, 1) == false);
    assert(isDissonant(66, context, 1) == true);
    assert(isDissonant(70, context, 1) == true);

    std::cout << "isDissonant tests passed!" << std::endl;
}

void test_predictError() {
    std::cout << "Testing predictError..." << std::endl;

    predictError(50);
    predictError(60);
    int pred = predictError(70);
    assert(pred == 70);

    predictError(60);
    pred = predictError(50);
    assert(pred == 46);

    std::cout << "predictError tests passed!" << std::endl;
}

void test_sendChord() {
    std::cout << "Testing sendChord..." << std::endl;
    MIDI.events.clear();

    int chord[] = {60, 64, 67}; // C Major Triad
    sendChord(chord, 3, 0);

    int onCount = 0;
    for(const auto& e : MIDI.events) if(e.on) onCount++;
    assert(onCount == 3);

    int chord2[] = {62, 65, 69}; // D minor Triad
    sendChord(chord2, 3, 0);

    int offCount = 0;
    for(const auto& e : MIDI.events) if(!e.on) offCount++;
    assert(offCount == 3);

    std::cout << "sendChord tests passed!" << std::endl;
}

void test_playChordProgression() {
    std::cout << "Testing playChordProgression..." << std::endl;
    MIDI.events.clear();

    playChordProgression(10, 60);

    int onCount = 0;
    int offCount = 0;
    for(const auto& e : MIDI.events) {
        if(e.on) onCount++;
        else offCount++;
    }

    assert(onCount == 6);
    assert(offCount >= 6);

    std::cout << "playChordProgression tests passed!" << std::endl;
}

int main() {
    test_isDissonant();
    test_predictError();
    test_sendChord();
    test_playChordProgression();
    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}
