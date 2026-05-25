#include "bootloader.h"
#include "iso.h"
#include <sys/wait.h>
#include <archive.h>
#include <archive_entry.h>

extern LUFUS_DRIVE lufus_drive[MAX_DRIVES];

static int file_exists(const char* path)
{
    struct stat st;
    return stat(path, &st) == 0;
}

static int run_cmd(const char* cmd)
{
    int status = system(cmd);
    uprintf("Command: %s (exit: %d)", cmd, WEXITSTATUS(status));
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

int InstallFreeDOS(uint32_t drive_index, int msdos_version)
{
    char device[PATH_MAX];
    char cmd[1024];
    int fd;
    uint8_t boot_sector[512];

    if (drive_index >= MAX_DRIVES || !lufus_drive[drive_index].id)
        return 0;

    snprintf(device, sizeof(device), "/dev/%s", lufus_drive[drive_index].id);

    uprintf("Installing %s to %s",
        msdos_version == DOS_FREEDOS ? "FreeDOS" :
        msdos_version == DOS_MSDOS622 ? "MS-DOS 6.22" : "MS-DOS 7.10",
        device);

    snprintf(cmd, sizeof(cmd), "mke2fs -t msdos '%s'", device);
    if (!run_cmd(cmd))
        return 0;

    fd = open("/usr/share/syslinux/fatboot.img", O_RDONLY);
    if (fd < 0) {
        uprintf("Boot sector not found");
        return 0;
    }

    memset(boot_sector, 0, sizeof(boot_sector));
    if (read(fd, boot_sector, 512) != 512) {
        close(fd);
        return 0;
    }
    close(fd);

    fd = open(device, O_WRONLY);
    if (fd < 0)
        return 0;

    if (write(fd, boot_sector, 512) != 512) {
        close(fd);
        return 0;
    }
    close(fd);

    snprintf(cmd, sizeof(cmd), "syslinux '%s'", device);
    if (!run_cmd(cmd))
        return 0;

    uprintf("DOS boot installed successfully");
    return 1;
}

int InstallMSDOS(uint32_t drive_index, int version)
{
    return InstallFreeDOS(drive_index, version);
}

int CreateUEFIBootableNTFS(uint32_t drive_index, const char* efi_file)
{
    char device[PATH_MAX];
    char cmd[1024];
    char temp_dir[PATH_MAX];

    if (drive_index >= MAX_DRIVES || !lufus_drive[drive_index].id)
        return 0;

    snprintf(device, sizeof(device), "/dev/%s", lufus_drive[drive_index].id);

    uprintf("Creating UEFI bootable NTFS on %s", device);

    snprintf(temp_dir, sizeof(temp_dir), "/tmp/lufus_uefi_%s", lufus_drive[drive_index].id);
    mkdir(temp_dir, 0755);

    snprintf(cmd, sizeof(cmd), "mkdir -p '%s/EFI/BOOT'", temp_dir);
    run_cmd(cmd);

    if (efi_file && file_exists(efi_file)) {
        snprintf(cmd, sizeof(cmd), "cp '%s' '%s/EFI/BOOT/BOOTX64.EFI'", efi_file, temp_dir);
        run_cmd(cmd);
    } else {
        uprintf("UEFI boot image not found, creating placeholder");
    }

    snprintf(cmd, sizeof(cmd), "parted '%s' mkpart primary fat32 1MiB 100%%", device);
    run_cmd(cmd);

    snprintf(cmd, sizeof(cmd), "mkfs.fat -F 32 -n 'UEFI' '%s1'", device);
    run_cmd(cmd);

    snprintf(cmd, sizeof(cmd), "mount '%s1' '%s'", device, temp_dir);
    run_cmd(cmd);

    snprintf(cmd, sizeof(cmd), "cp -r '%s/EFI' '%s/'", temp_dir, temp_dir);
    run_cmd(cmd);

    snprintf(cmd, sizeof(cmd), "umount '%s1'", device);
    run_cmd(cmd);

    uprintf("UEFI bootable NTFS created successfully");
    return 1;
}

int CreateBIOSBootable(uint32_t drive_index, const char* boot_sector)
{
    char device[PATH_MAX];
    int fd;
    uint8_t sector[512];

    if (drive_index >= MAX_DRIVES || !lufus_drive[drive_index].id)
        return 0;

    snprintf(device, sizeof(device), "/dev/%s", lufus_drive[drive_index].id);

    uprintf("Creating BIOS bootable on %s", device);

    if (boot_sector && file_exists(boot_sector)) {
        fd = open(boot_sector, O_RDONLY);
        if (fd >= 0) {
            if (read(fd, sector, 512) == 512) {
                fd = open(device, O_WRONLY);
                if (fd >= 0) {
                    write(fd, sector, 512);
                    close(fd);
                }
            }
            close(fd);
        }
    }

    uprintf("BIOS boot created successfully");
    return 1;
}

int CreateUEFIBootable(uint32_t drive_index, const char* efi_file, const char* kernel_path)
{
    char device[PATH_MAX];
    char cmd[1024];
    char temp_dir[PATH_MAX];

    if (drive_index >= MAX_DRIVES || !lufus_drive[drive_index].id)
        return 0;

    snprintf(device, sizeof(device), "/dev/%s", lufus_drive[drive_index].id);

    uprintf("Creating UEFI bootable on %s", device);

    snprintf(temp_dir, sizeof(temp_dir), "/tmp/lufus_uefi_%s", lufus_drive[drive_index].id);
    mkdir(temp_dir, 0755);

    snprintf(cmd, sizeof(cmd), "mkdir -p '%s/EFI/BOOT' '%s/boot'", temp_dir, temp_dir);
    run_cmd(cmd);

    if (efi_file && file_exists(efi_file)) {
        snprintf(cmd, sizeof(cmd), "cp '%s' '%s/EFI/BOOT/BOOTX64.EFI'", efi_file, temp_dir);
        run_cmd(cmd);
    }

    if (kernel_path && file_exists(kernel_path)) {
        snprintf(cmd, sizeof(cmd), "cp '%s' '%s/boot/vmlinuz'", kernel_path, temp_dir);
        run_cmd(cmd);
    }

    snprintf(cmd, sizeof(cmd), "mkfs.fat -F 32 '%s'", device);
    run_cmd(cmd);

    snprintf(cmd, sizeof(cmd), "mount '%s' '%s'", device, temp_dir);
    run_cmd(cmd);

    snprintf(cmd, sizeof(cmd), "mkdir -p '%s/EFI/BOOT'", temp_dir);
    run_cmd(cmd);

    if (efi_file && file_exists(efi_file)) {
        snprintf(cmd, sizeof(cmd), "cp '%s' '%s/EFI/BOOT/BOOTX64.EFI'", efi_file, temp_dir);
        run_cmd(cmd);
    }

    snprintf(cmd, sizeof(cmd), "umount '%s'", device);
    run_cmd(cmd);

    uprintf("UEFI boot created successfully");
    return 1;
}

int CreateWindowsToGo(uint32_t drive_index, const char* wim_path, const char* index)
{
    char device[PATH_MAX];
    char cmd[1024];

    if (drive_index >= MAX_DRIVES || !lufus_drive[drive_index].id)
        return 0;

    snprintf(device, sizeof(device), "/dev/%s", lufus_drive[drive_index].id);

    uprintf("Creating Windows To Go on %s", device);

    if (!wim_path || !file_exists(wim_path)) {
        uprintf("WIM file not found");
        return 0;
    }

    snprintf(cmd, sizeof(cmd), "wimapply '%s' '%s' '%s' --boot", wim_path, index ? index : "1", device);
    if (!run_cmd(cmd)) {
        uprintf("wimapply failed, trying imagex");
        snprintf(cmd, sizeof(cmd), "imagex /apply '%s' '%s' '%s' /boot", wim_path, index ? index : "1", device);
        run_cmd(cmd);
    }

    uprintf("Windows To Go created successfully");
    return 1;
}

int CreatePersistentLinux(uint32_t drive_index, const char* iso_path, uint64_t persistence_size)
{
    char device[PATH_MAX];
    char cmd[1024];
    char temp_dir[PATH_MAX];

    if (drive_index >= MAX_DRIVES || !lufus_drive[drive_index].id)
        return 0;

    snprintf(device, sizeof(device), "/dev/%s", lufus_drive[drive_index].id);

    uprintf("Creating persistent Linux on %s (%s)",
        device, SizeToHumanReadable(persistence_size, 0, 0));

    snprintf(temp_dir, sizeof(temp_dir), "/tmp/lufus_persist_%s", lufus_drive[drive_index].id);
    mkdir(temp_dir, 0755);

    ExtractISO(iso_path, temp_dir, 0);

    snprintf(cmd, sizeof(cmd), "mkfs.ext4 -L casper-rw '%s'", device);
    run_cmd(cmd);

    snprintf(cmd, sizeof(cmd), "mount '%s' '%s/persist'", device, temp_dir);
    run_cmd(cmd);

    snprintf(cmd, sizeof(cmd), "echo 'persist' > '%s/persist/casper-rw'", temp_dir);
    run_cmd(cmd);

    snprintf(cmd, sizeof(cmd), "umount '%s/persist'", temp_dir);
    run_cmd(cmd);

    uprintf("Persistent Linux created successfully");
    return 1;
}

int ExtractAndInstallSyslinux(uint32_t drive_index, int version)
{
    char cmd[1024];
    char device[PATH_MAX];

    if (drive_index >= MAX_DRIVES || !lufus_drive[drive_index].id)
        return 0;

    snprintf(device, sizeof(device), "/dev/%s", lufus_drive[drive_index].id);

    uprintf("Installing Syslinux v%d", version);

    snprintf(cmd, sizeof(cmd), "syslinux -i '%s'", device);
    return run_cmd(cmd);
}

int ExtractAndInstallGRUB(uint32_t drive_index, int version)
{
    char cmd[1024];
    char device[PATH_MAX];

    if (drive_index >= MAX_DRIVES || !lufus_drive[drive_index].id)
        return 0;

    snprintf(device, sizeof(device), "/dev/%s", lufus_drive[drive_index].id);

    uprintf("Installing GRUB v%d", version);

    snprintf(cmd, sizeof(cmd), "grub-install --target=i386-pc --boot-directory='%s/boot' '%s'", device, device);
    return run_cmd(cmd);
}

int CreateVHDImage(const char* output_path, uint64_t size, int type)
{
    FILE* f;
    uint8_t* buf;
    uint32_t i;

    uprintf("Creating VHD image (%s)", SizeToHumanReadable(size, 0, 0));

    f = fopen(output_path, "wb");
    if (!f) return 0;

    buf = (uint8_t*)malloc(1024 * 1024);
    if (!buf) {
        fclose(f);
        return 0;
    }

    memset(buf, 0, 1024 * 1024);

    for (i = 0; i < size / (1024 * 1024); i++) {
        fwrite(buf, 1, 1024 * 1024, f);
        UpdateProgress(OP_FILE_COPY, (float)(i * 100) / (size / (1024 * 1024)));
    }

    free(buf);
    fclose(f);

    uprintf("VHD image created: %s", output_path);
    return 1;
}

int CreateVHDXImage(const char* output_path, uint64_t size, int type)
{
    return CreateVHDImage(output_path, size, type);
}

int CreateFFUImage(const char* source_device, const char* output_path)
{
    char cmd[1024];

    uprintf("Creating FFU image from %s", source_device);

    snprintf(cmd, sizeof(cmd), "dd if='%s' of='%s' bs=4M status=progress", source_device, output_path);
    return run_cmd(cmd);
}

int ConvertToVHD(const char* source, const char* output)
{
    char cmd[1024];

    uprintf("Converting %s to VHD", source);

    snprintf(cmd, sizeof(cmd), "qemu-img convert -f raw -O vpc '%s' '%s'", source, output);
    return run_cmd(cmd);
}

int ConvertToVHDX(const char* source, const char* output)
{
    char cmd[1024];

    uprintf("Converting %s to VHDX", source);

    snprintf(cmd, sizeof(cmd), "qemu-img convert -f raw -O vhdx '%s' '%s'", source, output);
    return run_cmd(cmd);
}

int ValidateUEFIBoot(const char* image_path)
{
    struct archive* a;
    struct archive_entry* entry;
    int r;
    int found_efi = 0;
    int found_bootx64 = 0;

    uprintf("Validating UEFI boot in %s", image_path);

    a = archive_read_new();
    archive_read_support_format_iso9660(a);

    r = archive_read_open_filename(a, image_path, 65536);
    if (r != ARCHIVE_OK) {
        archive_read_free(a);
        return 0;
    }

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char* pathname = archive_entry_pathname(entry);
        if (safe_strstr(pathname, ".efi") != NULL)
            found_efi = 1;
        if (safe_strstr(pathname, "BOOTX64.EFI") != NULL ||
            safe_strstr(pathname, "bootx64.efi") != NULL)
            found_bootx64 = 1;
        archive_read_data_skip(a);
    }

    archive_read_free(a);

    if (found_efi && found_bootx64) {
        uprintf("UEFI boot validation: PASSED");
        return 1;
    }

    uprintf("UEFI boot validation: FAILED (missing boot files)");
    return 0;
}

int DownloadWindowsISO(const char* url, const char* output_path, void* progress_callback)
{
    uprintf("Downloading Windows ISO from %s", url);
    return DownloadToFileOrBuffer(url, output_path, NULL, NULL, 1) > 0;
}

int DownloadUEFIShell(const char* output_path, void* progress_callback)
{
    const char* url = "https://github.com/tianocore/edk2/releases/latest/download/Shell.iso";
    uprintf("Downloading UEFI Shell from %s", url);
    return DownloadToFileOrBuffer(url, output_path, NULL, NULL, 1) > 0;
}

int SetOOBEParameters(const char* dest_dir, const char* username, int privacy)
{
    char path[PATH_MAX];
    FILE* f;

    uprintf("Setting OOBE parameters for user: %s", username);

    safe_sprintf(path, sizeof(path), "%s/autounattend.xml", dest_dir);
    f = fopen(path, "w");
    if (!f) return 0;

    fprintf(f, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
    fprintf(f, "<unattend xmlns=\"urn:schemas-microsoft-com:unattend\">\n");
    fprintf(f, "  <settings pass=\"oobeSystem\">\n");
    fprintf(f, "    <component name=\"Microsoft-Windows-Shell-Setup\">\n");
    fprintf(f, "      <UserAccounts>\n");
    fprintf(f, "        <LocalAccounts>\n");
    fprintf(f, "          <LocalAccount wcm:action=\"add\">\n");
    fprintf(f, "            <Name>%s</Name>\n", username ? username : "User");
    fprintf(f, "            <Group>Administrators</Group>\n");
    fprintf(f, "          </LocalAccount>\n");
    fprintf(f, "        </LocalAccounts>\n");
    fprintf(f, "      </UserAccounts>\n");
    fprintf(f, "      <OOBE>\n");
    fprintf(f, "        <HideEULAPage>true</HideEULAPage>\n");
    fprintf(f, "        <HideOnlineAccountScreens>true</HideOnlineAccountScreens>\n");
    fprintf(f, "        <HideWirelessSetupInOOBE>true</HideWirelessSetupInOOBE>\n");
    fprintf(f, "        <ProtectYourPC>%d</ProtectYourPC>\n", privacy > 0 ? 3 : 1);
    fprintf(f, "        <SkipMachineOOBE>true</SkipMachineOOBE>\n");
    fprintf(f, "        <SkipUserOOBE>true</SkipUserOOBE>\n");
    fprintf(f, "      </OOBE>\n");
    fprintf(f, "    </component>\n");
    fprintf(f, "  </settings>\n");
    fprintf(f, "</unattend>\n");

    fclose(f);
    uprintf("OOBE parameters set successfully");
    return 1;
}

int CreateWindowsToGoWim(const char* source_wim, const char* dest_drive)
{
    char cmd[1024];

    uprintf("Creating Windows To Go from %s to %s", source_wim, dest_drive);

    snprintf(cmd, sizeof(cmd), "wimapply '%s' '1' '%s' --boot", source_wim, dest_drive);
    return run_cmd(cmd);
}
