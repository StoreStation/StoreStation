#include <pspkernel.h>
#include <pspfpu.h>
#include <pspsysmem_kernel.h>

constexpr int ATTR_PSP_WIDTH = 480;
constexpr int ATTR_PSP_HEIGHT = 272;

#if DEBUG == true
#undef ATTR_VERSION_STR
#define ATTR_VERSION_STR "DEV"
#endif
PSP_MODULE_INFO("StoreStation", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(18*1024);
PSP_MAIN_THREAD_STACK_SIZE_KB(1024);

#include <raylib.h>
#include <stdio.h>
#include <malloc.h>
#include <cjson/cJSON.h>

#include "include/control.h"
#include "include/net.h"
#include "include/state.h"
#include "include/utils.h"
#include "include/mp3.h"
#include "include/waves.h"

#include "include/views/widgets/topbar.h"
#include "include/views/widgets/storelist.h"
#include "include/views/widgets/storeinspect.h"
#include "include/views/widgets/buttons.h"

#include "include/views/connect.h"
#include "include/views/home.h"
#include "include/views/update.h"
#include "include/views/apps.h"
#include "include/views/plugins.h"
#include "include/views/featured.h"

int main(int argc, char *argv[]) {
    sceKernelSetCompiledSdkVersion(0x06060010);
    _DisableFPUExceptions();
    pspFpuSetEnable(0);
    scePowerSetClockFrequency(333, 333, 166);
    AppState appState = {.isRunning = 1, .onArk = onArk(), .start = {argc, argv}, .bgm = nullptr, .isRemoteAssetLoaded = false, .remoteAssets = {.buffer = nullptr}};
    SetupCallbacks(&appState);
    PHYSFS_init(argv[0]);
    MountEmbedded(argv[0]);
    if (file_exists((char*) EXTERNAL_ARCHIVE)) PHYSFS_mount(EXTERNAL_ARCHIVE, "/eboot/assets", 0);
    constexpr int screenWidth = ATTR_PSP_WIDTH;
    constexpr int screenHeight = ATTR_PSP_HEIGHT;
    InitWindow(screenWidth, screenHeight, nullptr);
    SetLoadFileDataCallback(PFS_LoadFileDataCallback);
    SetLoadFileTextCallback(PFS_LoadFileTextCallback);
    SetTraceLogCallback(CustomLog);
    appState.font = LoadFontEx("/eboot/assets/opensans.ttf", 25, 0, 270);
    SetTextureFilter(appState.font.texture, TEXTURE_FILTER_BILINEAR);

    connect_init(&appState);

    BeginDrawing(); ClearBackground(BLACK); EndDrawing();
  
    while (appState.isRunning) {
        sceKernelDelayThreadCB(1);
        int retVal;
        BeginDrawing();
        UpdateControls(&appState.controller);
        ClearBackground(BLACK);
        switch (appState.currentState) {
            case STATE_CONNECT: retVal = connect_handle(&appState); break;
            case STATE_HOME: retVal = home_handle(&appState); break;
            case STATE_UPDATE: retVal = update_handle(&appState); break;
            case STATE_APPS: retVal = apps_handle(&appState); break;
            case STATE_PLUGINS: retVal = plugins_handle(&appState); break;
            default: retVal = RETURN_STATE_EXIT;
        }
        DrawDiagnostic(appState);
        EndDrawing();
        switch (retVal) {
            case RETURN_STATE_CONTINUE: break;
            case RETURN_STATE_CONNECTED: { connect_destroy(&appState); home_init(&appState); ((HomeState*) appState.currentView)->introTransitionState = (float)ATTR_PSP_WIDTH; break; }
            case RETURN_STATE_UPDATE: { home_destroy(&appState); update_init(&appState); break; }
            case RETURN_STATE_NOUPDATE: { update_destroy(&appState); home_init(&appState); ((HomeState*) appState.currentView)->selectedItem = 2; ((HomeState*) appState.currentView)->transitionState = ATTR_PSP_WIDTH; break; }
            case RETURN_STATE_APPS: { home_destroy(&appState); apps_init(&appState); break; }
            case RETURN_STATE_APPSHOME: { apps_destroy(&appState); home_init(&appState); ((HomeState*) appState.currentView)->selectedItem = 0; ((HomeState*) appState.currentView)->transitionState = ATTR_PSP_WIDTH; break; }
            case RETURN_STATE_PLUGINS: { home_destroy(&appState); plugins_init(&appState); break; }
            case RETURN_STATE_PLUGINSHOME: { plugins_destroy(&appState); home_init(&appState); ((HomeState*) appState.currentView)->selectedItem = 1; ((HomeState*) appState.currentView)->transitionState = ATTR_PSP_WIDTH; break; }
            case RETURN_STATE_EXIT:
            case RETURN_STATE_REBOOT:
            default: appState.isRunning = 0;
        }
        if (appState.bgm != nullptr && appState.bgm->over) {
            char copy[256];
            strncpy(copy, appState.bgm->source, 255);
            mp3_stopPlayback(appState.bgm);
            TraceLog(LOG_INFO, copy);
            appState.bgm = mp3_beginPlayback(copy);
        }
    }

    connect_destroy(&appState, true);
    home_destroy(&appState);
    update_destroy(&appState);
    apps_destroy(&appState);
    plugins_destroy(&appState);

    if (appState.bgm != nullptr) { mp3_stopPlayback(appState.bgm); appState.bgm = nullptr; }
    ExitApp(appState);
    return 0;
}
