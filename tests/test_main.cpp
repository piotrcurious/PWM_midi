#include "jazz_logic.h"
#include "midi_file_writer.h"
#include <cassert>
#include <iostream>
#include <cstring>

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
    // trend should be 1. pred = 70 + 2 + random(-2,3)
    assert(pred >= 70 && pred <= 75);
    std::cout << "predictError tests passed!" << std::endl;
}

void run_progression_recording(const std::string& filename) {
    std::cout << "Recording progression to " << filename << "..." << std::endl;
    MIDI.events.clear();
    for(int e=0; e<128; e += 20) {
        playChordProgression(e, 60); // C
        playChordProgression(e, 65); // F
        playChordProgression(e, 67); // G
    }
    MIDIFileWriter::write(filename, MIDI.events);
    std::cout << "Recording complete." << std::endl;
}

int main(int argc, char** argv) {
    if (argc > 1 && strcmp(argv[1], "--live") == 0) {
        MIDI.liveMode = true;
        // Run a loop of progressions
        for(int i=0; i<100; i++) {
            playChordProgression(rand()%128, 60 + (rand()%12));
        }
        return 0;
    }

    test_isDissonant();
    test_predictError();
    run_progression_recording("progression.mid");

    std::cout << "All tests passed successfully!" << std::endl;
    return 0;
}
