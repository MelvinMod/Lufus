#include "ddwrite.h"
#include <sys/stat.h>

extern LUFUS_DRIVE lufus_drive[MAX_DRIVES];
extern int op_in_progress;

static int open_device_rw(const char* device)
{
    int fd;
    int retries = 5;

    while (retries > 0) {
        fd = open(device, O_RDWR | O_SYNC);
        if (fd >= 0)
            return fd;
        usleep(500000);
        retries--;
    }
    return -1;
}

static void unmount_device(const char* devname)
{
    char cmd[512];
    FILE* f;
    char line[1024];
    char devpath[256];

    snprintf(devpath, sizeof(devpath), "/dev/%s", devname);
    f = fopen("/proc/mounts", "r");
    if (!f) return;

    while (fgets(line, sizeof(line), f)) {
        char* p = strchr(line, ' ');
        if (!p) continue;
        *p = '\0';
        if (strncmp(line, devpath, strlen(devpath)) == 0) {
            char* mp = p + 1;
            p = strchr(mp, ' ');
            if (p) *p = '\0';
            safe_sprintf(cmd, sizeof(cmd), "umount '%s' 2>/dev/null", mp);
            IGNORE_RETVAL(system(cmd));
        }
    }
    fclose(f);

    safe_sprintf(cmd, sizeof(cmd), "umount /dev/%s* 2>/dev/null", devname);
    IGNORE_RETVAL(system(cmd));

    usleep(500000);
}

static void clear_mbr_pbr(const char* device)
{
    int fd;
    char buf[8192];

    fd = open(device, O_WRONLY);
    if (fd < 0) return;

    memset(buf, 0, sizeof(buf));
    write(fd, buf, sizeof(buf));
    close(fd);
    sync();
}

void* DDWriteThread(void* param)
{
    uint32_t device_index = (uint32_t)(uintptr_t)param;
    char device_path[PATH_MAX];
    int fd_src = -1, fd_dst = -1;
    uint8_t* buf = NULL;
    ssize_t nread, nwritten;
    off_t total_size, written_size;
    struct stat st;
    int ret = 0;

    op_in_progress = 1;
    EnableControls(0, 1);

    if (device_index >= MAX_DRIVES || !lufus_drive[device_index].id || !image_path) {
        uprintf("Invalid device or no image selected");
        ret = 1;
        goto out;
    }

    snprintf(device_path, sizeof(device_path), "/dev/%s", lufus_drive[device_index].id);

    if (stat(image_path, &st) < 0) {
        uprintf("Failed to stat image: %s", StrError(errno));
        ret = 1;
        goto out;
    }
    total_size = st.st_size;

    uprintf("DD Mode: Writing %s (%s) to %s",
        image_path,
        SizeToHumanReadable(total_size, 0, 0),
        device_path);

    unmount_device(lufus_drive[device_index].id);
    clear_mbr_pbr(device_path);

    fd_src = open(image_path, O_RDONLY);
    if (fd_src < 0) {
        uprintf("Failed to open image: %s", StrError(errno));
        ret = 1;
        goto out;
    }

    fd_dst = open_device_rw(device_path);
    if (fd_dst < 0) {
        uprintf("Failed to open device: %s", StrError(errno));
        ret = 1;
        goto out;
    }

    buf = (uint8_t*)malloc(DD_BUFFER_SIZE);
    if (!buf) {
        uprintf("Failed to allocate buffer");
        ret = 1;
        goto out;
    }

    written_size = 0;
    UpdateProgress(OP_FILE_COPY, 0.0f);

    while ((nread = read(fd_src, buf, DD_BUFFER_SIZE)) > 0) {
        nwritten = write(fd_dst, buf, nread);
        if (nwritten != nread) {
            uprintf("Write error: %s", StrError(errno));
            ret = 1;
            goto out;
        }
        written_size += nwritten;
        UpdateProgressWithInfo(OP_FILE_COPY, 0, (uint64_t)written_size, (uint64_t)total_size);
    }

    if (nread < 0) {
        uprintf("Read error: %s", StrError(errno));
        ret = 1;
        goto out;
    }

    sync();
    uprintf("DD write completed successfully: %s written", SizeToHumanReadable(written_size, 0, 0));

out:
    if (buf) free(buf);
    if (fd_src >= 0) close(fd_src);
    if (fd_dst >= 0) close(fd_dst);

    op_in_progress = 0;
    EnableControls(1, 1);
    return (void*)(uintptr_t)ret;
}
