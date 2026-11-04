#ifndef PLATFORM_H
#define PLATFORM_H
#include "save.h"
/* Calculator-only glue (CEdev fileioc). UNVERIFIED: written from memory, not compiled in this sandbox. */
void platform_save_write(const SaveData *s);
SaveStatus platform_save_load(SaveData *s);
#endif
