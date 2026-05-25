#include "net.h"
#include <curl/curl.h>

extern LUFUS_UPDATE lufus_update;

static size_t write_cb(void* ptr, size_t size, size_t nmemb, void* userdata)
{
    download_ctx* ctx = (download_ctx*)userdata;
    size_t total = size * nmemb;

    if (ctx->file) {
        FILE* f = fopen(ctx->file, "ab");
        if (f) {
            fwrite(ptr, 1, total, f);
            fclose(f);
        }
    } else if (ctx->buffer) {
        *ctx->buffer = realloc(*ctx->buffer, ctx->downloaded + total + 1);
        if (*ctx->buffer) {
            memcpy(*ctx->buffer + ctx->downloaded, ptr, total);
            ctx->downloaded += total;
            (*ctx->buffer)[ctx->downloaded] = 0;
        }
    }

    ctx->downloaded += total;
    if (ctx->total > 0)
        UpdateProgressWithInfo(OP_FILE_COPY, 0, ctx->downloaded, ctx->total);

    return total;
}

static int progress_cb(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow)
{
    download_ctx* ctx = (download_ctx*)clientp;
    if (dltotal > 0) {
        ctx->total = (uint64_t)dltotal;
        UpdateProgressWithInfo(OP_FILE_COPY, 0, (uint64_t)dlnow, (uint64_t)dltotal);
    }
    return 0;
}

uint64_t DownloadToFileOrBufferEx(const char* url, const char* file, const char* user_agent, uint8_t** buffer, void* hProgressDialog, int bTaskBarProgress)
{
    CURL* curl;
    CURLcode res;
    download_ctx ctx = {0};

    ctx.url = safe_strdup(url);
    ctx.file = safe_strdup(file);
    ctx.buffer = buffer;
    ctx.progress_dialog = hProgressDialog;
    ctx.taskbar_progress = bTaskBarProgress;

    if (file)
        remove(file);

    curl = curl_easy_init();
    if (!curl) {
        safe_free(ctx.url);
        safe_free(ctx.file);
        return 0;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_cb);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &ctx);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    if (user_agent)
        curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent);

    res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    safe_free(ctx.url);
    safe_free(ctx.file);

    if (res != CURLE_OK) {
        uprintf("Download failed: %s", curl_easy_strerror(res));
        return 0;
    }

    return ctx.downloaded;
}

int DownloadSignedFile(const char* url, const char* file, void* hProgressDialog, int PromptOnError)
{
    uint64_t sz = DownloadToFileOrBufferEx(url, file, NULL, NULL, hProgressDialog, 1);
    return (sz > 0);
}

int CheckForUpdates(int force)
{
    uprintf("Checking for updates...");
    return 0;
}

void DownloadNewVersion(void)
{
    uprintf("Downloading new version...");
}

int DownloadISO(void)
{
    uprintf("Downloading ISO...");
    return 0;
}

int IsDownloadable(const char* url)
{
    CURL* curl;
    CURLcode res;
    long code = 0;

    curl = curl_easy_init();
    if (!curl) return 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);

    res = curl_easy_perform(curl);
    if (res == CURLE_OK)
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

    curl_easy_cleanup(curl);
    return (code == 200);
}
