#ifndef NET_H
#define NET_H

#include "lufus.h"

typedef struct {
    char* url;
    char* file;
    uint8_t** buffer;
    void* progress_dialog;
    int taskbar_progress;
    uint64_t downloaded;
    uint64_t total;
} download_ctx;

extern uint64_t DownloadToFileOrBufferEx(const char* url, const char* file, const char* user_agent, uint8_t** buffer, void* hProgressDialog, int bTaskBarProgress);
#define DownloadToFileOrBuffer(url, file, buffer, hProgressDialog, bTaskBarProgress) \
    DownloadToFileOrBufferEx(url, file, NULL, buffer, hProgressDialog, bTaskBarProgress)
extern int DownloadSignedFile(const char* url, const char* file, void* hProgressDialog, int PromptOnError);
extern int CheckForUpdates(int force);
extern void DownloadNewVersion(void);
extern int DownloadISO(void);
extern int IsDownloadable(const char* url);

#endif
