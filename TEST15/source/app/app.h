/* SPDX-License-Identifier: MPL-2.0 */
#pragma once
#include <stdint.h>
#include "plat.h"
#define APP_NAME "BASS STATION"
#define APP_SPLASH_RGB RGB(42, 126, 220)
#define APP_BLOCK 256u
#define APP_TOUCH
#define APP_SYSEX
#define APP_BLE_AT_BOOT 0

enum { OBJ_SETTINGS, OBJ_PANEL, OBJ_BANK0, OBJ_BANK1, OBJ_BANK2, OBJ_BANK3, OBJ_BANK4, OBJ_BANK5, OBJ_BANK6, OBJ_NOBJ };
/* Each object owns an A/B 4 KiB sector pair. 0xE0000..0xFBFFF stays reserved for update staging. */
#define APP_STORE_MAP {0xFC000u, 0xDC000u, 0xD2000u, 0xD4000u, 0xD6000u, 0xD8000u, 0xDA000u, 0xCE000u, 0xD0000u}

void app_init(void);
void app_render(int32_t *lr, uint32_t n, uint32_t master_q12);
void app_panic(void);
void app_ui_init(void);
void app_frame(void);
void app_input_only(void);
void app_say(const char *a, const char *b);
int app_sysex(const uint8_t *p,uint32_t n);
#ifdef PLAT_HOST
#define APP_HOST_EXPECT
#define APP_HOST_REBOOT
#define APP_HOST_CMD
int app_expect(const char *what, int *got);
void app_host_reboot(void);
int app_host_cmd(const char *cmd, const char *a, const char *b, const char *c, const char *d);
#endif
