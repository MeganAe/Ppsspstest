/* Audio PSP : thread dedie + sceAudioChOutput. SFX = ondes synthetiques, musique = mini-sequenceur. */
#include <string.h>
#include <math.h>
#include <pspkernel.h>
#include <pspaudio.h>

#include "audio.h"
#include "util.h"

#define SAMPLE_RATE 44100
#define CHUNK 256            /* echantillons stereo par bloc */
#define MAX_SFX 16

typedef struct {
    int active;
    SfxId id;
    float t, dur;
    float f0, f1;            /* frequence debut/fin */
    float vol;
    int kind;                /* 0 square, 1 sine, 2 bruit */
    float lp;                /* filtre passe-bas pour le bruit */
} Sfx;

static volatile Sfx sfx_queue[MAX_SFX];
static volatile int sfx_head = 0, sfx_tail = 0;
static volatile int audio_running = 0;
static int audio_ch = -1;
static volatile float dawn_gain = 0.0f;   /* 0 = nuit, 1 = aube */
static float music_pos = 0.0f;             /* position en echantillons dans le pattern */

void sfx_play(SfxId id) {
    int next = (sfx_tail + 1) % MAX_SFX;
    if (next == sfx_head) return;           /* file pleine : on ignore */
    volatile Sfx *s = &sfx_queue[sfx_tail];
    s->active = 1;
    s->id = id;
    s->t = 0;
    switch (id) {
        case SFX_SHOOT:   s->kind = 0; s->f0 = 880; s->f1 = 220; s->dur = 0.07f; s->vol = 0.25f; break;
        case SFX_RELOAD:  s->kind = 0; s->f0 = 300; s->f1 = 500; s->dur = 0.18f; s->vol = 0.18f; break;
        case SFX_EMPTY:   s->kind = 0; s->f0 = 200; s->f1 = 150; s->dur = 0.05f; s->vol = 0.15f; break;
        case SFX_HIT:     s->kind = 2; s->f0 = 0;   s->f1 = 0;   s->dur = 0.06f; s->vol = 0.20f; s->lp = 0.4f; break;
        case SFX_DIE:     s->kind = 0; s->f0 = 500; s->f1 = 60;  s->dur = 0.35f; s->vol = 0.25f; break;
        case SFX_HURT:    s->kind = 0; s->f0 = 160; s->f1 = 90;  s->dur = 0.25f; s->vol = 0.35f; break;
        case SFX_PICKUP:  s->kind = 1; s->f0 = 660; s->f1 = 990; s->dur = 0.18f; s->vol = 0.25f; break;
        case SFX_FREEZE:  s->kind = 1; s->f0 = 1200; s->f1 = 1800; s->dur = 0.5f; s->vol = 0.20f; break;
        case SFX_ROLL:    s->kind = 2; s->f0 = 0;   s->f1 = 0;   s->dur = 0.15f; s->vol = 0.15f; s->lp = 0.2f; break;
        case SFX_BELL:    s->kind = 1; s->f0 = 520; s->f1 = 520; s->dur = 1.2f;  s->vol = 0.30f; break;
        case SFX_WIN:     s->kind = 1; s->f0 = 784; s->f1 = 1175; s->dur = 0.8f;  s->vol = 0.30f; break;
        case SFX_LOSE:    s->kind = 0; s->f0 = 300; s->f1 = 50;  s->dur = 1.0f;  s->vol = 0.30f; break;
        case SFX_EXPLODE: s->kind = 2; s->f0 = 0;   s->f1 = 0;   s->dur = 0.6f;  s->vol = 0.5f;  s->lp = 0.7f; break;
        case SFX_SWITCH:  s->kind = 0; s->f0 = 440; s->f1 = 660; s->dur = 0.08f; s->vol = 0.15f; break;
        case SFX_THROW:   s->kind = 0; s->f0 = 600; s->f1 = 900; s->dur = 0.12f; s->vol = 0.15f; break;
        default:          s->kind = 1; s->f0 = 440; s->f1 = 440; s->dur = 0.1f;  s->vol = 0.2f; break;
    }
    sfx_tail = next;
}

void music_set_dawn(float f) { dawn_gain = f; }

/* --- mini-sequenceur : pattern 32 pas de doubles croches --- */
static float midi2freq(int m) { return 440.0f * powf(2.0f, (m - 69) / 12.0f); }

/* nuit : arpege mineur lent (La mineur) + basse grave */
static const int night_lead[32] = {
    69, -1, 72, -1, 76, -1, 72, -1, 69, -1, 72, -1, 79, -1, 76, -1,
    69, -1, 72, -1, 76, -1, 81, -1, 79, -1, 76, -1, 74, -1, 71, -1
};
static const int night_bass[32] = {
    33, -1, -1, -1, -1, -1, -1, -1, 33, -1, -1, -1, -1, -1, -1, -1,
    36, -1, -1, -1, -1, -1, -1, -1, 31, -1, -1, -1, -1, -1, -1, -1
};
/* aube : arpege majeur plus lumineux */
static const int dawn_lead[32] = {
    72, -1, 76, -1, 79, -1, 84, -1, 81, -1, 84, -1, 86, -1, 84, -1,
    79, -1, 83, -1, 86, -1, 88, -1, 84, -1, 86, -1, 83, -1, 79, -1
};
static const int dawn_bass[32] = {
    36, -1, -1, -1, -1, -1, -1, -1, 41, -1, -1, -1, -1, -1, -1, -1,
    43, -1, -1, -1, -1, -1, -1, -1, 36, -1, -1, -1, -1, -1, -1, -1
};

#define BPM 100
#define STEP_SAMPLES (SAMPLE_RATE * 60 / BPM / 4)   /* double croche */

static float render_sfx(volatile Sfx *s, float t) {
    float dur = s->dur;
    if (t > dur) { s->active = 0; return 0; }
    float k = t / dur;
    float f = s->f0 + (s->f1 - s->f0) * k;
    float env = (1.0f - k);
    env *= env;
    switch (s->kind) {
        case 0: return env * s->vol * (sinf(2 * (float)M_PI * f * t) > 0 ? 0.6f : -0.6f);
        case 1: return env * s->vol * sinf(2 * (float)M_PI * f * t);
        default: {
            float n = (float)rand() / (float)RAND_MAX * 2 - 1;
            s->lp = s->lp * 0.7f + n * 0.3f;
            return env * s->vol * s->lp;
        }
    }
}

static float render_music(void) {
    int step = (int)(music_pos / STEP_SAMPLES) % 32;
    float ph = (music_pos - (float)step * STEP_SAMPLES) / SAMPLE_RATE; /* phase dans le pas */
    float in_step = ph * SAMPLE_RATE / STEP_SAMPLES;
    float g_night = 1.0f - dawn_gain;
    float g_dawn = dawn_gain;
    float out = 0;
    int nl = night_lead[step], nb = night_bass[step];
    int dl = dawn_lead[step], db = dawn_bass[step];
    if (nl >= 0) out += g_night * 0.10f * sinf(2 * (float)M_PI * midi2freq(nl) * ph);
    if (nb >= 0) out += g_night * 0.16f * sinf(2 * (float)M_PI * midi2freq(nb) * ph);
    if (dl >= 0) out += g_dawn * 0.10f * sinf(2 * (float)M_PI * midi2freq(dl) * ph);
    if (db >= 0) out += g_dawn * 0.16f * sinf(2 * (float)M_PI * midi2freq(db) * ph);
    /* cloche au debut de chaque boucle */
    if (step == 0) out += 0.08f * sinf(2 * (float)M_PI * midi2freq(88) * ph) * expf(-in_step * 2);
    music_pos += 1;
    if (music_pos >= 32.0f * STEP_SAMPLES) music_pos = 0;
    return out;
}

static short mixbuf[CHUNK * 2];

static int audio_thread(SceSize args, void *argp) {
    (void)args; (void)argp;
    while (audio_running) {
        for (int i = 0; i < CHUNK; i++) {
            float s = render_music();
            for (int k = 0; k < MAX_SFX; k++) {
                volatile Sfx *fx = &sfx_queue[k];
                if (fx->active) {
                    s += render_sfx(fx, fx->t);
                    fx->t += 1.0f / SAMPLE_RATE;
                }
            }
            if (s > 1) s = 1;
            if (s < -1) s = -1;
            short v = (short)(s * 32767 * 0.8f);
            mixbuf[i * 2] = v;
            mixbuf[i * 2 + 1] = v;
        }
        sceAudioOutputBlocking(audio_ch, PSP_AUDIO_VOLUME_MAX, mixbuf);
    }
    sceKernelExitDeleteThread(0);
    return 0;
}

void audio_init(void) {
    audio_ch = sceAudioChReserve(0, CHUNK, PSP_AUDIO_FORMAT_STEREO);
    if (audio_ch < 0) return;
    audio_running = 1;
    SceUID th = sceKernelCreateThread("audio_thread", audio_thread, 0x30, 0x4000, 0, NULL);
    if (th >= 0) sceKernelStartThread(th, 0, NULL);
}
