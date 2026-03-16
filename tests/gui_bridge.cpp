#include "tests/mock_arduino.h"
#include "jazz_logic.h"
#include <iostream>
#include <string>
#include <fcntl.h>
#include <poll.h>

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    MIDI.liveMode = true;
    int error = 0;
    int base = 60;

    set_nonblocking(0); // Set stdin to non-blocking

    char buffer[256];
    std::string line;

    while (true) {
        // Run one progression step
        playChordProgression(error, base);

        // Non-blockingly check for updates from stdin
        while (true) {
            ssize_t n = read(0, buffer, sizeof(buffer)-1);
            if (n <= 0) break;
            buffer[n] = '\0';
            line += buffer;

            size_t pos;
            while ((pos = line.find('\n')) != std::string::npos) {
                std::string cmd = line.substr(0, pos);
                line.erase(0, pos + 1);

                if (cmd.empty()) continue;
                try {
                    if (cmd[0] == 'e') {
                        error = std::stoi(cmd.substr(1));
                    } else if (cmd[0] == 'b') {
                        base = std::stoi(cmd.substr(1));
                    } else if (cmd == "q") {
                        return 0;
                    }
                } catch (...) {}
            }
        }

        // Short sleep to avoid maxing CPU if playChordProgression returns too fast
        // though playChordProgression has internal delays.
        usleep(10000);
    }

    return 0;
}
