#include "badblocks.h"
#include <pthread.h>

extern LUFUS_DRIVE lufus_drive[MAX_DRIVES];
extern int op_in_progress;

void* BadBlocksThread(void* param)
{
    uint32_t DriveIndex = (uint32_t)(uintptr_t)param;
    char device[PATH_MAX];
    int fd;
    uint8_t *buf = NULL;
    uint8_t *cmp = NULL;
    int64_t i;
    uint64_t drive_size;
    int pass, pattern;
    int bad = 0;

    if (DriveIndex >= MAX_DRIVES || !lufus_drive[DriveIndex].id)
        return NULL;

    safe_sprintf(device, sizeof(device), "/dev/%s", lufus_drive[DriveIndex].id);
    fd = open(device, O_RDWR | O_SYNC);
    if (fd < 0) {
        uprintf("Failed to open %s: %s", device, StrError(errno));
        return NULL;
    }

    if (ioctl(fd, BLKGETSIZE64, &drive_size) < 0) {
        close(fd);
        return NULL;
    }

    buf = (uint8_t*)malloc(BADBLOCK_BLOCK_SIZE);
    cmp = (uint8_t*)malloc(BADBLOCK_BLOCK_SIZE);
    if (!buf || !cmp) {
        safe_free(buf);
        safe_free(cmp);
        close(fd);
        return NULL;
    }

    op_in_progress = 1;
    EnableControls(0, 1);

    for (pass = 0; pass < 1; pass++) {
        for (pattern = 0; pattern < BADBLOCK_PATTERN_COUNT; pattern++) {
            uprintf("Pass %d, pattern %d", pass + 1, pattern + 1);
            memset(buf, (pattern == 0) ? 0x00 : (pattern == 1) ? 0xFF : (pattern == 2) ? 0x55 : 0xAA, BADBLOCK_BLOCK_SIZE);

            for (i = 0; i < (int64_t)(drive_size / BADBLOCK_BLOCK_SIZE); i++) {
                if (write(fd, buf, BADBLOCK_BLOCK_SIZE) != BADBLOCK_BLOCK_SIZE) {
                    uprintf("Bad block at offset %lld", i * BADBLOCK_BLOCK_SIZE);
                    bad++;
                }
                UpdateProgressWithInfo(OP_BADBLOCKS, 0, (uint64_t)i * BADBLOCK_BLOCK_SIZE, drive_size);
            }
            sync();
            lseek(fd, 0, SEEK_SET);
            for (i = 0; i < (int64_t)(drive_size / BADBLOCK_BLOCK_SIZE); i++) {
                if (read(fd, cmp, BADBLOCK_BLOCK_SIZE) != BADBLOCK_BLOCK_SIZE) {
                    uprintf("Read error at offset %lld", i * BADBLOCK_BLOCK_SIZE);
                    bad++;
                } else if (memcmp(buf, cmp, BADBLOCK_BLOCK_SIZE) != 0) {
                    uprintf("Verify error at offset %lld", i * BADBLOCK_BLOCK_SIZE);
                    bad++;
                }
                UpdateProgressWithInfo(OP_BADBLOCKS, 0, (uint64_t)i * BADBLOCK_BLOCK_SIZE, drive_size);
            }
            lseek(fd, 0, SEEK_SET);
        }
    }

    uprintf("Bad blocks check complete. Found %d bad blocks.", bad);

    safe_free(buf);
    safe_free(cmp);
    close(fd);

    op_in_progress = 0;
    EnableControls(1, 1);
    return NULL;
}
