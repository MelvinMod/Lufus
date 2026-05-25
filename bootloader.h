#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "lufus.h"

#define BOOT_FAT12        0
#define BOOT_FAT16        1
#define BOOT_FAT32        2
#define BOOT_NTFS         3
#define BOOT_EXFAT        4
#define BOOT_EXT2         5
#define BOOT_EXT3         6
#define BOOT_LINUX        7

#define DOS_FREEDOS       0
#define DOS_MSDOS622      1
#define DOS_MSDOS710      2

#define UEFI_NTFS_IMAGE   "uefi_ntfs.efi"
#define UEFI_SHELL_IMAGE  "uefi_shell.iso"

typedef struct {
    uint8_t  jump[3];
    char     oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t sectors_per_fat_16;
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint8_t  drive_number;
    uint8_t  reserved1;
    uint8_t  boot_signature;
    uint32_t volume_id;
    char     volume_label[11];
    char     filesystem_type[8];
    uint8_t  boot_code[448];
    uint16_t signature;
} __attribute__((packed)) pbr_fat;

extern int InstallFreeDOS(uint32_t drive_index, int msdos_version);
extern int InstallMSDOS(uint32_t drive_index, int version);
extern int CreateUEFIBootableNTFS(uint32_t drive_index, const char* efi_file);
extern int CreateBIOSBootable(uint32_t drive_index, const char* boot_sector);
extern int CreateUEFIBootable(uint32_t drive_index, const char* efi_file, const char* kernel_path);
extern int CreateWindowsToGo(uint32_t drive_index, const char* wim_path, const char* index);
extern int CreatePersistentLinux(uint32_t drive_index, const char* iso_path, uint64_t persistence_size);
extern int ExtractAndInstallSyslinux(uint32_t drive_index, int version);
extern int ExtractAndInstallGRUB(uint32_t drive_index, int version);
extern int CreateVHDImage(const char* output_path, uint64_t size, int type);
extern int CreateVHDXImage(const char* output_path, uint64_t size, int type);
extern int CreateFFUImage(const char* source_device, const char* output_path);
extern int ConvertToVHD(const char* source, const char* output);
extern int ConvertToVHDX(const char* source, const char* output);
extern int ValidateUEFIBoot(const char* image_path);
extern int DownloadWindowsISO(const char* url, const char* output_path, void* progress_callback);
extern int DownloadUEFIShell(const char* output_path, void* progress_callback);
extern int SetOOBEParameters(const char* dest_dir, const char* username, int privacy);
extern int CreateWindowsToGoWim(const char* source_wim, const char* dest_drive);

#endif
