#pragma once

#include <sys/unistd.h>

typedef struct {
    char currentVerBuf[128];
    bool wasPressing;
    uint8_t state;
    net_Response update;
    TopbarState topbarState;
    Model download;
} UpdateState;

void update_init(AppState *appState) {
    UpdateState * updateState = (UpdateState*) malloc(sizeof(UpdateState));
    memset(updateState, 0, sizeof(UpdateState));
    appState->currentView = updateState;
    appState->currentState = STATE_UPDATE;
    updateState->wasPressing = true;
    updateState->update = {.code = -1};
    strncpy(updateState->currentVerBuf, "You are currently running version: " ATTR_VERSION_STR "                   ", 128);
    topbar_init(&updateState->topbarState, appState);
    updateState->topbarState.scrollingText = updateState->currentVerBuf;
    updateState->download = LoadModel("/eboot/assets/update.glb");
}

void update_destroy(AppState *appState) {
    if (appState->currentState != STATE_UPDATE) return;
    appState->currentState = STATE_NONE;
    UpdateState * updateState = (UpdateState*) appState->currentView;
    BeginDrawing();
    ClearBackground(GRAY);
    topbar_draw(&updateState->topbarState);
    EndDrawing();
    net_closeget(&updateState->update);
    topbar_destroy(&updateState->topbarState);
    UnloadModelFull(updateState->download);
    free(updateState);
}

int update_handle(AppState *appState) {
    int rVal = RETURN_STATE_CONTINUE;
    UpdateState * updateState = (UpdateState*) appState->currentView;
    if (updateState->state == 1) {
        updateState->update = net_get("/update", net_timeouts(30));
        updateState->topbarState.scrollingX = -1000;
        if (updateState->update.code == 200) {
            updateState->state = 2;
        } else {
            updateState->state = 3;
        }
    } else if (updateState->state == 0) {
        updateState->state = 1;
    }

    DrawRectangle(0, 0, ATTR_PSP_WIDTH, ATTR_PSP_HEIGHT, GRAY);
    const Camera cam = {
        {0, 0, -10},
        {0, 0, 0},
        {0, 1, 0},
        45,
        CAMERA_PERSPECTIVE
    };
    BeginMode3D(cam);
    DrawModel(updateState->download, {}, 1, WHITE);
    EndMode3D();

    if (updateState->state == 1) {
        const char* txt = "Looking for updates please wait...";
        const float txx = MeasureTextEx(appState->font, txt, 30, 0).x;
        DrawTextEx(appState->font, txt, {ATTR_PSP_WIDTH/2-txx/2, ATTR_PSP_HEIGHT/3*2}, 30, 0, WHITE);
    } else if (updateState->state == 2) {
        if (updateState->update.size == 0) {
            const char* txt = "You are on the latest version!";
            const float txx = MeasureTextEx(appState->font, txt, 30, 0).x;
            DrawTextEx(appState->font, txt, {ATTR_PSP_WIDTH/2-txx/2, ATTR_PSP_HEIGHT/3*2}, 30, 0, GREEN);
        } else {
            const char* txt = "Update downloaded, press X to install";
            const float txx = MeasureTextEx(appState->font, txt, 30, 0).x;
            DrawTextEx(appState->font, txt, {ATTR_PSP_WIDTH/2-txx/2, ATTR_PSP_HEIGHT/3*2}, 30, 0, GREEN);
        }
    } else if (updateState->state == 3) {
        const char* txt = "An error has occurred, try again later";
        const float txx = MeasureTextEx(appState->font, txt, 30, 0).x;
        DrawTextEx(appState->font, txt, {ATTR_PSP_WIDTH/2-txx/2, ATTR_PSP_HEIGHT/3*2}, 30, 0, RED);
    }

    topbar_draw(&updateState->topbarState);
    if (updateState->state >= 2) {
        DrawTriangle({ATTR_PSP_WIDTH-140, ATTR_PSP_HEIGHT}, {ATTR_PSP_WIDTH-130, ATTR_PSP_HEIGHT}, {ATTR_PSP_WIDTH-130, ATTR_PSP_HEIGHT-20}, BLACK);
        DrawRectangle(ATTR_PSP_WIDTH-130, ATTR_PSP_HEIGHT-20, 150, 20, BLACK);
        DrawTextEx(appState->font, "Press O to go back", {ATTR_PSP_WIDTH-125, ATTR_PSP_HEIGHT-20}, 20, 0, WHITE);
        if (appState->controller.o) {
            return RETURN_STATE_NOUPDATE;
        }
    }
    if (!updateState->wasPressing && updateState->state == 2 && updateState->update.size != 0) {
        if (appState->controller.x) {
            updateState->state = 0;
            rVal = RETURN_STATE_REBOOT;
            FILE * eboot = fopen("EBOOT.PBP", "wb");
            fwrite(updateState->update.buffer, 1, updateState->update.size, eboot);
            fclose(eboot);
        }
    }

    if (appState->controller.x) {
        updateState->wasPressing = true;
    } else if (appState->controller.o) {
        updateState->wasPressing = true;
    } else updateState->wasPressing = false;
    return rVal;
}