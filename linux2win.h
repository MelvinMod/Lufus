#ifndef LINUX2WIN_H
#define LINUX2WIN_H

#include "lufus.h"

#define DE_UNKNOWN      0
#define DE_GNOME        1
#define DE_KDE          2
#define DE_XFCE         3
#define DE_CINNAMON     4
#define DE_I3WM         5
#define DE_SWAY         6
#define DE_MATE         7
#define DE_LXDE         8
#define DE_LXQT         9
#define DE_BUDGIE       10
#define DE_PANTHEON     11
#define DE_MAX          12

#define DISPLAY_X11     0
#define DISPLAY_WAYLAND 1
#define DISPLAY_UNKNOWN 2

#define OS_LINUX        0
#define OS_FREEBSD      1
#define OS_MACOS        2
#define OS_ALPINE       3
#define OS_MAX          4

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

#endif
