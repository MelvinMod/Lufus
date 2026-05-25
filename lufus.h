#ifndef LUFUS_H
#define LUFUS_H

#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <linux/hdreg.h>
#include <dirent.h>
#include <pthread.h>
#include <stdarg.h>
#include <ctype.h>
#include <signal.h>
#include <time.h>
#include <poll.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <libgen.h>
#include <limits.h>

#define KB                          1024LL
#define MB                          1048576LL
#define GB                          1073741824LL
#define TB                          1099511627776LL

#define APPLICATION_NAME            "Lufus"
#define APPLICATION_ARCH            "x64"
#define LUFUS_VERSION               "0.1"
#define DRIVE_ACCESS_TIMEOUT        15000
#define DRIVE_ACCESS_RETRIES        150
#define MIN_DRIVE_SIZE              (8 * MB)
#define MIN_EXTRA_PART_SIZE         (1 * MB)
#define MIN_EXT_SIZE                (256 * MB)
#define MAX_DRIVES                  64
#define MAX_TOOLTIPS                128
#define MAX_SIZE_SUFFIXES           6
#define MAX_CLUSTER_SIZES           18
#define MAX_PROGRESS                0xFFFF
#define MAX_LOG_SIZE                0x7FFFFFFE
#define MAX_REFRESH                 25
#define MARQUEE_TIMER_REFRESH       10
#define MAX_GUID_STRING_LENGTH      40
#define MAX_PARTITIONS              16
#define MAX_ESP_TOGGLE              8
#define MAX_IGNORE_USB              8
#define MAX_ISO_TO_ESP_SIZE         (1 * GB)
#define MAX_DEFAULT_LIST_CARD_SIZE  (500 * GB)
#define MAX_SECTORS_TO_CLEAR        (8 * MB / 512)
#define MAX_USERNAME_LENGTH         128
#define MAX_WININST                 4
#define MAX_MARKER                  80.0f
#define MBR_UEFI_MARKER             0x49464555
#define MORE_INFO_URL               0xFFFF
#define PROJECTED_SIZE_RATIO        110
#define STATUS_MSG_TIMEOUT          3500
#define WRITE_RETRIES               4
#define WRITE_TIMEOUT               5000
#define SEARCH_PROCESS_TIMEOUT      5000
#define NET_SESSION_TIMEOUT         3500
#define FS_DEFAULT                  FS_FAT32
#define SINGLE_CLUSTERSIZE_DEFAULT  0x00000100
#define BADBLOCKS_PATTERN_TYPES     5
#define BADBLOCK_PATTERN_COUNT      4
#define BADBLOCK_BLOCK_SIZE         (512 * KB)
#define LARGE_FAT32_SIZE            (32 * GB)
#define UDF_FORMAT_SPEED            3.1f
#define UDF_FORMAT_WARN             20
#define MAX_FAT32_SIZE              (2 * TB)
#define FAT32_CLUSTER_THRESHOLD     1.011f
#define DD_BUFFER_SIZE              (32 * MB)
#define UBUFFER_SIZE                4096
#define ISO_BUFFER_SIZE             (64 * KB)
#define RSA_SIGNATURE_SIZE          256

#define IGNORE_RETVAL(expr)         do { (void)(expr); } while(0)
#ifndef ARRAYSIZE
#define ARRAYSIZE(A)                (sizeof(A)/sizeof((A)[0]))
#endif
#ifndef STRINGIFY
#define STRINGIFY(x)                #x
#endif
#define PERCENTAGE(percent, value)  ((1ULL * (percent) * (value)) / 100ULL)
#define IS_POWER_OF_2(x)            ((x != 0) && (((x) & ((x) - 1)) == 0))

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#define safe_free(p) do { free((void*)p); p = NULL; } while(0)
static __inline void safe_strcp(char* dst, const size_t dst_max, const char* src, const size_t count) {
    memmove(dst, src, min(count, dst_max));
    if (dst != NULL) dst[min(count, dst_max) - 1] = 0;
}
#define safe_strcpy(dst, dst_max, src) safe_strcp(dst, dst_max, src, safe_strlen(src) + 1)
#define static_strcpy(dst, src) safe_strcpy(dst, sizeof(dst), src)
#define safe_strcat(dst, dst_max, src) strncat(dst, src, dst_max - strlen(dst) - 1)
#define static_strcat(dst, src) safe_strcat(dst, sizeof(dst), src)
#define safe_strcmp(str1, str2) strcmp(((str1 == NULL) ? "<NULL>" : str1), ((str2 == NULL) ? "<NULL>" : str2))
#define safe_strstr(str1, str2) strstr(((str1 == NULL) ? "<NULL>" : str1), ((str2 == NULL) ? "<NULL>" : str2))
#define safe_strncmp(str1, str2, count) strncmp(((str1 == NULL) ? "<NULL>" : str1), ((str2 == NULL) ? "<NULL>" : str2), count)
#define safe_strlen(str) ((((char*)(str))==NULL) ? 0 : strlen(str))
#define safe_strdup(str) ((((char*)(str))==NULL) ? NULL : strdup(str))
#define safe_sprintf(dst, count, ...) do { snprintf(dst, count, __VA_ARGS__); \
    if (dst != NULL) dst[(count) - 1] = 0; } while(0)
#define static_sprintf(dst, ...) safe_sprintf(dst, sizeof(dst), __VA_ARGS__)
#define safe_atoi(str) ((((char*)(str))==NULL) ? 0 : atoi(str))

#define to_unix_path(str) do { char*_p=(str); while(*_p){ if(*_p=='\\')*_p='/'; _p++;} } while(0)
#define to_windows_path(str) do { char*_p=(str); while(*_p){ if(*_p=='/')*_p='\\'; _p++;} } while(0)

#define IsChecked(widget)           (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))

enum fs_type {
    FS_UNKNOWN = -1,
    FS_FAT16 = 0,
    FS_FAT32,
    FS_NTFS,
    FS_UDF,
    FS_EXFAT,
    FS_REFS,
    FS_EXT2,
    FS_EXT3,
    FS_EXT4,
    FS_MAX
};

enum boot_type {
    BT_NON_BOOTABLE = 0,
    BT_MSDOS,
    BT_FREEDOS,
    BT_IMAGE,
    BT_SYSLINUX_V4,
    BT_SYSLINUX_V6,
    BT_REACTOS,
    BT_GRUB4DOS,
    BT_GRUB2,
    BT_UEFI_NTFS,
    BT_MAX
};

enum target_type {
    TT_BIOS = 0,
    TT_UEFI,
    TT_MAX
};

enum partition_style {
    PARTITION_STYLE_MBR = 0,
    PARTITION_STYLE_GPT,
    PARTITION_STYLE_SFD,
    PARTITION_STYLE_MAX
};

enum image_option_type {
    IMOP_WIN_STANDARD = 0,
    IMOP_WIN_EXTENDED,
    IMOP_WIN_TO_GO,
    IMOP_MAX
};

enum action_type {
    OP_NOOP_WITH_TASKBAR = -3,
    OP_NOOP = -2,
    OP_INIT = -1,
    OP_ANALYZE_MBR = 0,
    OP_BADBLOCKS,
    OP_ZERO_MBR,
    OP_PARTITION,
    OP_FORMAT,
    OP_CREATE_FS,
    OP_FIX_MBR,
    OP_FILE_COPY,
    OP_PATCH,
    OP_FINALIZE,
    OP_EXTRACT_ZIP,
    OP_MAX
};

enum hash_type {
    HASH_MD5 = 0,
    HASH_SHA1,
    HASH_SHA256,
    HASH_SHA512,
    HASH_MAX
};

typedef struct {
    uint16_t major;
    uint16_t minor;
    uint16_t build;
    uint16_t revision;
} winver_t;

typedef struct {
    char label[192];
    char usb_label[192];
    char cfg_path[128];
    char reactos_path[128];
    char wininst_path[MAX_WININST][64];
    char efi_img_path[128];
    uint64_t image_size;
    uint64_t projected_size;
    int64_t mismatch_size;
    uint32_t wininst_version;
    int is_iso;
    int8_t is_bootable_img;
    int is_vhd;
    int is_windows_img;
    int disable_iso;
    int rh8_derivative;
    uint16_t winpe;
    uint16_t has_efi;
    uint8_t has_secureboot_bootloader;
    uint8_t has_md5sum;
    uint8_t wininst_index;
    uint8_t has_symlinks;
    uint8_t has_4GB_file;
    int has_long_filename;
    int has_deep_directories;
    int has_bootmgr;
    int has_bootmgr_efi;
    int has_autorun;
    int has_old_c32[2];
    int has_old_vesamenu;
    int has_efi_syslinux;
    int has_grub4dos;
    uint8_t has_grub2;
    uint8_t has_grub2_fs;
    int has_compatresources_dll;
    int has_panther_unattend;
    int has_kolibrios;
    int needs_syslinux_overwrite;
    int needs_ntfs;
    int uses_casper;
    int uses_minint;
    uint8_t compression_type;
    winver_t win_version;
    uint16_t sl_version;
    char sl_version_str[12];
    char sl_version_ext[32];
    char grub2_version[192];
} LUFUS_IMG_REPORT;

typedef struct {
    char* id;
    char* name;
    char* display_name;
    char* label;
    char* hub;
    uint32_t index;
    uint32_t port;
    uint64_t size;
} LUFUS_DRIVE;

typedef struct {
    uint32_t version[3];
    uint32_t platform_min[2];
    char* download_url;
    char* release_notes;
} LUFUS_UPDATE;

typedef struct {
    uint32_t Type;
    uint32_t DeviceNum;
    uint32_t BufSize;
    long long DeviceSize;
    char* DevicePath;
    char* ImagePath;
    char* Label;
} IMG_SAVE;

typedef struct {
    size_t count;
    const char* filename;
    const char** extension;
    const char** description;
} ext_t;

#define EXT_X(prefix, ...)  const char* _##prefix##_x[] = { __VA_ARGS__ }
#define EXT_D(prefix, ...)  const char* _##prefix##_d[] = { __VA_ARGS__ }
#define EXT_DECL(var, filename, extensions, descriptions)                   \
    EXT_X(var, extensions);                                                 \
    EXT_D(var, descriptions);                                               \
    ext_t var = { ARRAYSIZE(_##var##_x), filename, _##var##_x, _##var##_d }

typedef enum TASKBAR_PROGRESS_FLAGS {
    TASKBAR_NOPROGRESS = 0,
    TASKBAR_INDETERMINATE = 0x1,
    TASKBAR_NORMAL = 0x2,
    TASKBAR_ERROR = 0x4,
    TASKBAR_PAUSED = 0x8
} TASKBAR_PROGRESS_FLAGS;

typedef struct {
    uint8_t buf[128];
    uint64_t state[8];
    uint64_t bytecount;
} HASH_CONTEXT;

typedef struct {
    char name[256];
    uint8_t thumbprint[20];
} cert_info_t;

typedef void hash_init_t(HASH_CONTEXT* ctx);
typedef void hash_write_t(HASH_CONTEXT* ctx, const uint8_t* buf, size_t len);
typedef void hash_final_t(HASH_CONTEXT* ctx);

extern LUFUS_UPDATE lufus_update;
extern LUFUS_IMG_REPORT img_report;
extern LUFUS_DRIVE lufus_drive[MAX_DRIVES];
extern int lufus_version[3];
extern uint32_t dur_mins, dur_secs;
extern int dialog_showing, force_update, fs_type, boot_type, partition_type, target_type;
extern unsigned long syslinux_ldlinux_len[2];
extern char szFolderPath[PATH_MAX], app_dir[PATH_MAX], temp_dir[PATH_MAX], system_dir[PATH_MAX];
extern char app_data_dir[PATH_MAX], user_dir[PATH_MAX], cur_dir[PATH_MAX];
extern char embedded_sl_version_str[2][12];
extern char embedded_sl_version_ext[2][32];
extern char ClusterSizeLabel[MAX_CLUSTER_SIZES][64];
extern char msgbox[1024], msgbox_title[32], *image_path, *short_image_path;
extern char *archive_path, image_option_txt[128], *fido_url;
extern int nb_steps[FS_MAX];
extern const char* flash_type[BADBLOCKS_PATTERN_TYPES];
extern float fScale;
extern int op_in_progress;
extern int advanced_mode_device, advanced_mode_format;
extern int enable_HDDs, enable_VHDs, detect_fakes;
extern int allow_dual_uefi_bios, large_drive;
extern int write_as_image, save_image;
extern int write_mode;
extern int win_bypass_tpm;
extern int win_bypass_secureboot;
extern int win_bypass_ram;
extern int win_bypass_account;
extern int default_fs, selected_fs, preselected_fs;
extern int selection_default, image_index;
extern uint64_t persistence_size;
extern int64_t iso_blocking_status;
extern int right_to_left_mode;
extern int verbose;
extern int usb_debug;
extern int write_mode;
extern int win_bypass_tpm;
extern int win_bypass_secureboot;
extern int win_bypass_ram;
extern int win_bypass_account;
extern int linux2win_enable_transfer;
extern int linux2win_export_theme;
extern int linux2win_export_wallpaper;
extern int linux2win_export_fonts;
extern int linux2win_export_proxy;
extern int linux2win_export_ssh;
extern int linux2win_export_git;
extern int linux2win_export_bash;
extern int linux2win_export_terminal;
extern int linux2win_dd_direct_write;
extern int linux2win_dd_sync;
extern int linux2win_dd_verify;

typedef struct {
    int desktop_env;
    int display_server;
    int os_type;
    char distro[64];
    char de_version[32];
    char kernel_version[32];
    char hostname[128];
    char username[128];
    char timezone[64];
    char language[16];
    char keyboard_layout[16];
} linux_profile;

typedef struct {
    int enable_theme_transfer;
    int enable_wallpaper_transfer;
    int enable_font_transfer;
    int enable_proxy_transfer;
    int enable_ssh_transfer;
    int enable_git_transfer;
    int enable_bash_transfer;
    int enable_terminal_transfer;
    int enable_vim_transfer;
    int enable_browser_transfer;
    int enable_app_settings;
} transfer_options;

extern linux_profile g_linux_profile;

extern void uprintf(const char *format, ...);
extern void uprintfs(const char *str);
extern void uprint_progress(uint64_t cur_value, uint64_t max_value);
#define vuprintf(...) do { if (verbose) uprintf(__VA_ARGS__); } while(0)
#define vvuprintf(...) do { if (verbose > 1) uprintf(__VA_ARGS__); } while(0)
#define suprintf(...) do { uprintf(__VA_ARGS__); } while(0)

extern void PrintStatusInfo(int info, int debug, unsigned int duration, int msg_id, ...);
#define PrintStatus(...) PrintStatusInfo(0, 0, __VA_ARGS__)
#define PrintStatusDebug(...) PrintStatusInfo(0, 1, __VA_ARGS__)
#define PrintInfo(...) PrintStatusInfo(1, 0, __VA_ARGS__)
#define PrintInfoDebug(...) PrintStatusInfo(1, 1, __VA_ARGS__)

extern void UpdateProgress(int op, float percent);
extern void _UpdateProgressWithInfo(int op, int msg, uint64_t processed, uint64_t total, int force);
#define UpdateProgressWithInfo(op, msg, processed, total) _UpdateProgressWithInfo(op, msg, processed, total, 0)
#define UpdateProgressWithInfoUpTo(upto, op, msg, processed, total) _UpdateProgressWithInfo(op, msg, (processed) * (upto), (total) * 100, 0)
#define UpdateProgressWithInfoForce(op, msg, processed, total) _UpdateProgressWithInfo(op, msg, processed, total, 1)

extern const char* StrError(int error_code);
extern char* SizeToHumanReadable(uint64_t size, int copy_to_log, int fake_units);
extern char* TimestampToHumanReadable(uint64_t ts);
extern char* GuidToString(const uint8_t* guid, int bDecorated);
extern void DumpBufferHex(void *buf, size_t size);
extern int sanitize_label(char* label);

extern char* FileDialog(int save, char* path, const ext_t* ext, uint32_t* selected_ext);
extern uint32_t read_file(const char* path, uint8_t** buf);
extern uint32_t write_file(const char* path, const uint8_t* buf, const uint32_t size);
extern char* get_token_data_file_indexed(const char* token, const char* filename, int index);
#define get_token_data_file(token, filename) get_token_data_file_indexed(token, filename, 1)
extern char* set_token_data_file(const char* token, const char* data, const char* filename);
extern char* get_token_data_buffer(const char* token, unsigned int n, const char* buffer, size_t buffer_size);
extern char* insert_section_data(const char* filename, const char* section, const char* data, int dos2unix);
extern char* replace_in_token_data(const char* filename, const char* token, const char* src, const char* rep, int dos2unix);
extern char* replace_char(const char* src, const char c, const char* rep);
extern void filter_chars(char* str, const char* rem, const char rep);
extern void trim(char* str);
extern char* remove_substr(const char* src, const char* sub);
extern void parse_update(char* buf, size_t len);

extern int IsHDD(uint32_t DriveIndex, uint16_t vid, uint16_t pid, const char* strid);
extern void ClearDrives(void);
extern int GetDevices(uint32_t devnum);
extern int CycleDevice(int index);
extern int CyclePort(int index);
extern int GetDriveNumber(int fd, const char* path);
extern int IsMediaPresent(uint32_t drive_index);
extern uint64_t GetDriveSize(uint32_t drive_index);
extern int GetDriveLabel(uint32_t drive_index, char* drive_letters, char** label, int silent);
extern int GetDrivePartitionData(uint32_t DeviceNumber, char* fs_name, size_t fs_name_size, int silent);
extern int IsMsDevDrive(uint32_t drive_index);
extern int IsFilteredDrive(uint32_t drive_index);

extern int WritePBR(int fd);
extern int FormatLargeFAT32(uint32_t DriveIndex, uint64_t PartitionOffset, uint32_t ClusterSize, const char* FSName, const char* Label, uint32_t Flags);
extern int FormatExtFs(uint32_t DriveIndex, uint64_t PartitionOffset, uint32_t BlockSize, const char* FSName, const char* Label, uint32_t Flags);
extern int FormatPartition(uint32_t DriveIndex, uint64_t PartitionOffset, uint32_t UnitAllocationSize, uint16_t FSType, const char* Label, uint32_t Flags);
extern void* FormatThread(void* param);
extern void* DDWriteThread(void* param);

extern int ExtractISO(const char* src_iso, const char* dest_dir, int scan);
extern int ExtractZip(const char* src_zip, const char* dest_dir);
extern int64_t ExtractISOFile(const char* iso, const char* iso_file, const char* dest_file, uint32_t attributes);
extern uint32_t ReadISOFileToBuffer(const char* iso, const char* iso_file, uint8_t** buf);
extern int HasEfiImgBootLoaders(void* iso);
extern int DumpFatDir(void* iso, const char* path, int32_t cluster);
extern int InstallSyslinux(uint32_t drive_index, char drive_letter, int fs);
extern uint16_t GetSyslinuxVersion(char* buf, size_t buf_size, char** ext);
extern int SetAutorun(const char* path);

typedef struct {
    int is_windows;
    int version;
    char edition[64];
    char arch[16];
    int has_install_wim;
    int has_install_esd;
    int has_boot_wim;
    int is_win11;
    int needs_bypass;
} win_iso_info;

extern int DetectWindowsISO(const char* iso_path, win_iso_info* info);
extern int PatchWindowsISO(const char* iso_path, const char* dest_dir, win_iso_info* info);
extern int CreateBypassFiles(const char* dest_dir, win_iso_info* info);
extern int CreateAutounattendXML(const char* path, int win_version);

extern int DetectDesktopEnvironment(void);
extern int DetectDisplayServer(void);
extern int DetectOSType(void);
extern int DetectLinuxProfile(linux_profile* profile);
extern int ExportLinuxSettings(linux_profile* profile, const char* output_dir);
extern int GenerateWindowsConfig(linux_profile* profile, transfer_options* opts, const char* output_dir);
extern int CreateWindowsRegistry(linux_profile* profile, transfer_options* opts, const char* output_file);
extern int CreatePowerShellScript(linux_profile* profile, transfer_options* opts, const char* output_file);
extern int CreateBatchScript(linux_profile* profile, transfer_options* opts, const char* output_file);
extern int TransferSettingsToISO(const char* iso_path, const char* settings_dir);

extern int HashFile(unsigned type, const char* path, uint8_t* sum);
extern int HashBuffer(unsigned type, const uint8_t* buf, const size_t len, uint8_t* sum);
extern uint8_t* StringToHash(const char* str);
extern int FileMatchesHash(const char* path, const char* str);
extern int BufferMatchesHash(const uint8_t* buf, const size_t len, const char* str);
extern int IsFileInDB(const char* path);
extern int IsBufferInDB(const unsigned char* buf, const size_t len);
extern void* HashThread(void* param);

extern uint64_t DownloadToFileOrBufferEx(const char* url, const char* file, const char* user_agent, uint8_t** buffer, void* hProgressDialog, int bTaskBarProgress);
#define DownloadToFileOrBuffer(url, file, buffer, hProgressDialog, bTaskBarProgress) \
    DownloadToFileOrBufferEx(url, file, NULL, buffer, hProgressDialog, bTaskBarProgress)
extern int DownloadSignedFile(const char* url, const char* file, void* hProgressDialog, int PromptOnError);
extern int CheckForUpdates(int force);
extern void DownloadNewVersion(void);
extern int DownloadISO(void);
extern int IsDownloadable(const char* url);

extern void EnableControls(int enable, int remove_checkboxes);
extern void InitProgress(int bOnlyFormat);
extern void SetPersistenceSize(void);
extern void TogglePersistenceControls(int display);
extern void ToggleAdvancedDeviceOptions(int enable);
extern void ToggleAdvancedFormatOptions(int enable);
extern void ToggleImageOptions(void);
extern void SetComboEntry(GtkWidget* combo, int data);
extern void OnPaint(cairo_t* cr);

extern int RunCommandWithProgress(const char* cmdline, const char* dir, int log, int msg, const char* pattern);
#define RunCommand(cmd, dir, log) RunCommandWithProgress(cmd, dir, log, 0, NULL)

extern GtkWidget* hMainDialog, *hLogDialog, *hStatus, *hDeviceList, *hCapacity, *hImageOption;
extern GtkWidget* hPartitionScheme, *hTargetSystem, *hFileSystem, *hClusterSize, *hLabel, *hBootType;
extern GtkWidget* hNBPasses, *hLog, *hInfo, *hProgress;
extern GtkWidget* hMultiToolbar, *hSaveToolbar, *hHashToolbar, *hAdvancedDeviceToolbar, *hAdvancedFormatToolbar;
extern GtkWidget* hSelectImage, *hStart;

#endif

