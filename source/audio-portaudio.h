#ifndef AUDIO_PORTAUDIO_H
#define AUDIO_PORTAUDIO_H

#include "audio.h"
#include <portaudio.h>

class PA_Audio : public Audio {
public:
    PA_Audio(const char* appname, Lfq_u32* qnote, Lfq_u32* qcomm);
    ~PA_Audio();
    virtual void thr_main (void) {}

    int callback(
        const void *input,
        void *output,
        unsigned long frameCount,
        const PaStreamCallbackTimeInfo *timeInfo,
        PaStreamCallbackFlags statusFlags);

protected:
    virtual void proc_midi(int) {}

    PaStream* _stream;
};

#endif
