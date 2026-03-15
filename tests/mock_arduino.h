#ifndef MOCK_ARDUINO_H
#define MOCK_ARDUINO_H

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>

#define A0 0
#define A1 1
#define OUTPUT 1
#define HIGH 1
#define LOW 0
#define MIDI_CHANNEL_OMNI 0

// Mock Serial
class MockSerial {
public:
    void begin(int baud) {}
    void print(const char* s) { /* std::cout << s; */ }
    void print(int i) { /* std::cout << i; */ }
    void println(const char* s) { /* std::cout << s << std::endl; */ }
    void println(int i) { /* std::cout << i << std::endl; */ }
};

// Mock MIDI
class MockMIDI {
public:
    struct NoteEvent {
        int note;
        int velocity;
        int channel;
        bool on;
    };
    std::vector<NoteEvent> events;

    void begin(int channel) {}
    void sendNoteOn(int note, int velocity, int channel) {
        events.push_back({note, velocity, channel, true});
    }
    void sendNoteOff(int note, int velocity, int channel) {
        events.push_back({note, velocity, channel, false});
    }
};

// Global functions
inline int analogRead(int pin) { return 0; }
inline void pinMode(int pin, int mode) {}
inline void digitalWrite(int pin, int val) {}
inline void delay(int ms) {}

inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

template<class T, class L, class H>
inline T constrain(T mvg, L lo, H hi) {
  return (mvg < lo) ? lo : ((mvg > hi) ? hi : mvg);
}

inline long random(long min, long max) {
    return min; // Predictable for testing
}

#endif
