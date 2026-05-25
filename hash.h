#ifndef HASH_H
#define HASH_H

#include "lufus.h"

extern int HashFile(unsigned type, const char* path, uint8_t* sum);
extern int HashBuffer(unsigned type, const uint8_t* buf, const size_t len, uint8_t* sum);
extern uint8_t* StringToHash(const char* str);
extern int FileMatchesHash(const char* path, const char* str);
extern int BufferMatchesHash(const uint8_t* buf, const size_t len, const char* str);
extern int IsFileInDB(const char* path);
extern int IsBufferInDB(const unsigned char* buf, const size_t len);
extern void* HashThread(void* param);

#endif
