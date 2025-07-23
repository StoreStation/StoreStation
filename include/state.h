#pragma once

#include <stdint.h>
#include <pspjpeg.h>
#include <psputility_modules.h>
#include <physfs.h>
#include <psputility.h>
#include <pspsdk.h>
#include <pspnet.h>
#include <pspnet_inet.h>
#include <pspnet_apctl.h>
#include <psphttp.h>
#include <sys/stat.h>
#include "control.h"

#define INTERNAL_ARCHIVE    "psar.zip"
#define REMOTE_ARCHIVE      "psarOverrideRemoteData.zip"
#define EXTERNAL_ARCHIVE    "psarOverride.zip"

#include "mp3.h"

typedef struct {
    uint8_t isRunning;
    uint8_t onArk;
    int8_t currentState;
    void * currentView;
    Font font;
    Controller controller;
    struct {
        int argc;
        char **argv;
    } start;
    MP3Instance * bgm;
    bool isRemoteAssetLoaded;
    net_Response remoteAssets;
} AppState;

bool file_exists (char *filename) {
  struct stat buffer;   
  return (stat (filename, &buffer) == 0);
}

void ExitApp(AppState &appState) {
    appState.isRunning = false;
    UnloadFont(appState.font);
    CloseWindow();
    if (appState.isRemoteAssetLoaded) {
        PHYSFS_unmount(REMOTE_ARCHIVE);
        net_closeget(&appState.remoteAssets);
    }
    if (file_exists((char*) EXTERNAL_ARCHIVE)) {
        PHYSFS_unmount(EXTERNAL_ARCHIVE);
    }
    PHYSFS_unmount(INTERNAL_ARCHIVE);
    PHYSFS_unmount("ms0:/");
    PHYSFS_deinit();
    sceJpegFinishMJpeg();
    sceHttpEnd();
    sceHttpsEnd();
    sceNetApctlDisconnect();
    sceNetApctlTerm();
    sceNetInetTerm();
    sceNetTerm();
    sceKernelExitGame();
}

int exit_callback(int arg1, int arg2, void* common) {
    ((AppState *) common)->isRunning = false;
    sceKernelSleepThreadCB();
	return 0;
}

int CallbackThread(SceSize args, void* argp) {
	int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, argp);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}

int SetupCallbacks(AppState * appState) {
    sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
    pspAudioInit();
	sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
	sceUtilityLoadNetModule(PSP_NET_MODULE_INET);
	sceUtilityLoadNetModule(PSP_NET_MODULE_PARSEURI);
	sceUtilityLoadNetModule(PSP_NET_MODULE_PARSEHTTP);
	sceUtilityLoadNetModule(PSP_NET_MODULE_HTTP);
	sceUtilityLoadNetModule(PSP_NET_MODULE_SSL);
    sceUtilityLoadModule(PSP_MODULE_AV_AVCODEC);
    sceUtilityLoadModule(PSP_MODULE_AV_MP3);
    sceJpegInitMJpeg();
	int thid = sceKernelCreateThread("update_thread", CallbackThread, 0x18, 0xFA0, 0, 0);
	if (thid >= 0) sceKernelStartThread(thid, sizeof(AppState), appState);
    sceNetInit(128*1024, 42, 4*1024, 42, 4*1024);
	sceNetInetInit();
	sceNetApctlInit(0x8000, 48);
	return thid;
}

#define STATE_NONE                  (-1)
#define STATE_CONNECT               0
#define STATE_HOME                  1
#define STATE_UPDATE                2
#define STATE_APPS                  3
#define STATE_PLUGINS               4

#define RETURN_STATE_CONTINUE       0
#define RETURN_STATE_EXIT           1
#define RETURN_STATE_CONNECTED      2
#define RETURN_STATE_UPDATE         3
#define RETURN_STATE_NOUPDATE       4
#define RETURN_STATE_REBOOT         5
#define RETURN_STATE_APPS           6
#define RETURN_STATE_APPSHOME       7
#define RETURN_STATE_PLUGINS        8
#define RETURN_STATE_PLUGINSHOME    9