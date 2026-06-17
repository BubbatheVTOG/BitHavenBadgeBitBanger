#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int N51PGM_init(void);
void N51PGM_deinit(uint8_t leave_reset_high);
uint8_t N51PGM_is_init();
void N51PGM_set_dat(uint8_t val);
uint8_t N51PGM_get_dat(void);
void N51PGM_set_rst(uint8_t val);
void N51PGM_set_clk(uint8_t val);
void N51PGM_set_trigger(uint8_t val);
void N51PGM_dat_dir(uint8_t state);
uint32_t N51PGM_usleep(uint32_t usec);
uint64_t N51PGM_get_time(void);
void N51PGM_print(const char *msg);

#ifdef __cplusplus
}
#endif
