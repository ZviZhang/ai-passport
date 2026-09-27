// tests/test_badge_model.c —— badge_model 的主机单元测试(翻页 / 息屏 / 唤醒)。
#include "badge_model.h"

#include <assert.h>
#include <stdio.h>

static void test_initial_state(void) {
    badge_model_t m;
    badge_model_init(&m);
    assert(m.page == BADGE_PAGE_ID);
    assert(!m.screen_off);
    assert(m.idle_seconds == 0);
}

// UP / DOWN 短按都按 P1 → P2 → P3 → P1 循环(规格 §4)。
static void test_cycle_forward(void) {
    badge_model_t m;
    badge_model_init(&m);

    assert(badge_model_handle_input(&m, BADGE_INPUT_DOWN_CLICK) == BADGE_ACTION_PAGE_CHANGED);
    assert(m.page == BADGE_PAGE_HONOR);

    assert(badge_model_handle_input(&m, BADGE_INPUT_DOWN_CLICK) == BADGE_ACTION_PAGE_CHANGED);
    assert(m.page == BADGE_PAGE_DEDICATION);

    assert(badge_model_handle_input(&m, BADGE_INPUT_UP_CLICK) == BADGE_ACTION_PAGE_CHANGED);
    assert(m.page == BADGE_PAGE_ID);
}

static void test_ok_long_jumps_to_dedication(void) {
    badge_model_t m;
    badge_model_init(&m);

    assert(badge_model_handle_input(&m, BADGE_INPUT_OK_LONG) == BADGE_ACTION_PAGE_CHANGED);
    assert(m.page == BADGE_PAGE_DEDICATION);

    // 已在赠言页时长按不再产生变化。
    assert(badge_model_handle_input(&m, BADGE_INPUT_OK_LONG) == BADGE_ACTION_NONE);
    assert(m.page == BADGE_PAGE_DEDICATION);
}

static void test_ok_click_only_pulses_on_honor(void) {
    badge_model_t m;
    badge_model_init(&m);

    assert(badge_model_handle_input(&m, BADGE_INPUT_OK_CLICK) == BADGE_ACTION_NONE);

    assert(badge_model_handle_input(&m, BADGE_INPUT_DOWN_CLICK) == BADGE_ACTION_PAGE_CHANGED);
    assert(m.page == BADGE_PAGE_HONOR);
    assert(badge_model_handle_input(&m, BADGE_INPUT_OK_CLICK) == BADGE_ACTION_PULSE_MEDAL);
}

static void test_screen_off_after_timeout(void) {
    badge_model_t m;
    badge_model_init(&m);

    assert(!badge_model_tick(&m, BADGE_SCREEN_OFF_SECONDS - 1));
    assert(!m.screen_off);

    assert(badge_model_tick(&m, 1));
    assert(m.screen_off);

    // 熄屏后继续 tick 不重复触发。
    assert(!badge_model_tick(&m, 10));
}

// 熄屏时的按键只唤醒背光,不翻页;唤醒后计时重新开始。
static void test_input_wakes_without_navigation(void) {
    badge_model_t m;
    badge_model_init(&m);

    assert(badge_model_tick(&m, BADGE_SCREEN_OFF_SECONDS));
    assert(m.screen_off);

    assert(badge_model_handle_input(&m, BADGE_INPUT_DOWN_CLICK) == BADGE_ACTION_WAKE);
    assert(m.page == BADGE_PAGE_ID);
    assert(!m.screen_off);
    assert(m.idle_seconds == 0);

    assert(!badge_model_tick(&m, BADGE_SCREEN_OFF_SECONDS - 1));
    assert(badge_model_tick(&m, 1));
}

static void test_activity_resets_idle(void) {
    badge_model_t m;
    badge_model_init(&m);

    assert(!badge_model_tick(&m, BADGE_SCREEN_OFF_SECONDS - 5));
    assert(badge_model_handle_input(&m, BADGE_INPUT_OK_CLICK) == BADGE_ACTION_NONE);
    assert(m.idle_seconds == 0);
    assert(!badge_model_tick(&m, BADGE_SCREEN_OFF_SECONDS - 1));
    assert(!m.screen_off);
}

static void test_other_input_is_ignored(void) {
    badge_model_t m;
    badge_model_init(&m);
    assert(badge_model_handle_input(&m, BADGE_INPUT_OTHER) == BADGE_ACTION_NONE);
    assert(badge_model_handle_input(&m, BADGE_INPUT_NONE) == BADGE_ACTION_NONE);
    assert(m.page == BADGE_PAGE_ID);
}

int main(void) {
    test_initial_state();
    test_cycle_forward();
    test_ok_long_jumps_to_dedication();
    test_ok_click_only_pulses_on_honor();
    test_screen_off_after_timeout();
    test_input_wakes_without_navigation();
    test_activity_resets_idle();
    test_other_input_is_ignored();
    printf("badge_model: all tests passed\n");
    return 0;
}