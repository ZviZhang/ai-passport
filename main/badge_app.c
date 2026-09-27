// main/badge_app.c —— 工牌应用编排。
//
// 线程约定(见 AGENTS.md):按键回调只入队;LVGL 只在持锁时访问;音频写入
// 放在独立 worker 任务里,不占用按键回调与 LVGL 任务。
#include "badge_app.h"
#include "badge_model.h"
#include "badge_ui.h"
#include "bsp_audio.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"

#include <math.h>

static const char *TAG = "badge";

#define INPUT_QUEUE_DEPTH        8
#define IDLE_TICK_MS             1000
#define BATTERY_REFRESH_SECONDS  30
#define LVGL_LOCK_MS             500

#define AUDIO_SAMPLE_HZ          16000
#define AUDIO_CHUNK_SAMPLES      512
#define NOTE_MS                  130
// 0.67 满量程。实测音量 80 时 esp_codec_dev 走的默认曲线是 0%=-50dB → 100%=0dB
// 的线性映射,即 80 只有 -10dB;叠加 ES8311 的 hw_gain=-3.6dB 后更小。故把数字域
// 幅度从 0.21 提到 0.67(+9.9dB),峰值 22000 仍 < 32767,不会削波。
#define TONE_AMPLITUDE           22000.0f
#define TONE_FADE_MS             5
#define PI_F                     3.14159265f

// C5-E5-G5-C6 上行音阶。
static const float k_notes[4] = {523.25f, 659.25f, 783.99f, 1046.50f};

typedef struct {
    bsp_btn_t btn;
    bsp_btn_ev_t event;
} input_event_t;

static badge_model_t s_model;
static QueueHandle_t s_input_queue;
static TaskHandle_t s_input_task;
static TaskHandle_t s_audio_task;
static volatile bool s_input_ready;
static volatile bool s_audio_busy;

static void synth_arpeggio(void) {
    if (bsp_audio_set_format(AUDIO_SAMPLE_HZ, 16, 1) != ESP_OK) {
        ESP_LOGW(TAG, "音频格式设置失败,跳过音效");
        return;
    }
    bsp_audio_set_volume(100);

    int16_t buf[AUDIO_CHUNK_SAMPLES];
    const int total = AUDIO_SAMPLE_HZ * NOTE_MS / 1000;
    const int fade = AUDIO_SAMPLE_HZ * TONE_FADE_MS / 1000;

    for (size_t n = 0; n < sizeof(k_notes) / sizeof(k_notes[0]); n++) {
        for (int done = 0; done < total; done += AUDIO_CHUNK_SAMPLES) {
            int cnt = total - done;
            if (cnt > AUDIO_CHUNK_SAMPLES) cnt = AUDIO_CHUNK_SAMPLES;
            for (int i = 0; i < cnt; i++) {
                const int idx = done + i;
                float env = 1.0f;
                if (idx < fade) {
                    env = (float)idx / (float)fade;
                } else if (idx > total - fade) {
                    env = (float)(total - idx) / (float)fade;
                }
                const float phase = 2.0f * PI_F * k_notes[n] * (float)idx / (float)AUDIO_SAMPLE_HZ;
                buf[i] = (int16_t)(sinf(phase) * env * TONE_AMPLITUDE);
            }
            if (bsp_audio_write(buf, (size_t)cnt * sizeof(int16_t)) != ESP_OK) {
                ESP_LOGW(TAG, "音频写入失败,跳过后段音阶");
                return;
            }
        }
    }
}

static void audio_worker(void *arg) {
    (void)arg;
    for (;;) {
        uint32_t command = 0;
        if (xTaskNotifyWait(0, UINT32_MAX, &command, portMAX_DELAY) != pdTRUE) continue;
        if (command == 0) continue;
        synth_arpeggio();
        s_audio_busy = false;
    }
}

static void play_arpeggio(void) {
    if (!s_audio_task || s_audio_busy) return;
    s_audio_busy = true;
    xTaskNotify(s_audio_task, 1, eSetValueWithOverwrite);
}

static badge_input_t normalize(bsp_btn_t btn, bsp_btn_ev_t event) {
    if (event == BSP_BTN_LONG) {
        return btn == BSP_BTN_OK ? BADGE_INPUT_OK_LONG : BADGE_INPUT_OTHER;
    }
    if (event != BSP_BTN_CLICK) return BADGE_INPUT_OTHER;
    switch (btn) {
        case BSP_BTN_UP: return BADGE_INPUT_UP_CLICK;
        case BSP_BTN_DOWN: return BADGE_INPUT_DOWN_CLICK;
        case BSP_BTN_OK: return BADGE_INPUT_OK_CLICK;
        default: return BADGE_INPUT_OTHER;
    }
}

// 现场调试用:把 button 组件报上来的原始事件与最终动作打进串口,便于区分
// "短按被判成长按" 与 "脉冲动画不可见" 这两类现象。
static const char *btn_name(bsp_btn_t btn) {
    switch (btn) {
        case BSP_BTN_UP: return "UP";
        case BSP_BTN_DOWN: return "DOWN";
        case BSP_BTN_OK: return "OK";
        default: return "?";
    }
}

static const char *ev_name(bsp_btn_ev_t ev) {
    switch (ev) {
        case BSP_BTN_PRESS: return "PRESS";
        case BSP_BTN_CLICK: return "CLICK";
        case BSP_BTN_DOUBLE: return "DOUBLE";
        case BSP_BTN_LONG: return "LONG";
        default: return "?";
    }
}

static void refresh_battery(void) {
    const int soc = bsp_battery_soc();
    if (bsp_lvgl_lock(LVGL_LOCK_MS)) {
        badge_ui_refresh_battery(soc);
        bsp_lvgl_unlock();
    }
}

static void input_task(void *arg) {
    (void)arg;
    uint32_t since_battery = BATTERY_REFRESH_SECONDS;
    input_event_t ev;

    for (;;) {
        if (xQueueReceive(s_input_queue, &ev, pdMS_TO_TICKS(IDLE_TICK_MS)) == pdTRUE) {
            const badge_input_t input = normalize(ev.btn, ev.event);
            const badge_action_t action = badge_model_handle_input(&s_model, input);
            ESP_LOGI(TAG, "按键 %s/%s → input=%d action=%d page=P%d", btn_name(ev.btn),
                     ev_name(ev.event), (int)input, (int)action, (int)s_model.page + 1);
            switch (action) {
                case BADGE_ACTION_WAKE:
                    bsp_display_backlight(100);
                    break;
                case BADGE_ACTION_PAGE_CHANGED:
                    if (bsp_lvgl_lock(LVGL_LOCK_MS)) {
                        badge_ui_show(s_model.page);
                        bsp_lvgl_unlock();
                    }
                    break;
                case BADGE_ACTION_PULSE_MEDAL:
                    if (bsp_lvgl_lock(LVGL_LOCK_MS)) {
                        badge_ui_pulse_medal();
                        bsp_lvgl_unlock();
                    }
                    play_arpeggio();
                    break;
                default:
                    break;
            }
            since_battery = 0;
            continue;
        }

        if (badge_model_tick(&s_model, IDLE_TICK_MS / 1000)) {
            bsp_display_backlight(0);
            ESP_LOGI(TAG, "无操作 %u 秒,背光已熄", (unsigned)BADGE_SCREEN_OFF_SECONDS);
        }

        if (!s_model.screen_off && ++since_battery >= BATTERY_REFRESH_SECONDS) {
            since_battery = 0;
            refresh_battery();
        }
    }
}

// 按键回调跑在共享 esp_timer 任务:只入队,立刻返回。
static void on_key(bsp_btn_t btn, bsp_btn_ev_t ev, void *user) {
    (void)user;
    if (!s_input_ready || !s_input_queue) return;
    const input_event_t e = {.btn = btn, .event = ev};
    (void)xQueueSend(s_input_queue, &e, 0);
}

void badge_app_start(void) {
    badge_model_init(&s_model);

    s_input_queue = xQueueCreate(INPUT_QUEUE_DEPTH, sizeof(input_event_t));
    if (!s_input_queue) {
        ESP_LOGE(TAG, "输入队列创建失败");
        return;
    }
    if (xTaskCreate(input_task, "badge_input", 4096, NULL, 5, &s_input_task) != pdPASS) {
        ESP_LOGE(TAG, "输入任务创建失败");
        vQueueDelete(s_input_queue);
        s_input_queue = NULL;
        return;
    }
    if (bsp_button_init(on_key, NULL) != ESP_OK) {
        ESP_LOGE(TAG, "按键初始化失败,无法翻页");
    }

    if (bsp_audio_init() == ESP_OK) {
        if (xTaskCreate(audio_worker, "badge_audio", 4096, NULL, 4, &s_audio_task) != pdPASS) {
            s_audio_task = NULL;
            ESP_LOGW(TAG, "音频任务创建失败,静默运行");
        }
    } else {
        ESP_LOGW(TAG, "音频初始化失败,静默运行");
    }
    if (bsp_battery_init() != ESP_OK) {
        ESP_LOGW(TAG, "电量计初始化失败,只画电池图标");
    }

    if (bsp_lvgl_lock(1000)) {
        badge_ui_init();
        badge_ui_refresh_battery(bsp_battery_soc());
        bsp_lvgl_unlock();
        s_input_ready = true;
    } else {
        ESP_LOGE(TAG, "LVGL 加锁失败,界面未建立");
    }

    ESP_LOGI(TAG, "工牌应用已启动");
}