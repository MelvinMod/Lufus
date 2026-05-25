#ifndef WIM_H
#define WIM_H

#include "lufus.h"

#define WIN_VERSION_UNKNOWN     0
#define WIN_VERSION_10          10
#define WIN_VERSION_11          11

extern int DetectWindowsISO(const char* iso_path, win_iso_info* info);
extern int PatchWindowsISO(const char* iso_path, const char* dest_dir, win_iso_info* info);
extern int CreateBypassFiles(const char* dest_dir, win_iso_info* info);
extern int CreateAutounattendXML(const char* path, int win_version);

#endif
