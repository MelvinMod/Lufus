#ifndef UI_H
#define UI_H

#include "lufus.h"

#define TOOLBAR_ICON_COLOR              0x2980B9
#define PROGRESS_BAR_NORMAL_COLOR       0x06B025
#define PROGRESS_BAR_PAUSED_COLOR       0xDACB26
#define PROGRESS_BAR_ERROR_COLOR        0xDA2626

extern GtkWidget* hMainDialog, *hLogDialog, *hStatus, *hDeviceList, *hCapacity, *hImageOption;
extern GtkWidget* hPartitionScheme, *hTargetSystem, *hFileSystem, *hClusterSize, *hLabel, *hBootType;
extern GtkWidget* hNBPasses, *hLog, *hInfo, *hProgress;
extern GtkWidget* hMultiToolbar, *hSaveToolbar, *hHashToolbar, *hAdvancedDeviceToolbar, *hAdvancedFormatToolbar;
extern GtkWidget* hSelectImage, *hStart;

extern void UI_Init(int argc, char* argv[]);
extern void UI_Run(void);
extern void EnableControls(int enable, int remove_checkboxes);
extern void InitProgress(int bOnlyFormat);
extern void SetPersistenceSize(void);
extern void TogglePersistenceControls(int display);
extern void ToggleAdvancedDeviceOptions(int enable);
extern void ToggleAdvancedFormatOptions(int enable);
extern void ToggleImageOptions(void);
extern void SetComboEntry(GtkWidget* combo, int data);
extern void OnPaint(cairo_t* cr);

#endif
