#include "iso.h"
#include <archive.h>
#include <archive_entry.h>

extern LUFUS_DRIVE lufus_drive[MAX_DRIVES];

int ExtractISO(const char* src_iso, const char* dest_dir, int scan)
{
    struct archive* a;
    struct archive_entry* entry;
    int r;
    char outpath[PATH_MAX];

    a = archive_read_new();
    archive_read_support_format_iso9660(a);
    archive_read_support_format_zip(a);

    r = archive_read_open_filename(a, src_iso, 65536);
    if (r != ARCHIVE_OK) {
        uprintf("Failed to open ISO: %s", archive_error_string(a));
        archive_read_free(a);
        return 0;
    }

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char* pathname = archive_entry_pathname(entry);
        if (scan) {
            uprintf("ISO entry: %s", pathname);
            archive_read_data_skip(a);
            continue;
        }
        safe_sprintf(outpath, sizeof(outpath), "%s/%s", dest_dir, pathname);
        archive_entry_set_pathname(entry, outpath);
        r = archive_read_extract(a, entry, ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_TIME);
        if (r != ARCHIVE_OK)
            uprintf("Extract error: %s", archive_error_string(a));
    }

    r = archive_read_free(a);
    return (r == ARCHIVE_OK);
}

int ExtractZip(const char* src_zip, const char* dest_dir)
{
    struct archive* a;
    struct archive_entry* entry;
    int r;
    char outpath[PATH_MAX];

    a = archive_read_new();
    archive_read_support_format_zip(a);

    r = archive_read_open_filename(a, src_zip, 65536);
    if (r != ARCHIVE_OK) {
        uprintf("Failed to open ZIP: %s", archive_error_string(a));
        archive_read_free(a);
        return 0;
    }

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char* pathname = archive_entry_pathname(entry);
        safe_sprintf(outpath, sizeof(outpath), "%s/%s", dest_dir, pathname);
        archive_entry_set_pathname(entry, outpath);
        r = archive_read_extract(a, entry, ARCHIVE_EXTRACT_PERM | ARCHIVE_EXTRACT_TIME);
        if (r != ARCHIVE_OK)
            uprintf("Extract error: %s", archive_error_string(a));
    }

    r = archive_read_free(a);
    return (r == ARCHIVE_OK);
}

int64_t ExtractISOFile(const char* iso, const char* iso_file, const char* dest_file, uint32_t attributes)
{
    struct archive* a;
    struct archive_entry* entry;
    int r;
    int64_t total = 0;
    FILE* out;
    const void* buff;
    size_t size;
    int64_t offset;

    a = archive_read_new();
    archive_read_support_format_iso9660(a);
    archive_read_support_format_zip(a);

    r = archive_read_open_filename(a, iso, 65536);
    if (r != ARCHIVE_OK) {
        archive_read_free(a);
        return -1;
    }

    out = fopen(dest_file, "wb");
    if (!out) {
        archive_read_free(a);
        return -1;
    }

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char* pathname = archive_entry_pathname(entry);
        if (safe_strcmp(pathname, iso_file) == 0) {
            while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
                fwrite(buff, 1, size, out);
                total += size;
            }
            break;
        }
        archive_read_data_skip(a);
    }

    fclose(out);
    archive_read_free(a);
    return total;
}

uint32_t ReadISOFileToBuffer(const char* iso, const char* iso_file, uint8_t** buf)
{
    struct archive* a;
    struct archive_entry* entry;
    int r;
    uint32_t total = 0;
    uint32_t alloc = 65536;
    const void* buff;
    size_t size;
    int64_t offset;

    *buf = (uint8_t*)malloc(alloc);
    if (!*buf)
        return 0;

    a = archive_read_new();
    archive_read_support_format_iso9660(a);
    archive_read_support_format_zip(a);

    r = archive_read_open_filename(a, iso, 65536);
    if (r != ARCHIVE_OK) {
        archive_read_free(a);
        safe_free(*buf);
        return 0;
    }

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char* pathname = archive_entry_pathname(entry);
        if (safe_strcmp(pathname, iso_file) == 0) {
            while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
                if (total + size > alloc) {
                    alloc = (total + size) * 2;
                    *buf = (uint8_t*)realloc(*buf, alloc);
                }
                memcpy(*buf + total, buff, size);
                total += size;
            }
            break;
        }
        archive_read_data_skip(a);
    }

    archive_read_free(a);
    return total;
}

int HasEfiImgBootLoaders(void* iso)
{
    return 0;
}

int DumpFatDir(void* iso, const char* path, int32_t cluster)
{
    return 0;
}

int InstallSyslinux(uint32_t drive_index, char drive_letter, int fs)
{
    char device[PATH_MAX];
    char cmd[1024];
    int status;

    if (drive_index >= MAX_DRIVES || !lufus_drive[drive_index].id)
        return 0;
    safe_sprintf(device, sizeof(device), "/dev/%s", lufus_drive[drive_index].id);

    safe_sprintf(cmd, sizeof(cmd), "syslinux -i '%s'", device);
    uprintf("Running: %s", cmd);
    status = system(cmd);
    return (WIFEXITED(status) && WEXITSTATUS(status) == 0);
}

uint16_t GetSyslinuxVersion(char* buf, size_t buf_size, char** ext)
{
    FILE* f = popen("syslinux --version 2>&1", "r");
    if (!f) return 0;
    if (fgets(buf, (int)buf_size, f) != NULL) {
        char* p = strstr(buf, "version ");
        if (p) {
            p += 8;
            int maj = atoi(p);
            int min = 0;
            char* dot = strchr(p, '.');
            if (dot) min = atoi(dot + 1);
            pclose(f);
            return (uint16_t)((maj << 8) | min);
        }
    }
    pclose(f);
    return 0;
}

int SetAutorun(const char* path)
{
    FILE* f;
    char autorun[PATH_MAX];
    safe_sprintf(autorun, sizeof(autorun), "%s/autorun.inf", path);
    f = fopen(autorun, "w");
    if (!f) return 0;
    fprintf(f, "[autorun]\n");
    fclose(f);
    return 1;
}
