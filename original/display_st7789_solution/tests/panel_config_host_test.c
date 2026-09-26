#include <stdio.h>

#include "p169h002_st7789_profile.h"
#include "platform_st7789_panel.h"

int main(void)
{
    platform_st7789_t display = PLATFORM_ST7789_INITIALIZER;

    if (platform_st7789_apply_panel_config(
            &display,
            &g_p169h002St7789PanelConfig) != PLATFORM_ERR_OK) {
        return 1;
    }

    if ((display.width != 240U) ||
        (display.height != 280U) ||
        (display.xOffset != 0U) ||
        (display.yOffset != 20U) ||
        (display.spiConfig.mode != PLATFORM_SPI_MODE_3) ||
        (display.spiConfig.dataBits != 8U) ||
        (display.spiConfig.maxClockHz != 12500000U)) {
        return 1;
    }

    (void)printf("ST7789 panel config host test passed.\n");
    return 0;
}
