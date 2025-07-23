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
} PluginsState;

void plugins_noinspect(AppState * appState) {
    PluginsState * pluginsState = (PluginsState*) appState->currentView;
    if (pluginsState->storeInspectState != nullptr) {
        storeinspect_destroy(pluginsState->storeInspectState);
        free(pluginsState->storeInspectState);
    }
    pluginsState->storeInspectState = nullptr;
}

void plugins_init(AppState * appState) {
    PluginsState * pluginsState = (PluginsState*) malloc(sizeof(PluginsState));
    memset(pluginsState, 0, sizeof(PluginsState));
    appState->currentView = pluginsState;
    appState->currentState = STATE_PLUGINS;
    pluginsState->currentSort = (char*) "new";
    pluginsState->wasPressing = true;
    buttons_init(&pluginsState->buttonsState, appState);
    pluginsState->buttonsState.showBack = true;
    storelist_init(&pluginsState->storelistState, appState, (char*) "plugins", pluginsState->currentSort, 0);
    topbar_init(&pluginsState->topbarState, appState);
    pluginsState->topbarState.currentPage = pluginsState->storelistState.page;
    pluginsState->topbarState.pageCount = pluginsState->storelistState.pageCount;
    if (pluginsState->storelistState.itemCount > 0) pluginsState->buttonsState.showNext = true;
    pluginsState->currentPage = pluginsState->topbarState.currentPage;
}

void plugins_destroy(AppState * appState) {
    if (appState->currentView == nullptr) return;
    if (appState->currentState != STATE_PLUGINS) return;
    PluginsState * pluginsState = (PluginsState*) appState->currentView;
    buttons_destroy(&pluginsState->buttonsState);
    topbar_destroy(&pluginsState->topbarState);
    storelist_destroy(&pluginsState->storelistState);
    plugins_noinspect(appState);
    free(pluginsState);
}

void plugins_changepage(AppState * appState, bool next, int force = -1) {
    PluginsState * pluginsState = (PluginsState*) appState->currentView;
    const int currentPage = pluginsState->storelistState.page;
    int nextPage = currentPage + (next ? 1 : -1);
    if (force == -1) {if (currentPage == -1 || nextPage < 0 || nextPage >= pluginsState->storelistState.pageCount) return;}
    if (force != -1) nextPage = force;
    storelist_destroy(&pluginsState->storelistState);
    topbar_destroy(&pluginsState->topbarState);
    storelist_init(&pluginsState->storelistState, appState, (char*) "plugins", pluginsState->currentSort, nextPage);
    topbar_init(&pluginsState->topbarState, appState);
    if (force == -1) pluginsState->topbarState.currentPage = pluginsState->storelistState.page;
    if (force == -1) pluginsState->topbarState.pageCount = pluginsState->storelistState.pageCount;
    if (pluginsState->storelistState.itemCount > 0) pluginsState->buttonsState.showNext = true; else pluginsState->buttonsState.showNext = false;
    if (force == -1) pluginsState->currentPage = pluginsState->topbarState.currentPage;
}

void plugins_inspect(AppState * appState, char* id) {
    PluginsState * pluginsState = (PluginsState*) appState->currentView;
    plugins_noinspect(appState);
    pluginsState->currentPage = pluginsState->storelistState.page;
    pluginsState->lastSelected = pluginsState->storelistState.selectedItem;
    plugins_changepage(appState, false, pluginsState->storelistState.pageCount + 100);
    pluginsState->storeInspectState = (StoreInspectState*) malloc(sizeof(StoreInspectState));
    storeinspect_init(pluginsState->storeInspectState, appState, (char*) "plugins", id);
    if (pluginsState->storeInspectState->ok && pluginsState->storeInspectState->item._ver) {
        pluginsState->topbarState.scrollingText = (char*) "To install press the START button or use SELECT to download the item without registering in PLUGINS.TXT (can be used to update existing or reinstall)";
    }
}

int plugins_handle(AppState * appState) {
    int rVal = RETURN_STATE_CONTINUE;
    PluginsState * pluginsState = (PluginsState*) appState->currentView;
    DrawRectangle(0, 0, ATTR_PSP_WIDTH, ATTR_PSP_HEIGHT, GRAY);

    if (pluginsState->storeInspectState != nullptr) {
        if (appState->controller.o) {
            if (!pluginsState->wasPressing) {
                plugins_noinspect(appState);
                plugins_changepage(appState, false, pluginsState->currentPage);
                if (pluginsState->lastSelected < pluginsState->storelistState.itemCount) pluginsState->storelistState.selectedItem = pluginsState->lastSelected;
                pluginsState->topbarState.currentPage = pluginsState->storelistState.page;
                pluginsState->topbarState.pageCount = pluginsState->storelistState.pageCount;
            }
            pluginsState->wasPressing = true;
        } else if (appState->controller.start) {
            if (!pluginsState->wasPressing) {
                if (pluginsState->storeInspectState->installable) {
                    if (storeinspect_install(pluginsState->storeInspectState)) {
                        pluginsState->storeInspectState->completed = true;
                    } else {
                        pluginsState->storeInspectState->failed = true;
                    }
                }
            }
            pluginsState->wasPressing = true;
        } else if (appState->controller.select) {
            if (!pluginsState->wasPressing) {
                if (pluginsState->storeInspectState->installable) {
                    pluginsState->storeInspectState->skipRegister = true;
                    if (storeinspect_install(pluginsState->storeInspectState)) {
                        pluginsState->storeInspectState->completed = true;
                    } else {
                        pluginsState->storeInspectState->failed = true;
                    }
                }
            }
            pluginsState->wasPressing = true;
        } else pluginsState->wasPressing = false;
    } else {
        if (appState->controller.o) {
            if (!pluginsState->wasPressing) {
                rVal = RETURN_STATE_PLUGINSHOME;
            }
            pluginsState->wasPressing = true;
        } else if (appState->controller.x) {
            if (!pluginsState->wasPressing) {
                char* id = storelist_id(&pluginsState->storelistState);
                if (id != nullptr) {
                    char idCopy[strlen(id)+1];
                    memcpy(idCopy, id, strlen(id));
                    idCopy[strlen(id)] = 0;
                    plugins_inspect(appState, idCopy);
                }
            }
            pluginsState->wasPressing = true;
        } else if (appState->controller.up) {
            if (!pluginsState->wasPressing) {
                storelist_up(&pluginsState->storelistState);
            }
            pluginsState->wasPressing = true;
        } else if (appState->controller.down) {
            if (!pluginsState->wasPressing) {
                storelist_down(&pluginsState->storelistState);
            }
            pluginsState->wasPressing = true;
        } else if (appState->controller.left || appState->controller.ltrigger) {
            if (!pluginsState->wasPressing) {
                plugins_changepage(appState, false);
            }
            pluginsState->wasPressing = true;
        } else if (appState->controller.right || appState->controller.rtrigger) {
            if (!pluginsState->wasPressing) {
                plugins_changepage(appState, true);
            }
            pluginsState->wasPressing = true;
        } else {
            pluginsState->wasPressing = false;
        }
    }

    pluginsState->wasPressing = storelist_draw(&pluginsState->storelistState) || pluginsState->wasPressing;
    storeinspect_draw(pluginsState->storeInspectState);
    topbar_draw(&pluginsState->topbarState);
    buttons_draw(&pluginsState->buttonsState);
    return rVal;
}
