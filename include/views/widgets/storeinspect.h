#pragma once

#define STORE_MIN_VER   1

typedef struct {
    char file[256];
    char install[256];
} StoreInspect_Install;

typedef struct {
    char line[512];
} StoreInspect_Register;

typedef struct {
    bool _ver;
    char _type[32];

    char name[256];
    char description[1024];
    char author[128];
    char version[128];
    char link[256];
    char download[512];

    bool isCompressed;
    StoreInspect_Install *targets;
    uint32_t targetsCount;
    StoreInspect_Register *registers;
    uint32_t registersCount;
    char mountFrom[64];
    net_Response bytes;
    Texture image;
} StoreInspect_Item;

typedef struct {
    StoreInspect_Item item;
    AppState *appState;
    bool ok, installable, completed, failed, skipRegister, installing;
    float buttonAnimation;
} StoreInspectState;

bool storeinspect_install(StoreInspectState *storeInspectState) {
    storeInspectState->installing = false;
    const char* mountPoint = "/installer/";

    if (storeInspectState == nullptr) return false;
    if (!storeInspectState->installable) return false;
    storeInspectState->installable = false;
    StoreInspect_Item * item = &storeInspectState->item;
    if (item->download[0] == 0) return false;
    const char* ext = GetFileExtension(item->download);
    char archiveName[strlen(ext)+8+strlen("download")];
    if (item->isCompressed) {
        char urlBuf[strlen(item->download)+strlen("/proxy/")+16];
        strcpy(urlBuf, "/proxy/");
        strcat(urlBuf, item->download);
        strcpy(archiveName, "download");
        strcat(archiveName, ext);
        FILE * dl = fopen(archiveName, "wb");
        if (dl == nullptr) {
            TraceLog(LOG_ERROR, "Failed to download archive");
            goto err;
        }
        net_SimpleResponse archive = net_download(urlBuf, dl, net_timeouts(30));
        fclose(dl);
        if (archive.code != 200 || archive.size <= 0) goto err;
        TraceLog(LOG_INFO, "Mounting installer (%d) from %s to %s", archive.size, archiveName, mountPoint);
        if (PHYSFS_mount(archiveName, mountPoint, 0) == 0) {
            TraceLog(LOG_ERROR, "Failed to mount installer");
            remove(archiveName);
            goto err;
        }

        char tmp[512];
        char copyBuffer[1024];
        for (uint32_t i = 0; i < item->targetsCount; i++) {
            tmp[0] = 0;
            sceKernelDelayThreadCB(1);
            StoreInspect_Install *target = &item->targets[i];
            if (target->file[0] == 0 || target->install[0] == 0) continue;
            char* fileNamePtr = strrchr(target->install, '/');
            if (fileNamePtr != nullptr) {
                *fileNamePtr = 0;
                mkdir_recursive(target->install, 0777);
                *fileNamePtr = '/';
            }
            snprintf(tmp, 512, "%s%s", mountPoint, target->file);
            TraceLog(LOG_INFO, "] Copying %s", tmp);
            PHYSFS_File * f = PHYSFS_openRead(tmp);
            if (f == nullptr) {
                TraceLog(LOG_ERROR, "Failed to open file %s", tmp);
                continue;
            }
            FILE * t = fopen(target->install, "wb");
            if (t == nullptr) {
                PHYSFS_close(f);
                TraceLog(LOG_ERROR, "Failed to create file %s", target->install);
                continue;
            }
            int bytes_read;
            while ((bytes_read = PHYSFS_readBytes(f, copyBuffer, 1024)) > 0) {
                int bytes_written = fwrite(copyBuffer, 1, bytes_read, t);
                if (bytes_written != bytes_read) {
                    TraceLog(LOG_ERROR, "Write error for file %s", tmp);
                    break;
                }
            }
            fclose(t);
            PHYSFS_close(f);
        }
    } else {
        if (item->targetsCount != 1 || item->targets[0].install[0] == 0) {
            TraceLog(LOG_ERROR, "Too many or too few targets available Limit: 1, Found: %d", item->targetsCount);
            return false;
        }
        char* fileNamePtr = strrchr(item->targets[0].install, '/');
        if (fileNamePtr != nullptr) {
            *fileNamePtr = 0;
            mkdir_recursive(item->targets[0].install, 0777);
            *fileNamePtr = '/';
        }
        FILE * res = fopen(item->targets[0].install, "wb");
        if (res == nullptr) {
            TraceLog(LOG_ERROR, "Failed to create file %s", item->targets[0].install);
            return false;
        }
        char urlBuf[strlen(item->download)+strlen("/proxy/")+16];
        strcpy(urlBuf, "/proxy/");
        strcat(urlBuf, item->download);
        strcpy(archiveName, "download");
        strcat(archiveName, ext);
        net_SimpleResponse resp = net_download(urlBuf, res, net_timeouts(30));
        if (resp.code != 200) {
            TraceLog(LOG_ERROR, "Failed to download file %s", item->download);
            fclose(res);
            return false;
        }
        fclose(res);
    }
    
    sceKernelDelayThreadCB(1);
    if (!storeInspectState->skipRegister && item->registersCount > 0) {
        mkdir("ms0:/SEPLUGINS/", 0777);
        FILE * f = fopen("ms0:/SEPLUGINS/PLUGINS.txt", "r+");
        if (f == nullptr) f = fopen("ms0:/SEPLUGINS/PLUGINS.txt", "w");
        if (f == nullptr) goto donew;
        fseek(f, 0, SEEK_END);
        for (uint32_t i = 0; i < item->registersCount; i++) {
            StoreInspect_Register *registerItem = &item->registers[i];
            if (registerItem->line[0] == 0) continue;
            fprintf(f, "\n%s", registerItem->line);
        }
        fclose(f);
    }
    donew:
    
    if (item->isCompressed) {
        PHYSFS_unmount(archiveName);
        remove(archiveName);
    }
    return true;
    err:
    return false;
}

void storeinspect_init(StoreInspectState *storeInspectState, AppState * appState, char* cat, char* id) {
    memset(storeInspectState, 0, sizeof(StoreInspectState));
    storeInspectState->appState = appState;
    storeInspectState->buttonAnimation = 0;
    char urlBuf[256];
    snprintf(urlBuf, 255, "/%s/info/%s", cat, id);
    net_Response resp = net_get(urlBuf, net_timeouts(2));
    if (resp.code == 200) {
        cJSON *json = cJSON_ParseWithLength(resp.buffer, resp.size);
        if (json == nullptr) goto err;
        cJSON *item, *downloads;
        item = cJSON_GetObjectItemCaseSensitive(json, "ok");
        storeInspectState->ok = item != nullptr && cJSON_IsTrue(item);
        item = cJSON_GetObjectItemCaseSensitive(json, "pkgver");
        if (item == nullptr || item->valueint > STORE_MIN_VER) {
            storeInspectState->item._ver = false;
            goto done;
        } else storeInspectState->item._ver = true;
        item = cJSON_GetObjectItemCaseSensitive(json, "type");
        if (item != nullptr) strncpy(storeInspectState->item._type, item->valuestring, sizeof(storeInspectState->item._type)-1);
        item = cJSON_GetObjectItemCaseSensitive(json, "name");
        if (item != nullptr) strncpy(storeInspectState->item.name, item->valuestring, sizeof(storeInspectState->item.name)-1);
        item = cJSON_GetObjectItemCaseSensitive(json, "author");
        if (item != nullptr) strncpy(storeInspectState->item.author, item->valuestring, sizeof(storeInspectState->item.author)-1);
        item = cJSON_GetObjectItemCaseSensitive(json, "description");
        if (item != nullptr) strncpy(storeInspectState->item.description, item->valuestring, sizeof(storeInspectState->item.description)-1);
        item = cJSON_GetObjectItemCaseSensitive(json, "version");
        if (item != nullptr) strncpy(storeInspectState->item.version, item->valuestring, sizeof(storeInspectState->item.version)-1);
        item = cJSON_GetObjectItemCaseSensitive(json, "link");
        if (item != nullptr) strncpy(storeInspectState->item.link, item->valuestring, sizeof(storeInspectState->item.link)-1);
        downloads = cJSON_GetObjectItemCaseSensitive(json, "download");
        if (downloads != nullptr) {
            item = cJSON_GetObjectItemCaseSensitive(downloads, "remote");
            if (item != nullptr) {
                strncpy(storeInspectState->item.download, item->valuestring, sizeof(storeInspectState->item.download)-1);
                storeInspectState->installable = true;
            }
            item = cJSON_GetObjectItemCaseSensitive(downloads, "compressed");
            storeInspectState->item.isCompressed = item != nullptr && cJSON_IsTrue(item);
        }
        item = cJSON_GetObjectItemCaseSensitive(json, "image");
        if (item != nullptr) storeInspectState->item.image = fetch_texture_link(item->valuestring); else storeInspectState->item.image = {.id = 0};
        if (storeInspectState->item.image.id == 0) {
            storeInspectState->item.image = LoadTexture("/eboot/assets/nopreview.png");
            SetTextureFilter(storeInspectState->item.image, TEXTURE_FILTER_POINT);
        }
        if (downloads != nullptr) {
            const cJSON * targets = cJSON_GetObjectItemCaseSensitive(downloads, "targets");
            if (targets != nullptr) {
                storeInspectState->item.targetsCount = cJSON_GetArraySize(targets);
                storeInspectState->item.targets = (StoreInspect_Install *) malloc(sizeof(StoreInspect_Install) * storeInspectState->item.targetsCount);
                memset(storeInspectState->item.targets, 0, sizeof(StoreInspect_Install) * storeInspectState->item.targetsCount);
                for (uint32_t i = 0; i < storeInspectState->item.targetsCount; i++) {
                    cJSON * target = cJSON_GetArrayItem(targets, i);
                    if (target == nullptr) continue;
                    cJSON * file = cJSON_GetObjectItemCaseSensitive(target, "file");
                    cJSON * install = cJSON_GetObjectItemCaseSensitive(target, "target");
                    if (file != nullptr && install != nullptr) {
                        strncpy(storeInspectState->item.targets[i].file, file->valuestring, sizeof(storeInspectState->item.targets[i].file)-1);
                        strncpy(storeInspectState->item.targets[i].install, install->valuestring, sizeof(storeInspectState->item.targets[i].install)-1);
                    }
                }
            }
            const cJSON * registers = cJSON_GetObjectItemCaseSensitive(downloads, "register");
            if (registers != nullptr) {
                storeInspectState->item.registersCount = cJSON_GetArraySize(registers);
                storeInspectState->item.registers = (StoreInspect_Register *) malloc(sizeof(StoreInspect_Register) * storeInspectState->item.registersCount);
                memset(storeInspectState->item.registers, 0, sizeof(StoreInspect_Register) * storeInspectState->item.registersCount);
                for (uint32_t i = 0; i < storeInspectState->item.registersCount; i++) {
                    cJSON * registerItem = cJSON_GetArrayItem(registers, i);
                    if (registerItem == nullptr) continue;  
                    strncpy(storeInspectState->item.registers[i].line, registerItem->valuestring, sizeof(storeInspectState->item.registers[i].line)-1);
                }
            }
        }

        done:
        cJSON_Delete(json);
    } else {
        err: 
        storeInspectState->ok = false;
        TraceLog(LOG_ERROR, "There has been an error reading this item %d (%d)", resp.code, resp.size);
    }

    net_closeget(&resp);
}

void storeinspect_destroy(StoreInspectState *storeInspectState) {
    if (storeInspectState->item.targets != nullptr) free(storeInspectState->item.targets);
    if (storeInspectState->item.registers != nullptr) free(storeInspectState->item.registers);
    if (storeInspectState->item.mountFrom[0] != 0) PHYSFS_unmount(storeInspectState->item.mountFrom);
    if (storeInspectState->item.image.id != 0) UnloadTexture(storeInspectState->item.image);
    net_closeget(&storeInspectState->item.bytes);
}

void storeinspect_draw(StoreInspectState *storeInspectState) {
    if (storeInspectState == nullptr) return;
    DrawRectangle(0, 0, ATTR_PSP_WIDTH, ATTR_PSP_HEIGHT, GRAY);
    if (!storeInspectState->ok) {
        const char* text = "An error has occurred press O to go back";
        const float tx = MeasureTextEx(storeInspectState->appState->font, text, 30, 0).x;
        DrawTextEx(storeInspectState->appState->font, text, {ATTR_PSP_WIDTH/2-tx/2, ATTR_PSP_HEIGHT/2-15}, 30, 0, RED);
        return;
    }
    if (!storeInspectState->item._ver) {
        const char* text = "This item requires a newer store version";
        const float tx = MeasureTextEx(storeInspectState->appState->font, text, 30, 0).x;
        DrawTextEx(storeInspectState->appState->font, text, {ATTR_PSP_WIDTH/2-tx/2, ATTR_PSP_HEIGHT/2-15}, 30, 0, RED);
        return;
    }
    storeInspectState->buttonAnimation+=0.06f*GetFrameTime()*1000;
    if (storeInspectState->buttonAnimation >= 100) {
        storeInspectState->buttonAnimation = -99;
    }

    DrawTextureEx(storeInspectState->item.image, {0, 48}, 0, 1.75f, WHITE);
    DrawTextEx(storeInspectState->appState->font, TextFormat("by %s", storeInspectState->item.author), {STORE_PREV_W*1.75f+5, 70}, 20, 0, {220, 220, 220, 255});
    DrawTextEx(storeInspectState->appState->font, storeInspectState->item.name, {STORE_PREV_W*1.75f+5, 50}, 30, 0, WHITE);
    DrawTextEx(storeInspectState->appState->font, TextFormat("Version %s", storeInspectState->item.version), {STORE_PREV_W*1.75f+5, 84}, 17, 0, {200, 200, 200, 255});
    DrawTextEx(storeInspectState->appState->font, storeInspectState->item.link, {STORE_PREV_W*1.75f+5, 98}, 17, 0, {105, 0, 166, 255});
    DrawTextEx(storeInspectState->appState->font, storeInspectState->item.description, {5, STORE_PREV_H*1.75f+52}, 20, 0, WHITE);

    if (storeInspectState->installable) {
        const float rem = ATTR_PSP_WIDTH - STORE_PREV_W*1.75f;
        const Color b1 = {112, 2, 254, 255};
        const Color b2 = {188, 140, 250, 255};
        const Color c1 = ColorLerp(b1, b2, pspFpuAbs(storeInspectState->buttonAnimation)/100);
        const Color c2 = ColorLerp(b2, b1, pspFpuAbs(storeInspectState->buttonAnimation)/100);
        const Color c3 = ColorLerp(b1, b2, (100-pspFpuMin(pspFpuAbs(storeInspectState->buttonAnimation+50), 50))/100);
        DrawRectangleGradientEx({ATTR_PSP_WIDTH-rem/2-rem/4, 48+STORE_PREV_H*1.75f-32, rem/2, 32},
            c1,
            c3,
            c2,
            c3
        );
        DrawRectangleRec({ATTR_PSP_WIDTH-rem/2-rem/4+2, 48+STORE_PREV_H*1.75f-32+2, rem/2-4, 32-4}, DARKGRAY);
        const char* install = "INSTALL";
        const float txs = (ATTR_PSP_WIDTH-rem/2)-MeasureTextEx(storeInspectState->appState->font, install, 20, 0).x/2;
        DrawTextEx(storeInspectState->appState->font, install, {txs, 48+STORE_PREV_H*1.75f-16-10}, 20, 0, WHITE);
    }

    if (storeInspectState->completed || storeInspectState->failed || storeInspectState->installing) {
        DrawRectangle(ATTR_PSP_WIDTH/4, ATTR_PSP_HEIGHT/2-ATTR_PSP_HEIGHT/8, ATTR_PSP_WIDTH/2, ATTR_PSP_HEIGHT/4, DARKGRAY);
        const char* msg = "Installing, please wait";
        Color color = ORANGE;
        if (storeInspectState->completed) {
            color = GREEN;
            msg = "Install successful";
        }
        if (storeInspectState->failed) {
            color = RED;
            msg = "Install failed";
        }
        const float tx = MeasureTextEx(storeInspectState->appState->font, msg, 30, 0).x;
        DrawTextEx(storeInspectState->appState->font, msg, {ATTR_PSP_WIDTH/2-tx/2, ATTR_PSP_HEIGHT/2-15}, 30, 0, color);
    }
}