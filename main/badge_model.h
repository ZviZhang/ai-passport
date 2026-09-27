// main/badge_model.h —— 赠礼工牌的纯逻辑状态机(页面轮转 + 息屏判定)。
//
// 本文件不依赖 LVGL 与 ESP-IDF,便于在主机上直接单元测试。
#pragma once

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// 无操作多久后熄背光(秒)。MCU 不休眠,保持按键即时响应。
#define BADGE_SCREEN_OFF_SECONDS 180u

typedef enum {
    BADGE_PAGE_ID = 0,     // P1 工作证
    BADGE_PAGE_HONOR,      // P2 荣誉页(银牌)
    BADGE_PAGE_DEDICATION, // P3 中文赠言
    BADGE_PAGE_COUNT,
} badge_page_t;

// 归一化后的按键输入;映射由 badge_app 完成,模型只认这几种。
typedef enum {
    BADGE_INPUT_NONE = 0,
    BADGE_INPUT_UP_CLICK,
    BADGE_INPUT_DOWN_CLICK,
    BADGE_INPUT_OK_CLICK,
    BADGE_INPUT_OK_LONG,
    BADGE_INPUT_OTHER,
} badge_input_t;

// 模型要求应用执行的动作。UI 副作用一律由调用方完成。
typedef enum {
    BADGE_ACTION_NONE = 0,
    BADGE_ACTION_WAKE,         // 背光原本已熄,本次按键只负责点亮,不再翻页
    BADGE_ACTION_PAGE_CHANGED, // 当前页已改变,按 model->page 重新送显
    BADGE_ACTION_PULSE_MEDAL,  // 荣誉页上短按 OK:银牌闪光 + 音阶
} badge_action_t;

typedef struct {
    badge_page_t page;
    bool screen_off;
    uint32_t idle_seconds; // 连续无操作秒数
} badge_model_t;

void badge_model_init(badge_model_t *model);

// 处理一次按键。返回应用需要执行的动作。
badge_action_t badge_model_handle_input(badge_model_t *model, badge_input_t input);

// 推进时间。返回 true 表示"本次 tick 刚刚熄屏",应用应立即关背光。
bool badge_model_tick(badge_model_t *model, uint32_t elapsed_seconds);

#ifdef __cplusplus
}
#endif