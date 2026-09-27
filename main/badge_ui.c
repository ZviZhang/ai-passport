// main/badge_ui.c —— 工牌应用的三页 UI 与开机动画(自建,不复用 demo 外壳)。
//
// 坐标全部取自实施规格 §3/§4;单位为设备像素。屏幕 240x320 竖屏,四角被 BSP
// 裁成半径 30 的黑色圆角,故左右留 ≥14px、上下留 ≥8px 安全区。
#include "badge_ui.h"
#include "badge_assets.h"

#include <math.h>
#include <stdio.h>

// --- 品牌色(规格 §1,取自 Rule/ 素材) ---
#define C_INK      0x00223A
#define C_RED      0xEB002A
#define C_ORANGE   0xEE7522
#define C_YELLOW   0xFFB81B
#define C_BLUE     0x0563A1
#define C_MAROON   0x681628
#define C_TEAL     0x3DB6B6
#define C_GOLD     0xD4C242
#define C_PAPER    0xFFFFFF
#define C_HAIRLINE 0xE6EAEE
#define C_MUTED    0x6B7785
#define C_FAINT    0xA8B2BD
#define C_CREAM    0xF5EFE0
#define C_SMT_BLUE 0x0096E0

#define SCREEN_W         240
#define SCREEN_H         320
#define BOOT_MS          1250
#define BOOT_BLOCK       8
#define BOOT_HEX_RADIUS  44
#define DEG2RAD          0.017453292f

static const uint32_t k_emblem_colors[6] = {C_RED, C_ORANGE, C_YELLOW, C_BLUE, C_INK, C_MAROON};

static lv_obj_t *s_page[BADGE_PAGE_COUNT];
static lv_obj_t *s_batt_label[BADGE_PAGE_COUNT];
static lv_obj_t *s_medal_img;

static lv_obj_t *s_boot_scr;
static lv_obj_t *s_boot_block[6];
static lv_obj_t *s_boot_emblem;

// ---------------------------------------------------------------------------
// 小工具
// ---------------------------------------------------------------------------

// 去掉主题默认样式与内边距的裸容器(背景透明)。
static lv_obj_t *obj_plain(lv_obj_t *parent) {
    lv_obj_t *o = lv_obj_create(parent);
    lv_obj_remove_style_all(o);
    lv_obj_remove_flag(o, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(o, 0, 0);
    return o;
}

static lv_obj_t *fill(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color, int radius) {
    lv_obj_t *o = obj_plain(parent);
    lv_obj_set_pos(o, x, y);
    lv_obj_set_size(o, w, h);
    lv_obj_set_style_radius(o, radius, 0);
    lv_obj_set_style_bg_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(o, LV_OPA_COVER, 0);
    return o;
}

static lv_obj_t *outline(lv_obj_t *parent, int x, int y, int w, int h, uint32_t color,
                         int radius, int width) {
    lv_obj_t *o = fill(parent, x, y, w, h, color, radius);
    lv_obj_set_style_bg_opa(o, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(o, width, 0);
    lv_obj_set_style_border_color(o, lv_color_hex(color), 0);
    lv_obj_set_style_border_opa(o, LV_OPA_COVER, 0);
    return o;
}

static lv_obj_t *text(lv_obj_t *parent, const char *str, const lv_font_t *font, uint32_t color) {
    lv_obj_t *l = lv_label_create(parent);
    lv_label_set_text(l, str);
    lv_obj_set_style_text_font(l, font, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    return l;
}

// 横向居中、纵向按设备像素定位(y = 文本顶边)。
static lv_obj_t *text_top_mid(lv_obj_t *parent, const char *str, const lv_font_t *font,
                              uint32_t color, int y) {
    lv_obj_t *l = text(parent, str, font, color);
    lv_obj_align(l, LV_ALIGN_TOP_MID, 0, y);
    return l;
}

static lv_obj_t *text_centered(lv_obj_t *parent, const char *str, const lv_font_t *font,
                               uint32_t color) {
    lv_obj_t *l = text(parent, str, font, color);
    lv_obj_center(l);
    return l;
}

static lv_obj_t *screen_new(uint32_t bg) {
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_style_bg_color(scr, lv_color_hex(bg), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    return scr;
}

// 右上角电量:空心电池图标 + 数字(<0 时只留图标)。
static void battery_create(lv_obj_t *scr, badge_page_t page) {
    outline(scr, 200, 13, 20, 11, C_INK, 2, 1);
    fill(scr, 220, 16, 3, 5, C_INK, 0);

    s_batt_label[page] = text(scr, "", &badge_sans_11, C_MUTED);
    lv_obj_set_width(s_batt_label[page], 34);
    lv_obj_set_style_text_align(s_batt_label[page], LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_set_pos(s_batt_label[page], 160, 13);
}

// 固定图案的像素条码,保证每次开机一致。
static void barcode_create(lv_obj_t *scr, int x, int y, int w, int h) {
    static const uint8_t pattern[] = {3, 1, 2, 1, 1, 3, 2, 1, 2, 3, 1, 2, 1, 1, 3, 1};
    const int n = (int)(sizeof(pattern) / sizeof(pattern[0]));
    for (int i = 0, cx = x; cx + 2 <= x + w; i++) {
        int bw = pattern[i % n];
        if (cx + bw > x + w) bw = x + w - cx;
        fill(scr, cx, y, bw, h, C_INK, 0);
        cx += bw + 1;
    }
}

// ---------------------------------------------------------------------------
// P1 工作证
// ---------------------------------------------------------------------------
static void build_page_id(void) {
    lv_obj_t *scr = screen_new(C_PAPER);
    s_page[BADGE_PAGE_ID] = scr;

    battery_create(scr, BADGE_PAGE_ID);

    lv_obj_t *logo = lv_image_create(scr);
#if BADGE_HAVE_SMT_LOGO
    lv_image_set_src(logo, &wss_logo_red);
    lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 8);

    // "Presented by" + SMT logo 作为一组居中(规格 §4:整组居中,y=70..88)。
    // 用 flex 行容器自动排布,避免按固定偏移摆放时文字与 logo 宽度变化导致重叠。
    lv_obj_t *by_row = obj_plain(scr);
    lv_obj_set_size(by_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_flex_flow(by_row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(by_row, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(by_row, 6, 0);
    lv_obj_align(by_row, LV_ALIGN_TOP_MID, 0, 70);
    text(by_row, "Presented by", &badge_sans_11, C_MUTED);
    lv_obj_t *smt = lv_image_create(by_row);
    lv_image_set_src(smt, &smt_logo);
#else
    // 降级方案(规格 §4):SMT logo 未到位,顶部只放放大的 WSS2026 logo,
    // 'Presented by' 行留空。
    lv_image_set_src(logo, &wss_logo_red_lg);
    lv_obj_align(logo, LV_ALIGN_TOP_MID, 0, 8);
#endif

    fill(scr, 0, 94, SCREEN_W, 1, C_HAIRLINE, 0);

    // 头像:84x84,圆角 8,1px 描边;由容器裁掉四角。
    lv_obj_t *frame = outline(scr, 15, 101, 86, 86, C_HAIRLINE, 9, 1);
    lv_obj_set_style_bg_color(frame, lv_color_hex(C_PAPER), 0);
    lv_obj_set_style_bg_opa(frame, LV_OPA_COVER, 0);
    lv_obj_set_style_clip_corner(frame, true, 0);
    lv_obj_t *photo = lv_image_create(frame);
    lv_image_set_src(photo, &portrait_sample);
    lv_obj_set_pos(photo, 0, 0);

    lv_obj_t *crew = fill(scr, 110, 102, 114, 32, C_INK, 6);
    text_centered(crew, "CREW", &badge_sans_18, C_PAPER);

    barcode_create(scr, 110, 142, 114, 20);

    text_top_mid(scr, "ALEX MORGAN", &badge_sans_bold_22, C_INK, 194);

    static const char *const labels[3] = {"Role", "Unit", "Nat."};
    static const char *const values[3] = {"Software Engineer", "WorldSkills Secretariat", "Indonesia"};
    for (int i = 0; i < 3; i++) {
        int y = 222 + i * 26;
        lv_obj_t *cap = fill(scr, 16, y, 48, 20, C_TEAL, 10);
        text_centered(cap, labels[i], &badge_sans_11, C_PAPER);
        lv_obj_t *val = text(scr, values[i], &badge_sans_14, C_INK);
        lv_obj_set_pos(val, 72, y + 3);
    }

    fill(scr, 0, 300, SCREEN_W, 1, C_HAIRLINE, 0);
    text_top_mid(scr, "SMT · WorldSkills Shanghai 2026", &badge_sans_11, C_MUTED, 304);
}

// ---------------------------------------------------------------------------
// P2 荣誉页
// ---------------------------------------------------------------------------
static void build_page_honor(void) {
    lv_obj_t *scr = screen_new(C_PAPER);
    s_page[BADGE_PAGE_HONOR] = scr;

    battery_create(scr, BADGE_PAGE_HONOR);

    // 该标题在 badge_sans_11 下宽 145px,居中会横跨 x=48..192,与右上角电量数字的
    // 文本框(x=160..194)在同一 y 波段重叠;屏幕高度也容不下"标题与奖牌上下错开"。
    // 故与电量组成常规页眉:标题左对齐、电量居右。
    lv_obj_t *title = text(scr, "WORLDSKILLS COMPETITION", &badge_sans_11, C_MUTED);
    lv_obj_set_pos(title, 14, 16);

    s_medal_img = lv_image_create(scr);
    lv_image_set_src(s_medal_img, &medal_silver);
    lv_obj_set_pos(s_medal_img, (SCREEN_W - 120) / 2, 28);
    lv_obj_set_style_image_recolor_opa(s_medal_img, LV_OPA_TRANSP, 0);

    text_top_mid(scr, "SILVER MEDALLIST", &badge_sans_18, C_INK, 154);
    text_top_mid(scr, "Kazan 2019", &badge_sans_14, C_MUTED, 178);

    fill(scr, (SCREEN_W - 120) / 2, 200, 120, 1, C_HAIRLINE, 0);

    lv_obj_t *skill = text(scr, "IT Software Solutions\nfor Business", &badge_sans_14, C_INK);
    lv_obj_set_width(skill, 212);
    lv_obj_set_style_text_align(skill, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(skill, LV_ALIGN_TOP_MID, 0, 206);

    lv_obj_t *cap = fill(scr, 36, 250, 168, 20, C_TEAL, 10);
    text_centered(cap, "Best of Nation · Indonesia", &badge_sans_11, C_PAPER);

    // 规格中的中文提示会超出 badge_sans_* 的字形子集,改用等义英文。
    text_top_mid(scr, "Hold OK for dedication", &badge_sans_11, C_FAINT, 290);
}

// ---------------------------------------------------------------------------
// P3 赠言页
// ---------------------------------------------------------------------------

// 赠言页的文字骨架按品牌规范走:拉丁用 Frutiger(商用,不随仓库分发),汉字用
// 思源黑体 Heavy。两种字体在生成阶段就用 lv_font_conv 的 --lv-fallback 串好了
// (见 tools/make_badge_fonts.py),所以这里直接引用 badge_frutiger_* 即可,不需要
// 在运行时改字体描述符。中英混排的字符串("致 朋友"、落款)里,Frutiger 没有的
// 汉字会经 fallback 落到思源黑体,两者的 line_height/base_line 由生成脚本统一,
// 保证落在同一基线上。

static void build_page_dedication(void) {
    lv_obj_t *scr = screen_new(C_INK);
    s_page[BADGE_PAGE_DEDICATION] = scr;

    // 背景图案:素材是横构图的无缝几何纹样,已按屏幅 240x320 原尺寸裁切(见
    // tools/make_badge_assets.py),故此处不缩放、不平铺,正好铺满整屏。
    lv_obj_set_style_bg_image_src(scr, &bg_page3, 0);

    lv_obj_t *col = obj_plain(scr);
    lv_obj_set_size(col, 220, 200);
    lv_obj_center(col);
    lv_obj_set_flex_flow(col, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(col, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(col, 14, 0);

    // 文字底板:obj_plain() 已 remove_style_all(背景透明),而背景纹样是细密线条,
    // 金色/月白文本直接压在上面时笔画与纹样混在一起、可读性差。加一块半透明黑底
    // 把文字区与纹样隔开,仍能透出图案。不透明度取 60%:再低压不住亮色纹样,
    // 再高就看不出背景了。
    // 底板宽 220、pad 6,内容宽 208px。最宽的一行是落款首行(13px 拉丁 + 11px
    // 思源黑体逐字形实测 108px,按 LVGL 的 (adv_w + 8) >> 4 取整规则,字体未启用
    // 字距调整故为精确值),赠言行 120px,都不会折行;左右各留 10px 露出纹样。
    lv_obj_set_style_bg_color(col, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(col, LV_OPA_60, 0);
    lv_obj_set_style_radius(col, 12, 0);
    lv_obj_set_style_pad_all(col, 6, 0);

    text(col, "致 朋友", &badge_frutiger_20, C_GOLD);

    // 显式换行,避免依赖 CJK 自动断行;两行都远窄于容器。
    lv_obj_t *verse = text(col, "千里送鹅毛，\n礼轻情意重", &badge_han_heavy_20, C_CREAM);
    lv_obj_set_style_text_align(verse, LV_TEXT_ALIGN_CENTER, 0);

    // 落款三行:拉丁 13px 走 Frutiger、汉字 11px 经 fallback 落到思源黑体,同一条
    // 基线;右对齐让三行的尾端齐平(块本身仍由 flex 居中)。
    lv_obj_t *sign = text(col, "Ziwei And SMTers\n张子威和SMT朋友们\n2026.09.27",
                          &badge_frutiger_13, C_FAINT);
    lv_obj_set_style_text_align(sign, LV_TEXT_ALIGN_RIGHT, 0);

#if BADGE_HAVE_SMT_LOGO
    // 落款下方留白处放 SMT 组合标。此处用 A8 版本并以 image_recolor 上色,避免
    // P1 那块白底(RGB565 无 alpha)压在深色图案上;y=286..304,避开圆角与文本。
    lv_obj_t *smt = lv_image_create(scr);
    lv_image_set_src(smt, &smt_logo_a8);
    lv_obj_set_style_image_recolor(smt, lv_color_hex(C_SMT_BLUE), 0);
    lv_obj_align(smt, LV_ALIGN_BOTTOM_MID, 0, -16);
#endif
}

// ---------------------------------------------------------------------------
// P0 开机动画
// ---------------------------------------------------------------------------
static void anim_set_x(void *var, int32_t value) { lv_obj_set_x((lv_obj_t *)var, value); }
static void anim_set_y(void *var, int32_t value) { lv_obj_set_y((lv_obj_t *)var, value); }
static void anim_set_opa(void *var, int32_t value) {
    lv_obj_set_style_opa((lv_obj_t *)var, (lv_opa_t)value, 0);
}
static void anim_set_recolor_opa(void *var, int32_t value) {
    lv_obj_set_style_image_recolor_opa((lv_obj_t *)var, (lv_opa_t)value, 0);
}

// 动画全部结束后再切页并删掉开机页,避免遗留 anim 引用已删对象。
static void boot_finish(lv_timer_t *timer) {
    (void)timer;
    for (int i = 0; i < 6; i++) {
        if (s_boot_block[i]) {
            lv_anim_delete(s_boot_block[i], NULL);
            s_boot_block[i] = NULL;
        }
    }
    if (s_boot_emblem) {
        lv_anim_delete(s_boot_emblem, NULL);
        s_boot_emblem = NULL;
    }

    if (s_page[BADGE_PAGE_ID]) lv_screen_load(s_page[BADGE_PAGE_ID]);
    if (s_boot_scr) {
        lv_obj_delete(s_boot_scr);
        s_boot_scr = NULL;
    }
}

static void build_boot(void) {
    lv_obj_t *scr = screen_new(C_PAPER);
    s_boot_scr = scr;

    const int start_x = SCREEN_W / 2 - (6 * BOOT_BLOCK) / 2;
    const int row_y = SCREEN_H / 2 - BOOT_BLOCK / 2;

    for (int i = 0; i < 6; i++) {
        s_boot_block[i] = fill(scr, start_x + i * BOOT_BLOCK, row_y, BOOT_BLOCK, BOOT_BLOCK,
                               k_emblem_colors[i], 0);
    }

    s_boot_emblem = lv_image_create(scr);
    lv_image_set_src(s_boot_emblem, &emblem_pixel);
    lv_obj_set_pos(s_boot_emblem, (SCREEN_W - 120) / 2, SCREEN_H / 2 - 60);
    lv_obj_set_style_opa(s_boot_emblem, LV_OPA_TRANSP, 0);

    // 0–250ms 中心横排;250–500ms 散开成六边形环(规格 §4 P0)。
    for (int i = 0; i < 6; i++) {
        const float ang = (float)i * 60.0f * DEG2RAD;
        int tx = SCREEN_W / 2 - BOOT_BLOCK / 2 + (int)(BOOT_HEX_RADIUS * cosf(ang));
        int ty = SCREEN_H / 2 - BOOT_BLOCK / 2 + (int)(BOOT_HEX_RADIUS * sinf(ang));

        lv_anim_t ax;
        lv_anim_init(&ax);
        lv_anim_set_var(&ax, s_boot_block[i]);
        lv_anim_set_exec_cb(&ax, anim_set_x);
        lv_anim_set_values(&ax, start_x + i * BOOT_BLOCK, tx);
        lv_anim_set_duration(&ax, 250);
        lv_anim_set_delay(&ax, 250);
        lv_anim_set_path_cb(&ax, lv_anim_path_ease_in_out);
        lv_anim_start(&ax);

        lv_anim_t ay;
        lv_anim_init(&ay);
        lv_anim_set_var(&ay, s_boot_block[i]);
        lv_anim_set_exec_cb(&ay, anim_set_y);
        lv_anim_set_values(&ay, row_y, ty);
        lv_anim_set_duration(&ay, 250);
        lv_anim_set_delay(&ay, 250);
        lv_anim_set_path_cb(&ay, lv_anim_path_ease_in_out);
        lv_anim_start(&ay);
    }

    // 500–900ms 徽记淡入盖住色块。
    lv_anim_t ae;
    lv_anim_init(&ae);
    lv_anim_set_var(&ae, s_boot_emblem);
    lv_anim_set_exec_cb(&ae, anim_set_opa);
    lv_anim_set_values(&ae, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_duration(&ae, 400);
    lv_anim_set_delay(&ae, 500);
    lv_anim_start(&ae);

    // 900–1200ms 色块淡出,徽记保持。
    for (int i = 0; i < 6; i++) {
        lv_anim_t af;
        lv_anim_init(&af);
        lv_anim_set_var(&af, s_boot_block[i]);
        lv_anim_set_exec_cb(&af, anim_set_opa);
        lv_anim_set_values(&af, LV_OPA_COVER, LV_OPA_TRANSP);
        lv_anim_set_duration(&af, 300);
        lv_anim_set_delay(&af, 900);
        lv_anim_start(&af);
    }

    lv_screen_load(scr);

    lv_timer_t *t = lv_timer_create(boot_finish, BOOT_MS, NULL);
    lv_timer_set_repeat_count(t, 1);
}

// ---------------------------------------------------------------------------
// 对外接口
// ---------------------------------------------------------------------------
void badge_ui_init(void) {
    build_page_id();
    build_page_honor();
    build_page_dedication();
    build_boot();
}

void badge_ui_show(badge_page_t page) {
    if ((int)page < 0 || (int)page >= (int)BADGE_PAGE_COUNT) return;
    if (s_page[page]) lv_screen_load(s_page[page]);
}

void badge_ui_refresh_battery(int soc_percent) {
    char buf[16]; // 容量需覆盖 "%d%%" 的最坏情况,否则触发 -Werror=format-truncation
    if (soc_percent >= 0) {
        snprintf(buf, sizeof(buf), "%d%%", soc_percent);
    } else {
        buf[0] = '\0';
    }

    static const badge_page_t k_pages[2] = {BADGE_PAGE_ID, BADGE_PAGE_HONOR};
    for (int i = 0; i < 2; i++) {
        lv_obj_t *l = s_batt_label[k_pages[i]];
        if (l) lv_label_set_text(l, buf);
    }
}

void badge_ui_pulse_medal(void) {
    if (!s_medal_img) return;

    // 银牌闪烁。奖牌本身是银白配色、页面又是白底,所以用白色叠加几乎看不见;
    // 改用品牌金色叠加,才能在银牌上形成明确的高光闪烁。
    lv_obj_set_style_image_recolor(s_medal_img, lv_color_hex(C_GOLD), 0);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, s_medal_img);
    lv_anim_set_exec_cb(&a, anim_set_recolor_opa);
    lv_anim_set_values(&a, LV_OPA_TRANSP, LV_OPA_COVER);
    lv_anim_set_duration(&a, 200);
    lv_anim_set_playback_duration(&a, 200);
    lv_anim_set_repeat_count(&a, 3); // 3 个完整亮暗循环
    lv_anim_start(&a);
}