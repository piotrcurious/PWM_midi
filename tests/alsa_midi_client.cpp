#include "alsa_midi_client.h"
#include <dlfcn.h>
#include <iostream>
#include <cstring>

// ALSA Constants (from asoundlib.h)
#define SND_SEQ_OPEN_OUTPUT 1
#define SND_SEQ_PORT_CAP_READ (1<<10)
#define SND_SEQ_PORT_CAP_SUBS_READ (1<<11)
#define SND_SEQ_PORT_TYPE_MIDI_GENERIC (1<<21)
#define SND_SEQ_EVENT_NOTEON 6
#define SND_SEQ_EVENT_NOTEOFF 7

typedef struct _snd_seq snd_seq_t;
typedef struct _snd_seq_event {
    unsigned char type;
    unsigned char flags;
    unsigned char tag;
    struct {
        unsigned char client;
        unsigned char port;
    } queue;
    struct {
        unsigned int tv_sec;
        unsigned int tv_nsec;
    } time;
    struct {
        unsigned char client;
        unsigned char port;
    } source;
    struct {
        unsigned char client;
        unsigned char port;
    } dest;
    union {
        struct {
            unsigned char channel;
            unsigned char note;
            unsigned char velocity;
            unsigned char off_velocity;
            unsigned int duration;
        } note;
        // ... other event types omitted for brevity
        unsigned char raw8[8];
    } data;
} snd_seq_event_t;

// Function pointers for ALSA functions
typedef int (*snd_seq_open_t)(snd_seq_t **, const char *, int, int);
typedef int (*snd_seq_close_t)(snd_seq_t *);
typedef int (*snd_seq_set_client_name_t)(snd_seq_t *, const char *);
typedef int (*snd_seq_create_simple_port_t)(snd_seq_t *, const char *, unsigned int, unsigned int);
typedef int (*snd_seq_connect_to_t)(snd_seq_t *, int, int, int);
typedef int (*snd_seq_event_output_direct_t)(snd_seq_t *, snd_seq_event_t *);

static snd_seq_open_t p_snd_seq_open;
static snd_seq_close_t p_snd_seq_close;
static snd_seq_set_client_name_t p_snd_seq_set_client_name;
static snd_seq_create_simple_port_t p_snd_seq_create_simple_port;
static snd_seq_connect_to_t p_snd_seq_connect_to;
static snd_seq_event_output_direct_t p_snd_seq_event_output_direct;

ALSAMIDIClient::ALSAMIDIClient() : handle(nullptr), seq(nullptr), port(-1) {}

ALSAMIDIClient::~ALSAMIDIClient() {
    close();
}

bool ALSAMIDIClient::open(const std::string& clientName) {
    handle = dlopen("libasound.so.2", RTLD_LAZY);
    if (!handle) {
        std::cerr << "Error: libasound.so.2 not found" << std::endl;
        return false;
    }

    p_snd_seq_open = (snd_seq_open_t)dlsym(handle, "snd_seq_open");
    p_snd_seq_close = (snd_seq_close_t)dlsym(handle, "snd_seq_close");
    p_snd_seq_set_client_name = (snd_seq_set_client_name_t)dlsym(handle, "snd_seq_set_client_name");
    p_snd_seq_create_simple_port = (snd_seq_create_simple_port_t)dlsym(handle, "snd_seq_create_simple_port");
    p_snd_seq_connect_to = (snd_seq_connect_to_t)dlsym(handle, "snd_seq_connect_to");
    p_snd_seq_event_output_direct = (snd_seq_event_output_direct_t)dlsym(handle, "snd_seq_event_output_direct");

    if (!p_snd_seq_open || !p_snd_seq_close || !p_snd_seq_set_client_name ||
        !p_snd_seq_create_simple_port || !p_snd_seq_connect_to || !p_snd_seq_event_output_direct) {
        std::cerr << "Error: ALSA symbols not found" << std::endl;
        return false;
    }

    int ret = p_snd_seq_open((snd_seq_t**)&seq, "default", SND_SEQ_OPEN_OUTPUT, 0);
    if (ret < 0) {
        std::cerr << "Error: snd_seq_open failed: " << ret << std::endl;
        return false;
    }

    p_snd_seq_set_client_name((snd_seq_t*)seq, clientName.c_str());
    port = p_snd_seq_create_simple_port((snd_seq_t*)seq, "Output Port",
                                        SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ,
                                        SND_SEQ_PORT_TYPE_MIDI_GENERIC);
    return true;
}

void ALSAMIDIClient::close() {
    if (seq && p_snd_seq_close) {
        p_snd_seq_close((snd_seq_t*)seq);
        seq = nullptr;
    }
    if (handle) {
        dlclose(handle);
        handle = nullptr;
    }
}

bool ALSAMIDIClient::connect(int destClient, int destPort) {
    if (!seq) return false;
    int ret = p_snd_seq_connect_to((snd_seq_t*)seq, port, destClient, destPort);
    return ret >= 0;
}

void ALSAMIDIClient::sendNoteOn(int channel, int note, int velocity) {
    if (!seq) return;
    snd_seq_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = SND_SEQ_EVENT_NOTEON;
    ev.flags = 0;
    ev.tag = 0;
    ev.source.port = port;
    ev.dest.client = 254; // SND_SEQ_ADDRESS_SUBSCRIBERS
    ev.dest.port = 254;
    ev.queue.client = 253; // SND_SEQ_QUEUE_DIRECT
    ev.data.note.channel = channel;
    ev.data.note.note = note;
    ev.data.note.velocity = velocity;
    p_snd_seq_event_output_direct((snd_seq_t*)seq, &ev);
}

void ALSAMIDIClient::sendNoteOff(int channel, int note, int velocity) {
    if (!seq) return;
    snd_seq_event_t ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = SND_SEQ_EVENT_NOTEOFF;
    ev.flags = 0;
    ev.source.port = port;
    ev.dest.client = 254;
    ev.dest.port = 254;
    ev.queue.client = 253;
    ev.data.note.channel = channel;
    ev.data.note.note = note;
    ev.data.note.velocity = velocity;
    p_snd_seq_event_output_direct((snd_seq_t*)seq, &ev);
}

std::vector<ALSAMIDIClient::PortInfo> ALSAMIDIClient::listPorts() {
    // Port listing is complex without full headers/structs.
    // For now, we'll return an empty list or skip it.
    return {};
}
