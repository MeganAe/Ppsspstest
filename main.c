/* La Nuit des Citrouilles — Edition PSP (PPSSPP).
   Point d'entree : boucle principale 60 Hz. */
#include <pspkernel.h>
#include <pspdisplay.h>

#include "game.h"
#include "input.h"
#include "render.h"
#include "audio.h"

PSP_MODULE_INFO("CitroPSP", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static volatile int running = 1;

static int exit_callback(int arg1, int arg2, void *common) {
    (void)arg1; (void)arg2; (void)common;
    running = 0;
    return 0;
}

static int callback_thread(SceSize args, void *argp) {
    (void)args; (void)argp;
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

static void setup_callbacks(void) {
    int thid = sceKernelCreateThread("update_thread", callback_thread, 0x11, 0xFA0, 0, NULL);
    if (thid >= 0)
        sceKernelStartThread(thid, 0, NULL);
}

int main(void) {
    setup_callbacks();
    input_init();
    audio_init();
    render_init();

    Game g;
    game_init(&g);

    Input in;
    memset(&in, 0, sizeof(in));

    while (running) {
        input_poll(&in);
        game_update(&g, &in, 1.0f / 60.0f);
        render_frame(&g);
        sceDisplayWaitVblankStart();
    }

    sceKernelExitGame();
    return 0;
}
