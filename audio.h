#ifndef AUDIO_H
#define AUDIO_H

/* Effets sonores synthetises (pas de fichiers : tout est genere). */
typedef enum {
    SFX_SHOOT, SFX_RELOAD, SFX_EMPTY, SFX_HIT, SFX_DIE, SFX_HURT, SFX_PICKUP,
    SFX_FREEZE, SFX_ROLL, SFX_BELL, SFX_WIN, SFX_LOSE, SFX_EXPLODE, SFX_SWITCH, SFX_THROW,
    SFX_COUNT
} SfxId;

void audio_init(void);
void sfx_play(SfxId id);
/* f = 0 nuit, 1 aube : crossfade entre les deux musiques */
void music_set_dawn(float f);

#endif
