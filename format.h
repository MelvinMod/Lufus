#ifndef FORMAT_H
#define FORMAT_H

#include "lufus.h"

typedef enum {
    FCC_PROGRESS,
    FCC_DONE_WITH_STRUCTURE,
    FCC_UNKNOWN2,
    FCC_INCOMPATIBLE_FILE_SYSTEM,
    FCC_UNKNOWN4,
    FCC_UNKNOWN5,
    FCC_ACCESS_DENIED,
    FCC_MEDIA_WRITE_PROTECTED,
    FCC_VOLUME_IN_USE,
    FCC_CANT_QUICK_FORMAT,
    FCC_UNKNOWNA,
    FCC_DONE,
    FCC_BAD_LABEL,
    FCC_UNKNOWND,
    FCC_OUTPUT,
    FCC_STRUCTURE_PROGRESS,
    FCC_CLUSTER_SIZE_TOO_SMALL,
    FCC_CLUSTER_SIZE_TOO_BIG,
    FCC_VOLUME_TOO_SMALL,
    FCC_VOLUME_TOO_BIG,
    FCC_NO_MEDIA_IN_DRIVE,
    FCC_UNKNOWN15,
    FCC_UNKNOWN16,
    FCC_UNKNOWN17,
    FCC_DEVICE_NOT_READY,
    FCC_CHECKDISK_PROGRESS,
    FCC_UNKNOWN1A,
    FCC_UNKNOWN1B,
    FCC_UNKNOWN1C,
    FCC_UNKNOWN1D,
    FCC_UNKNOWN1E,
    FCC_UNKNOWN1F,
    FCC_READ_ONLY_MODE,
    FCC_UNKNOWN21,
    FCC_UNKNOWN22,
    FCC_UNKNOWN23,
    FCC_UNKNOWN24,
    FCC_ALIGNMENT_VIOLATION,
} FILE_SYSTEM_CALLBACK_COMMAND;

#define IMG_COMPRESSION_FFU     0x100
#define IMG_COMPRESSION_VHD     0x101
#define IMG_COMPRESSION_VHDX    0x102

extern int WritePBR(int fd);
extern int FormatLargeFAT32(uint32_t DriveIndex, uint64_t PartitionOffset, uint32_t ClusterSize, const char* FSName, const char* Label, uint32_t Flags);
extern int FormatExtFs(uint32_t DriveIndex, uint64_t PartitionOffset, uint32_t BlockSize, const char* FSName, const char* Label, uint32_t Flags);
extern int FormatPartition(uint32_t DriveIndex, uint64_t PartitionOffset, uint32_t UnitAllocationSize, uint16_t FSType, const char* Label, uint32_t Flags);
extern void* FormatThread(void* param);

#endif
