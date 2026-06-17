#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include "n51_icp.h"
#include "n51_pgm.h"

#define DEFAULT_BIT_DELAY      2
#define ENTRY_BIT_DELAY 60

#define ICP_CMD_READ_UID        0x04
#define ICP_CMD_READ_CID        0x0b
#define ICP_CMD_READ_DEVICE_ID  0x0c
#define ICP_CMD_READ_FLASH      0x00

#define ENTRY_BITS    0x5aa503
#define ICP_RESET_SEQ 0x9e1cb6
#define ALT_RESET_SEQ 0xAE1CB6
#define ALT_ENTRY_BITS 0x0075BA03
#define EXIT_BITS     0xF78F0

#define USLEEP(x) if (x > 0) N51PGM_usleep(x)

static void N51ICP_bitsend(uint32_t data, int len, uint32_t udelay)
{
    N51PGM_dat_dir(1);
    int i = len;
    while (i--) {
        N51PGM_set_dat((data >> i) & 1);
        USLEEP(udelay);
        N51PGM_set_clk(1);
        USLEEP(udelay);
        N51PGM_set_clk(0);
    }
}

static void N51ICP_send_command(uint8_t cmd, uint32_t dat)
{
    N51ICP_bitsend((dat << 6) | cmd, 24, DEFAULT_BIT_DELAY);
}

static int send_reset_seq(uint32_t reset_seq, int len)
{
    for (int i = 0; i < len + 1; i++) {
        N51PGM_set_rst((reset_seq >> (len - i)) & 1);
        USLEEP(10000);
    }
    return 0;
}

void N51ICP_send_entry_bits(void)
{
    N51ICP_bitsend(ENTRY_BITS, 24, ENTRY_BIT_DELAY);
}

void N51ICP_send_exit_bits(void)
{
    N51ICP_bitsend(EXIT_BITS, 24, ENTRY_BIT_DELAY);
}

int N51ICP_init(void)
{
    int rc;
    if (!N51PGM_is_init()) {
        rc = N51PGM_init();
        if (rc < 0) {
            return rc;
        } else if (rc != 0) {
            return -1;
        }
    }
    return 0;
}

static uint8_t N51ICP_read_byte(int end)
{
    N51PGM_dat_dir(0);
    USLEEP(DEFAULT_BIT_DELAY);
    uint8_t data = 0;
    int i = 8;

    while (i--) {
        USLEEP(DEFAULT_BIT_DELAY);
        int state = N51PGM_get_dat();
        N51PGM_set_clk(1);
        USLEEP(DEFAULT_BIT_DELAY);
        N51PGM_set_clk(0);
        data |= (state << i);
    }

    N51PGM_dat_dir(1);
    USLEEP(DEFAULT_BIT_DELAY);
    N51PGM_set_dat(end);
    USLEEP(DEFAULT_BIT_DELAY);
    N51PGM_set_clk(1);
    USLEEP(DEFAULT_BIT_DELAY);
    N51PGM_set_clk(0);
    USLEEP(DEFAULT_BIT_DELAY);
    N51PGM_set_dat(0);

    return data;
}

uint32_t N51ICP_read_device_id(void)
{
    N51ICP_send_command(ICP_CMD_READ_DEVICE_ID, 0);

    uint8_t devid[2];
    devid[0] = N51ICP_read_byte(0);
    devid[1] = N51ICP_read_byte(1);

    return (devid[1] << 8) | devid[0];
}

uint32_t N51ICP_read_pid(void)
{
    N51ICP_send_command(ICP_CMD_READ_DEVICE_ID, 2);
    uint8_t pid[2];
    pid[0] = N51ICP_read_byte(0);
    pid[1] = N51ICP_read_byte(1);
    return (pid[1] << 8) | pid[0];
}

uint8_t N51ICP_read_cid(void)
{
    N51ICP_send_command(ICP_CMD_READ_CID, 0);
    return N51ICP_read_byte(1);
}

void N51ICP_read_uid(uint8_t *buf)
{
    for (uint8_t i = 0; i < 12; i++) {
        N51ICP_send_command(ICP_CMD_READ_UID, i);
        buf[i] = N51ICP_read_byte(1);
    }
}

void N51ICP_read_ucid(uint8_t *buf)
{
    for (uint8_t i = 0; i < 16; i++) {
        N51ICP_send_command(ICP_CMD_READ_UID, i + 0x20);
        buf[i] = N51ICP_read_byte(1);
    }
}

uint32_t N51ICP_read_flash(uint32_t addr, uint32_t len, uint8_t *data)
{
    if (len == 0) {
        return 0;
    }
    N51ICP_send_command(ICP_CMD_READ_FLASH, addr);

    for (uint32_t i = 0; i < len; i++) {
        data[i] = N51ICP_read_byte(i == (len - 1));
    }
    return addr + len;
}

uint32_t N51ICP_enter_icp_mode(uint8_t do_reset)
{
    uint32_t devid = 0;

    for (int attempt = 0; attempt < 3; attempt++) {
        if (do_reset) {
            if (attempt == 2) {
                send_reset_seq(ALT_RESET_SEQ, 24);
            } else {
                send_reset_seq(ICP_RESET_SEQ, 24);
            }
            N51PGM_set_rst(0);
        } else {
            N51PGM_set_rst(1);
            USLEEP(5000);
            N51PGM_set_rst(0);
            USLEEP(1000);
        }

        USLEEP(100);
        if (attempt == 2) {
            N51ICP_bitsend(ALT_ENTRY_BITS, 24, ENTRY_BIT_DELAY);
        } else {
            N51ICP_send_entry_bits();
        }
        USLEEP(10);

        devid = N51ICP_read_device_id();

        if (devid != 0 && devid != 0xFFFF) {
            return devid;
        }

        N51PGM_set_rst(1);
        USLEEP(50000);
    }

    return devid;
}

void N51ICP_reentry(uint32_t delay1, uint32_t delay2, uint32_t delay3)
{
    USLEEP(10);
    if (delay1 > 0) {
        N51PGM_set_rst(1);
        USLEEP(delay1);
    }
    N51PGM_set_rst(0);
    USLEEP(delay2);
    N51ICP_send_entry_bits();
    USLEEP(delay3);
}

void N51ICP_exit_icp_mode(void)
{
    N51PGM_set_rst(1);
    USLEEP(5000);
    N51PGM_set_rst(0);
    USLEEP(10000);
    N51ICP_send_exit_bits();
    USLEEP(500);
    N51PGM_set_rst(1);
}

void N51ICP_deinit(uint8_t leave_reset_high)
{
    N51PGM_deinit(leave_reset_high);
}

void N51ICP_outputf(const char *s, ...)
{
    char buf[160];
    va_list ap;
    va_start(ap, s);
    vsnprintf(buf, 160, s, ap);
    va_end(ap);
    N51PGM_print(buf);
}
