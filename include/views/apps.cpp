#pragma once

typedef struct {
    TopbarState topbarState;
} AppsState;

void apps_init(AppState * appState) {
    AppsState * appsState = (AppsState *) malloc(sizeof(AppsState));
    appState->currentView = appsState;
    appState->currentState = STATE_APPS;
    topbar_init(&appsState->topbarState, appState);
}

void apps_destroy(AppState * appState) {
    if (appState->currentState != STATE_APPS) return;
    AppsState * appsState = (AppsState *) appState->currentView;
    topbar_destroy(&appsState->topbarState);
    free(appsState);
}

int apps_handle(AppState * appState) {
    AppsState * appsState = (AppsState *) appState->currentView;
    DrawRectangle(0, 0, ATTR_PSP_WIDTH, ATTR_PSP_HEIGHT, GRAY);

    topbar_draw(&appsState->topbarState);
    return RETURN_STATE_APPSHOME;
}