/*! \brief Sound output implementation using alsa library.
    \file program-sound.c
    \author Seungwoo Kang (ki6080@gmail.com)
    \version 0.1
    \date 2019-11-26
    \copyright Copyright (c) 2019. Seungwoo Kang. All rights reserved.
    
    \details
 */
#include "core/program.h"
#include <alsa/asoundlib.h>
#include <pthread.h>

#define CHUNK_SIZE 1024
#define STANDARD_SAMPLE_RATE 22050
#define MAX_WAV_QUEUE_SECONDS 600 // 10 minutes
#define NUM_CHUNKS ((STANDARD_SAMPLE_RATE * MAX_WAV_QUEUE_SECONDS - 1) / CHUNK_SIZE + 1)
#define MAX_QUEABLE_WAV_SOURCE 4
#define DEFAULT_CHANNEL_NUMB 2

#define countof(arr) (sizeof(arr) / sizeof(*arr))
typedef uint16_t sample_t;

typedef struct wavchunk
{
    sample_t pcm[CHUNK_SIZE];
} wavchunk_t;

typedef struct sound
{
    // Wave circular queue properties.
    size_t queue_playidx;
    size_t queue_lastidx;

    // Sound output thread
    pthread_t hOutpThr;

    // Sound process thread
    pthread_t hProcThr;
    void *WavData[MAX_QUEABLE_WAV_SOURCE];

    // Deactivation notifier.
    bool bActive;

    // Place in tail because of its huge size.
    wavchunk_t queue[NUM_CHUNKS];
} sound_t;

/*! \brief Wave header 
 */
#pragma pack(push, 4)
struct wavheader
{
    char RIFF[4];
    uint32_t riff_chunkSz;
    char riff_format[4];

    char fmt_[4];
    uint32_t fmt_chunkSz;
    uint16_t fmt_audioFmt;
    uint16_t fmt_numCh;
    uint32_t fmt_sampleRate;
    uint32_t fmt_byteRate;
    uint16_t fmt_blkAlign;
    uint16_t fmt_bitsPerSmpl;

    char data[4];
    uint32_t data_chunkSz;
};
#pragma pack(pop)

typedef struct wav_rsrc
{
    size_t numSamples;
    intptr_t ptr[1];
} wav_rsrc_t;

static inline void
idx_next(size_t *idx)
{
    static const size_t next_add[2] = {1, (size_t)(1 - NUM_CHUNKS)};
    *idx += next_add[*idx == 1 - NUM_CHUNKS];
}

static void sound_procedure(void *hSound);
static void sound_output(void *hSound);

void *Internal_PInst_InitSound(struct ProgramInstance *Inst)
{
    sound_t *s = malloc(sizeof(sound_t));
    s->bActive = true;

    // Init queue
    s->queue_playidx = 0;
    s->queue_lastidx = 0;

    // Init procedure thread
    for (size_t i = 0; i < MAX_QUEABLE_WAV_SOURCE; i++)
    {
        s->WavData[i] = NULL;
    }
    pthread_cond_t cond;
    pthread_cond_init(&cond, NULL);
    pthread_create(&s->hProcThr, &cond, sound_procedure, s);

    // Init output thread
    pthread_create(&s->hProcThr, &cond, sound_output, s);
    pthread_cond_destroy(&cond);

    return s;
}

void Internal_PInst_DeinitSound(void *hSound)
{
    sound_t *s = hSound;
    s->bActive = false;

    pthread_join(s->hProcThr, NULL);
    pthread_join(s->hOutpThr, NULL);

    free(hSound);
}
void *Internal_PInst_LoadWav(struct ProgramInstance *Inst, char const *Path)
{
}
void Internal_PInst_PlayWav(void *hSound, void *WavData, float Volume)
{
}

static void sound_procedure(void *hSound)
{
    sound_t *s = hSound;
    size_t wav_in_idx = 0;
    static const size_t wav_next_add[2] = {1, 1 - MAX_QUEABLE_WAV_SOURCE};
    void *procesing_wave;

    while (s->bActive)
    {
        // Look for new wav to proceed ...
    }
}

static void sound_output(void *hSound)
{
    sound_t *s = hSound;

    while (s->bActive)
    {
    }
}
