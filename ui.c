#include "ui.h"
#include "badblocks.h"
#include "format.h"
#include "ddwrite.h"
#include "wim.h"
#include "linux2win.h"
#include "bootloader.h"
#include <cairo.h>

GtkWidget* hMainDialog = NULL;
GtkWidget* hLogDialog = NULL;
GtkWidget* hStatus = NULL;
GtkWidget* hDeviceList = NULL;
GtkWidget* hCapacity = NULL;
GtkWidget* hImageOption = NULL;
GtkWidget* hPartitionScheme = NULL;
GtkWidget* hTargetSystem = NULL;
GtkWidget* hFileSystem = NULL;
GtkWidget* hClusterSize = NULL;
GtkWidget* hLabel = NULL;
GtkWidget* hBootType = NULL;
GtkWidget* hNBPasses = NULL;
GtkWidget* hLog = NULL;
GtkWidget* hInfo = NULL;
GtkWidget* hProgress = NULL;
GtkWidget* hMultiToolbar = NULL;
GtkWidget* hSaveToolbar = NULL;
GtkWidget* hHashToolbar = NULL;
GtkWidget* hAdvancedDeviceToolbar = NULL;
GtkWidget* hAdvancedFormatToolbar = NULL;
GtkWidget* hSelectImage = NULL;
GtkWidget* hStart = NULL;

static GtkWidget* hBootSelection = NULL;
static GtkWidget* hCheckQuickFormat = NULL;
static GtkWidget* hCheckBadBlocks = NULL;
static GtkWidget* hCheckExtendedLabel = NULL;
static GtkWidget* hCheckShowAdvancedDevice = NULL;
static GtkWidget* hCheckShowAdvancedFormat = NULL;
static GtkWidget* hPersistenceSize = NULL;
static GtkWidget* hLogScrolled = NULL;
static GtkWidget* hWriteMode = NULL;
static GtkWidget* hCheckBypassTPM = NULL;
static GtkWidget* hCheckBypassSecureBoot = NULL;
static GtkWidget* hCheckBypassRAM = NULL;
static GtkWidget* hCheckBypassAccount = NULL;
static GtkWidget* hWindowsOptionsBox = NULL;
static GtkWidget* hDDDirectWrite = NULL;
static GtkWidget* hDDSync = NULL;
static GtkWidget* hDDVerify = NULL;
static GtkWidget* hDDOptionsBox = NULL;
static GtkWidget* hLinux2WinTransfer = NULL;
static GtkWidget* hLinux2WinTheme = NULL;
static GtkWidget* hLinux2WinWallpaper = NULL;
static GtkWidget* hLinux2WinFonts = NULL;
static GtkWidget* hLinux2WinProxy = NULL;
static GtkWidget* hLinux2WinSSH = NULL;
static GtkWidget* hLinux2WinGit = NULL;
static GtkWidget* hLinux2WinBash = NULL;
static GtkWidget* hLinux2WinTerminal = NULL;
static GtkWidget* hLinux2WinBox = NULL;
static GtkWidget* hBootMode = NULL;
static GtkWidget* hCheckWindowsToGo = NULL;
static GtkWidget* hCheckPersistent = NULL;
static GtkWidget* hPersistentSize = NULL;
static GtkWidget* hCheckValidateUEFI = NULL;
static GtkWidget* hCheckComputeHash = NULL;
static GtkWidget* hHashType = NULL;
static GtkWidget* hCheckFakeDrive = NULL;
static GtkWidget* hBootOptionsBox = NULL;

static GtkTextBuffer* log_buffer = NULL;
static volatile int log_size = 0;

void uprintf(const char *format, ...)
{
    va_list args;
    char buf[4096];
    GtkTextIter iter;

    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    strncat(buf, "\n", sizeof(buf) - strlen(buf) - 1);

    if (log_buffer) {
        gtk_text_buffer_get_end_iter(log_buffer, &iter);
        gtk_text_buffer_insert(log_buffer, &iter, buf, -1);
        GtkTextMark* mark = gtk_text_buffer_get_insert(log_buffer);
        gtk_text_view_scroll_to_mark(GTK_TEXT_VIEW(hLog), mark, 0.0, FALSE, 0.0, 0.0);
    }

    fprintf(stderr, "%s", buf);
}

void uprintfs(const char* str)
{
    uprintf("%s", str);
}

void uprint_progress(uint64_t cur_value, uint64_t max_value)
{
    UpdateProgressWithInfo(OP_FILE_COPY, 0, cur_value, max_value);
}

void PrintStatusInfo(int info, int debug, unsigned int duration, int msg_id, ...)
{
    va_list args;
    char buf[256];
    const char* prefix = info ? "INFO: " : "";

    va_start(args, msg_id);
    vsnprintf(buf, sizeof(buf), msg_id ? "Status %d" : "Ready", args);
    va_end(args);

    if (hStatus) {
        gtk_label_set_text(GTK_LABEL(hStatus), buf);
    }
    uprintf("%s%s", prefix, buf);
}

void UpdateProgress(int op, float percent)
{
    double fraction = percent / 100.0;
    if (fraction < 0.0) fraction = 0.0;
    if (fraction > 1.0) fraction = 1.0;
    if (hProgress) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(hProgress), fraction);
    }
}

void _UpdateProgressWithInfo(int op, int msg, uint64_t processed, uint64_t total, int force)
{
    double fraction = 0.0;
    if (total > 0)
        fraction = (double)processed / (double)total;
    if (fraction < 0.0) fraction = 0.0;
    if (fraction > 1.0) fraction = 1.0;
    if (hProgress) {
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(hProgress), fraction);
    }
}

const char* StrError(int error_code)
{
    return strerror(error_code);
}

char* SizeToHumanReadable(uint64_t size, int copy_to_log, int fake_units)
{
    static char str[64];
    const char* suffix[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int i = 0;
    double dsize = (double)size;
    while (dsize >= 1024.0 && i < 5) {
        dsize /= 1024.0;
        i++;
    }
    static_sprintf(str, "%.2f %s", dsize, suffix[i]);
    if (copy_to_log)
        uprintf("%s", str);
    return str;
}

char* TimestampToHumanReadable(uint64_t ts)
{
    static char str[64];
    time_t t = (time_t)ts;
    struct tm* tm_info = localtime(&t);
    strftime(str, sizeof(str), "%Y-%m-%d %H:%M:%S", tm_info);
    return str;
}

char* GuidToString(const uint8_t* guid, int bDecorated)
{
    static char str[40];
    if (bDecorated)
        static_sprintf(str, "{%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X}",
            guid[3], guid[2], guid[1], guid[0], guid[5], guid[4], guid[7], guid[6],
            guid[8], guid[9], guid[10], guid[11], guid[12], guid[13], guid[14], guid[15]);
    else
        static_sprintf(str, "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
            guid[0], guid[1], guid[2], guid[3], guid[4], guid[5], guid[6], guid[7],
            guid[8], guid[9], guid[10], guid[11], guid[12], guid[13], guid[14], guid[15]);
    return str;
}

void DumpBufferHex(void *buf, size_t size)
{
    unsigned char* p = (unsigned char*)buf;
    size_t i;
    for (i = 0; i < size; i++) {
        if (i % 16 == 0)
            fprintf(stderr, "\n%08zX  ", i);
        fprintf(stderr, "%02X ", p[i]);
        if (i % 16 == 15) {
            size_t j;
            fprintf(stderr, " ");
            for (j = i - 15; j <= i; j++)
                fprintf(stderr, "%c", (p[j] >= 32 && p[j] < 127) ? p[j] : '.');
        }
    }
    fprintf(stderr, "\n");
}

int sanitize_label(char* label)
{
    int i;
    for (i = 0; label[i]; i++) {
        if (label[i] < 32 || label[i] > 126)
            label[i] = '_';
    }
    return 0;
}

char* FileDialog(int save, char* path, const ext_t* ext, uint32_t* selected_ext)
{
    GtkWidget* dialog;
    char* filename = NULL;

    if (save)
        dialog = gtk_file_chooser_dialog_new("Save File", GTK_WINDOW(hMainDialog),
            GTK_FILE_CHOOSER_ACTION_SAVE, "_Cancel", GTK_RESPONSE_CANCEL,
            "_Save", GTK_RESPONSE_ACCEPT, NULL);
    else
        dialog = gtk_file_chooser_dialog_new("Open File", GTK_WINDOW(hMainDialog),
            GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel", GTK_RESPONSE_CANCEL,
            "_Open", GTK_RESPONSE_ACCEPT, NULL);

    if (ext) {
        size_t i;
        for (i = 0; i < ext->count; i++) {
            GtkFileFilter* filter = gtk_file_filter_new();
            gtk_file_filter_set_name(filter, ext->description[i]);
            gtk_file_filter_add_pattern(filter, ext->extension[i]);
            gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
        }
    }

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
        filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
    }

    gtk_widget_destroy(dialog);
    return filename;
}

uint32_t read_file(const char* path, uint8_t** buf)
{
    FILE* f;
    uint32_t sz;

    f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    sz = (uint32_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    *buf = (uint8_t*)malloc(sz);
    if (*buf)
        fread(*buf, 1, sz, f);
    fclose(f);
    return sz;
}

uint32_t write_file(const char* path, const uint8_t* buf, const uint32_t size)
{
    FILE* f;
    uint32_t written;

    f = fopen(path, "wb");
    if (!f) return 0;
    written = (uint32_t)fwrite(buf, 1, size, f);
    fclose(f);
    return written;
}

char* get_token_data_file_indexed(const char* token, const char* filename, int index)
{
    static char line[1024];
    FILE* f;
    int found = 0;

    f = fopen(filename, "r");
    if (!f) return NULL;
    while (fgets(line, sizeof(line), f)) {
        char* p = strstr(line, token);
        if (p) {
            found++;
            if (found == index) {
                p += strlen(token);
                while (*p == ' ' || *p == '=' || *p == '\t') p++;
                char* end = p + strlen(p) - 1;
                while (end > p && (*end == '\n' || *end == '\r')) *end-- = '\0';
                fclose(f);
                return p;
            }
        }
    }
    fclose(f);
    return NULL;
}

char* set_token_data_file(const char* token, const char* data, const char* filename)
{
    return NULL;
}

char* get_token_data_buffer(const char* token, unsigned int n, const char* buffer, size_t buffer_size)
{
    static char line[1024];
    const char* p = buffer;
    const char* end = buffer + buffer_size;
    unsigned int found = 0;

    while (p < end) {
        const char* nl = memchr(p, '\n', end - p);
        size_t len = nl ? (size_t)(nl - p) : (size_t)(end - p);
        if (len >= sizeof(line)) len = sizeof(line) - 1;
        memcpy(line, p, len);
        line[len] = '\0';
        if (strstr(line, token)) {
            found++;
            if (found == n) {
                char* q = line + strlen(token);
                while (*q == ' ' || *q == '=' || *q == '\t') q++;
                return q;
            }
        }
        p = nl ? nl + 1 : end;
    }
    return NULL;
}

char* insert_section_data(const char* filename, const char* section, const char* data, int dos2unix)
{
    return NULL;
}

char* replace_in_token_data(const char* filename, const char* token, const char* src, const char* rep, int dos2unix)
{
    return NULL;
}

char* replace_char(const char* src, const char c, const char* rep)
{
    static char buf[1024];
    size_t i, j = 0;
    size_t replen = strlen(rep);
    for (i = 0; src[i] && j < sizeof(buf) - replen - 1; i++) {
        if (src[i] == c) {
            memcpy(buf + j, rep, replen);
            j += replen;
        } else {
            buf[j++] = src[i];
        }
    }
    buf[j] = '\0';
    return buf;
}

void filter_chars(char* str, const char* rem, const char rep)
{
    size_t i;
    for (i = 0; str[i]; i++) {
        if (strchr(rem, str[i]))
            str[i] = rep;
    }
}

void trim(char* str)
{
    char* start = str;
    char* end;
    while (*start && isspace((unsigned char)*start)) start++;
    if (*start == 0) {
        str[0] = '\0';
        return;
    }
    end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    if (start != str)
        memmove(str, start, strlen(start) + 1);
}

char* remove_substr(const char* src, const char* sub)
{
    static char buf[1024];
    const char* p = strstr(src, sub);
    if (!p) {
        static_strcpy(buf, src);
        return buf;
    }
    size_t prefix = (size_t)(p - src);
    memcpy(buf, src, prefix);
    buf[prefix] = '\0';
    strncat(buf, p + strlen(sub), sizeof(buf) - prefix - 1);
    return buf;
}

void parse_update(char* buf, size_t len)
{
}

void EnableControls(int enable, int remove_checkboxes)
{
    gtk_widget_set_sensitive(hDeviceList, enable);
    gtk_widget_set_sensitive(hBootSelection, enable);
    gtk_widget_set_sensitive(hPartitionScheme, enable);
    gtk_widget_set_sensitive(hTargetSystem, enable);
    gtk_widget_set_sensitive(hFileSystem, enable);
    gtk_widget_set_sensitive(hClusterSize, enable);
    gtk_widget_set_sensitive(hLabel, enable);
    gtk_widget_set_sensitive(hCheckQuickFormat, enable);
    gtk_widget_set_sensitive(hCheckBadBlocks, enable);
    gtk_widget_set_sensitive(hWriteMode, enable);
    gtk_widget_set_sensitive(hSelectImage, enable);
    gtk_widget_set_sensitive(hStart, enable);
    if (hWindowsOptionsBox)
        gtk_widget_set_sensitive(hWindowsOptionsBox, enable);
    if (hDDOptionsBox)
        gtk_widget_set_sensitive(hDDOptionsBox, enable);
    if (hLinux2WinBox)
        gtk_widget_set_sensitive(hLinux2WinBox, enable);
    if (hBootOptionsBox)
        gtk_widget_set_sensitive(hBootOptionsBox, enable);
}

void InitProgress(int bOnlyFormat)
{
    if (hProgress)
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(hProgress), 0.0);
}

void SetPersistenceSize(void)
{
}

void TogglePersistenceControls(int display)
{
    if (hPersistenceSize)
        gtk_widget_set_visible(hPersistenceSize, display);
}

void ToggleAdvancedDeviceOptions(int enable)
{
}

void ToggleAdvancedFormatOptions(int enable)
{
}

void ToggleImageOptions(void)
{
}

void SetComboEntry(GtkWidget* combo, int data)
{
    gtk_combo_box_set_active(GTK_COMBO_BOX(combo), data);
}

void OnPaint(cairo_t* cr)
{
    cairo_set_source_rgb(cr, 0.16, 0.50, 0.73);
    cairo_paint(cr);
}

static void on_select_image(GtkWidget* widget, gpointer data)
{
    EXT_DECL(iso_ext, "Image files", "*.iso;*.img;*.zip", "ISO/IMG/ZIP files");
    char* path = FileDialog(0, NULL, &iso_ext, NULL);
    if (path) {
        gtk_entry_set_text(GTK_ENTRY(hBootSelection), path);
        safe_free(path);
    }
}

static void on_start(GtkWidget* widget, gpointer data)
{
    const char* path = gtk_entry_get_text(GTK_ENTRY(hBootSelection));
    int device_index = gtk_combo_box_get_active(GTK_COMBO_BOX(hDeviceList));
    int write_mode_idx = gtk_combo_box_get_active(GTK_COMBO_BOX(hWriteMode));
    int boot_mode = gtk_combo_box_get_active(GTK_COMBO_BOX(hBootMode));
    win_iso_info win_info;
    char temp_dir[PATH_MAX];
    pthread_t tid;
    uint64_t persist_size = 0;

    if (!path || strlen(path) == 0) {
        GtkWidget* dlg = gtk_message_dialog_new(GTK_WINDOW(hMainDialog),
            GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "Please select a boot image.");
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(dlg);
        return;
    }

    if (device_index < 0) {
        GtkWidget* dlg = gtk_message_dialog_new(GTK_WINDOW(hMainDialog),
            GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            "Please select a device.");
        gtk_dialog_run(GTK_DIALOG(dlg));
        gtk_widget_destroy(dlg);
        return;
    }

    memset(&win_info, 0, sizeof(win_info));

    if (write_mode_idx == 0) {
        DetectWindowsISO(path, &win_info);
        if (win_info.is_windows && win_info.needs_bypass) {
            GtkWidget* dlg = gtk_message_dialog_new(GTK_WINDOW(hMainDialog),
                GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
                "Windows 11 detected.\nEnable bypass for TPM/SecureBoot/RAM checks?");
            int response = gtk_dialog_run(GTK_DIALOG(dlg));
            gtk_widget_destroy(dlg);
            if (response != GTK_RESPONSE_YES)
                win_info.is_windows = 0;
        }
    }

    if (IsChecked(hCheckComputeHash)) {
        uprintf("Computing hash...");
        int hash_type = gtk_combo_box_get_active(GTK_COMBO_BOX(hHashType));
        uint8_t hash[64];
        int len = HashFile(hash_type == 3 ? HASH_SHA512 :
                           hash_type == 2 ? HASH_SHA256 :
                           hash_type == 1 ? HASH_SHA1 : HASH_MD5, path, hash);
        if (len > 0) {
            char hex[129] = {0};
            for (int i = 0; i < len; i++)
                snprintf(hex + i * 2, sizeof(hex) - i * 2, "%02x", hash[i]);
            uprintf("Hash: %s", hex);
        }
    }

    if (IsChecked(hCheckFakeDrive)) {
        uprintf("Checking for fake flash drive...");
        uint64_t reported_size = lufus_drive[device_index].size;
        uint64_t actual_size = GetDriveSize(device_index);
        if (actual_size > 0 && actual_size < reported_size * 0.9) {
            GtkWidget* dlg = gtk_message_dialog_new(GTK_WINDOW(hMainDialog),
                GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                "WARNING: Possible fake flash drive detected!\nReported: %s\nActual: %s",
                SizeToHumanReadable(reported_size, 0, 0),
                SizeToHumanReadable(actual_size, 0, 0));
            gtk_dialog_run(GTK_DIALOG(dlg));
            gtk_widget_destroy(dlg);
        } else {
            uprintf("Flash drive integrity: OK");
        }
    }

    if (IsChecked(hCheckValidateUEFI) && write_mode_idx == 0) {
        if (!ValidateUEFIBoot(path)) {
            GtkWidget* dlg = gtk_message_dialog_new(GTK_WINDOW(hMainDialog),
                GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
                "WARNING: UEFI boot validation failed!\nMissing or invalid EFI boot files.");
            gtk_dialog_run(GTK_DIALOG(dlg));
            gtk_widget_destroy(dlg);
        }
    }

    GtkWidget* dlg = gtk_message_dialog_new(GTK_WINDOW(hMainDialog),
        GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING, GTK_BUTTONS_YES_NO,
        "WARNING: All data on /dev/%s will be destroyed.\nAre you sure?",
        lufus_drive[device_index].id);
    if (gtk_dialog_run(GTK_DIALOG(dlg)) != GTK_RESPONSE_YES) {
        gtk_widget_destroy(dlg);
        return;
    }
    gtk_widget_destroy(dlg);

    if (IsChecked(hCheckBadBlocks)) {
        pthread_create(&tid, NULL, BadBlocksThread, (void*)(uintptr_t)device_index);
        pthread_detach(tid);
        return;
    }

    if (write_mode_idx == 1) {
        image_path = safe_strdup(path);
        pthread_create(&tid, NULL, DDWriteThread, (void*)(uintptr_t)device_index);
        pthread_detach(tid);
        return;
    }

    if (IsChecked(hCheckPersistent)) {
        persist_size = gtk_spin_button_get_value(GTK_SPIN_BUTTON(hPersistentSize)) * MB;
        uprintf("Creating persistent Linux partition (%s)", SizeToHumanReadable(persist_size, 0, 0));
        pthread_create(&tid, NULL, (void *(*)(void *))CreatePersistentLinux,
            (void*)(uintptr_t)device_index);
        pthread_detach(tid);
        return;
    }

    if (boot_mode == 4 || boot_mode == 5) {
        uprintf("Installing %s", boot_mode == 4 ? "FreeDOS" : "MS-DOS 6.22");
        pthread_create(&tid, NULL, (void *(*)(void *))InstallFreeDOS,
            (void*)(uintptr_t)device_index);
        pthread_detach(tid);
        return;
    }

    if (IsChecked(hCheckWindowsToGo)) {
        uprintf("Creating Windows To Go drive");
        pthread_create(&tid, NULL, (void *(*)(void *))CreateWindowsToGo,
            (void*)(uintptr_t)device_index);
        pthread_detach(tid);
        return;
    }

    if (win_info.is_windows && win_info.needs_bypass) {
        safe_sprintf(temp_dir, sizeof(temp_dir), "/tmp/lufus_%s", lufus_drive[device_index].id);
        mkdir(temp_dir, 0755);
        if (PatchWindowsISO(path, temp_dir, &win_info)) {
            image_path = safe_strdup(temp_dir);
            pthread_create(&tid, NULL, FormatThread, (void*)(uintptr_t)device_index);
            pthread_detach(tid);
            return;
        }
    }

    if (IsChecked(hLinux2WinTransfer)) {
        linux_profile profile;
        transfer_options opts;
        char settings_dir[PATH_MAX];
        char powershell_script[PATH_MAX];
        char batch_script[PATH_MAX];

        memset(&profile, 0, sizeof(profile));
        memset(&opts, 0, sizeof(opts));

        DetectLinuxProfile(&profile);

        opts.enable_theme_transfer = IsChecked(hLinux2WinTheme);
        opts.enable_wallpaper_transfer = IsChecked(hLinux2WinWallpaper);
        opts.enable_font_transfer = IsChecked(hLinux2WinFonts);
        opts.enable_proxy_transfer = IsChecked(hLinux2WinProxy);
        opts.enable_ssh_transfer = IsChecked(hLinux2WinSSH);
        opts.enable_git_transfer = IsChecked(hLinux2WinGit);
        opts.enable_bash_transfer = IsChecked(hLinux2WinBash);
        opts.enable_terminal_transfer = IsChecked(hLinux2WinTerminal);

        safe_sprintf(settings_dir, sizeof(settings_dir), "/tmp/lufus_migration_%s", lufus_drive[device_index].id);
        mkdir(settings_dir, 0755);

        ExportLinuxSettings(&profile, settings_dir);
        GenerateWindowsConfig(&profile, &opts, settings_dir);
        safe_sprintf(powershell_script, sizeof(powershell_script), "%s/migrate.ps1", settings_dir);
        CreatePowerShellScript(&profile, &opts, powershell_script);
        safe_sprintf(batch_script, sizeof(batch_script), "%s/migrate.bat", settings_dir);
        CreateBatchScript(&profile, &opts, batch_script);

        uprintf("Linux settings exported to: %s", settings_dir);
        uprintf("Migration scripts generated: migrate.ps1, migrate.bat");
    }

    image_path = safe_strdup(path);
    pthread_create(&tid, NULL, FormatThread, (void*)(uintptr_t)device_index);
    pthread_detach(tid);
}

static void on_refresh_devices(GtkWidget* widget, gpointer data)
{
    GetDevices(0);
}

static void on_write_mode_changed(GtkWidget* widget, gpointer data)
{
    int mode = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
    if (hDDOptionsBox)
        gtk_widget_set_visible(hDDOptionsBox, mode == 1);
    if (hWindowsOptionsBox)
        gtk_widget_set_visible(hWindowsOptionsBox, mode == 0);
    if (hLinux2WinBox)
        gtk_widget_set_visible(hLinux2WinBox, mode == 0);
}

static void create_main_dialog(void)
{
    GtkWidget* vbox;
    GtkWidget* hbox;
    GtkWidget* label;
    GtkWidget* scroll;
    GtkWidget* btn_refresh;

    hMainDialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(hMainDialog), APPLICATION_NAME " " LUFUS_VERSION);
    gtk_window_set_default_size(GTK_WINDOW(hMainDialog), 680, 520);
    g_signal_connect(hMainDialog, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);
    gtk_container_add(GTK_CONTAINER(hMainDialog), vbox);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("Device:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hDeviceList = gtk_combo_box_text_new();
    gtk_box_pack_start(GTK_BOX(hbox), hDeviceList, TRUE, TRUE, 0);

    btn_refresh = gtk_button_new_with_label("Refresh");
    g_signal_connect(btn_refresh, "clicked", G_CALLBACK(on_refresh_devices), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), btn_refresh, FALSE, FALSE, 0);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("Boot selection:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hBootSelection = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(hBootSelection), FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), hBootSelection, TRUE, TRUE, 0);

    hSelectImage = gtk_button_new_with_label("SELECT");
    g_signal_connect(hSelectImage, "clicked", G_CALLBACK(on_select_image), NULL);
    gtk_box_pack_start(GTK_BOX(hbox), hSelectImage, FALSE, FALSE, 0);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("Partition scheme:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hPartitionScheme = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hPartitionScheme), "MBR");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hPartitionScheme), "GPT");
    gtk_combo_box_set_active(GTK_COMBO_BOX(hPartitionScheme), 0);
    gtk_box_pack_start(GTK_BOX(hbox), hPartitionScheme, TRUE, TRUE, 0);

    label = gtk_label_new("Target system:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hTargetSystem = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hTargetSystem), "BIOS (or UEFI-CSM)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hTargetSystem), "UEFI (non CSM)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(hTargetSystem), 1);
    gtk_box_pack_start(GTK_BOX(hbox), hTargetSystem, TRUE, TRUE, 0);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("File system:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hFileSystem = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hFileSystem), "FAT32");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hFileSystem), "NTFS");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hFileSystem), "exFAT");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hFileSystem), "UDF");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hFileSystem), "ext2");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hFileSystem), "ext3");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hFileSystem), "ext4");
    gtk_combo_box_set_active(GTK_COMBO_BOX(hFileSystem), 0);
    gtk_box_pack_start(GTK_BOX(hbox), hFileSystem, TRUE, TRUE, 0);

    label = gtk_label_new("Cluster size:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hClusterSize = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hClusterSize), "Default");
    gtk_combo_box_set_active(GTK_COMBO_BOX(hClusterSize), 0);
    gtk_box_pack_start(GTK_BOX(hbox), hClusterSize, TRUE, TRUE, 0);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("Volume label:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hLabel = gtk_entry_new();
    gtk_entry_set_text(GTK_ENTRY(hLabel), "NO_LABEL");
    gtk_box_pack_start(GTK_BOX(hbox), hLabel, TRUE, TRUE, 0);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    hCheckQuickFormat = gtk_check_button_new_with_label("Quick format");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hCheckQuickFormat), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), hCheckQuickFormat, FALSE, FALSE, 0);

    hCheckBadBlocks = gtk_check_button_new_with_label("Check device for bad blocks");
    gtk_box_pack_start(GTK_BOX(hbox), hCheckBadBlocks, FALSE, FALSE, 0);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

    label = gtk_label_new("Write mode:");
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);

    hWriteMode = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hWriteMode), "ISO (File copy)");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hWriteMode), "DD (Raw image)");
    gtk_combo_box_set_active(GTK_COMBO_BOX(hWriteMode), 0);
    gtk_box_pack_start(GTK_BOX(hbox), hWriteMode, TRUE, TRUE, 0);
    g_signal_connect(hWriteMode, "changed", G_CALLBACK(on_write_mode_changed), NULL);

    hDDOptionsBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hDDOptionsBox, FALSE, FALSE, 0);
    gtk_widget_set_visible(hDDOptionsBox, FALSE);

    label = gtk_label_new("DD mode options:");
    gtk_box_pack_start(GTK_BOX(hDDOptionsBox), label, FALSE, FALSE, 0);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(hDDOptionsBox), hbox, FALSE, FALSE, 0);

    hDDDirectWrite = gtk_check_button_new_with_label("Direct device write");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hDDDirectWrite), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), hDDDirectWrite, FALSE, FALSE, 0);

    hDDSync = gtk_check_button_new_with_label("Sync after write");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hDDSync), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), hDDSync, FALSE, FALSE, 0);

    hDDVerify = gtk_check_button_new_with_label("Verify after write");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hDDVerify), FALSE);
    gtk_box_pack_start(GTK_BOX(hbox), hDDVerify, FALSE, FALSE, 0);

    hLinux2WinBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hLinux2WinBox, FALSE, FALSE, 0);

    label = gtk_label_new("Linux to Windows migration:");
    gtk_box_pack_start(GTK_BOX(hLinux2WinBox), label, FALSE, FALSE, 0);

    hLinux2WinTransfer = gtk_check_button_new_with_label("Enable migration profile");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hLinux2WinTransfer), FALSE);
    gtk_box_pack_start(GTK_BOX(hLinux2WinBox), hLinux2WinTransfer, FALSE, FALSE, 0);

    hLinux2WinTheme = gtk_check_button_new_with_label("Export theme settings");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hLinux2WinTheme), TRUE);
    gtk_box_pack_start(GTK_BOX(hLinux2WinBox), hLinux2WinTheme, FALSE, FALSE, 0);

    hLinux2WinWallpaper = gtk_check_button_new_with_label("Export wallpaper");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hLinux2WinWallpaper), TRUE);
    gtk_box_pack_start(GTK_BOX(hLinux2WinBox), hLinux2WinWallpaper, FALSE, FALSE, 0);

    hLinux2WinFonts = gtk_check_button_new_with_label("Export font settings");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hLinux2WinFonts), TRUE);
    gtk_box_pack_start(GTK_BOX(hLinux2WinBox), hLinux2WinFonts, FALSE, FALSE, 0);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(hLinux2WinBox), hbox, FALSE, FALSE, 0);

    hLinux2WinProxy = gtk_check_button_new_with_label("Export proxy settings");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hLinux2WinProxy), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), hLinux2WinProxy, FALSE, FALSE, 0);

    hLinux2WinSSH = gtk_check_button_new_with_label("Export SSH keys");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hLinux2WinSSH), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), hLinux2WinSSH, FALSE, FALSE, 0);

    hLinux2WinGit = gtk_check_button_new_with_label("Export Git config");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hLinux2WinGit), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), hLinux2WinGit, FALSE, FALSE, 0);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(hLinux2WinBox), hbox, FALSE, FALSE, 0);

    hLinux2WinBash = gtk_check_button_new_with_label("Export Bash config");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hLinux2WinBash), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), hLinux2WinBash, FALSE, FALSE, 0);

    hLinux2WinTerminal = gtk_check_button_new_with_label("Export terminal config");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hLinux2WinTerminal), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), hLinux2WinTerminal, FALSE, FALSE, 0);

    hBootOptionsBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hBootOptionsBox, FALSE, FALSE, 0);

    label = gtk_label_new("Boot options:");
    gtk_box_pack_start(GTK_BOX(hBootOptionsBox), label, FALSE, FALSE, 0);

    hBootMode = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hBootMode), "Non-bootable");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hBootMode), "BIOS only");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hBootMode), "UEFI only");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hBootMode), "BIOS + UEFI");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hBootMode), "FreeDOS");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hBootMode), "MS-DOS 6.22");
    gtk_combo_box_set_active(GTK_COMBO_BOX(hBootMode), 3);
    gtk_box_pack_start(GTK_BOX(hBootOptionsBox), hBootMode, FALSE, FALSE, 0);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(hBootOptionsBox), hbox, FALSE, FALSE, 0);

    hCheckWindowsToGo = gtk_check_button_new_with_label("Create Windows To Go");
    gtk_box_pack_start(GTK_BOX(hbox), hCheckWindowsToGo, FALSE, FALSE, 0);

    hCheckPersistent = gtk_check_button_new_with_label("Create persistent partition");
    gtk_box_pack_start(GTK_BOX(hbox), hCheckPersistent, FALSE, FALSE, 0);

    hPersistentSize = gtk_spin_button_new_with_range(128.0, 8192.0, 512.0);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(hPersistentSize), 1024.0);
    gtk_box_pack_start(GTK_BOX(hBootOptionsBox), hPersistentSize, FALSE, FALSE, 0);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(hBootOptionsBox), hbox, FALSE, FALSE, 0);

    hCheckValidateUEFI = gtk_check_button_new_with_label("Validate UEFI boot");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hCheckValidateUEFI), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), hCheckValidateUEFI, FALSE, FALSE, 0);

    hCheckComputeHash = gtk_check_button_new_with_label("Compute image hash");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hCheckComputeHash), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), hCheckComputeHash, FALSE, FALSE, 0);

    hHashType = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hHashType), "MD5");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hHashType), "SHA-1");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hHashType), "SHA-256");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(hHashType), "SHA-512");
    gtk_combo_box_set_active(GTK_COMBO_BOX(hHashType), 2);
    gtk_box_pack_start(GTK_BOX(hBootOptionsBox), hHashType, FALSE, FALSE, 0);

    hCheckFakeDrive = gtk_check_button_new_with_label("Detect fake flash drives");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hCheckFakeDrive), TRUE);
    gtk_box_pack_start(GTK_BOX(hBootOptionsBox), hCheckFakeDrive, FALSE, FALSE, 0);

    hWindowsOptionsBox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hWindowsOptionsBox, FALSE, FALSE, 0);

    label = gtk_label_new("Windows bloat removal:");
    gtk_box_pack_start(GTK_BOX(hWindowsOptionsBox), label, FALSE, FALSE, 0);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    gtk_box_pack_start(GTK_BOX(hWindowsOptionsBox), hbox, FALSE, FALSE, 0);

    hCheckBypassTPM = gtk_check_button_new_with_label("Bypass TPM 2.0");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hCheckBypassTPM), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), hCheckBypassTPM, FALSE, FALSE, 0);

    hCheckBypassSecureBoot = gtk_check_button_new_with_label("Bypass Secure Boot");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hCheckBypassSecureBoot), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), hCheckBypassSecureBoot, FALSE, FALSE, 0);

    hCheckBypassRAM = gtk_check_button_new_with_label("Bypass RAM check");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hCheckBypassRAM), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), hCheckBypassRAM, FALSE, FALSE, 0);

    hCheckBypassAccount = gtk_check_button_new_with_label("Disable MS account req");
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hCheckBypassAccount), TRUE);
    gtk_box_pack_start(GTK_BOX(hbox), hCheckBypassAccount, FALSE, FALSE, 0);

    hProgress = gtk_progress_bar_new();
    gtk_box_pack_start(GTK_BOX(vbox), hProgress, FALSE, FALSE, 0);

    hStart = gtk_button_new_with_label("START");
    g_signal_connect(hStart, "clicked", G_CALLBACK(on_start), NULL);
    gtk_box_pack_start(GTK_BOX(vbox), hStart, FALSE, FALSE, 0);

    hStatus = gtk_label_new("Ready");
    gtk_box_pack_start(GTK_BOX(vbox), hStatus, FALSE, FALSE, 0);

    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(vbox), scroll, TRUE, TRUE, 0);

    hLog = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(hLog), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(hLog), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(hLog), TRUE);
    log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(hLog));
    gtk_container_add(GTK_CONTAINER(scroll), hLog);

    gtk_widget_show_all(hMainDialog);
}

int RunCommandWithProgress(const char* cmdline, const char* dir, int log, int msg, const char* pattern)
{
    int status = system(cmdline);
    if (log)
        uprintf("Command: %s (status=%d)", cmdline, WEXITSTATUS(status));
    return WEXITSTATUS(status);
}

void UI_Init(int argc, char* argv[])
{
    gtk_init(&argc, &argv);
    create_main_dialog();
    GetDevices(0);
}

void UI_Run(void)
{
    gtk_main();
}
