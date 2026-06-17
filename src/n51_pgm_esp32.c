#include "n51_pgm.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ICP_CLK_GPIO  4
#define ICP_DAT_GPIO  5
#define ICP_RST_GPIO  6

static uint8_t initialized = 0;

static void icp_set_pullups(uint8_t enable)
{
    if (enable) {
        gpio_pullup_en(ICP_DAT_GPIO);
        gpio_pullup_en(ICP_CLK_GPIO);
        gpio_pullup_en(ICP_RST_GPIO);
    } else {
        gpio_pullup_dis(ICP_DAT_GPIO);
        gpio_pullup_dis(ICP_CLK_GPIO);
        gpio_pullup_dis(ICP_RST_GPIO);
    }
    gpio_pulldown_dis(ICP_DAT_GPIO);
    gpio_pulldown_dis(ICP_CLK_GPIO);
    gpio_pulldown_dis(ICP_RST_GPIO);
}

int N51PGM_init(void)
{
    gpio_reset_pin(ICP_CLK_GPIO);
    gpio_reset_pin(ICP_DAT_GPIO);
    gpio_reset_pin(ICP_RST_GPIO);

    icp_set_pullups(1);

    gpio_set_direction(ICP_CLK_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(ICP_RST_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_direction(ICP_DAT_GPIO, GPIO_MODE_INPUT);
    gpio_set_level(ICP_CLK_GPIO, 0);
    gpio_set_level(ICP_RST_GPIO, 0);
    initialized = 1;
    return 0;
}

uint8_t N51PGM_is_init(void)
{
    return initialized;
}

void N51PGM_set_dat(uint8_t val)
{
    gpio_set_level(ICP_DAT_GPIO, val);
}

uint8_t N51PGM_get_dat(void)
{
    return gpio_get_level(ICP_DAT_GPIO);
}

void N51PGM_set_rst(uint8_t val)
{
    gpio_set_level(ICP_RST_GPIO, val);
}

void N51PGM_set_clk(uint8_t val)
{
    gpio_set_level(ICP_CLK_GPIO, val);
}

void N51PGM_dat_dir(uint8_t state)
{
    gpio_set_direction(ICP_DAT_GPIO, state ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT);
}

void N51PGM_set_trigger(uint8_t val)
{
}

void N51PGM_deinit(uint8_t leave_reset_high)
{
    gpio_set_direction(ICP_CLK_GPIO, GPIO_MODE_INPUT);
    gpio_set_direction(ICP_DAT_GPIO, GPIO_MODE_INPUT);
    if (leave_reset_high) {
        N51PGM_set_rst(1);
    } else {
        gpio_set_direction(ICP_RST_GPIO, GPIO_MODE_INPUT);
    }
    initialized = 0;
}

uint32_t N51PGM_usleep(uint32_t usec)
{
    if (usec == 0) {
        return 0;
    }
    if (usec < 1000) {
        // Timing-critical short delays: precise busy-wait.
        esp_rom_delay_us(usec);
    } else {
        // Long delays: yield to feed the task watchdog, then busy-wait the
        // sub-millisecond remainder for accuracy.
        uint32_t msec = usec / 1000;
        uint32_t rem = usec % 1000;
        vTaskDelay(pdMS_TO_TICKS(msec));
        if (rem) {
            esp_rom_delay_us(rem);
        }
    }
    return usec;
}

uint64_t N51PGM_get_time(void)
{
    return esp_timer_get_time();
}

void N51PGM_print(const char *msg)
{
    printf("%s", msg);
}
