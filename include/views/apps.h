#pragma once

typedef struct {
    TopbarState topbarState;
    StorelistState storelistState;
    StoreInspectState *storeInspectState;
    ButtonsState buttonsState;
    char* currentSort;
    int currentPage;
    int lastSelected;
    bool wasPressing;
} AppsState;

void apps_noinspect(AppState * appState) {
    AppsState * appsState = (AppsState*) appState->currentView;
    if (appsState->storeInspectState != nullptr) {
        storeinspect_destroy(appsState->storeInspectState);
        free(appsState->storeInspectState);
    }
    appsState->storeInspectState = nullptr;
}

void apps_init(AppState * appState) {
    AppsState * appsState = (AppsState*) malloc(sizeof(AppsState));
    memset(appsState, 0, sizeof(AppsState));
    appState->currentView = appsState;
    appState->currentState = STATE_APPS;
    appsState->currentSort = (char*) "new";
    appsState->wasPressing = true;
    buttons_init(&appsState->buttonsState, appState);
    appsState->buttonsState.showBack = true;
    storelist_init(&appsState->storelistState, appState, (char*) "apps", appsState->currentSort, 0);
    topbar_init(&appsState->topbarState, appState);
    appsState->topbarState.currentPage = appsState->storelistState.page;
    appsState->topbarState.pageCount = appsState->storelistState.pageCount;
    if (appsState->storelistState.itemCount > 0) appsState->buttonsState.showNext = true;
    appsState->currentPage = appsState->topbarState.currentPage;
}

void apps_destroy(AppState * appState) {
    if (appState->currentView == nullptr) return;
    if (appState->currentState != STATE_APPS) return;
    AppsState * appsState = (AppsState*) appState->currentView;
    buttons_destroy(&appsState->buttonsState);
    topbar_destroy(&appsState->topbarState);
    storelist_destroy(&appsState->storelistState);
    apps_noinspect(appState);
    free(appsState);
}

void apps_changepage(AppState * appState, bool next, int force = -1) {
    AppsState * appsState = (AppsState*) appState->currentView;
    const int currentPage = appsState->storelistState.page;
    int nextPage = currentPage + (next ? 1 : -1);
    if (force == -1) {if (currentPage == -1 || nextPage < 0 || nextPage >= appsState->storelistState.pageCount) return;}
    if (force != -1) nextPage = force;
    storelist_destroy(&appsState->storelistState);
    topbar_destroy(&appsState->topbarState);
    storelist_init(&appsState->storelistState, appState, (char*) "apps", appsState->currentSort, nextPage);
    topbar_init(&appsState->topbarState, appState);
    if (force == -1) appsState->topbarState.currentPage = appsState->storelistState.page;
    if (force == -1) appsState->topbarState.pageCount = appsState->storelistState.pageCount;
    if (appsState->storelistState.itemCount > 0) appsState->buttonsState.showNext = true; else appsState->buttonsState.showNext = false;
    if (force == -1) appsState->currentPage = appsState->topbarState.currentPage;
}

void apps_inspect(AppState * appState, char* id) {
    AppsState * appsState = (AppsState*) appState->currentView;
    apps_noinspect(appState);
    appsState->currentPage = appsState->storelistState.page;
    appsState->lastSelected = appsState->storelistState.selectedItem;
    apps_changepage(appState, false, appsState->storelistState.pageCount + 100);
    appsState->storeInspectState = (StoreInspectState*) malloc(sizeof(StoreInspectState));
    storeinspect_init(appsState->storeInspectState, appState, (char*) "apps", id);
    if (appsState->storeInspectState->ok && appsState->storeInspectState->item._ver) {
        appsState->topbarState.scrollingText = (char*) "To install press the START button";
        appsState->topbarState.doScroll = false;
    }
}

int apps_handle(AppState * appState) {
    int rVal = RETURN_STATE_CONTINUE;
    AppsState * appsState = (AppsState*) appState->currentView;
    DrawRectangle(0, 0, ATTR_PSP_WIDTH, ATTR_PSP_HEIGHT, GRAY);

    if (appsState->storeInspectState != nullptr) {
        if (appState->controller.o) {
            if (!appsState->wasPressing) {
                apps_noinspect(appState);
                apps_changepage(appState, false, appsState->currentPage);
                if (appsState->lastSelected < appsState->storelistState.itemCount) appsState->storelistState.selectedItem = appsState->lastSelected;
                appsState->topbarState.currentPage = appsState->storelistState.page;
                appsState->topbarState.pageCount = appsState->storelistState.pageCount;
            }
            appsState->wasPressing = true;
        } else if (appState->controller.start) {
            if (!appsState->wasPressing) {
                if (appsState->storeInspectState->installable) {
                    if (storeinspect_install(appsState->storeInspectState)) {
                        appsState->storeInspectState->completed = true;
                    } else {
                        appsState->storeInspectState->failed = true;
                    }
                }
            }
            appsState->wasPressing = true;
        } else if (appState->controller.select) {
            if (!appsState->wasPressing) {
                if (appsState->storeInspectState->installable) {
                    appsState->storeInspectState->skipRegister = true;
                    if (storeinspect_install(appsState->storeInspectState)) {
                        appsState->storeInspectState->completed = true;
                    } else {
                        appsState->storeInspectState->failed = true;
                    }
                }
            }
            appsState->wasPressing = true;
        } else appsState->wasPressing = false;
    } else {
        if (appState->controller.o) {
            if (!appsState->wasPressing) {
                rVal = RETURN_STATE_APPSHOME;
            }
            appsState->wasPressing = true;
        } else if (appState->controller.x) {
            if (!appsState->wasPressing) {
                char* id = storelist_id(&appsState->storelistState);
                if (id != nullptr) {
                    char idCopy[strlen(id)+1];
                    memcpy(idCopy, id, strlen(id));
                    idCopy[strlen(id)] = 0;
                    apps_inspect(appState, idCopy);
                }
            }
            appsState->wasPressing = true;
        } else if (appState->controller.up) {
            if (!appsState->wasPressing) {
                storelist_up(&appsState->storelistState);
            }
            appsState->wasPressing = true;
        } else if (appState->controller.down) {
            if (!appsState->wasPressing) {
                storelist_down(&appsState->storelistState);
            }
            appsState->wasPressing = true;
        } else if (appState->controller.left || appState->controller.ltrigger) {
            if (!appsState->wasPressing) {
                apps_changepage(appState, false);
            }
            appsState->wasPressing = true;
        } else if (appState->controller.right || appState->controller.rtrigger) {
            if (!appsState->wasPressing) {
                apps_changepage(appState, true);
            }
            appsState->wasPressing = true;
        } else {
            appsState->wasPressing = false;
        }
    }

    appsState->wasPressing = storelist_draw(&appsState->storelistState) || appsState->wasPressing;
    storeinspect_draw(appsState->storeInspectState);
    topbar_draw(&appsState->topbarState);
    buttons_draw(&appsState->buttonsState);
    return rVal;
}
