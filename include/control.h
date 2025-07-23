#pragma once

#include <pspkernel.h>
#include <pspctrl.h>
#include <stdint.h>
#include <string.h>
#include <pspaudio.h>
#include <pspaudiolib.h>

typedef struct {
    uint8_t x, o, s, t;
    uint8_t up, down, left, right;
    uint8_t start, select, ltrigger, rtrigger;
} Controller;

void UpdateControls(Controller *ret) {
    SceCtrlData pad;
    memset(ret, 0, sizeof(Controller));
    sceCtrlReadBufferPositive(&pad, 1);
    if (pad.Buttons != 0) {
        ret->s = (pad.Buttons & PSP_CTRL_SQUARE) == 0 ? 0 : 1;
        ret->t = (pad.Buttons & PSP_CTRL_TRIANGLE) == 0 ? 0 : 1;
        ret->o = (pad.Buttons & PSP_CTRL_CIRCLE) == 0 ? 0 : 1;
        ret->x = (pad.Buttons & PSP_CTRL_CROSS) == 0 ? 0 : 1;
        ret->up = (pad.Buttons & PSP_CTRL_UP) == 0 ? 0 : 1;
        ret->down = (pad.Buttons & PSP_CTRL_DOWN) == 0 ? 0 : 1;
        ret->left = (pad.Buttons & PSP_CTRL_LEFT) == 0 ? 0 : 1;
        ret->right = (pad.Buttons & PSP_CTRL_RIGHT) == 0 ? 0 : 1;
        ret->start = (pad.Buttons & PSP_CTRL_START) == 0 ? 0 : 1;
        ret->select = (pad.Buttons & PSP_CTRL_SELECT) == 0 ? 0 : 1;
        ret->ltrigger = (pad.Buttons & PSP_CTRL_LTRIGGER) == 0 ? 0 : 1;
        ret->rtrigger = (pad.Buttons & PSP_CTRL_RTRIGGER) == 0 ? 0 : 1;
    }
}
