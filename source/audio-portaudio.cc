#include "audio-portaudio.h"

static int static_callback(
    const void *input,
    void *output,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo *timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData)
{
    return ((PA_Audio*)userData)
        ->callback(input, output, frameCount, timeInfo, statusFlags);
}

PA_Audio::PA_Audio(const char* appname, Lfq_u32* qnote, Lfq_u32* qcomm) :
    Audio(appname, qnote, qcomm)
{
    // TODO detect these?
    _fsize = 256;
    _fsamp = 48000;

    _nplay = 2;

    PaError err;
#define ERRCHECK(expr) \
    if ((err = (expr)) != paNoError) goto error;

    ERRCHECK(Pa_Initialize());
    ERRCHECK(
        Pa_OpenDefaultStream(
            &_stream,
            0, // input channels
            _nplay, // stereo output
            paFloat32, // 32-bit floating point
            _fsamp, // sample rate
            _fsize, // frames/buf
            static_callback,
            this));
    ERRCHECK(Pa_StartStream(_stream));
#undef ERRCHECK

    for (int i = 0; i < _nplay; i++) _outbuf [i] = new float [_fsize];
    init_audio ();

    return;

error:
    Pa_Terminate();
    fprintf(stderr, "PortAudio error %d: %s\n", err, Pa_GetErrorText(err));
}

PA_Audio::~PA_Audio()
{
    Pa_StopStream(_stream);
    Pa_CloseStream(_stream);
    Pa_Terminate();
    _stream = NULL;
    for (int i = 0; i < _nplay; i++) delete[] _outbuf [i];
}

int PA_Audio::callback(
    const void *input,
    void *outputVP,
    unsigned long nframes,
    const PaStreamCallbackTimeInfo *timeInfo,
    PaStreamCallbackFlags statusFlags)
{
    float* output = (float*)outputVP;

    proc_queue (_qnote);
    proc_queue (_qcomm);
    proc_keys1 ();
    proc_keys2 ();
    proc_synth (nframes);
    for (unsigned long frame = 0; frame < nframes; frame++)
        for (unsigned int channel = 0; channel < _nplay; channel++)
            output[frame * _nplay + channel] = _outbuf[channel][frame];
    proc_mesg ();
    return paContinue;
}

