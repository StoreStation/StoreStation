#pragma once

#include <raylib.h>
#include <physfs.h>
#include <ctype.h>
#include <cstring>
#include <stb_image.h>
#include <psploadcore.h>

#define STORE_PREV_W    128
#define STORE_PREV_H    64

extern "C" {
    __asm__(""
    "	.global _DisableFPUExceptions\n"
    "    .set push \n"
    "    .set noreorder \n"
    "_DisableFPUExceptions: \n"
    "    cfc1    $2, $31 \n"
    "    lui     $8, 0x80 \n"
    "    and     $8, $2, $8\n"
    "    ctc1    $8, $31 \n"
    "    jr      $31 \n"
    "    nop \n"
    "    .set pop \n"
    "");
    void _DisableFPUExceptions();
}

void UnloadModelFull(Model &model) {
    if (model.meshes == nullptr) {
        TraceLog(LOG_WARNING, "ModelFull double free attempted");
        return;
    }
    for (int i = 0; i < model.materialCount; ++i) UnloadMaterial(model.materials[i]);
    model.materialCount = 0;
    UnloadModel(model);
    model.meshes = nullptr;
}

void utf8_to_ascii(const char* utf8_input, char* ascii_output, size_t output_size) {
    if (utf8_input == NULL || ascii_output == NULL || output_size == 0) return;
    size_t output_index = 0;
    const unsigned char* input = (const unsigned char*)utf8_input;
    while (*input != '\0' && output_index < output_size - 1) {
        if ((*input & 0x80) == 0) {
            ascii_output[output_index++] = *input;
            input++;
        } else {
            ascii_output[output_index++] = '?';
            if ((*input & 0xE0) == 0xC0) {
                input += 2;
            } else if ((*input & 0xF0) == 0xE0) {
                input += 3;
            } else if ((*input & 0xF8) == 0xF0) {
                input += 4;
            } else {
                input++;
            }
        }
    }
    ascii_output[output_index] = '\0';
}

unsigned char* resize_image_to_x_y(unsigned char* dst_data, const unsigned char* src_data, int src_width, int src_height, int bytes_per_pixel, int dst_width, int dst_height) {
    float x_ratio = (float) src_width / dst_width;
    float y_ratio = (float) src_height / dst_height;

    for (int y = 0; y < dst_height; y++) {
        for (int x = 0; x < dst_width; x++) {
            int src_x = (int) (x * x_ratio);
            int src_y = (int) (y * y_ratio);
            int dst_index = (y * dst_width + x) * bytes_per_pixel;
            int src_index = (src_y * src_width + src_x) * bytes_per_pixel;
            memcpy(&dst_data[dst_index], &src_data[src_index], bytes_per_pixel);
        }
    }

    return dst_data;
}

int syncsafe_to_int(unsigned char bytes[4]) {
    return (bytes[0] << 21) | (bytes[1] << 14) | (bytes[2] << 7) | bytes[3];
}

int be_to_int(unsigned char bytes[4]) {
    return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}

void max_alpha(unsigned char* data, const int width, const int height) {
    if (!data) return;
    for (long i = 0; i < width * height*4; i+=4) {
        data[i+3] = 255;
    }
}

static unsigned char* PFS_LoadFileDataCallback(const char* fileName, int* dataSize) {
    PHYSFS_File* f = PHYSFS_openRead(fileName);
    if (f == nullptr) return nullptr;
    *dataSize = PHYSFS_fileLength(f);
    auto * mem = (unsigned char*) malloc(*dataSize);
    if (mem == nullptr) return nullptr;
    PHYSFS_readBytes(f, mem, *dataSize);
    PHYSFS_close(f);
    return mem;
}

static char* PFS_LoadFileTextCallback(const char* fileName) {
    PHYSFS_File* f = PHYSFS_openRead(fileName);
    if (f == nullptr) return nullptr;
    const int dataSize = PHYSFS_fileLength(f) + 1;
    auto * mem = (char*) malloc(dataSize);
    if (mem == nullptr) return nullptr;
    PHYSFS_readBytes(f, mem, dataSize);
    PHYSFS_close(f);
    mem[dataSize-1] = 0;
    return mem;
}

#define DEBUG_LOG_SIZE 24
static char CustomLogBuffer[DEBUG_LOG_SIZE][128] = { 0 };
void CustomLog(int msgType, const char *text, va_list args) {
    char* customLogLine = CustomLogBuffer[0];
    for (int i = DEBUG_LOG_SIZE-1; i > 0 ; i--) strcpy(CustomLogBuffer[i], CustomLogBuffer[i-1]);
    switch (msgType) {
        case LOG_INFO: sprintf(customLogLine,    "[INFO] : "); break;
        case LOG_ERROR: sprintf(customLogLine,   "[ERROR]: "); break;
        case LOG_WARNING: sprintf(customLogLine, "[WARN] : "); break;
        case LOG_DEBUG: sprintf(customLogLine,   "[DEBUG]: "); break;
        default: break;
    }
    const int cur = strlen(customLogLine);
    vsnprintf(customLogLine+cur, 127-cur, text, args);
    sceIoWrite(1, customLogLine, strlen(customLogLine));
}

inline void DrawLogData() {
    for (int i = 0; i < DEBUG_LOG_SIZE; i++) DrawText(CustomLogBuffer[i], 10, i * 10 + 30, 10, WHITE);
}

inline void DrawDiagnostic(AppState &appState) {
    if constexpr (!DEBUG) return;
    DrawText(TextFormat("%d", GetFPS()), 465, 10, 10, GREEN);
    if (!appState.controller.t) return;
    struct mallinfo mi = mallinfo();
    int free_bytes = mi.fordblks;
    int allocated_bytes = mi.arena;
    const char* text = TextFormat("FM: %d MM: %d FH: %d UH: %d FPS: %d", sceKernelTotalFreeMemSize(), sceKernelMaxFreeMemSize(), free_bytes, allocated_bytes, GetFPS());
    DrawTextEx(appState.font, text, {10, 10}, 20, 0, GREEN);
    DrawLogData();
}

void UnmountCallback(void *data) {
    free(data);
}

void MountEmbedded(char *path) {
    #define PSAR_OFFSET 0x24
    FILE* fd = fopen(path, "rb");
    if (fd == nullptr) return;
    fseek(fd, PSAR_OFFSET, SEEK_SET);
    uint32_t data_offset = 0;
    fread(&data_offset, 4, 1, fd);
    fseek(fd, 0, SEEK_END);
    size_t data_size = ftell(fd) - data_offset;
    fseek(fd, data_offset, SEEK_SET);
    void * data = malloc(data_size);
    if (data == nullptr) {
        fclose(fd);
        return;
    }
    fread(data, 1, data_size, fd);
    fclose(fd);
    PHYSFS_mountMemory(data, data_size, UnmountCallback, INTERNAL_ARCHIVE, "/eboot/", 0);
}

uint32_t sctrlHENFindFunction(char* szMod, char* szLib, uint32_t nid) {
	struct SceLibraryEntryTable *entry;
	SceModule *pMod = nullptr;
	void *entTab;
	int entLen;
	//pMod = sceKernelFindModuleByName(szMod);
	if (!pMod) {
		TraceLog(LOG_WARNING, "Cannot find module %s", szMod);
		return 0;
	}
	int i = 0;
	entTab = pMod->ent_top;
	entLen = pMod->ent_size;
	while (i < entLen) {
		int count;
		int total;
		unsigned int *vars;
		entry = &((struct SceLibraryEntryTable *) entTab)[i];
		if (entry->libname == szLib || (entry->libname && !strcmp(entry->libname, szLib))) {
			total = entry->stubcount + entry->vstubcount;
			vars = (unsigned int*) entry->entrytable;
			if (entry->stubcount > 0) {
				for(count = 0; count < entry->stubcount; count++) {
					if (vars[count] == nid) return vars[count+total];					
				}
			}
		}
		i += (entry->len * 4);
	}
	return 0;
}

bool onArk() {
    #define ARK_MODULE  "SystemCtrlForUser"
    #define ARK_ADDRESS 0xB00B1E55
    #define ARK_CHECK   "sctrlArkGetConfig"
    return sctrlHENFindFunction((char*) ARK_MODULE, (char*) ARK_CHECK, ARK_ADDRESS) != 0;
}

void jpeg_size(const uint8_t* buf, const int data_size, int* out_w, int* out_h) {
    int w = -1, h = -1;
    for (int i = 2; i < data_size;) {
        if (buf[i] == 0xFF) {
            i++;
            switch(buf[i]) {
                case 0xC0: case 0xC1: case 0xC2: case 0xC3: case 0xC5: case 0xC6: case 0xC7: case 0xC9: case 0xCA: case 0xCB: case 0xCD: case 0xCE: case 0xCF:
                    i += 4;
                    h = (buf[i] << 8) | (buf[i+1]);
                    w = (buf[i+2] << 8) | (buf[i+3]);
                    i = data_size; break;
                case 0xDA: case 0xD9: break;
                default:
                    i += ((buf[i+1] << 8) | (buf[i+2])) + 1;
                    break;
            }
        } else i++;
    }
    *out_w = w;
    *out_h = h;
}

Texture fetch_texture_link(char* link, int maxW = STORE_PREV_W, int maxH = STORE_PREV_H) {
    char urlBuf[400];
    int w, h, c = 0, jres;
    uint8_t rgbaBuf[STORE_PREV_W * STORE_PREV_H * 4] __attribute__ ((aligned(64)));
    Texture ret = {.id = 0};
    Image tmp;

    snprintf(urlBuf, 399, "/proxy/%s", link);
    net_Response resp = net_get(urlBuf, net_timeouts(2));
    if (resp.code != 200) goto err;
    if (resp.size == 0) goto closeerr;
    if (resp.size < 2 || memcmp(resp.buffer, "\xFF\xD8", 2) != 0) {
        TraceLog(LOG_ERROR, "Invalid image was downloaded");
        goto closeerr;
    }
    jpeg_size((uint8_t*) resp.buffer, resp.size, &w, &h);
    if ((maxW != -1 && (w != maxW || h != maxH)) || (maxW == -1 && (w <= 0 || h <= 0))) {
        TraceLog(LOG_ERROR, "Jpeg has incorrect size");
        goto closeerr;
    }
    if (sceJpegCreateMJpeg(w, h) != 0) {
        TraceLog(LOG_ERROR, "Unable to instance jpeg decoder");
        goto closeerr;
    }
    c = 4;
    jres = sceJpegDecodeMJpeg((uint8_t*) resp.buffer, resp.size, rgbaBuf, 0);
    if (jres < 0) {
        TraceLog(LOG_ERROR, "Unable to decode jpeg");
        goto closeerr;
    }
    max_alpha((uint8_t *) rgbaBuf, w, h);
    tmp.data = rgbaBuf;
    tmp.width = w;
    tmp.height = h;
    tmp.mipmaps = 1;
    tmp.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;
    ret = LoadTextureFromImage(tmp);
    SetTextureFilter(ret, TEXTURE_FILTER_BILINEAR);

    closeerr:
    if (c != 0) sceJpegDeleteMJpeg();
    net_closeget(&resp);
    err:
    return ret;
}

int mkdir_recursive(const char *path, mode_t mode) {
    char *p = nullptr;
    char temp_path[strlen(path)+16];
    strcpy(temp_path, path);

    for (p = temp_path + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            if (strcmp(temp_path, "ms0:") != 0) mkdir(temp_path, mode);
            *p = '/';
        }
    }

    if (strcmp(temp_path, "ms0:") != 0) mkdir(temp_path, mode);

    return 0;
}
