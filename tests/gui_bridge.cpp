#include "tests/mock_arduino.h"
#include "jazz_logic.h"
#include <iostream>
#include <string>

int main() {
    MIDI.liveMode = true;
    int error = 0;
    int base = 60;

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        try {
            if (line[0] == 'e') {
                error = std::stoi(line.substr(1));
            } else if (line[0] == 'b') {
                base = std::stoi(line.substr(1));
            } else if (line == "q") {
                break;
            } else if (line == "p") {
                playChordProgression(error, base);
            }
        } catch (...) {}
    }
    return 0;
}
