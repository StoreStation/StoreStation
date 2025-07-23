#pragma once

typedef struct {
    AppState *appState;
    Texture back, next;
    bool showBack, showNext;
} ButtonsState;

void buttons_init(ButtonsState *buttonsState, AppState *appState) {
    *buttonsState = {};
    buttonsState->appState = appState;
    buttonsState->back = LoadTexture("/eboot/assets/backbutton.png");
    SetTextureFilter(buttonsState->back, TEXTURE_FILTER_BILINEAR);
    buttonsState->next = LoadTexture("/eboot/assets/nextbutton.png");
    SetTextureFilter(buttonsState->next, TEXTURE_FILTER_BILINEAR);
    buttonsState->showBack = false;
    buttonsState->showNext = false;
}

void buttons_destroy(ButtonsState *buttonsState) {
    UnloadTexture(buttonsState->back);
    UnloadTexture(buttonsState->next);
}

void buttons_draw(ButtonsState *buttonsState) {
    if (buttonsState->showBack) {
        DrawTextureEx(buttonsState->back, {0, (float) (ATTR_PSP_HEIGHT-buttonsState->back.height*0.35f)}, 0, 0.35f, WHITE);
        DrawTextEx(buttonsState->appState->font, "o", {2, (float) (ATTR_PSP_HEIGHT-13)}, 15, 0, GRAY);
    }
    if (buttonsState->showNext) {
        DrawTextureEx(buttonsState->next, {(float) (ATTR_PSP_WIDTH-buttonsState->back.width*0.35f), (float) (ATTR_PSP_HEIGHT-buttonsState->back.height*0.35f)}, 0, 0.35f, WHITE);
        DrawTextEx(buttonsState->appState->font, "x", {(float) (ATTR_PSP_WIDTH-8), (float) (ATTR_PSP_HEIGHT-13)}, 15, 0, GRAY);
    }
}
