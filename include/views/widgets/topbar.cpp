#pragma once

typedef struct {
    AppState *appState;
    Texture signal;
    char* scrollingText;
    bool doScroll;
    float scrollingX;
    int pageCount;
    int currentPage;
} TopbarState;

void topbar_init(TopbarState *topbarState, AppState *appState) {
    *topbarState = {};
    topbarState->scrollingText = nullptr;
    topbarState->currentPage = -1;
    topbarState->appState = appState;
    topbarState->signal = LoadTexture("/eboot/assets/signal.png");
    topbarState->scrollingX = 40;
    topbarState->doScroll = true;
}

void topbar_destroy(TopbarState *topbarState) {
    UnloadTexture(topbarState->signal);
}

void topbar_draw(TopbarState *topbarState) {
    SceNetApctlInfo apctl;
    if (sceNetApctlGetInfo(PSP_NET_APCTL_INFO_STRENGTH, &apctl) < 0) apctl.strength = 0;
    if (topbarState->scrollingText != nullptr && topbarState->doScroll) {
        topbarState->scrollingX -= GetFrameTime() * 20;
        if (MeasureTextEx(topbarState->appState->font, topbarState->scrollingText, 17, 0).x+50 < -topbarState->scrollingX) {
            topbarState->scrollingX = 40;
        }
    }

    constexpr Color c0 = {0x00, 0x00, 0x00, 85};
    constexpr Color c0b = {0x00, 0x00, 0x00, 110};
    constexpr Color c1 = {0xB0, 0xAF, 0xAF, 255};
    constexpr Color c2 = {0x8E, 0x8D, 0x8D, 255};
    constexpr int ys = 32;
    constexpr int xs = 64;
    DrawRectangle(xs, ys+16, ATTR_PSP_WIDTH, 4, c0);
    DrawRectangle(0, 0, ATTR_PSP_WIDTH, ys+16, c1);
    DrawRectangle(0, 0, ATTR_PSP_WIDTH, 16, {0, 0, 0, 180});
    if (topbarState->scrollingText != nullptr) {
        DrawTextEx(topbarState->appState->font, topbarState->scrollingText, {xs+20+(topbarState->scrollingX >= 0 ? 0 : topbarState->scrollingX), 24}, 17, 0, WHITE);
    }
    if (topbarState->currentPage != -1) {
        constexpr Color rc = {110, 110, 110, 255};
        if (topbarState->currentPage != 0) {
            DrawTriangle({xs+20, 32}, {xs+20+16, 40}, {xs+20+16, 24}, GRAY);
            DrawTextEx(topbarState->appState->font, "L", {xs+20+9, 16+10}, 12, 0, WHITE);
        }
        DrawRectangle(xs+24+16, 24, 16, 16, rc);
        const char* cl = TextFormat("%d", topbarState->currentPage+1);
        const float cx = MeasureTextEx(topbarState->appState->font, cl, 17, 0).x;
        DrawTextEx(topbarState->appState->font, cl, {xs+32+16-cx/2, 23}, 17, 0, WHITE);
        if (topbarState->currentPage+1 != topbarState->pageCount) {
            DrawTriangle({xs+60, 24}, {xs+60, 40}, {xs+60+16, 32}, GRAY);
            DrawTextEx(topbarState->appState->font, "R", {xs+44+18, 16+10}, 12, 0, WHITE);
            for (int i = topbarState->currentPage + 1; i < topbarState->pageCount && i - topbarState->currentPage < 19; i++) {
                const float x = xs+76+4+(16+4)*(i-topbarState->currentPage-1);
                DrawRectangle(x, 24, 16, 16, rc);
                const char* pl = TextFormat("%d", i+1);
                const float px = MeasureTextEx(topbarState->appState->font, pl, 17, 0).x;
                DrawTextEx(topbarState->appState->font, pl, {x+8-px/2, 23}, 17, 0, WHITE);
            }
        }
    }
    DrawTriangle({xs+4, 0}, {xs+4, ys+16+8}, {22+xs+4, 0}, c0b);
    DrawRectangle(0, ys, xs+4, 24, c0b);
    DrawTriangle({xs, 0}, {xs, ys+16+4}, {22+xs, 0}, c2);
    DrawRectangle(0, 0, 22+xs-22, ys+16+4, c2);

    DrawTextEx(topbarState->appState->font, TextFormat("%d", apctl.strength), {ATTR_PSP_WIDTH-40, 0}, 17, 0, apctl.strength > 50 ? WHITE : (apctl.strength < 25 ? RED : GOLD));
    DrawRectangle(ATTR_PSP_WIDTH-16, 0, 16, 16, DARKGRAY);
    DrawTexture(topbarState->signal, ATTR_PSP_WIDTH-15, -1, WHITE);
}
