#include "dev.h"
#include <sys/stat.h>
#include <libudev.h>

extern LUFUS_DRIVE lufus_drive[MAX_DRIVES];
extern int enable_HDDs, enable_VHDs, detect_fakes, usb_debug;
extern int list_non_usb_removable_drives;

static struct udev* g_udev = NULL;

static int read_sys_int(const char* path)
{
    FILE* f = fopen(path, "r");
    int val = 0;
    if (f) {
        IGNORE_RETVAL(fscanf(f, "%d", &val));
        fclose(f);
    }
    return val;
}

static uint64_t read_sys_uint64(const char* path)
{
    FILE* f = fopen(path, "r");
    uint64_t val = 0;
    if (f) {
        IGNORE_RETVAL(fscanf(f, "%llu", (unsigned long long*)&val));
        fclose(f);
    }
    return val;
}

static int read_sys_string(const char* path, char* out, size_t out_len)
{
    FILE* f = fopen(path, "r");
    if (!f) return 0;
    if (fgets(out, (int)out_len, f) != NULL) {
        size_t len = strlen(out);
        while (len > 0 && (out[len-1] == '\n' || out[len-1] == '\r'))
            out[--len] = '\0';
    }
    fclose(f);
    return 1;
}

static int get_block_device_size(const char* devname, uint64_t* size)
{
    char path[256];
    int fd;
    int r = 0;
    snprintf(path, sizeof(path), "/dev/%s", devname);
    fd = open(path, O_RDONLY);
    if (fd >= 0) {
        if (ioctl(fd, BLKGETSIZE64, size) == 0)
            r = 1;
        close(fd);
    }
    if (!r) {
        snprintf(path, sizeof(path), "/sys/class/block/%s/size", devname);
        *size = read_sys_uint64(path) * 512ULL;
        r = (*size > 0);
    }
    return r;
}

static int is_removable(const char* devname)
{
    char path[256];
    int removable;
    snprintf(path, sizeof(path), "/sys/class/block/%s/removable", devname);
    removable = read_sys_int(path);
    if (removable)
        return 1;
    snprintf(path, sizeof(path), "/sys/class/block/%s/ro", devname);
    if (read_sys_int(path))
        return 0;
    snprintf(path, sizeof(path), "/sys/class/block/%s/device/type", devname);
    char type[32] = {0};
    read_sys_string(path, type, sizeof(type));
    if (safe_strcmp(type, "SD") == 0 || safe_strcmp(type, "MMC") == 0)
        return 1;
    return 0;
}

static int is_readonly(const char* devname)
{
    char path[256];
    snprintf(path, sizeof(path), "/sys/class/block/%s/ro", devname);
    return read_sys_int(path);
}

static void get_usb_properties(const char* devname, usb_device_props* props)
{
    struct udev_device* dev;
    struct udev_device* parent;
    struct udev_device* usb_dev;
    const char* str;

    memset(props, 0, sizeof(*props));
    props->vid = -1;
    props->pid = -1;

    if (!g_udev)
        g_udev = udev_new();
    if (!g_udev)
        return;

    char path[256];
    snprintf(path, sizeof(path), "/sys/class/block/%s", devname);
    dev = udev_device_new_from_syspath(g_udev, path);
    if (!dev)
        return;

    parent = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
    if (parent) {
        props->is_USB = 1;
        str = udev_device_get_sysattr_value(parent, "idVendor");
        if (str) props->vid = (int32_t)strtol(str, NULL, 16);
        str = udev_device_get_sysattr_value(parent, "idProduct");
        if (str) props->pid = (int32_t)strtol(str, NULL, 16);
        str = udev_device_get_sysattr_value(parent, "speed");
        if (str) {
            int speed = atoi(str);
            if (speed >= 5000) props->speed = USB_SPEED_SUPER;
            else if (speed >= 480) props->speed = USB_SPEED_HIGH;
            else if (speed >= 12) props->speed = USB_SPEED_FULL;
            else if (speed > 0) props->speed = USB_SPEED_LOW;
        }
        str = udev_device_get_sysattr_value(parent, "bDeviceClass");
        if (str && atoi(str) == 8)
            props->is_UASP = 1;
    } else {
        parent = udev_device_get_parent_with_subsystem_devtype(dev, "scsi", "scsi_device");
        if (parent) {
            props->is_SCSI = 1;
            str = udev_device_get_sysattr_value(parent, "type");
            if (str && (safe_strcmp(str, "SD") == 0 || safe_strcmp(str, "MMC") == 0))
                props->is_CARD = 1;
            usb_dev = udev_device_get_parent_with_subsystem_devtype(parent, "usb", "usb_device");
            if (usb_dev) {
                props->is_USB = 1;
                str = udev_device_get_sysattr_value(usb_dev, "idVendor");
                if (str) props->vid = (int32_t)strtol(str, NULL, 16);
                str = udev_device_get_sysattr_value(usb_dev, "idProduct");
                if (str) props->pid = (int32_t)strtol(str, NULL, 16);
            }
        }
    }

    udev_device_unref(dev);
}

static int get_device_model(const char* devname, char* out, size_t out_len)
{
    char path[256];
    int r = 0;
    out[0] = '\0';
    snprintf(path, sizeof(path), "/sys/class/block/%s/device/model", devname);
    r = read_sys_string(path, out, out_len);
    if (!r) {
        snprintf(path, sizeof(path), "/sys/class/block/%s/device/name", devname);
        r = read_sys_string(path, out, out_len);
    }
    trim(out);
    return r;
}

static int get_device_vendor(const char* devname, char* out, size_t out_len)
{
    char path[256];
    int r = 0;
    out[0] = '\0';
    snprintf(path, sizeof(path), "/sys/class/block/%s/device/vendor", devname);
    r = read_sys_string(path, out, out_len);
    trim(out);
    return r;
}

static int is_vhd(const char* devname)
{
    char path[256];
    char vendor[64] = {0};
    snprintf(path, sizeof(path), "/sys/class/block/%s/device/vendor", devname);
    read_sys_string(path, vendor, sizeof(vendor));
    if (safe_strstr(vendor, "Virtual") != NULL)
        return 1;
    return 0;
}

static int get_mountpoints(const char* devname, char mounts[8][PATH_MAX], int* count)
{
    FILE* f;
    char line[1024];
    char devpath[256];
    int i = 0;
    *count = 0;
    snprintf(devpath, sizeof(devpath), "/dev/%s", devname);
    f = fopen("/proc/mounts", "r");
    if (!f) return 0;
    while (fgets(line, sizeof(line), f) && i < 8) {
        char* p = strchr(line, ' ');
        if (!p) continue;
        *p = '\0';
        if (strcmp(line, devpath) == 0 || strncmp(line, devpath, strlen(devpath)) == 0) {
            char* mp = p + 1;
            p = strchr(mp, ' ');
            if (p) *p = '\0';
            strncpy(mounts[i], mp, PATH_MAX - 1);
            mounts[i][PATH_MAX - 1] = '\0';
            i++;
        }
    }
    fclose(f);
    *count = i;
    return i;
}

static void unmount_device(const char* devname)
{
    char mounts[8][PATH_MAX];
    int count, i;
    char cmd[512];
    get_mountpoints(devname, mounts, &count);
    for (i = 0; i < count; i++) {
        safe_sprintf(cmd, sizeof(cmd), "umount '%s' 2>/dev/null", mounts[i]);
        IGNORE_RETVAL(system(cmd));
    }
    snprintf(cmd, sizeof(cmd), "umount /dev/%s* 2>/dev/null", devname);
    IGNORE_RETVAL(system(cmd));
}

void ClearDrives(void)
{
    int i;
    for (i = 0; i < MAX_DRIVES && lufus_drive[i].size != 0; i++) {
        safe_free(lufus_drive[i].id);
        safe_free(lufus_drive[i].name);
        safe_free(lufus_drive[i].display_name);
        safe_free(lufus_drive[i].label);
        safe_free(lufus_drive[i].hub);
    }
    memset(lufus_drive, 0, sizeof(lufus_drive));
}

int GetDevices(uint32_t devnum)
{
    DIR* dir;
    struct dirent* ent;
    int num_drives = 0;
    char devpath[PATH_MAX];
    char display_msg[128];
    char letter_name[32];
    char* display_name;
    char* label;
    uint64_t drive_size;
    int drive_number;
    usb_device_props props;
    const char* usb_speed_name[USB_SPEED_MAX] = { "USB", "USB 1.0", "USB 1.1", "USB 2.0", "USB 3.0", "USB 3.1" };

    IGNORE_RETVAL(gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(hDeviceList)));
    ClearDrives();

    dir = opendir("/sys/class/block");
    if (!dir) {
        uprintf("Failed to open /sys/class/block: %s", StrError(errno));
        return 0;
    }

    while ((ent = readdir(dir)) != NULL && num_drives < MAX_DRIVES) {
        if (ent->d_name[0] == '.')
            continue;
        if (strncmp(ent->d_name, "sd", 2) != 0 &&
            strncmp(ent->d_name, "mmcblk", 6) != 0 &&
            strncmp(ent->d_name, "nvme", 4) != 0 &&
            strncmp(ent->d_name, "vd", 2) != 0)
            continue;

        if (!is_removable(ent->d_name) && !enable_HDDs)
            continue;

        drive_number = atoi(ent->d_name + 2);
        if (strncmp(ent->d_name, "mmcblk", 6) == 0)
            drive_number = atoi(ent->d_name + 6);
        else if (strncmp(ent->d_name, "nvme", 4) == 0)
            drive_number = atoi(ent->d_name + 4);

        snprintf(devpath, sizeof(devpath), "/dev/%s", ent->d_name);

        if (!get_block_device_size(ent->d_name, &drive_size))
            continue;
        if (drive_size < MIN_DRIVE_SIZE)
            continue;

        get_usb_properties(ent->d_name, &props);

        if (!props.is_USB && !props.is_SCSI && !enable_HDDs)
            continue;
        if (is_vhd(ent->d_name) && !enable_VHDs)
            continue;

        char model[128] = {0}, vendor[64] = {0};
        get_device_model(ent->d_name, model, sizeof(model));
        get_device_vendor(ent->d_name, vendor, sizeof(vendor));

        lufus_drive[num_drives].id = safe_strdup(ent->d_name);
        lufus_drive[num_drives].name = safe_strdup(model);
        lufus_drive[num_drives].size = drive_size;
        lufus_drive[num_drives].index = (uint32_t)drive_number;

        if (props.vid >= 0 && props.pid >= 0) {
            static_sprintf(display_msg, "%s %s (%04X:%04X)", vendor, model, props.vid, props.pid);
        } else {
            static_sprintf(display_msg, "%s %s", vendor, model);
        }
        lufus_drive[num_drives].display_name = safe_strdup(display_msg);

        char* size_str = SizeToHumanReadable(drive_size, 0, 0);
        if (props.speed > 0 && props.speed < USB_SPEED_MAX) {
            static_sprintf(display_msg, "%s [%s] %s", ent->d_name, size_str, usb_speed_name[props.speed]);
        } else {
            static_sprintf(display_msg, "%s [%s]", ent->d_name, size_str);
        }

        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hDeviceList), display_msg);
        num_drives++;
    }
    closedir(dir);

    if (num_drives > 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(hDeviceList), 0);

    return num_drives;
}

int CycleDevice(int index)
{
    if (index < 0 || index >= MAX_DRIVES || !lufus_drive[index].id)
        return -1;
    uprintf("Cycling device %s...", lufus_drive[index].id);
    char cmd[256];
    safe_sprintf(cmd, sizeof(cmd), "udisksctl power-off -b /dev/%s 2>/dev/null || echo 1 > /sys/class/block/%s/device/delete 2>/dev/null",
        lufus_drive[index].id, lufus_drive[index].id);
    IGNORE_RETVAL(system(cmd));
    return 0;
}

int CyclePort(int index)
{
    if (index < 0 || index >= MAX_DRIVES || !lufus_drive[index].id)
        return -1;
    uprintf("Cycling USB port for %s...", lufus_drive[index].id);
    return CycleDevice(index);
}

int GetDriveNumber(int fd, const char* path)
{
    struct stat st;
    if (fstat(fd, &st) < 0)
        return -1;
    if (!S_ISBLK(st.st_mode))
        return -1;
    return (int)minor(st.st_rdev);
}

int IsMediaPresent(uint32_t drive_index)
{
    if (drive_index >= MAX_DRIVES || !lufus_drive[drive_index].id)
        return 0;
    char path[256];
    snprintf(path, sizeof(path), "/sys/class/block/%s", lufus_drive[drive_index].id);
    struct stat st;
    return (stat(path, &st) == 0);
}

uint64_t GetDriveSize(uint32_t drive_index)
{
    if (drive_index >= MAX_DRIVES || !lufus_drive[drive_index].id)
        return 0;
    uint64_t size = 0;
    get_block_device_size(lufus_drive[drive_index].id, &size);
    return size;
}

int GetDriveLabel(uint32_t drive_index, char* drive_letters, char** label, int silent)
{
    if (drive_index >= MAX_DRIVES || !lufus_drive[drive_index].id)
        return 0;
    if (drive_letters)
        drive_letters[0] = '\0';
    if (label)
        *label = lufus_drive[drive_index].label;
    return 1;
}

int GetDrivePartitionData(uint32_t DeviceNumber, char* fs_name, size_t fs_name_size, int silent)
{
    if (DeviceNumber >= MAX_DRIVES || !lufus_drive[DeviceNumber].id)
        return 0;
    if (fs_name)
        fs_name[0] = '\0';
    return 1;
}

int IsMsDevDrive(uint32_t drive_index)
{
    return 0;
}

int IsFilteredDrive(uint32_t drive_index)
{
    return 0;
}

int IsHDD(uint32_t DriveIndex, uint16_t vid, uint16_t pid, const char* strid)
{
    if (DriveIndex >= MAX_DRIVES || !lufus_drive[DriveIndex].id)
        return 0;
    if (!is_removable(lufus_drive[DriveIndex].id))
        return 1;
    return 0;
}
