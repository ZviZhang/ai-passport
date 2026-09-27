// main/badge_model.c —— 见 badge_model.h。
#include "badge_model.h"

static badge_page_t next_page(badge_page_t page) {
    return (badge_page_t)(((int)page + 1) % (int)BADGE_PAGE_COUNT);
}

void badge_model_init(badge_model_t *model) {
    if (!model) return;
    model->page = BADGE_PAGE_ID;
    model->screen_off = false;
    model->idle_seconds = 0;
}

badge_action_t badge_model_handle_input(badge_model_t *model, badge_input_t input) {
    if (!model) return BADGE_ACTION_NONE;
    if (input == BADGE_INPUT_NONE) return BADGE_ACTION_NONE;

    // 任意按键先唤醒背光,且该次按键被"吃掉",不再触发翻页。
    if (model->screen_off) {
        model->screen_off = false;
        model->idle_seconds = 0;
        return BADGE_ACTION_WAKE;
    }

    model->idle_seconds = 0;

    switch (input) {
        // 规格 §4:UP / DOWN 短按都按 P1 → P2 → P3 → P1 顺序循环翻页。
        case BADGE_INPUT_UP_CLICK:
        case BADGE_INPUT_DOWN_CLICK: {
            badge_page_t target = next_page(model->page);
            if (target == model->page) return BADGE_ACTION_NONE;
            model->page = target;
            return BADGE_ACTION_PAGE_CHANGED;
        }
        // 规格 §4:OK 长按直达赠言页。
        case BADGE_INPUT_OK_LONG: {
            if (model->page == BADGE_PAGE_DEDICATION) return BADGE_ACTION_NONE;
            model->page = BADGE_PAGE_DEDICATION;
            return BADGE_ACTION_PAGE_CHANGED;
        }
        // 规格 §4:OK 短按仅在荣誉页触发银牌闪光。
        case BADGE_INPUT_OK_CLICK:
            if (model->page == BADGE_PAGE_HONOR) return BADGE_ACTION_PULSE_MEDAL;
            return BADGE_ACTION_NONE;
        default:
            return BADGE_ACTION_NONE;
    }
}

bool badge_model_tick(badge_model_t *model, uint32_t elapsed_seconds) {
    if (!model || model->screen_off) return false;
    if (elapsed_seconds == 0) return false;

    model->idle_seconds += elapsed_seconds;
    if (model->idle_seconds < BADGE_SCREEN_OFF_SECONDS) return false;

    model->idle_seconds = 0;
    model->screen_off = true;
    return true;
}