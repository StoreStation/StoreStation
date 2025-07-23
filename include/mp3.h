#pragma once

#include <pspmp3.h>
#include <physfs.h>

#define MP3_MP3_BUF (16*1024)
#define MP3_PCM_BUF (16*(1152/2))
#define MP3_USE_THREAD true

typedef struct {
    SceMp3InitArg mp3Init;
    PHYSFS_File * fd;
    uint32_t phyFileLen;
	bool paused;
	bool error;
	bool looping;
	bool over;
    int32_t handle;
	int32_t numPlayed;
	int32_t samplingRate;
	int32_t maxSample;
	int32_t playedSamples;
	int32_t totalSamples;
	int32_t numChannels;
	int32_t channel;
	char source[256];
	char title[256];
	char author[256];
    unsigned char pcmBuf[MP3_PCM_BUF] __attribute__((aligned(64)));
	unsigned char mp3Buf[MP3_MP3_BUF] __attribute__((aligned(64)));
	int32_t thread;
} MP3Instance;

int mp3_read_title_author(PHYSFS_File *fp, MP3Instance *meta) {
    if (!fp || !meta) return -1;

    if (PHYSFS_seek(fp, meta->phyFileLen-128) != 0) {
        return -2;
    }

    unsigned char tag[128];
    if (PHYSFS_readBytes(fp, tag, 128) != 128) {
        return -3;
    }

    if (memcmp(tag, "TAG", 3) != 0) {
        return -4;
    }

    strncpy(meta->title, (char *)(tag + 3), 30);
    meta->title[30] = '\0';

    strncpy(meta->author, (char *)(tag + 33), 30);
    meta->author[30] = '\0';

    return 0;
}

int mp3_read_title_author_v2(PHYSFS_File *fp, MP3Instance *meta) {
    if (!fp || !meta) return -1;

    if (PHYSFS_seek(fp, 0) != 0) {
        return -2;
    }

    unsigned char header[10];
    if (PHYSFS_readBytes(fp, header, 10) != 10) {
        return -3;
    }

    if (memcmp(header, "ID3", 3) != 0) {
        return -4;
    }

    int tag_size = ((header[6] & 0x7F) << 21) |
                   ((header[7] & 0x7F) << 14) |
                   ((header[8] & 0x7F) << 7)  |
                   (header[9] & 0x7F);

    long tag_end = PHYSFS_tell(fp) + tag_size;

    while (PHYSFS_tell(fp) < tag_end) {
        unsigned char frame_header[10];
        if (PHYSFS_readBytes(fp, frame_header, 10) != 10) break;

        char frame_id[5] = {0};
        memcpy(frame_id, frame_header, 4);

        uint32_t frame_size = (frame_header[4] << 24) |
                              (frame_header[5] << 16) |
                              (frame_header[6] << 8)  |
                               frame_header[7];

        if (frame_size == 0 || frame_size > 1024) {
            PHYSFS_seek(fp, frame_size+PHYSFS_tell(fp));
            continue;
        }

        unsigned char frame_data[1025] = {0};
        if (PHYSFS_readBytes(fp, frame_data, frame_size) != frame_size) break;

        char *dest = NULL;
        if (strcmp(frame_id, "TIT2") == 0) {
            dest = meta->title;
        } else if (strcmp(frame_id, "TPE1") == 0) {
            dest = meta->author;
        }

        if (dest) {
            int encoding = frame_data[0];
            if (encoding == 0 || encoding == 3) {
                strncpy(dest, (char *)(frame_data + 1), 255);
                dest[255] = 0;
			} else {
                dest[0] = 0;
            }
        }

        if (meta->title[0] && meta->author[0]) {
            break;
        }
    }

    return 0;
}

uint32_t mp3_findStreamStart(PHYSFS_File * file_handle, uint8_t* tmp_buf){
    uint8_t* buf = (uint8_t*) tmp_buf;
	PHYSFS_readBytes(file_handle, buf, MP3_MP3_BUF/2);
    if (buf[0] == 'I' && buf[1] == 'D' && buf[2] == '3') {
        uint32_t header_size = (buf[9] | (buf[8]<<7) | (buf[7]<<14) | (buf[6]<<21));
        return header_size+10;
    } else if (buf[0] == 'A' && buf[1] == 'P' && buf[2] == 'E') {
        uint32_t header_size = (buf[12] | (buf[13]<<8) | (buf[14]<<16) | (buf[15]<<24));
        return header_size+32;
    }
    return 0;
}

bool mp3_fillStreamBuffer(PHYSFS_File * fd, int handle, MP3Instance * instance) {
	unsigned char* dst;
	long int write;
	long int pos;
	int status = sceMp3GetInfoToAddStreamData(handle, &dst, &write, &pos);
	if (status < 0) {
		TraceLog(LOG_ERROR, "sceMp3GetInfoToAddStreamData returned 0x%08X\n", status);
	}

	status = PHYSFS_seek(fd, pos);
	if (status == 0) {
		TraceLog(LOG_ERROR, "fread returned 0x%08X\n", status);
	}
	
	int read = PHYSFS_readBytes(fd, dst, write);
	if (read < 0) {
		TraceLog(LOG_ERROR, "Could not read from file - 0x%08X\n", read);
	}
	
	if (read == 0) {
		TraceLog(LOG_INFO, "Reached end of file");
		return false;
	}
	
	status = sceMp3NotifyAddStreamData(handle, read);
	if (status < 0) {
		TraceLog(LOG_ERROR, "sceMp3NotifyAddStreamData returned 0x%08X\n", status);
	}
	
	return (pos > 0);
}

bool mp3_feed(MP3Instance * mp3Instance) {
	if (mp3Instance == nullptr || mp3Instance->error) return true;
	if (mp3Instance->paused) {
		sceKernelDelayThread(10000);
		return false;
	}

    if (sceMp3CheckStreamDataNeeded(mp3Instance->handle) > 0) {
		mp3_fillStreamBuffer(mp3Instance->fd, mp3Instance->handle, mp3Instance);
    }

    short* buf;
    int bytesDecoded;
	for (int retries = 0; retries < 1; retries++) {
		bytesDecoded = sceMp3Decode(mp3Instance->handle, &buf);
		if (bytesDecoded>0) break;
		if (sceMp3CheckStreamDataNeeded(mp3Instance->handle) <= 0) break;
		if (!mp3_fillStreamBuffer(mp3Instance->fd, mp3Instance->handle, mp3Instance)) {
			mp3Instance->numPlayed = 0;
		}
	}
	
    if (bytesDecoded < 0 && ((uint32_t) bytesDecoded) != 0x80671402) {
		TraceLog(LOG_ERROR, "sceMp3Decode returned 0x%08X\n", bytesDecoded);
		mp3Instance->error = true;
		return true;
	}

	if (bytesDecoded==0 || ((uint32_t) bytesDecoded) == 0x80671402) {
		mp3Instance->paused = true;
		mp3Instance->over = true;
	    sceMp3ResetPlayPosition(mp3Instance->handle);
		mp3Instance->numPlayed = 0;
		return true;
	} else {
		mp3Instance->numPlayed += sceAudioSRCOutputBlocking(0x8000, buf);
	}
	return false;
}

bool mp3_tick(MP3Instance * mp3Instance) {
	if constexpr (!MP3_USE_THREAD) mp3_feed(mp3Instance);
	return mp3Instance == nullptr || mp3Instance->error || mp3Instance->over;
}

int mp3_thread(SceSize args, void* argp) {
	MP3Instance * mp3Instance = *(MP3Instance **) argp;
	while (!mp3Instance->over && mp3Instance->error == 0) mp3_feed(mp3Instance);
	sceKernelExitDeleteThread(0);
	return 0;
}

MP3Instance * mp3_beginPlayback(char * file) {
    MP3Instance * mp3Instance = (MP3Instance *) malloc(sizeof(MP3Instance));
	memset(mp3Instance, 0, sizeof(MP3Instance));
	strncpy(mp3Instance->source, file, 255);
    mp3Instance->fd = PHYSFS_openRead(file);
    mp3Instance->phyFileLen = PHYSFS_fileLength(mp3Instance->fd);
	if (mp3Instance->fd == nullptr) {
		TraceLog(LOG_ERROR, "Could not open file %s\n", file);
	}

    mp3Instance->mp3Init = SceMp3InitArg{};
	memset(&mp3Instance->mp3Init, 0, sizeof(SceMp3InitArg));
	mp3Instance->mp3Init.mp3StreamEnd = PHYSFS_fileLength(mp3Instance->fd);
	PHYSFS_seek(mp3Instance->fd, 0);
	mp3Instance->mp3Init.mp3StreamStart = mp3_findStreamStart(mp3Instance->fd, mp3Instance->mp3Buf);
	mp3Instance->mp3Init.mp3Buf = mp3Instance->mp3Buf;
	mp3Instance->mp3Init.mp3BufSize = MP3_MP3_BUF;
	mp3Instance->mp3Init.pcmBuf = mp3Instance->pcmBuf;
	mp3Instance->mp3Init.pcmBufSize = MP3_PCM_BUF;

	int status = sceMp3InitResource();
	if (status < 0) {
		TraceLog(LOG_ERROR, "sceMp3InitResource returned 0x%08X\n", status);
	}

    mp3Instance->handle = sceMp3ReserveMp3Handle(&mp3Instance->mp3Init);
	if (mp3Instance->handle < 0) {
		TraceLog(LOG_ERROR, "sceMp3ReserveMp3Handle returned 0x%08X\n", mp3Instance->handle);
	}

	mp3_fillStreamBuffer(mp3Instance->fd, mp3Instance->handle, mp3Instance);

    status = sceMp3Init(mp3Instance->handle);
	if (status < 0) {
		TraceLog(LOG_ERROR, "sceMp3Init returned 0x%08X\n", status);
	}

	sceAudioSRCChRelease();
	sceMp3SetLoopNum(mp3Instance->handle, 0);

	mp3Instance->samplingRate = sceMp3GetSamplingRate(mp3Instance->handle);
	mp3Instance->numChannels = sceMp3GetMp3ChannelNum(mp3Instance->handle);
	mp3Instance->maxSample = sceMp3GetMaxOutputSample(mp3Instance->handle);
	mp3Instance->totalSamples = sceMp3GetFrameNum(mp3Instance->handle) * mp3Instance->maxSample;
	mp3Instance->channel = sceAudioSRCChReserve(mp3Instance->maxSample, mp3Instance->samplingRate, mp3Instance->numChannels);

	TraceLog(LOG_DEBUG, "%d %d %d %d %d", mp3Instance->samplingRate, mp3Instance->numChannels, mp3Instance->maxSample, mp3_read_title_author(mp3Instance->fd, mp3Instance), mp3Instance->title[0] == 0 && mp3Instance->author[0] == 0 ? mp3_read_title_author_v2(mp3Instance->fd, mp3Instance) : 0);
	if constexpr (MP3_USE_THREAD) mp3Instance->thread = sceKernelCreateThread("mp3_play_thread", (SceKernelThreadEntry) mp3_thread, 0x10, 0x10000, PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU, nullptr);
	if constexpr (MP3_USE_THREAD) sceKernelStartThread(mp3Instance->thread, sizeof(MP3Instance*), &mp3Instance);
    return mp3Instance;
}

void mp3_stopPlayback(MP3Instance * mp3Instance) {
	if (mp3Instance == nullptr) return;
	mp3Instance->over = true;
	mp3Instance->paused = true;
	if constexpr (MP3_USE_THREAD) sceKernelWaitThreadEnd(mp3Instance->thread, NULL);

	if (mp3Instance->channel >= 0) while (sceAudioSRCChRelease() < 0) {
		sceKernelDelayThreadCB(100);
	}
	
	int status = sceMp3ReleaseMp3Handle(mp3Instance->handle);
	if (status < 0) {
		TraceLog(LOG_ERROR, "sceMp3ReleaseMp3Handle returned 0x%08X\n", status);
	}
	
	status = sceMp3TermResource();
	if (status < 0) {
		TraceLog(LOG_ERROR, "sceMp3TermResource returned 0x%08X\n", status);
	}
	
	PHYSFS_close(mp3Instance->fd);
	free(mp3Instance);
}