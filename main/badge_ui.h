// main/badge_ui.h —— 三个页面的 LVGL 构建与更新(自建 UI,不复用 demo 外壳)。
//
// 所有函数都只能在持有 bsp_lvgl_lock() 时调用。
#pragma once

#include "lvgl.h"
#include "badge_model.h"

#ifdef __cplusplus
extern "C" {
#endif

// 创建全部页面(含开机动画页)。开机动画结束时自动切到 P1。
void badge_ui_init(void);

// 切到指定页面。
void badge_ui_show(badge_page_t page);

// 刷新 P1/P2 右上角电量;soc_percent < 0 时只画电池图标不画数字。
void badge_ui_refresh_battery(int soc_percent);

// P2 银牌亮度脉冲闪 3 次。
void badge_ui_pulse_medal(void);

#ifdef __cplusplus
}
#endif