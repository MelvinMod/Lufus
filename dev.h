#ifndef DEV_H
#define DEV_H

#include "lufus.h"

#define USB_SPEED_UNKNOWN           0
#define USB_SPEED_LOW               1
#define USB_SPEED_FULL              2
#define USB_SPEED_HIGH              3
#define USB_SPEED_SUPER             4
#define USB_SPEED_SUPER_PLUS        5
#define USB_SPEED_MAX               6

typedef struct usb_device_props {
    int32_t  vid;
    int32_t  pid;
    uint32_t speed;
    uint32_t lower_speed;
    uint32_t port;
    int      is_USB;
    int      is_SCSI;
    int      is_CARD;
    int      is_UASP;
    int      is_VHD;
    int      is_Removable;
} usb_device_props;

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
extern int IsHDD(uint32_t DriveIndex, uint16_t vid, uint16_t pid, const char* strid);

#endif
