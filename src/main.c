#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "n51_icp.h"
#include "n51_pgm.h"

#define FLASH_SIZE 0x4000

static void print_hex_line(uint32_t addr, const uint8_t *data, int len)
{
    printf("%04lX  ", (unsigned long)addr);
    for (int i = 0; i < len; i++) {
        printf("%02X ", data[i]);
        if (i == 7) printf(" ");
    }
    printf("\n");
}

static void dump_flash(void)
{
    uint8_t buf[16];
    uint32_t addr = 0;

    printf("\n--- FLASH DUMP ---\n\n");

    while (addr < FLASH_SIZE) {
        N51ICP_read_flash(addr, 16, buf);
        print_hex_line(addr, buf, 16);
        addr += 16;
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    printf("\n--- END FLASH DUMP ---\n");
}

void app_main(void)
{
    vTaskDelay(pdMS_TO_TICKS(2000));
    printf("\n\n=== Nuvoton MS51 ICP Flash Dumper (READ ONLY) ===\n");

    printf("Using ESP32-C6 on ICP pins: CLK=GPIO4, DAT=GPIO5, RST=GPIO6\n");
    printf("WARNING: This firmware contains NO write/erase commands.\n");
    printf("The target MCU cannot be modified by this dumper.\n\n");

    if (N51ICP_init() != 0) {
        printf("Failed to init ICP interface\n");
        return;
    }

    printf("Entering ICP mode...\n");
    uint32_t devid = N51ICP_enter_icp_mode(1);

    if (devid == 0 || devid == 0xFFFF) {
        printf("No device detected (devid=0x%04lX)\n", (unsigned long)devid);
        printf("Check wiring: V(3.3V) -> 3.3V, G -> GND, R -> GPIO6, D -> GPIO5, C -> GPIO4\n");
        N51ICP_deinit(0);
        return;
    }

    printf("Device ID:  0x%04lX\n", (unsigned long)devid);

    uint32_t pid = N51ICP_read_pid();
    printf("Product ID: 0x%04lX\n", (unsigned long)pid);

    uint8_t cid = N51ICP_read_cid();
    printf("Customer ID: 0x%02X\n", cid);

    uint8_t uid[12];
    N51ICP_read_uid(uid);
    printf("UID: ");
    for (int i = 0; i < 12; i++) printf("%02X ", uid[i]);
    printf("\n");

    uint8_t ucid[16];
    N51ICP_read_ucid(ucid);
    printf("UCID: ");
    for (int i = 0; i < 16; i++) printf("%02X ", ucid[i]);
    printf("\n");

    dump_flash();

    printf("Flash dump complete! Copy the hex output above and convert with:\n");
    printf("  grep -E '^[0-9A-F]{4}  ' capture.log | sed 's/^....  //' | sed 's/ //g' > dump.hex\n");
    printf("  xxd -r -p dump.hex firmware.bin\n\n");

    N51ICP_exit_icp_mode();
    N51ICP_deinit(0);

    printf("Done. Device reset and released.\n");
}
