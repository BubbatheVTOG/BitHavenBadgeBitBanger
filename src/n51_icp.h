#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int N51ICP_init(void);
void N51ICP_deinit(uint8_t leave_reset_high);
uint32_t N51ICP_read_device_id(void);
uint32_t N51ICP_read_pid(void);
uint8_t N51ICP_read_cid(void);
void N51ICP_read_uid(uint8_t *buf);
void N51ICP_read_ucid(uint8_t *buf);
uint32_t N51ICP_read_flash(uint32_t addr, uint32_t len, uint8_t *data);
uint32_t N51ICP_enter_icp_mode(uint8_t do_reset);
void N51ICP_exit_icp_mode(void);
void N51ICP_reentry(uint32_t delay1, uint32_t delay2, uint32_t delay3);
void N51ICP_send_entry_bits(void);
void N51ICP_send_exit_bits(void);
void N51ICP_outputf(const char *fmt, ...);

#ifdef __cplusplus
}
#endif
