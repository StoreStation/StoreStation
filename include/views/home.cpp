#pragma once

typedef struct {
    Model plugins;
    Model apps;
    Model update;
    int selectedItem;
    float hover;
    float rot;
    float nextWaveIn;
    volatile float transitionState;
    volatile float introTransitionState;
    Waves waves;
    bool wasPressing;
    bool isSubmenu;
    TopbarState topbarState;
    ButtonsState buttonsState;
    net_SimpleResponse motd;
} HomeState;

const char* home_options[] = {
    "Unavailable",
    "Plugins",
    "Update",
};

void home_init(AppState *appState) {
    HomeState * homeState = (HomeState *) malloc(sizeof(HomeState));
    *homeState = {};
    appState->currentView = homeState;
    appState->currentState = STATE_HOME;
    homeState->wasPressing = true;
    homeState->waves = {};
    homeState->nextWaveIn = 0;
    homeState->transitionState = -1000;
    homeState->introTransitionState = 0;
    homeState->selectedItem = 1;
    homeState->plugins = LoadModel("/eboot/assets/plugins.glb");
    homeState->apps = LoadModel("/eboot/assets/apps.glb");
    homeState->update = LoadModel("/eboot/assets/update.glb");
    homeState->rot = 360;
    homeState->hover = GetTime();
    buttons_init(&homeState->buttonsState, appState);
    homeState->buttonsState.showNext = true;
    topbar_init(&homeState->topbarState, appState);
    homeState->motd = net_simpleget("/ping/motd", net_timeouts(2));
    if (homeState->motd.code == 200 && homeState->motd.size != 0) homeState->topbarState.scrollingText = homeState->motd.buffer;
}

void home_destroy(AppState *appState) {
    if (appState->currentState != STATE_HOME) return;
    HomeState * homeState = (HomeState *) appState->currentView;
    buttons_destroy(&homeState->buttonsState);
    topbar_destroy(&homeState->topbarState);
    appState->currentState = STATE_NONE;
    UnloadModelFull(homeState->plugins);
    UnloadModelFull(homeState->apps);
    UnloadModelFull(homeState->update);
    free(homeState);
}

float home_rotation(float in) {
    return in >= 360 ? 0 : in;
}

float home_hover(float in) {
    return (-pspFpuCos((GetTime()-in)*3.f))/4.f+1.f/4.f;
}

int home_handle(AppState *appState) {
    int rVal = RETURN_STATE_CONTINUE;
    HomeState * homeState = (HomeState *) appState->currentView;
    DrawRectangle(0, 0, ATTR_PSP_WIDTH, ATTR_PSP_HEIGHT, ORANGE);
    if (homeState->nextWaveIn <= 0) {
        Waves_lineWorker(&homeState->waves);
        homeState->nextWaveIn = 50;
    }
    homeState->nextWaveIn -= GetFrameTime() * 1000;
    homeState->rot += GetFrameTime() * 50;
    if (homeState->rot >= 360*1.25f) {
        homeState->rot -= 360*1.25f;
    }

    if (appState->controller.left) {
        if (!homeState->wasPressing) {
            homeState->rot = 0;
            homeState->hover = GetTime();
            homeState->selectedItem = homeState->selectedItem - 1;
            if (homeState->selectedItem < 0) {
                homeState->selectedItem = sizeof(home_options)/sizeof(home_options[0]) - 1;
            }
        }
        homeState->wasPressing = true;
    } else if (appState->controller.right) {
        if (!homeState->wasPressing) {
            homeState->rot = 0;
            homeState->hover = GetTime();
            homeState->selectedItem = (homeState->selectedItem + 1) % (sizeof(home_options)/sizeof(home_options[0]));
        }
        homeState->wasPressing = true;
    } else if (appState->controller.x) {
        if (!homeState->wasPressing) {
            homeState->transitionState = -128;
            homeState->isSubmenu = true;
        }
    } else if (appState->controller.select) {
        if (!homeState->wasPressing) {
            rVal = RETURN_STATE_EXIT;
        }
        homeState->wasPressing = true;
    } else {
        homeState->wasPressing = false;
    }
    if (homeState->transitionState != -1000) {
        homeState->wasPressing = true;
        if (homeState->isSubmenu) {
            homeState->transitionState+=GetFrameTime()*1500;
            if (homeState->transitionState >= ATTR_PSP_WIDTH*1.25f) {
                switch (homeState->selectedItem) {
                case 0: {
                    rVal = RETURN_STATE_APPS;
                } break;
                case 1: {
                    rVal = RETURN_STATE_PLUGINS;
                } break;
                case 2: {
                    rVal = RETURN_STATE_UPDATE;
                } break;
                default: break;
            }
            }
        } else {
            homeState->transitionState-=GetFrameTime()*1500;
            if (homeState->transitionState < -128) {
                homeState->transitionState = -1000;
            }
        }
    }
    if (homeState->introTransitionState > 0) {
        homeState->wasPressing = true;
        homeState->introTransitionState-=GetFrameTime()*500;
    }

    Waves_draw(&homeState->waves);
    DrawRectangle(0, 0, ATTR_PSP_WIDTH, ATTR_PSP_HEIGHT, {255, 255, 255, 32});
    BeginMode3D({
        {0, 0, -10},
        {0, 0, 0},
        {0, 1, 0},
        45,
        CAMERA_PERSPECTIVE,
    });
    constexpr float fl = 0;
    DrawModelEx(homeState->apps, {5, homeState->selectedItem != 0 ? fl : fl+home_hover(homeState->hover), 0}, {0, 1, 0}, homeState->selectedItem != 0 ? 180 : 180+home_rotation(homeState->rot), {1, 1, 1}, {255, 255, 255, 90});
    DrawModelEx(homeState->plugins, {0, homeState->selectedItem != 1 ? fl : fl+home_hover(homeState->hover), 0}, {0, 1, 0}, homeState->selectedItem != 1 ? 0 : home_rotation(homeState->rot), {1, 1, 1}, {255, 255, 255, 220});
    DrawModelEx(homeState->update, {-5, homeState->selectedItem != 2 ? fl : fl+home_hover(homeState->hover), 0}, {0, 1, 0}, homeState->selectedItem != 2 ? 0 : home_rotation(homeState->rot), {1, 1, 1}, {255, 255, 255, 220});
    EndMode3D();

    const char* title = home_options[homeState->selectedItem % (sizeof(home_options)/sizeof(home_options[0]))];
    const float titlex = MeasureTextEx(appState->font, title, 30, 0).x;
    Vector2 tp = {ATTR_PSP_WIDTH/2-titlex/2, ATTR_PSP_HEIGHT/2};
    if (homeState->selectedItem == 0) {
        tp = {ATTR_PSP_WIDTH/6-titlex/2, ATTR_PSP_HEIGHT/2};
    } else if (homeState->selectedItem == 2) {
        tp = {ATTR_PSP_WIDTH/6*5-titlex/2, ATTR_PSP_HEIGHT/2};
    }
    tp.y += 60;
    DrawTextEx(appState->font, title, {tp.x+1, tp.y+1}, 30, 0, ColorAlpha(GRAY, 0.8));
    DrawTextEx(appState->font, title, tp, 30, 0, WHITE);

    buttons_draw(&homeState->buttonsState);
    if (homeState->isSubmenu) {
        DrawTriangle({homeState->transitionState-2, 0}, {homeState->transitionState-2, ATTR_PSP_HEIGHT}, {homeState->transitionState+128, 0}, GRAY);
        DrawRectangle(0, 0, homeState->transitionState, ATTR_PSP_HEIGHT, GRAY);
    } else {
        DrawTriangle({ATTR_PSP_WIDTH - homeState->transitionState + 2, 0}, {ATTR_PSP_WIDTH - homeState->transitionState - 128, 0}, {ATTR_PSP_WIDTH - homeState->transitionState + 2, ATTR_PSP_HEIGHT}, GRAY);
        DrawRectangle(ATTR_PSP_WIDTH - homeState->transitionState, 0, homeState->transitionState+8, ATTR_PSP_HEIGHT, GRAY);
    }

    topbar_draw(&homeState->topbarState);
    if (homeState->introTransitionState > 0) DrawCircleV({ATTR_PSP_WIDTH/2, ATTR_PSP_HEIGHT/2}, homeState->introTransitionState, BLACK);
    return rVal;
}
