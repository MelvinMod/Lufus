#ifndef ISO_H
#define ISO_H

#include "lufus.h"

extern int ExtractISO(const char* src_iso, const char* dest_dir, int scan);
extern int ExtractZip(const char* src_zip, const char* dest_dir);
extern int64_t ExtractISOFile(const char* iso, const char* iso_file, const char* dest_file, uint32_t attributes);
extern uint32_t ReadISOFileToBuffer(const char* iso, const char* iso_file, uint8_t** buf);
extern int HasEfiImgBootLoaders(void* iso);
extern int DumpFatDir(void* iso, const char* path, int32_t cluster);
extern int InstallSyslinux(uint32_t drive_index, char drive_letter, int fs);
extern uint16_t GetSyslinuxVersion(char* buf, size_t buf_size, char** ext);
extern int SetAutorun(const char* path);

#endif
