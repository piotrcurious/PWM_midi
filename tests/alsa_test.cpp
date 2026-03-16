#include <alsa/asoundlib.h>
int main() {
    snd_seq_t *seq;
    snd_seq_open(&seq, "default", SND_SEQ_OPEN_OUTPUT, 0);
    return 0;
}
