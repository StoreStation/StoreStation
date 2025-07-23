#pragma once

#define STORELIST_ITEM_SIZE 60

typedef struct {
    char name[128];
    char author[128];
    char description[512];
    char id[128];
    uint64_t uploaded;
} StoreItem;

typedef struct {
    AppState *appState;
    StoreItem *storeItemsList;
    int selectedItem;
    int page;
    int pageCount;
    int itemCount;
    int totalCount;
    int pageSize;
    float blend;
    float transitionStatus;
} StorelistState;

void storelist_init(StorelistState *storelistState, AppState *appState, char* cat, char *sort, int page) {
    *storelistState = {};
    storelistState->appState = appState;
    storelistState->storeItemsList = nullptr;
    storelistState->page = -1;
    storelistState->selectedItem = 0;
    storelistState->blend = 0;
    storelistState->transitionStatus = 0;
    char endpoint[128];
    snprintf(endpoint, 128, "/%s/list/%s/%d", cat, sort, page);
    net_Response resp = net_get(endpoint);
    if (resp.code == 200) {
        cJSON *json = cJSON_ParseWithLength(resp.buffer, resp.size);
        if (json == nullptr) goto err;
        cJSON *item;
        item = cJSON_GetObjectItemCaseSensitive(json, "total");
        storelistState->totalCount = item->valueint;
        item = cJSON_GetObjectItemCaseSensitive(json, "split");
        storelistState->pageSize = item->valueint;
        item = cJSON_GetObjectItemCaseSensitive(json, "page");
        storelistState->page = item->valueint;
        storelistState->pageCount = (int) pspFpuCeil((float) storelistState->totalCount / (float) storelistState->pageSize);

        item = cJSON_GetObjectItemCaseSensitive(json, cat);
        storelistState->itemCount = cJSON_GetArraySize(item);
        storelistState->storeItemsList = (StoreItem*) malloc(sizeof(StoreItem) * storelistState->itemCount);
        memset(storelistState->storeItemsList, 0, sizeof(StoreItem) * storelistState->itemCount);

        const cJSON *array = item;
        for (int i = 0; i < storelistState->itemCount; i++) {
            cJSON *currentKey;
            item = cJSON_GetArrayItem(array, i);
            currentKey = cJSON_GetObjectItemCaseSensitive(item, "name");
            strncpy(storelistState->storeItemsList[i].name, currentKey->valuestring, 128);
            currentKey = cJSON_GetObjectItemCaseSensitive(item, "author");
            strncpy(storelistState->storeItemsList[i].author, currentKey->valuestring, 128);
            currentKey = cJSON_GetObjectItemCaseSensitive(item, "description");
            strncpy(storelistState->storeItemsList[i].description, currentKey->valuestring, 512);
            currentKey = cJSON_GetObjectItemCaseSensitive(item, "repo");
            strncpy(storelistState->storeItemsList[i].id, currentKey->valuestring, 128);
            currentKey = cJSON_GetObjectItemCaseSensitive(item, "uploaded");
            storelistState->storeItemsList[i].uploaded = (uint64_t) currentKey->valuedouble;

            TraceLog(LOG_INFO, "] %s", storelistState->storeItemsList[i].name);
        }
        
        cJSON_Delete(json);
        TraceLog(LOG_INFO, "%d %d %d %d", storelistState->page, storelistState->itemCount, storelistState->totalCount, storelistState->pageSize);
    } else {
        err: 
        storelistState->page = -1;
        TraceLog(LOG_ERROR, "There has been an error reading the list %d (%d)", resp.code, resp.size);
    }
    net_closeget(&resp);
}

void storelist_destroy(StorelistState *storelistState) {
    if (storelistState->storeItemsList != nullptr) {
        free(storelistState->storeItemsList);
    }
}

void storelist_up(StorelistState *storelistState) {
    if (storelistState->transitionStatus != 0 || storelistState->page == -1) return;
    storelistState->selectedItem--;
    if (storelistState->selectedItem < 0) storelistState->selectedItem = 0; else storelistState->transitionStatus = -STORELIST_ITEM_SIZE;
}

void storelist_down(StorelistState *storelistState) {
    if (storelistState->transitionStatus != 0 || storelistState->page == -1) return;
    storelistState->selectedItem++;
    if (storelistState->selectedItem >= storelistState->itemCount) storelistState->selectedItem--; else storelistState->transitionStatus = STORELIST_ITEM_SIZE;
}

char* storelist_id(StorelistState *storelistState) {
    if (storelistState->selectedItem >= storelistState->itemCount) return nullptr;
    return storelistState->storeItemsList[storelistState->selectedItem].id;
}

bool storelist_draw(StorelistState *storelistState) {
    if (storelistState->page == -1) {
        DrawRectangle(0, 0, ATTR_PSP_WIDTH, ATTR_PSP_HEIGHT, GRAY);
        const char* text = "An error has occurred press O to go back";
        const float tx = MeasureTextEx(storelistState->appState->font, text, 30, 0).x;
        DrawTextEx(storelistState->appState->font, text, {ATTR_PSP_WIDTH/2-tx/2, ATTR_PSP_HEIGHT/2-15}, 30, 0, RED);
        return false;
    }
    if (storelistState->totalCount == 0) {
        DrawRectangle(0, 0, ATTR_PSP_WIDTH, ATTR_PSP_HEIGHT, GRAY);
        const char* text = "The store seems to be a little empty right now";
        const float tx = MeasureTextEx(storelistState->appState->font, text, 30, 0).x;
        DrawTextEx(storelistState->appState->font, text, {ATTR_PSP_WIDTH/2-tx/2, ATTR_PSP_HEIGHT/2-15}, 30, 0, WHITE);
        return false;
    }
    storelistState->blend += GetFrameTime();
    if (storelistState->blend >= 1) storelistState->blend = -1;
    if (storelistState->transitionStatus != 0) {
        if (storelistState->transitionStatus > 0) {
            storelistState->transitionStatus -= GetFrameTime() * 800;
            if (storelistState->transitionStatus <= 0) storelistState->transitionStatus = 0;
        } else {
            storelistState->transitionStatus += GetFrameTime() * 800;
            if (storelistState->transitionStatus >= 0) storelistState->transitionStatus = 0;
        }
    }
    

    DrawRectangle(0, 0, ATTR_PSP_WIDTH, ATTR_PSP_HEIGHT, MAGENTA);
    constexpr int size = STORELIST_ITEM_SIZE;
    int i = 0;
    for (; i < storelistState->itemCount; i++) {
        StoreItem * item = &storelistState->storeItemsList[i];
        float baseY = i*size + 48 - storelistState->selectedItem*size + storelistState->transitionStatus;
        const Color blockColor = (i+1) % 2 ? GRAY : DARKGRAY;
        constexpr int addAmt = 50;
        const Color blendColor = {(uint8_t) (blockColor.r + addAmt), (uint8_t) (blockColor.g + addAmt), (uint8_t) (blockColor.b + addAmt), 255};
        DrawRectangleGradientH(0, baseY, ATTR_PSP_WIDTH, size, i == storelistState->selectedItem ? ColorLerp(blockColor, blendColor, pspFpuAbs(storelistState->blend)) : blockColor, blockColor);
        baseY+=2;
        const float titlex = MeasureTextEx(storelistState->appState->font, item->name, 20, 0).x;
        DrawTextEx(storelistState->appState->font, TextFormat("%s ", item->name), {5, (float) baseY}, 20, 0, WHITE);
        DrawTextEx(storelistState->appState->font, TextFormat(" by %s", item->author), {5 + titlex + 2, (float) baseY + 3}, 15, 0, WHITE);
        DrawTextEx(storelistState->appState->font, item->description, {5, (float) (baseY+18)}, 18, 0, WHITE);
    }
    char pgFormat[64];
    snprintf(pgFormat, 64, "Page: %d/%d", storelistState->page+1, storelistState->pageCount);
    float tx = MeasureTextEx(storelistState->appState->font, pgFormat, 20, 0).x;
    DrawRectangle(0, i*size + 48 - storelistState->selectedItem*size + storelistState->transitionStatus, ATTR_PSP_WIDTH, ATTR_PSP_HEIGHT, BLACK);
    DrawTextEx(storelistState->appState->font, pgFormat, {ATTR_PSP_WIDTH/2-tx/2, (float) (i*size+size/2-10 + 48 - storelistState->selectedItem*size) + storelistState->transitionStatus}, 20, 0, WHITE);
    snprintf(pgFormat, 64, "%d total items", storelistState->totalCount);
    tx = MeasureTextEx(storelistState->appState->font, pgFormat, 18, 0).x;
    DrawTextEx(storelistState->appState->font, pgFormat, {ATTR_PSP_WIDTH/2-tx/2, (float) (i*size+size/2-10 + 48 - storelistState->selectedItem*size+18) + storelistState->transitionStatus}, 18, 0, WHITE);
    return storelistState->transitionStatus != 0;
}