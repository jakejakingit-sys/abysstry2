/* UNVERIFIED against CEdev docs/headers - no toolchain was available when this was written. */
#include "platform.h"
#ifdef __TICE__
#include <fileioc.h>
#define SAVE_NAME "ABYSSSV"
void platform_save_write(const SaveData *s) {
  uint8_t buf[SAVE_BYTES]; save_serialize(s, buf);
  uint8_t h = ti_Open(SAVE_NAME, "w");               /* creates a fresh RAM variable */
  if (!h) return;
  ti_Write(buf, SAVE_BYTES, 1, h);
  ti_SetArchiveStatus(true, h);                       /* single archive write, only on level end / purchase */
  ti_Close(h);
}
SaveStatus platform_save_load(SaveData *s) {
  uint8_t buf[SAVE_BYTES]; uint8_t h = ti_Open(SAVE_NAME, "r");
  if (!h) { save_default(s); return SAVE_BAD_LEN; }
  size_t n = ti_Read(buf, 1, SAVE_BYTES, h);
  ti_Close(h);
  return save_load_or_default(s, buf, (int)n);
}
#else
void platform_save_write(const SaveData *s) { (void)s; }
SaveStatus platform_save_load(SaveData *s) { save_default(s); return SAVE_BAD_LEN; }
#endif
