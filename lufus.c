#include "lufus.h"
#include "ui.h"

LUFUS_UPDATE lufus_update = {0};
LUFUS_IMG_REPORT img_report = {0};
LUFUS_DRIVE lufus_drive[MAX_DRIVES] = {0};
int lufus_version[3] = {4, 5, 0};
uint32_t dur_mins = 0, dur_secs = 0;
int dialog_showing = 0, force_update = 0;
int fs_type = FS_FAT32, boot_type = BT_IMAGE, partition_type = PARTITION_STYLE_MBR, target_type = TT_UEFI;
unsigned long syslinux_ldlinux_len[2] = {0};
char szFolderPath[PATH_MAX] = {0};
char app_dir[PATH_MAX] = {0};
char temp_dir[PATH_MAX] = {0};
char system_dir[PATH_MAX] = {0};
char app_data_dir[PATH_MAX] = {0};
char user_dir[PATH_MAX] = {0};
char cur_dir[PATH_MAX] = {0};
char embedded_sl_version_str[2][12] = {0};
char embedded_sl_version_ext[2][32] = {0};
char ClusterSizeLabel[MAX_CLUSTER_SIZES][64] = {0};
char msgbox[1024] = {0};
char msgbox_title[32] = {0};
char* image_path = NULL;
char* short_image_path = NULL;
char* archive_path = NULL;
char image_option_txt[128] = {0};
char* fido_url = NULL;
int nb_steps[FS_MAX] = {0};
const char* flash_type[BADBLOCKS_PATTERN_TYPES] = {"1 pass", "2 passes", "3 passes", "4 passes", "DD"};
float fScale = 1.0f;
int op_in_progress = 0;
int advanced_mode_device = 0, advanced_mode_format = 0;
int enable_HDDs = 0, enable_VHDs = 0, detect_fakes = 0;
int allow_dual_uefi_bios = 0, large_drive = 0;
int write_as_image = 0, save_image = 0;
int write_mode = 0;
int win_bypass_tpm = 1;
int win_bypass_secureboot = 1;
int win_bypass_ram = 1;
int win_bypass_account = 1;
int default_fs = FS_FAT32, selected_fs = FS_FAT32, preselected_fs = FS_FAT32;
int selection_default = 0, image_index = 0;
uint64_t persistence_size = 0;
int64_t iso_blocking_status = 0;
int right_to_left_mode = 0;
int verbose = 0;
int usb_debug = 0;

static void print_usage(const char* prog)
{
    fprintf(stderr, "Usage: %s [options]\n", prog);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, "  -h          Show this help\n");
    fprintf(stderr, "  -v          Verbose mode\n");
    fprintf(stderr, "  -g          Enable HDD detection\n");
    fprintf(stderr, "  -i <file>   Select image file\n");
    fprintf(stderr, "  -d <dev>    Select target device\n");
}

static void parse_args(int argc, char* argv[])
{
    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            exit(0);
        } else if (strcmp(argv[i], "-v") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-g") == 0) {
            enable_HDDs = 1;
        } else if (strcmp(argv[i], "-i") == 0 && i + 1 < argc) {
            image_path = strdup(argv[++i]);
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
        }
    }
}

int main(int argc, char* argv[])
{
    parse_args(argc, argv);

    if (getcwd(cur_dir, sizeof(cur_dir)) == NULL)
        static_strcpy(cur_dir, ".");

    static_strcpy(app_dir, cur_dir);
    static_strcpy(temp_dir, "/tmp");
    static_strcpy(system_dir, "/usr/bin");
    static_strcpy(app_data_dir, "/tmp/lufus");
    static_strcpy(user_dir, getenv("HOME") ? getenv("HOME") : "/tmp");

    UI_Init(argc, argv);
    UI_Run();

    return 0;
}
