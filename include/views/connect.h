#pragma once

#include <psppower.h>

typedef struct {
    pspUtilityNetconfData data;
    pspUtilityNetconfAdhoc adhocparam;
    Model bag;
    Texture noInternet;
    int done;
    float rot;
    float check;
    char msgBuf[1024];
    MP3Instance *mp3Inst;
} ConnectState;

void connect_init(AppState *appState) {
    ConnectState *connectState = (ConnectState *) malloc(sizeof(ConnectState));
    memset(connectState, 0, sizeof(ConnectState));
    appState->currentState = STATE_CONNECT;
    appState->currentView = connectState;
    connectState->rot = 180;
    connectState->check = 0.5f;

    if (PHYSFS_exists("/eboot/assets/loading_begin.mp3")) connectState->mp3Inst = mp3_beginPlayback((char*) "/eboot/assets/loading_begin.mp3");

	connectState->data.base.size = sizeof(pspUtilityNetconfData);
	connectState->data.base.language = PSP_SYSTEMPARAM_LANGUAGE_ENGLISH;
	connectState->data.base.buttonSwap = PSP_UTILITY_ACCEPT_CROSS;
	connectState->data.base.graphicsThread = 17;
	connectState->data.base.accessThread = 19;
	connectState->data.base.fontThread = 18;
	connectState->data.base.soundThread = 16;
	connectState->data.action = PSP_NETCONF_ACTION_CONNECTAP;
	connectState->data.adhocparam = &connectState->adhocparam;

    sceUtilityNetconfInitStart(&connectState->data);

    connectState->bag = LoadModel("/eboot/assets/bag.glb");
    connectState->noInternet = LoadTexture("/eboot/assets/nointernet.png");

    TraceLog(LOG_INFO, "Are we on ark? %s", appState->onArk ? "Yes" : "No");
}

void connect_destroy(AppState *appState, bool exiting = false) {
    if (appState->currentState != STATE_CONNECT) return;
    appState->currentState = STATE_NONE;
    ConnectState *connectState = (ConnectState *) appState->currentView;
    UnloadModelFull(connectState->bag);
    UnloadTexture(connectState->noInternet);
    mp3_stopPlayback(connectState->mp3Inst);
    free(appState->currentView);
    if (!exiting) {
        net_Response resp = net_get("/ping/assets");
        if (resp.code == 200 && resp.size > 0) {
            appState->isRemoteAssetLoaded = true;
            appState->remoteAssets = resp;
            PHYSFS_mountMemory(appState->remoteAssets.buffer, appState->remoteAssets.size, nullptr, REMOTE_ARCHIVE, "/eboot/assets", 0);
        } else {
            net_closeget(&resp);
        }
        if (PHYSFS_exists("/eboot/assets/store_loop.mp3")) appState->bgm = mp3_beginPlayback((char*) "/eboot/assets/store_loop.mp3");
    }
}

int connect_handle(AppState *appState) {
    ConnectState *connectState = (ConnectState *) appState->currentView;
    
    connectState->rot += GetFrameTime()*100;
    if (connectState->rot >= 360) connectState->rot -= 360;
    if (connectState->done == 1) {
        if (connectState->check >= 2) {
            mp3_stopPlayback(connectState->mp3Inst);
            if (PHYSFS_exists("/eboot/assets/loading_end.mp3")) connectState->mp3Inst = mp3_beginPlayback((char*) "/eboot/assets/loading_end.mp3");
            sceHttpInit(20000);
            connectState->check = 0;
            net_SimpleResponse res = net_simpleget("/ping");
            if (res.code != 200) {
                TraceLog(LOG_ERROR, "Unable to ping");
                connectState->done = 3;
            } else if (strcmp("pong", res.buffer) != 0) {
                TraceLog(LOG_ERROR, "Invalid ping response");
                strncpy(connectState->msgBuf, res.buffer, sizeof(connectState->msgBuf)-1);
                connectState->done = 3;
            } else {
                TraceLog(LOG_INFO, "Ping succeeded");
                connectState->done = 2;
            }
        } else if (connectState->check >= 0) connectState->check += GetFrameTime();
    } else if (connectState->done == 2) {
        connectState->check += GetFrameTime()/2;
        if (connectState->check >= 1) connectState->check = 1;
    }

    const Camera cam = {
        {0, 0, -10},
        {0, 0, 0},
        {0, 1, 0},
        45,
        CAMERA_PERSPECTIVE,
    };
    BeginMode3D(cam);
    Vector3 zoom = {.x = 1 + ((connectState->done == 2) ? connectState->check : 0) * 2};
    zoom.y = zoom.x;
    zoom.z = zoom.x;
    DrawModelEx(connectState->bag, {0, 1, 0}, {0, 1, 0}, -connectState->rot, zoom, WHITE);
    EndMode3D();

    if (connectState->done < 2 && connectState->mp3Inst != nullptr && connectState->mp3Inst->over) {
        mp3_stopPlayback(connectState->mp3Inst);
        if (PHYSFS_exists("/eboot/assets/loading_begin.mp3")) connectState->mp3Inst = mp3_beginPlayback((char*) "/eboot/assets/loading_begin.mp3");
    }

    const auto status = sceUtilityNetconfGetStatus();
    if (connectState->done == 0) {
        switch(status) {
            case PSP_UTILITY_DIALOG_NONE: break;
            case PSP_UTILITY_DIALOG_VISIBLE: 
                sceUtilityNetconfUpdate(1);
                break;
            case PSP_UTILITY_DIALOG_QUIT:
                sceUtilityNetconfShutdownStart();
                break;
            case PSP_UTILITY_DIALOG_FINISHED: {
                    connectState->done = 1;
                    connectState->check = 0;
                    sceCtrlSetSamplingCycle(0);
	                sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
                    break;
                }
            default: break;
        }
    }

    const char* connectText = "Connecting to the internet...";
    if (connectState->done != 2) DrawTextEx(appState->font, connectText, {ATTR_PSP_WIDTH/2-MeasureTextEx(appState->font, connectText, 25, 0).x/2, ATTR_PSP_HEIGHT/2+25}, 25, 0, WHITE);

    if (connectState->done == 0 || connectState->done == 2) {
        DrawRectangle(0, 0, ATTR_PSP_WIDTH, ATTR_PSP_HEIGHT, ColorAlpha(BLACK, connectState->check));
        if (connectState->check >= 1) {
            return RETURN_STATE_CONNECTED;
        }
    } else if (connectState->done == 3) {
        const char* connectionErrTitle = "Connection Failed";
        const char* connectionErrText1 = "The Store was unable to communicate with the server,";
        const char* connectionErrText2 = "please restart the app and try again later";
        const char* connectionErrExit = "Press X to exit";
        DrawRectangle(75, 75, ATTR_PSP_WIDTH-75*2, ATTR_PSP_HEIGHT-75*2, {20, 20, 20, 250});
        DrawTexture(connectState->noInternet, ATTR_PSP_WIDTH/2-connectState->noInternet.width/2, 80, WHITE);
        const float ttx = MeasureTextEx(appState->font, connectionErrTitle, 25, 0).x;
        const float t1x = MeasureTextEx(appState->font, connectionErrText1, 18, 0).x;
        const float t2x = MeasureTextEx(appState->font, connectionErrText2, 18, 0).x;
        const float tex = MeasureTextEx(appState->font, connectionErrExit, 18, 0).x;
        DrawTextEx(appState->font, connectionErrTitle, {ATTR_PSP_WIDTH/2-ttx/2, 115}, 25, 0, RED);
        if (connectState->msgBuf[0] == 0) {
            DrawTextEx(appState->font, connectionErrText1, {ATTR_PSP_WIDTH/2-t1x/2, 135}, 18, 0, WHITE);
            DrawTextEx(appState->font, connectionErrText2, {ATTR_PSP_WIDTH/2-t2x/2, 135+16}, 18, 0, WHITE);
        } else {
            const char* connectionSrvTitle = "The server returned an error:";
            const float tmt = MeasureTextEx(appState->font, connectionSrvTitle, 18, 0).x;
            const float tst = MeasureTextEx(appState->font, connectState->msgBuf, 18, 0).x;
            DrawTextEx(appState->font, connectionSrvTitle, {ATTR_PSP_WIDTH/2-tmt/2, 135}, 18, 0, WHITE);
            DrawTextEx(appState->font, connectState->msgBuf, {ATTR_PSP_WIDTH/2-tst/2, 135+16}, 18, 0, WHITE);
        }
        DrawTextEx(appState->font, connectionErrExit, {ATTR_PSP_WIDTH/2-tex/2, 135+40}, 18, 0, {255, 100, 100, 255});
        if (appState->controller.x) {
            return RETURN_STATE_EXIT;
        }
    }
	return RETURN_STATE_CONTINUE;
}