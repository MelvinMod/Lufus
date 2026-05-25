#include "wim.h"
#include "iso.h"
#include <archive.h>
#include <archive_entry.h>
#include <time.h>
#include <string.h>

static int check_iso_file_exists(const char* iso_path, const char* filename)
{
    struct archive* a;
    struct archive_entry* entry;
    int r;
    int found = 0;

    a = archive_read_new();
    archive_read_support_format_iso9660(a);

    r = archive_read_open_filename(a, iso_path, 65536);
    if (r != ARCHIVE_OK) {
        archive_read_free(a);
        return 0;
    }

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char* pathname = archive_entry_pathname(entry);
        if (safe_strcmp(pathname, filename) == 0) {
            found = 1;
            break;
        }
        archive_read_data_skip(a);
    }

    archive_read_free(a);
    return found;
}

static void detect_arch_from_path(const char* path, char* arch, size_t arch_len)
{
    if (safe_strstr(path, "x64") || safe_strstr(path, "amd64"))
        safe_strcpy(arch, arch_len, "x64");
    else if (safe_strstr(path, "x86") || safe_strstr(path, "32"))
        safe_strcpy(arch, arch_len, "x86");
    else if (safe_strstr(path, "arm64"))
        safe_strcpy(arch, arch_len, "arm64");
    else
        safe_strcpy(arch, arch_len, "x64");
}

int DetectWindowsISO(const char* iso_path, win_iso_info* info)
{
    struct archive* a;
    struct archive_entry* entry;
    int r;

    memset(info, 0, sizeof(*info));

    a = archive_read_new();
    archive_read_support_format_iso9660(a);

    r = archive_read_open_filename(a, iso_path, 65536);
    if (r != ARCHIVE_OK) {
        archive_read_free(a);
        return 0;
    }

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        const char* pathname = archive_entry_pathname(entry);

        if (safe_strstr(pathname, "sources/install.wim") != NULL)
            info->has_install_wim = 1;
        if (safe_strstr(pathname, "sources/install.esd") != NULL)
            info->has_install_esd = 1;
        if (safe_strstr(pathname, "sources/boot.wim") != NULL)
            info->has_boot_wim = 1;
        if (safe_strstr(pathname, "sources/appraiserres.dll") != NULL)
            info->is_win11 = 1;
        if (safe_strstr(pathname, "bootmgr.efi") != NULL)
            info->is_windows = 1;
        if (safe_strstr(pathname, "sources/setup.exe") != NULL)
            info->is_windows = 1;

        archive_read_data_skip(a);
    }

    archive_read_free(a);

    if (info->is_windows) {
        char* basename_path = basename(safe_strdup(iso_path));
        if (safe_strstr(basename_path, "11") || safe_strstr(basename_path, "Win11"))
            info->version = WIN_VERSION_11;
        else if (safe_strstr(basename_path, "10") || safe_strstr(basename_path, "Win10"))
            info->version = WIN_VERSION_10;
        else if (info->is_win11)
            info->version = WIN_VERSION_11;
        else
            info->version = WIN_VERSION_10;

        detect_arch_from_path(iso_path, info->arch, sizeof(info->arch));
        info->needs_bypass = (info->version >= WIN_VERSION_11);

        uprintf("Detected Windows %d %s ISO", info->version, info->arch);
        safe_free(basename_path);
    }

    return info->is_windows;
}

int CreateAutounattendXML(const char* path, int win_version)
{
    FILE* f;
    const char* xml_win11 =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<unattend xmlns=\"urn:schemas-microsoft-com:unattend\">\n"
        "  <settings pass=\"windowsPE\">\n"
        "    <component name=\"Microsoft-Windows-Setup\" processorArchitecture=\"amd64\" publicKeyToken=\"31bf3856ad364e35\" language=\"neutral\" versionScope=\"nonSxS\" xmlns:wcm=\"http://schemas.microsoft.com/WMIConfig/2002/State\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n"
        "      <UserData>\n"
        "        <AcceptEula>true</AcceptEula>\n"
        "      </UserData>\n"
        "      <RunSynchronous>\n"
        "        <RunSynchronousCommand wcm:action=\"add\">\n"
        "          <Order>1</Order>\n"
        "          <Path>reg.exe add \"HKLM\\SYSTEM\\Setup\\LabConfig\" /v BypassTPMCheck /t REG_DWORD /d 1 /f</Path>\n"
        "        </RunSynchronousCommand>\n"
        "        <RunSynchronousCommand wcm:action=\"add\">\n"
        "          <Order>2</Order>\n"
        "          <Path>reg.exe add \"HKLM\\SYSTEM\\Setup\\LabConfig\" /v BypassSecureBootCheck /t REG_DWORD /d 1 /f</Path>\n"
        "        </RunSynchronousCommand>\n"
        "        <RunSynchronousCommand wcm:action=\"add\">\n"
        "          <Order>3</Order>\n"
        "          <Path>reg.exe add \"HKLM\\SYSTEM\\Setup\\LabConfig\" /v BypassRAMCheck /t REG_DWORD /d 1 /f</Path>\n"
        "        </RunSynchronousCommand>\n"
        "        <RunSynchronousCommand wcm:action=\"add\">\n"
        "          <Order>4</Order>\n"
        "          <Path>reg.exe add \"HKLM\\SYSTEM\\Setup\\LabConfig\" /v BypassStorageCheck /t REG_DWORD /d 1 /f</Path>\n"
        "        </RunSynchronousCommand>\n"
        "        <RunSynchronousCommand wcm:action=\"add\">\n"
        "          <Order>5</Order>\n"
        "          <Path>reg.exe add \"HKLM\\SYSTEM\\Setup\\LabConfig\" /v BypassCPUCheck /t REG_DWORD /d 1 /f</Path>\n"
        "        </RunSynchronousCommand>\n"
        "      </RunSynchronous>\n"
        "    </component>\n"
        "  </settings>\n"
        "  <settings pass=\"specialize\">\n"
        "    <component name=\"Microsoft-Windows-Deployment\" processorArchitecture=\"amd64\" publicKeyToken=\"31bf3856ad364e35\" language=\"neutral\" versionScope=\"nonSxS\" xmlns:wcm=\"http://schemas.microsoft.com/WMIConfig/2002/State\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n"
        "      <RunSynchronous>\n"
        "        <RunSynchronousCommand wcm:action=\"add\">\n"
        "          <Order>1</Order>\n"
        "          <Path>reg.exe add \"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\OOBE\" /v BypassNRO /t REG_DWORD /d 1 /f</Path>\n"
        "        </RunSynchronousCommand>\n"
        "      </RunSynchronous>\n"
        "    </component>\n"
        "  </settings>\n"
        "  <settings pass=\"oobeSystem\">\n"
        "    <component name=\"Microsoft-Windows-Shell-Setup\" processorArchitecture=\"amd64\" publicKeyToken=\"31bf3856ad364e35\" language=\"neutral\" versionScope=\"nonSxS\" xmlns:wcm=\"http://schemas.microsoft.com/WMIConfig/2002/State\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n"
        "      <OOBE>\n"
        "        <HideEULAPage>true</HideEULAPage>\n"
        "        <HideOnlineAccountScreens>true</HideOnlineAccountScreens>\n"
        "        <HideWirelessSetupInOOBE>true</HideWirelessSetupInOOBE>\n"
        "        <NetworkLocation>Home</NetworkLocation>\n"
        "        <ProtectYourPC>3</ProtectYourPC>\n"
        "        <SkipMachineOOBE>true</SkipMachineOOBE>\n"
        "        <SkipUserOOBE>true</SkipUserOOBE>\n"
        "      </OOBE>\n"
        "      <UserAccounts>\n"
        "        <LocalAccounts>\n"
        "          <LocalAccount wcm:action=\"add\">\n"
        "            <Name>User</Name>\n"
        "            <Group>Administrators</Group>\n"
        "            <Password>\n"
        "              <Value></Value>\n"
        "              <PlainText>true</PlainText>\n"
        "            </Password>\n"
        "          </LocalAccount>\n"
        "        </LocalAccounts>\n"
        "      </UserAccounts>\n"
        "    </component>\n"
        "  </settings>\n"
        "</unattend>\n";

    const char* xml_win10 =
        "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
        "<unattend xmlns=\"urn:schemas-microsoft-com:unattend\">\n"
        "  <settings pass=\"specialize\">\n"
        "    <component name=\"Microsoft-Windows-Deployment\" processorArchitecture=\"amd64\" publicKeyToken=\"31bf3856ad364e35\" language=\"neutral\" versionScope=\"nonSxS\" xmlns:wcm=\"http://schemas.microsoft.com/WMIConfig/2002/State\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n"
        "      <RunSynchronous>\n"
        "        <RunSynchronousCommand wcm:action=\"add\">\n"
        "          <Order>1</Order>\n"
        "          <Path>reg.exe add \"HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\OOBE\" /v BypassNRO /t REG_DWORD /d 1 /f</Path>\n"
        "        </RunSynchronousCommand>\n"
        "      </RunSynchronous>\n"
        "    </component>\n"
        "  </settings>\n"
        "  <settings pass=\"oobeSystem\">\n"
        "    <component name=\"Microsoft-Windows-Shell-Setup\" processorArchitecture=\"amd64\" publicKeyToken=\"31bf3856ad364e35\" language=\"neutral\" versionScope=\"nonSxS\" xmlns:wcm=\"http://schemas.microsoft.com/WMIConfig/2002/State\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\">\n"
        "      <OOBE>\n"
        "        <HideEULAPage>true</HideEULAPage>\n"
        "        <HideOnlineAccountScreens>true</HideOnlineAccountScreens>\n"
        "        <HideWirelessSetupInOOBE>true</HideWirelessSetupInOOBE>\n"
        "        <NetworkLocation>Home</NetworkLocation>\n"
        "        <ProtectYourPC>3</ProtectYourPC>\n"
        "        <SkipMachineOOBE>true</SkipMachineOOBE>\n"
        "        <SkipUserOOBE>true</SkipUserOOBE>\n"
        "      </OOBE>\n"
        "      <UserAccounts>\n"
        "        <LocalAccounts>\n"
        "          <LocalAccount wcm:action=\"add\">\n"
        "            <Name>User</Name>\n"
        "            <Group>Administrators</Group>\n"
        "            <Password>\n"
        "              <Value></Value>\n"
        "              <PlainText>true</PlainText>\n"
        "            </Password>\n"
        "          </LocalAccount>\n"
        "        </LocalAccounts>\n"
        "      </UserAccounts>\n"
        "    </component>\n"
        "  </settings>\n"
        "</unattend>\n";

    f = fopen(path, "w");
    if (!f) return 0;

    if (win_version >= WIN_VERSION_11)
        fprintf(f, "%s", xml_win11);
    else
        fprintf(f, "%s", xml_win10);

    fclose(f);
    return 1;
}

int CreateBypassFiles(const char* dest_dir, win_iso_info* info)
{
    char path[PATH_MAX];
    int r = 1;

    if (!info->is_windows)
        return 1;

    uprintf("Creating Windows bypass files...");

    safe_sprintf(path, sizeof(path), "%s/autounattend.xml", dest_dir);
    if (CreateAutounattendXML(path, info->version)) {
        uprintf("Created %s", path);
    } else {
        uprintf("Failed to create autounattend.xml");
        r = 0;
    }

    if (info->version >= WIN_VERSION_11) {
        safe_sprintf(path, sizeof(path), "%s/sources/appraiserres.dll", dest_dir);
        FILE* f = fopen(path, "wb");
        if (f) {
            fclose(f);
            uprintf("Created dummy appraiserres.dll");
        } else {
            uprintf("Failed to create appraiserres.dll");
        }
    }

    return r;
}

int PatchWindowsISO(const char* iso_path, const char* dest_dir, win_iso_info* info)
{
    int r;

    if (!info->is_windows)
        return 1;

    uprintf("Patching Windows ISO for bypass...");

    r = ExtractISO(iso_path, dest_dir, 0);
    if (!r) {
        uprintf("Failed to extract ISO contents");
        return 0;
    }

    r = CreateBypassFiles(dest_dir, info);
    if (!r) {
        uprintf("Failed to create bypass files");
        return 0;
    }

    uprintf("Windows ISO patched successfully");
    return 1;
}
