#include "format.h"
#include <sys/wait.h>

extern LUFUS_DRIVE lufus_drive[MAX_DRIVES];
extern int op_in_progress;

static int run_mkfs(const char* fs, const char* device, const char* label, uint32_t cluster_size)
{
    char cmd[1024];
    int status;
    int r = 0;

    if (safe_strcmp(fs, "fat32") == 0 || safe_strcmp(fs, "vfat") == 0) {
        if (cluster_size > 0)
            safe_sprintf(cmd, sizeof(cmd), "mkfs.vfat -F 32 -s %u -n '%s' '%s'", cluster_size / 512, label, device);
        else
            safe_sprintf(cmd, sizeof(cmd), "mkfs.vfat -F 32 -n '%s' '%s'", label, device);
    } else if (safe_strcmp(fs, "ntfs") == 0) {
        safe_sprintf(cmd, sizeof(cmd), "mkfs.ntfs -f -L '%s' '%s'", label, device);
    } else if (safe_strcmp(fs, "exfat") == 0) {
        safe_sprintf(cmd, sizeof(cmd), "mkfs.exfat -n '%s' '%s'", label, device);
    } else if (safe_strcmp(fs, "ext2") == 0) {
        safe_sprintf(cmd, sizeof(cmd), "mkfs.ext2 -L '%s' '%s'", label, device);
    } else if (safe_strcmp(fs, "ext3") == 0) {
        safe_sprintf(cmd, sizeof(cmd), "mkfs.ext3 -L '%s' '%s'", label, device);
    } else if (safe_strcmp(fs, "ext4") == 0) {
        safe_sprintf(cmd, sizeof(cmd), "mkfs.ext4 -L '%s' '%s'", label, device);
    } else if (safe_strcmp(fs, "udf") == 0) {
        safe_sprintf(cmd, sizeof(cmd), "mkudfs '%s'", device);
    } else {
        uprintf("Unsupported filesystem: %s", fs);
        return 0;
    }

    uprintf("Running: %s", cmd);
    status = system(cmd);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        r = 1;
    else
        uprintf("Command failed with status %d", WEXITSTATUS(status));
    return r;
}

static int write_zero_mbr(const char* device)
{
    int fd;
    char buf[8 * 1024];
    int r = 0;

    fd = open(device, O_WRONLY);
    if (fd < 0) {
        uprintf("Failed to open %s: %s", device, StrError(errno));
        return 0;
    }
    memset(buf, 0, sizeof(buf));
    if (write(fd, buf, sizeof(buf)) == sizeof(buf))
        r = 1;
    close(fd);
    sync();
    return r;
}

static int create_partition_table(const char* device, int pt_type)
{
    char cmd[1024];
    int status;

    if (pt_type == PARTITION_STYLE_GPT) {
        safe_sprintf(cmd, sizeof(cmd), "parted -s '%s' mklabel gpt", device);
    } else if (pt_type == PARTITION_STYLE_MBR) {
        safe_sprintf(cmd, sizeof(cmd), "parted -s '%s' mklabel msdos", device);
    } else {
        return 1;
    }

    uprintf("Running: %s", cmd);
    status = system(cmd);
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        safe_sprintf(cmd, sizeof(cmd), "parted -s '%s' mkpart primary 1MiB 100%%", device);
        uprintf("Running: %s", cmd);
        status = system(cmd);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            safe_sprintf(cmd, sizeof(cmd), "partprobe '%s' 2>/dev/null", device);
            IGNORE_RETVAL(system(cmd));
            return 1;
        }
    }
    return 0;
}

int WritePBR(int fd)
{
    uint8_t pbr[512];
    if (read(fd, pbr, 512) != 512)
        return 0;
    pbr[510] = 0x55;
    pbr[511] = 0xAA;
    if (lseek(fd, 0, SEEK_SET) != 0)
        return 0;
    if (write(fd, pbr, 512) != 512)
        return 0;
    return 1;
}

int FormatLargeFAT32(uint32_t DriveIndex, uint64_t PartitionOffset, uint32_t ClusterSize, const char* FSName, const char* Label, uint32_t Flags)
{
    char device[PATH_MAX];
    if (DriveIndex >= MAX_DRIVES || !lufus_drive[DriveIndex].id)
        return 0;
    safe_sprintf(device, sizeof(device), "/dev/%s", lufus_drive[DriveIndex].id);
    return run_mkfs("fat32", device, Label, ClusterSize);
}

int FormatExtFs(uint32_t DriveIndex, uint64_t PartitionOffset, uint32_t BlockSize, const char* FSName, const char* Label, uint32_t Flags)
{
    char device[PATH_MAX];
    if (DriveIndex >= MAX_DRIVES || !lufus_drive[DriveIndex].id)
        return 0;
    safe_sprintf(device, sizeof(device), "/dev/%s", lufus_drive[DriveIndex].id);
    return run_mkfs(FSName, device, Label, BlockSize);
}

int FormatPartition(uint32_t DriveIndex, uint64_t PartitionOffset, uint32_t UnitAllocationSize, uint16_t FSType, const char* Label, uint32_t Flags)
{
    char device[PATH_MAX];
    const char* fsname = NULL;
    int r;

    if (DriveIndex >= MAX_DRIVES || !lufus_drive[DriveIndex].id)
        return 0;

    safe_sprintf(device, sizeof(device), "/dev/%s", lufus_drive[DriveIndex].id);

    switch (FSType) {
    case FS_FAT16: fsname = "fat16"; break;
    case FS_FAT32: fsname = "fat32"; break;
    case FS_NTFS:  fsname = "ntfs"; break;
    case FS_UDF:   fsname = "udf"; break;
    case FS_EXFAT: fsname = "exfat"; break;
    case FS_EXT2:  fsname = "ext2"; break;
    case FS_EXT3:  fsname = "ext3"; break;
    case FS_EXT4:  fsname = "ext4"; break;
    default:
        uprintf("Unknown filesystem type %d", FSType);
        return 0;
    }

    UpdateProgress(OP_ZERO_MBR, 0.0f);
    r = write_zero_mbr(device);
    if (!r) {
        uprintf("Failed to zero MBR");
        return 0;
    }
    UpdateProgress(OP_ZERO_MBR, 100.0f);

    UpdateProgress(OP_PARTITION, 0.0f);
    r = create_partition_table(device, partition_type);
    if (!r) {
        uprintf("Failed to create partition table");
        return 0;
    }
    UpdateProgress(OP_PARTITION, 100.0f);

    UpdateProgress(OP_FORMAT, 0.0f);
    r = run_mkfs(fsname, device, Label, UnitAllocationSize);
    if (!r) {
        uprintf("Failed to format partition");
        return 0;
    }
    UpdateProgress(OP_FORMAT, 100.0f);

    return 1;
}

void* FormatThread(void* param)
{
    uint32_t DriveIndex = (uint32_t)(uintptr_t)param;
    int r;

    op_in_progress = 1;
    EnableControls(0, 1);

    r = FormatPartition(DriveIndex, 0, 0, (uint16_t)fs_type, "NO_LABEL", 0);

    if (r) {
        uprintf("Format completed successfully");
    } else {
        uprintf("Format failed");
    }

    op_in_progress = 0;
    EnableControls(1, 1);
    return NULL;
}
