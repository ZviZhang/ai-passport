// main/badge_app.h —— 工牌应用编排(按键分发 / 开机动画 / 音效 / 息屏)。
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// 启动应用。必须在 bsp_i2c_init / bsp_display_init / bsp_lvgl_init 之后调用。
void badge_app_start(void);

#ifdef __cplusplus
}
#endif