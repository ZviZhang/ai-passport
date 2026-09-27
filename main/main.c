// main/main.c —— 工牌应用固件入口。
//
// 这是 FoloToy AI Passport 的派生应用:BSP 管硬件,界面与业务逻辑全部自建
// (badge_ui.c / badge_app.c / badge_model.c),不复用仓库内的 demo 测试菜单。
// 原 demo_*.c / ui_pixel.c 仍参与编译,但不进入调用路径。
#include "bsp_i2c.h"
#include "bsp_display.h"
#include "bsp_pins.h"   // 失败日志里要打印 BSP_LCD_* 引脚号
#include "badge_app.h"
#include "esp_log.h"

static const char *TAG = "main";

void app_main(void) {
    ESP_LOGI(TAG, "工牌应用启动");

    bsp_i2c_init();

    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE(TAG, "显示/LVGL 初始化失败,无法继续。"
                      "检查 SPI 接线(MOSI=%d SCLK=%d CS=%d DC=%d BL=%d)",
                 BSP_LCD_MOSI, BSP_LCD_SCLK, BSP_LCD_CS, BSP_LCD_DC, BSP_LCD_BL);
        return;
    }
    bsp_display_backlight(100);

    badge_app_start();
}