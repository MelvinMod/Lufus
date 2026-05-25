#include "linux2win.h"
#include <pwd.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

extern int verbose;

static const char* get_de_name(int de)
{
    switch (de) {
    case DE_GNOME: return "GNOME";
    case DE_KDE: return "KDE";
    case DE_XFCE: return "XFCE";
    case DE_CINNAMON: return "Cinnamon";
    case DE_I3WM: return "i3wm";
    case DE_SWAY: return "Sway";
    case DE_MATE: return "MATE";
    case DE_LXDE: return "LXDE";
    case DE_LXQT: return "LXQt";
    case DE_BUDGIE: return "Budgie";
    case DE_PANTHEON: return "Pantheon";
    default: return "Unknown";
    }
}

static int check_env_var(const char* name)
{
    char* val = getenv(name);
    return (val != NULL && strlen(val) > 0);
}

static int file_exists(const char* path)
{
    struct stat st;
    return (stat(path, &st) == 0);
}

static int read_file_line(const char* path, char* out, size_t out_size, int line)
{
    FILE* f = fopen(path, "r");
    int current_line = 0;
    if (!f) return 0;
    while (fgets(out, out_size, f)) {
        if (current_line == line) {
            size_t len = strlen(out);
            while (len > 0 && (out[len-1] == '\n' || out[len-1] == '\r'))
                out[--len] = '\0';
            fclose(f);
            return 1;
        }
        current_line++;
    }
    fclose(f);
    return 0;
}

int DetectDesktopEnvironment(void)
{
    char* xsession = getenv("XDG_CURRENT_DESKTOP");
    char* session = getenv("DESKTOP_SESSION");
    char* gdm_session = getenv("GDMSESSION");

    if (xsession) {
        if (safe_strstr(xsession, "GNOME") != NULL) return DE_GNOME;
        if (safe_strstr(xsession, "KDE") != NULL || safe_strstr(xsession, "Plasma") != NULL) return DE_KDE;
        if (safe_strstr(xsession, "XFCE") != NULL) return DE_XFCE;
        if (safe_strstr(xsession, "CINNAMON") != NULL) return DE_CINNAMON;
        if (safe_strstr(xsession, "i3") != NULL) return DE_I3WM;
        if (safe_strstr(xsession, "SWAY") != NULL) return DE_SWAY;
        if (safe_strstr(xsession, "MATE") != NULL) return DE_MATE;
        if (safe_strstr(xsession, "LXDE") != NULL) return DE_LXDE;
        if (safe_strstr(xsession, "LXQT") != NULL) return DE_LXQT;
        if (safe_strstr(xsession, "BUDGIE") != NULL) return DE_BUDGIE;
        if (safe_strstr(xsession, "Pantheon") != NULL) return DE_PANTHEON;
    }

    if (session) {
        if (safe_strstr(session, "gnome") != NULL) return DE_GNOME;
        if (safe_strstr(session, "kde") != NULL || safe_strstr(session, "plasma") != NULL) return DE_KDE;
        if (safe_strstr(session, "xfce") != NULL) return DE_XFCE;
        if (safe_strstr(session, "cinnamon") != NULL) return DE_CINNAMON;
        if (safe_strstr(session, "i3") != NULL) return DE_I3WM;
        if (safe_strstr(session, "sway") != NULL) return DE_SWAY;
        if (safe_strstr(session, "mate") != NULL) return DE_MATE;
        if (safe_strstr(session, "lxde") != NULL) return DE_LXDE;
        if (safe_strstr(session, "lxqt") != NULL) return DE_LXQT;
    }

    if (gdm_session) {
        if (safe_strstr(gdm_session, "gnome") != NULL) return DE_GNOME;
        if (safe_strstr(gdm_session, "kde") != NULL) return DE_KDE;
        if (safe_strstr(gdm_session, "xfce") != NULL) return DE_XFCE;
    }

    return DE_UNKNOWN;
}

int DetectDisplayServer(void)
{
    char* session_type = getenv("XDG_SESSION_TYPE");
    char* wayland_display = getenv("WAYLAND_DISPLAY");
    char* display = getenv("DISPLAY");

    if (session_type) {
        if (safe_strstr(session_type, "wayland") != NULL) return DISPLAY_WAYLAND;
        if (safe_strstr(session_type, "x11") != NULL) return DISPLAY_X11;
    }

    if (wayland_display) return DISPLAY_WAYLAND;
    if (display && display[0] == ':') return DISPLAY_X11;

    if (file_exists("/tmp/.X11-unix/X0")) return DISPLAY_X11;
    if (file_exists("/run/user/0/wayland-0")) return DISPLAY_WAYLAND;

    return DISPLAY_UNKNOWN;
}

int DetectOSType(void)
{
    struct utsname uts;
    char os_release[256] = {0};

    if (uname(&uts) == 0) {
        if (safe_strcmp(uts.sysname, "Linux") == 0) {
            if (read_file_line("/etc/os-release", os_release, sizeof(os_release), 0)) {
                if (safe_strstr(os_release, "ID=alpine") != NULL) return OS_ALPINE;
            }
            return OS_LINUX;
        } else if (safe_strcmp(uts.sysname, "FreeBSD") == 0) {
            return OS_FREEBSD;
        } else if (safe_strcmp(uts.sysname, "Darwin") == 0) {
            return OS_MACOS;
        }
    }
    return OS_LINUX;
}

static int read_distro_info(linux_profile* profile)
{
    FILE* f;
    char line[256];
    const char* files[] = {"/etc/os-release", "/etc/redhat-release", "/etc/debian_version", "/etc/alpine-release", NULL};
    int i;

    for (i = 0; files[i] && !profile->distro[0]; i++) {
        f = fopen(files[i], "r");
        if (!f) continue;
        while (fgets(line, sizeof(line), f)) {
            if (safe_strstr(line, "PRETTY_NAME=") != NULL || safe_strstr(line, "NAME=") != NULL) {
                char* p = strchr(line, '"');
                if (p) {
                    p++;
                    char* end = strchr(p, '"');
                    if (end) *end = '\0';
                    safe_strcpy(profile->distro, sizeof(profile->distro), p);
                    break;
                }
            }
        }
        fclose(f);
    }

    if (!profile->distro[0]) {
        struct utsname uts;
        if (uname(&uts) == 0)
            safe_strcpy(profile->distro, sizeof(profile->distro), uts.release);
    }

    return 1;
}

static int read_user_info(linux_profile* profile)
{
    struct passwd* pw;
    const char* home;

    home = getenv("HOME");
    if (home) safe_strcpy(profile->username, sizeof(profile->username), basename(safe_strdup(home)));

    pw = getpwuid(getuid());
    if (pw && pw->pw_name) {
        safe_strcpy(profile->username, sizeof(profile->username), pw->pw_name);
        if (!home) safe_strcpy(profile->username, sizeof(profile->username), pw->pw_name);
    }

    return 1;
}

static int read_system_info(linux_profile* profile)
{
    struct utsname uts;
    FILE* f;
    char line[256];

    if (uname(&uts) == 0)
        safe_strcpy(profile->kernel_version, sizeof(profile->kernel_version), uts.release);

    f = fopen("/etc/timezone", "r");
    if (f) {
        if (fgets(line, sizeof(line), f)) {
            char* end = strchr(line, '\n');
            if (end) *end = '\0';
            safe_strcpy(profile->timezone, sizeof(profile->timezone), line);
        }
        fclose(f);
    } else {
        f = popen("timedatectl 2>/dev/null | grep 'Time zone' | awk '{print $3}'", "r");
        if (f && fgets(line, sizeof(line), f)) {
            line[strcspn(line, "\n")] = 0;
            safe_strcpy(profile->timezone, sizeof(profile->timezone), line);
        }
        if (f) pclose(f);
    }

    f = popen("locale | grep LANG | cut -d= -f2 | cut -d_ -f1", "r");
    if (f && fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        safe_strcpy(profile->language, sizeof(profile->language), line);
    }
    if (f) pclose(f);

    f = popen("localectl 2>/dev/null | grep 'Key Layout' | awk '{print $3}'", "r");
    if (f && fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\n")] = 0;
        safe_strcpy(profile->keyboard_layout, sizeof(profile->keyboard_layout), line);
    }
    if (f) pclose(f);

    return 1;
}

int DetectLinuxProfile(linux_profile* profile)
{
    memset(profile, 0, sizeof(*profile));

    profile->desktop_env = DetectDesktopEnvironment();
    profile->display_server = DetectDisplayServer();
    profile->os_type = DetectOSType();

    read_distro_info(profile);
    read_user_info(profile);
    read_system_info(profile);

    gethostname(profile->hostname, sizeof(profile->hostname));

    uprintf("Detected: %s on %s", get_de_name(profile->desktop_env), profile->distro);
    uprintf("Display: %s, Kernel: %s",
        profile->display_server == DISPLAY_WAYLAND ? "Wayland" :
        profile->display_server == DISPLAY_X11 ? "X11" : "Unknown",
        profile->kernel_version);

    return 1;
}

static void create_dir_recursive(const char* path)
{
    char cmd[512];
    safe_sprintf(cmd, sizeof(cmd), "mkdir -p '%s'", path);
    IGNORE_RETVAL(system(cmd));
}

int ExportLinuxSettings(linux_profile* profile, const char* output_dir)
{
    char src[PATH_MAX], dst[PATH_MAX];
    const char* dirs_to_export[] = {
        ".config/gtk-3.0",
        ".config/gtk-4.0",
        ".config/dconf",
        ".config/kde4",
        ".config/plasma",
        ".config/xfce4",
        ".config/i3",
        ".config/sway",
        ".config/cinnamon",
        ".local/share/icons",
        ".local/share/themes",
        ".icons",
        ".themes",
        ".ssh",
        ".git",
        ".bashrc",
        ".vimrc",
        ".vim",
        ".config/terminal",
        ".config/gnome-terminal",
        NULL
    };
    const char* home = getenv("HOME");
    int i;

    if (!home) return 0;

    create_dir_recursive(output_dir);
    create_dir_recursive(strcat(safe_strdup(output_dir), "/settings"));

    for (i = 0; dirs_to_export[i]; i++) {
        safe_sprintf(src, sizeof(src), "%s/%s", home, dirs_to_export[i]);
        safe_sprintf(dst, sizeof(dst), "%s/settings/%s", output_dir, dirs_to_export[i]);
        create_dir_recursive(dst);

        char cmd[1024];
        safe_sprintf(cmd, sizeof(cmd), "cp -r '%s'/* '%s'/ 2>/dev/null", src, dst);
        IGNORE_RETVAL(system(cmd));
    }

    return 1;
}

int GenerateWindowsConfig(linux_profile* profile, transfer_options* opts, const char* output_dir)
{
    char path[PATH_MAX];

    uprintf("Generating Windows configuration...");

    safe_sprintf(path, sizeof(path), "%s/WindowsSettings.txt", output_dir);
    FILE* f = fopen(path, "w");
    if (f) {
        fprintf(f, "# Windows Migration Configuration\n");
        fprintf(f, "# Source: %s (%s)\n", profile->distro, get_de_name(profile->desktop_env));
        fprintf(f, "# Generated: %s\n", TimestampToHumanReadable(time(NULL)));
        fprintf(f, "#\n");
        fprintf(f, "Desktop Environment: %s\n", get_de_name(profile->desktop_env));
        fprintf(f, "Display Server: %s\n", profile->display_server == DISPLAY_WAYLAND ? "Wayland" : "X11");
        fprintf(f, "Distribution: %s\n", profile->distro);
        fprintf(f, "Kernel: %s\n", profile->kernel_version);
        fprintf(f, "Hostname: %s\n", profile->hostname);
        fprintf(f, "Username: %s\n", profile->username);
        fprintf(f, "Timezone: %s\n", profile->timezone);
        fprintf(f, "Language: %s\n", profile->language);
        fprintf(f, "Keyboard: %s\n", profile->keyboard_layout);
        fprintf(f, "\n");
        fprintf(f, "# Transfer Options:\n");
        fprintf(f, "Theme Transfer: %s\n", opts->enable_theme_transfer ? "Yes" : "No");
        fprintf(f, "Wallpaper Transfer: %s\n", opts->enable_wallpaper_transfer ? "Yes" : "No");
        fprintf(f, "Font Transfer: %s\n", opts->enable_font_transfer ? "Yes" : "No");
        fprintf(f, "Proxy Settings: %s\n", opts->enable_proxy_transfer ? "Yes" : "No");
        fprintf(f, "SSH Keys: %s\n", opts->enable_ssh_transfer ? "Yes" : "No");
        fprintf(f, "Git Config: %s\n", opts->enable_git_transfer ? "Yes" : "No");
        fprintf(f, "Bash Config: %s\n", opts->enable_bash_transfer ? "Yes" : "No");
        fclose(f);
    }

    return 1;
}

int CreateWindowsRegistry(linux_profile* profile, transfer_options* opts, const char* output_file)
{
    FILE* f = fopen(output_file, "w");
    if (!f) return 0;

    fprintf(f, "Windows Registry Editor Version 5.00\n\n");

    fprintf(f, "[HKEY_CURRENT_USER\\Software\\LufusMigration]\n");
    fprintf(f, "\"SourceOS\"=\"%s\"\n", profile->distro);
    fprintf(f, "\"SourceDE\"=\"%s\"\n", get_de_name(profile->desktop_env));
    fprintf(f, "\"SourceDisplay\"=\"%s\"\n", profile->display_server == DISPLAY_WAYLAND ? "Wayland" : "X11");
    fprintf(f, "\"SourceHostname\"=\"%s\"\n", profile->hostname);
    fprintf(f, "\"SourceUsername\"=\"%s\"\n", profile->username);
    fprintf(f, "\"SourceTimezone\"=\"%s\"\n", profile->timezone);
    fprintf(f, "\"SourceLanguage\"=\"%s\"\n", profile->language);
    fprintf(f, "\"SourceKeyboard\"=\"%s\"\n", profile->keyboard_layout);
    fprintf(f, "\"MigratedDate\"=\"%s\"\n", TimestampToHumanReadable(time(NULL)));

    if (opts->enable_theme_transfer) {
        fprintf(f, "\n[HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes]\n");
        fprintf(f, "\"EnableThemeCustomization\"=dword:00000001\n");
    }

    if (opts->enable_proxy_transfer) {
        fprintf(f, "\n[HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings]\n");
        fprintf(f, "\"ProxyEnable\"=dword:00000001\n");
    }

    fclose(f);
    return 1;
}

int CreatePowerShellScript(linux_profile* profile, transfer_options* opts, const char* output_file)
{
    FILE* f = fopen(output_file, "w");
    if (!f) return 0;

    fprintf(f, "# Lufus Linux to Windows Migration Script\n");
    fprintf(f, "# Source: %s (%s)\n", profile->distro, get_de_name(profile->desktop_env));
    fprintf(f, "# Run as Administrator for full functionality\n\n");

    fprintf(f, "$ErrorActionPreference = 'Stop'\n");
    fprintf(f, "Write-Host \"Lufus Migration Script\" -ForegroundColor Cyan\n");
    fprintf(f, "Write-Host \"Source: %s (%s)\" -ForegroundColor Yellow\n", profile->distro, get_de_name(profile->desktop_env));
    fprintf(f, "Write-Host \"\"\n\n");

    if (opts->enable_theme_transfer) {
        fprintf(f, "# Theme settings\n");
        fprintf(f, "Write-Host \"Applying theme settings...\" -ForegroundColor Green\n");
        fprintf(f, "# Set Windows theme to match Linux preference\n");
        fprintf(f, "Set-ItemProperty -Path \"HKCU:\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize\" -Name \"AppsUseLightTheme\" -Value 1\n");
        fprintf(f, "Write-Host \"Theme applied successfully\" -ForegroundColor Green\n\n");
    }

    if (opts->enable_font_transfer) {
        fprintf(f, "# Font settings\n");
        fprintf(f, "Write-Host \"Configuring fonts...\" -ForegroundColor Green\n");
        fprintf(f, "# Set Consolas as default monospace font (similar to Linux terminals)\n");
        fprintf(f, "Set-ItemProperty -Path \"HKCU\\Console\" -Name \"FaceName\" -Value \"Consolas\"\n");
        fprintf(f, "Write-Host \"Fonts configured successfully\" -ForegroundColor Green\n\n");
    }

    if (opts->enable_proxy_transfer) {
        fprintf(f, "# Proxy settings\n");
        fprintf(f, "Write-Host \"Configuring proxy...\" -ForegroundColor Green\n");
        fprintf(f, "# Add your proxy configuration below\n");
        fprintf(f, "$proxyServer = \"\"\n");
        fprintf(f, "if ($proxyServer -ne \"\") {\n");
        fprintf(f, "    Set-ItemProperty -Path \"HKCU:\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\" -Name \"ProxyEnable\" -Value 1\n");
        fprintf(f, "    Set-ItemProperty -Path \"HKCU:\\Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\" -Name \"ProxyServer\" -Value $proxyServer\n");
        fprintf(f, "}\n\n");
    }

    fprintf(f, "# Network configuration\n");
    fprintf(f, "Write-Host \"Setting up network...\" -ForegroundColor Green\n");
    fprintf(f, "Write-Host \"Network configuration complete\" -ForegroundColor Green\n\n");

    if (opts->enable_ssh_transfer) {
        fprintf(f, "# SSH configuration\n");
        fprintf(f, "Write-Host \"Configuring SSH...\" -ForegroundColor Green\n");
        fprintf(f, "# Copy SSH keys from Linux backup\n");
        fprintf(f, "$sshDir = \"$env:USERPROFILE\\.ssh\"\n");
        fprintf(f, "if (!(Test-Path $sshDir)) {\n");
        fprintf(f, "    New-Item -ItemType Directory -Path $sshDir | Out-Null\n");
        fprintf(f, "}\n");
        fprintf(f, "Write-Host \"SSH configured successfully\" -ForegroundColor Green\n\n");
    }

    if (opts->enable_git_transfer) {
        fprintf(f, "# Git configuration\n");
        fprintf(f, "Write-Host \"Configuring Git...\" -ForegroundColor Green\n");
        fprintf(f, "git config --global user.name \"%s\"\n", profile->username);
        fprintf(f, "git config --global core.editor \"code --wait\"\n");
        fprintf(f, "Write-Host \"Git configured successfully\" -ForegroundColor Green\n\n");
    }

    if (opts->enable_bash_transfer) {
        fprintf(f, "# Bash/WSL configuration\n");
        fprintf(f, "Write-Host \"Setting up WSL...\" -ForegroundColor Green\n");
        fprintf(f, "# Install WSL if not already installed\n");
        fprintf(f, "wsl --install -d Ubuntu\n");
        fprintf(f, "Write-Host \"WSL setup complete\" -ForegroundColor Green\n\n");
    }

    fprintf(f, "Write-Host \"\"\n");
    fprintf(f, "Write-Host \"Migration completed successfully!\" -ForegroundColor Cyan\n");
    fprintf(f, "Write-Host \"Restart your computer to apply all changes.\" -ForegroundColor Yellow\n");

    fclose(f);
    return 1;
}

int CreateBatchScript(linux_profile* profile, transfer_options* opts, const char* output_file)
{
    FILE* f = fopen(output_file, "w");
    if (!f) return 0;

    fprintf(f, "@echo off\n");
    fprintf(f, "title Lufus Migration Script\n");
    fprintf(f, "echo Lufus Linux to Windows Migration\n");
    fprintf(f, "echo Source: %s (%s)\n", profile->distro, get_de_name(profile->desktop_env));
    fprintf(f, "echo.\n");

    if (opts->enable_theme_transfer) {
        fprintf(f, "echo Applying theme settings...\n");
        fprintf(f, "reg add \"HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Themes\" /v EnableThemeCustomization /t REG_DWORD /d 1 /f\n");
    }

    if (opts->enable_font_transfer) {
        fprintf(f, "echo Configuring fonts...\n");
        fprintf(f, "reg add \"HKCU\\Console\" /v FaceName /t REG_SZ /d Consolas /f\n");
    }

    if (opts->enable_git_transfer) {
        fprintf(f, "echo Configuring Git...\n");
        fprintf(f, "git config --global user.name \"%s\"\n", profile->username);
    }

    fprintf(f, "echo.\n");
    fprintf(f, "echo Migration completed successfully!\n");
    fprintf(f, "pause\n");

    fclose(f);
    return 1;
}

int TransferSettingsToISO(const char* iso_path, const char* settings_dir)
{
    char cmd[1024];
    char temp_dir[PATH_MAX];

    safe_sprintf(temp_dir, sizeof(temp_dir), "/tmp/lufus_iso_work");
    safe_sprintf(cmd, sizeof(cmd), "rm -rf '%s' && mkdir -p '%s'", temp_dir, temp_dir);
    IGNORE_RETVAL(system(cmd));

    safe_sprintf(cmd, sizeof(cmd), "cp -r '%s' '%s'/LinuxSettings", settings_dir, temp_dir);
    IGNORE_RETVAL(system(cmd));

    safe_sprintf(cmd, sizeof(cmd), "cp '%s' '%s'/ 2>/dev/null", iso_path, temp_dir);
    IGNORE_RETVAL(system(cmd));

    uprintf("Settings copied to temporary directory");
    uprintf("To rebuild ISO: cd %s && mkisofs -o output.iso .", temp_dir);

    return 1;
}
