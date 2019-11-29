/*! \brief Sound output implementation using alsa library.
    \file program-sound.c
    \author Seungwoo Kang (ki6080@gmail.com)
    \version 0.1
    \date 2019-11-26
    \copyright Copyright (c) 2019. Seungwoo Kang. All rights reserved.
    
    \details
 */
#include <alsa/asoundlib.h>
#include <alsa/pcm.h>
#include "core/program.h"

#define CHUNK_SIZE 1024
#define STANDARD_SAMPLE_RATE 11000
#define MAX_WAV_QUEUE_SECONDS 600 // 10 minutes
#define NUM_CHUNKS ((STANDARD_SAMPLE_RATE * MAX_WAV_QUEUE_SECONDS - 1) / CHUNK_SIZE + 1)
#define MAX_QUEABLE_WAV_SOURCE 16
#define PCM_DEVICE "default"
#define DEFAULT_CHANNEL_NUMB 1

#define countof(arr) (sizeof(arr) / sizeof(*arr))
typedef int16_t sample_t;
#define SAMPLE_MAXV 32767
#define SAMPLE_MINV -32768

typedef struct wavchunk
{
    sample_t pcm[CHUNK_SIZE];
} wavchunk_t;

typedef struct wavequeuearg
{
    /*data*/
    void *data;
    uint32_t volume;
} wavequeuearg_t;

typedef struct sound
{
    // Wave circular queue properties.
    size_t queue_playidx;
    size_t queue_lastidx;

    // Sound output thread
    pthread_t hOutpThr;

    // Sound process thread
    pthread_t hProcThr;
    wavequeuearg_t WavData[MAX_QUEABLE_WAV_SOURCE];

    // Deactivation notifier.
    bool bActive;

    // Place in tail because of its huge size.
    wavchunk_t queue[NUM_CHUNKS];

    // Device data
    snd_pcm_t *PCM;
} sound_t;

/*! \brief Wave header 
 */
#pragma pack(push, 4)
typedef struct RIFF
{
    char RIFF[4];
    uint32_t riff_chunkSz;
    char riff_format[4];
} RIFF_t;

typedef struct FMT
{
    char fmt_[4];
    uint32_t fmt_chunkSz;
    uint16_t fmt_audioFmt;
    uint16_t fmt_numCh;
    uint32_t fmt_sampleRate;
    uint32_t fmt_byteRate;
    uint16_t fmt_blkAlign;
    uint16_t fmt_bitsPerSmpl;
} FMT_t;

typedef struct DATA
{
    char data[4];
    uint32_t data_chunkSz;
} DATA_t;
#pragma pack(pop)

typedef struct wav_rsrc
{
    size_t numSamples;
    intptr_t ptr[0];
} wav_rsrc_t;

static inline void
idx_next(size_t *idx)
{
    static const size_t next_add[2] = {1, (size_t)(1 - NUM_CHUNKS)};
    *idx += next_add[*idx == 1 - NUM_CHUNKS];
}

static void *sound_procedure(void *hSound);
static void *sound_output(void *hSound);

void *Internal_PInst_InitSound(struct ProgramInstance *Inst)
{
    return NULL;

    sound_t *s = calloc(1, sizeof(sound_t));
    if (s == NULL)
    {
        lvlog(LOGLEVEL_ERROR, "Failed to allocate sufficient sound memory.\n");
        return NULL;
    }

    s->bActive = true;

    // Init PCM Drive
    unsigned int pcm, tmp, dir;
    int rate, channels, seconds;
    snd_pcm_hw_params_t *params;
    snd_pcm_uframes_t frames;
    char *buff;
    int buff_size, loops;

    rate = STANDARD_SAMPLE_RATE;
    channels = DEFAULT_CHANNEL_NUMB;
    seconds = 0;

    /* Open the PCM device in playback mode */
    if (pcm = snd_pcm_open(&s->PCM, PCM_DEVICE,
                           SND_PCM_STREAM_PLAYBACK, 0) < 0)
        lvlog(LOGLEVEL_INFO, "ERROR: Can't open \"%s\" PCM device. %s\n",
              PCM_DEVICE, snd_strerror(pcm));

    /* Allocate parameters object and fill it with default values*/
    snd_pcm_hw_params_alloca(&params);

    snd_pcm_hw_params_any(s->PCM, params);

    /* Set parameters */
    if (pcm = snd_pcm_hw_params_set_access(s->PCM, params,
                                           SND_PCM_ACCESS_RW_INTERLEAVED) < 0)
        lvlog(LOGLEVEL_INFO, "ERROR: Can't set interleaved mode. %s\n", snd_strerror(pcm));

    if (pcm = snd_pcm_hw_params_set_format(s->PCM, params,
                                           SND_PCM_FORMAT_S16_LE) < 0)
        lvlog(LOGLEVEL_INFO, "ERROR: Can't set format. %s\n", snd_strerror(pcm));

    if (pcm = snd_pcm_hw_params_set_channels(s->PCM, params, channels) < 0)
        lvlog(LOGLEVEL_INFO, "ERROR: Can't set channels number. %s\n", snd_strerror(pcm));

    if (pcm = snd_pcm_hw_params_set_rate_near(s->PCM, params, &rate, 0) < 0)
        lvlog(LOGLEVEL_INFO, "ERROR: Can't set rate. %s\n", snd_strerror(pcm));

    /* Write parameters */
    if (pcm = snd_pcm_hw_params(s->PCM, params) < 0)
        lvlog(LOGLEVEL_INFO, "ERROR: Can't set harware parameters. %s\n", snd_strerror(pcm));

    /* Resume information */
    lvlog(LOGLEVEL_INFO, "PCM name: '%s'\n", snd_pcm_name(s->PCM));

    lvlog(LOGLEVEL_INFO, "PCM state: %s\n", snd_pcm_state_name(snd_pcm_state(s->PCM)));

    snd_pcm_hw_params_get_channels(params, &tmp);
    lvlog(LOGLEVEL_INFO, "channels: %i \n", tmp);

    if (tmp == 1)
    {
        lvlog(LOGLEVEL_INFO, "\t(mono)\n");
    }
    else if (tmp == 2)
    {
        lvlog(LOGLEVEL_INFO, "\t(stereo)\n");
    }

    snd_pcm_hw_params_get_rate(params, &tmp, 0);
    lvlog(LOGLEVEL_INFO, "rate: %d bps\n", tmp);
    lvlog(LOGLEVEL_INFO, "seconds: %d\n", seconds);

    // /* Allocate buffer to hold single period */
    // snd_pcm_hw_params_get_period_size(params, &frames, 0);

    // buff_size = frames * channels * 2 /* 2 -> sample size */;
    // s->buff = (char *)malloc(buff_size);

    // snd_pcm_hw_params_get_period_time(params, &tmp, NULL);

    // for (loops = (seconds * 1000000) / tmp; loops > 0; loops--)
    // {

    //     if ((pcm = read(0, s->buff, buff_size)) == 0)
    //     {
    //         printf("Early end of file.\n");
    //         return 0;
    //     }

    //     if ((pcm = snd_pcm_writei(s->pcm_handle, s->buff, frames)) == -EPIPE)
    //     {
    //         printf("XRUN.\n");
    //         snd_pcm_prepare(s->pcm_handle);
    //     }
    //     else if (pcm < 0)
    //     {
    //         printf("ERROR. Can't write to PCM device. %s\n", snd_strerror(pcm));
    //     }
    // }

    // Init queue
    s->queue_playidx = 0;
    s->queue_lastidx = 0;

    // Init procedure thread
    for (size_t i = 0; i < MAX_QUEABLE_WAV_SOURCE; i++)
    {
        s->WavData[i].data = NULL;
    }
    pthread_create(&s->hProcThr, NULL, sound_procedure, s);

    // Init output thread
    pthread_create(&s->hOutpThr, NULL, sound_output, s);

    lvlog(LOGLEVEL_INFO, "Sound device has successfully initialized. ... \n");

    return s;
}

void Internal_PInst_DeinitSound(void *hSound)
{
    return NULL;

    lvlog(LOGLEVEL_INFO, "Beginning Sound deinitialization ... \n");
    sound_t *s = hSound;
    if (s == NULL)
        return;

    s->bActive = false;

    pthread_join(s->hProcThr, NULL);
    pthread_join(s->hOutpThr, NULL);

    snd_pcm_drain(s->PCM);
    snd_pcm_close(s->PCM);

    free(hSound);
    lvlog(LOGLEVEL_INFO, "Sound deinitialized. \n");
}

void *Internal_PInst_LoadWav(struct ProgramInstance *Inst, char const *Path)
{
    return NULL;

    // Read wave header
    FILE *fp = fopen(Path, "rb");
    if (fp == NULL) // Not a file.
    {
        lvlog(LOGLEVEL_WARNING, "%s is not a file\n", Path);
        return NULL;
    }

    bool bDATA = false;
    FMT_t f;
    DATA_t d;

    struct chunkhead
    {
        char fmt[4];
        uint32_t sz;
    } head;

    while (true)
    {
        size_t rdcnt = fread(&head, sizeof(head), 1, fp);
        if (rdcnt != 1)
        {
            lvlog("warning: Invalid wave file. %s\n", Path);
            return NULL;
        }

        lvlog(LOGLEVEL_INFO, "Reading chunk [%c%c%c%c] ... %d bytes\n",
              head.fmt[0],
              head.fmt[1],
              head.fmt[2],
              head.fmt[3],
              head.sz);

        if (memcmp(head.fmt, "RIFF", 4) == 0)
        {
            // discard
            head.sz = sizeof(RIFF_t) - sizeof(head);
        }

        if (memcmp(head.fmt, "fmt ", 4) == 0)
        {
            memcpy(&f, &head, sizeof(head));
            void *dst = &f.fmt_chunkSz + 1;
            rdcnt = fread(dst, head.sz, 1, fp);
            uassert(rdcnt == 1);
            continue;
        }

        if (memcmp(head.fmt, "data", 4) == 0)
        {
            memcpy(&d, &head, sizeof(head));
            break;
        }

        // Discard rest of unused chunk data
        fseek(fp, head.sz, SEEK_CUR);
    }

    // Map to system sound format
    float length = d.data_chunkSz / (double)f.fmt_byteRate;
    size_t numSample = length * sizeof(sample_t) * STANDARD_SAMPLE_RATE;
    wav_rsrc_t *v = malloc(numSample * sizeof(sample_t) + sizeof(wav_rsrc_t));
    sample_t *dst = v->ptr; // Mono channel

    // Read and cache
    char *source = malloc(d.data_chunkSz);
    int bytePerSmpl = f.fmt_bitsPerSmpl / 8;
    uint64_t ByteMask = (1ull << f.fmt_bitsPerSmpl) - 1;
    int step = bytePerSmpl * f.fmt_numCh;
    int rd = fread(source, 1, d.data_chunkSz, fp);
    uassert(source && dst && rd == d.data_chunkSz);

    lvlog(LOGLEVEL_INFO, "Beginning mapping operation ... %s\n", Path);

    // Map to system buffer
    uint64_t loc, sum, chval;
    for (size_t i = 0, ch; i < numSample; i++)
    {
        // Map to source index
        loc = i * f.fmt_sampleRate / STANDARD_SAMPLE_RATE;
        char *beg = source + loc * step;

        // Add all values of channel
        sum = 0;

        for (ch = 0; ch < f.fmt_numCh; ch++)
        {
            chval = *(uint64_t *)beg & ByteMask;
            sum += chval;
            beg += bytePerSmpl;
        }

        // Assign to local
        dst[i] = sum > SAMPLE_MAXV ? SAMPLE_MAXV : sum < SAMPLE_MINV ? SAMPLE_MINV : sum;
    }

    free(source);
    fclose(fp);

    lvlog(LOGLEVEL_INFO, "Successfully loaded wave file %s\n", Path);

    return v;
}

EStatus PInst_StopAllSound(UProgramInstance *s)
{
}

void Internal_PInst_PlayWav(void *hSound, void *WavData, float Volume)
{
    return;
    uassert(hSound && WavData);

    sound_t *s = hSound;
    Volume = Volume < 0 ? 0 : Volume > 1 ? 1 : Volume;

    // Find empty queue.
    for (size_t i = 0; i < MAX_QUEABLE_WAV_SOURCE; i++)
    {
        if (s->WavData[i].data == NULL)
        {
            lvlog(LOGLEVEL_DISPLAY, "Queueing wave play for volume %f... \n", Volume);
            s->WavData[i].data = WavData;
            s->WavData[i].volume = (int)(Volume * 256.0f);
            break;
        }
    }
}

static inline size_t next_chunk(size_t i)
{
    static const size_t toAdd[] = {1, 1 - NUM_CHUNKS};
    return i += toAdd[i == NUM_CHUNKS - 1];
}

static void *sound_procedure(void *hSound)
{

    sound_t *s = hSound;
    size_t wav_in_idx = 0;
    static const size_t wav_next_add[2] = {1, 1 - MAX_QUEABLE_WAV_SOURCE};
    size_t queue_check = 0;
    wav_rsrc_t *processing;

    while (s->bActive)
    {
        // Look for new wav to proceed ...
        if ((processing = s->WavData[queue_check].data) == NULL)
        {
            queue_check += wav_next_add[queue_check == MAX_QUEABLE_WAV_SOURCE - 1];
            continue;
        }

        int32_t volume = s->WavData[queue_check].volume;
        s->WavData[queue_check].data = NULL;

        // Process wave ...
        size_t chunkIdx = s->queue_playidx;
        sample_t *chnk = &s->queue[chunkIdx].pcm;
        sample_t *src = processing->ptr;
        size_t i = 0, num = processing->numSamples;

        for (; num--; i++, ++src)
        {
            if (i == 1024)
            {
                if (chunkIdx == s->queue_lastidx)
                    s->queue_lastidx = next_chunk(s->queue_lastidx);
                chunkIdx = next_chunk(chunkIdx);
                chnk = (s->queue + chunkIdx)->pcm;
            }

            int32_t val = ((*src * volume) >> 8) + chnk[i];
            chnk[i] = val > SAMPLE_MAXV ? SAMPLE_MAXV : val < SAMPLE_MINV ? SAMPLE_MINV : val;
        }
    }

    lvlog(LOGLEVEL_INFO, "Destroying sound procedure thread ... \n");
    return NULL;
}

static void *sound_output(void *hSound)
{
    sound_t *s = hSound;

    while (s->bActive)
    {
        if (s->queue_lastidx == s->queue_playidx)
            continue;

        printf("Processing sound ... %d / %d\n", s->queue_playidx, s->queue_lastidx);
        sample_t *rd = s->queue[s->queue_playidx].pcm;
        s->queue_playidx = next_chunk(s->queue_playidx);

        for (int retry = 5; retry; --retry)
        {
            int write = snd_pcm_writei(s->PCM, rd, CHUNK_SIZE);
            if (write == CHUNK_SIZE)
                break;
            else if (write == -EPIPE)
                snd_pcm_prepare(s->PCM);
        }

        memset(rd, 0, CHUNK_SIZE);
    }

    // static int16_t buff[STANDARD_SAMPLE_RATE];
    // for (size_t i = 0; i < STANDARD_SAMPLE_RATE; i++)
    // {
    //     buff[i] = (int16_t)(32767 * sin(i * (M_PI * 2.0 * 440.0 / STANDARD_SAMPLE_RATE)));
    // }

    // int frames;

    // int16_t *head,
    //     *const end = buff + STANDARD_SAMPLE_RATE;

    // printf(" EPIPE %d, EBADFD %d, ESTRPIPE %d\n", EPIPE, EBADFD, ESTRPIPE);
    // while (s->bActive)
    // {
    //     head = buff;
    //     while (s->bActive && head != end)
    //     {
    //         int write = snd_pcm_writei(s->PCM, head, end - head);

    //         if (write > 0)
    //             head += write;
    //         else if (write == -EPIPE)
    //         {
    //             printf("XRUN\n");
    //             snd_pcm_prepare(s->PCM);
    //         }
    //         else
    //         {
    //             printf("SND FAILED %d\n", write);
    //             break;
    //         }
    //     }
    // }

    lvlog(LOGLEVEL_INFO, "Destroying sound output thread ... \n");
    return NULL;
}
