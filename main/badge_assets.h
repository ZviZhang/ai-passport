// main/badge_assets.h —— 资产符号声明(字体 + 图片)。
//
// 实际定义在 assets/fonts/*.c 与 assets/images/*.c,由 main/CMakeLists.txt 一并编译。
// 符号名与文件名同名,勿改名。
#pragma once

#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// 置 1 并准备好 assets/images/smt_logo.c(h 18,宽 ≤66)后,工作证页会启用官方
// "Presented by" 行;置 0 时该行留空、顶部 logo 放大顶替(规格 §4 降级方案)。
#define BADGE_HAVE_SMT_LOGO 1

// --- 字体(assets/fonts) ---
extern const lv_font_t badge_sans_11;
extern const lv_font_t badge_sans_14;
extern const lv_font_t badge_sans_18;
extern const lv_font_t badge_sans_bold_22;
// 赠言页专用(见 tools/make_badge_fonts.py):拉丁为 Frutiger(商用,不随仓库
// 分发),汉字为思源黑体 Heavy。两种字号各自成对,汉字节子集在生成时就
// 通过 --lv-fallback 挂在拉丁字体后面,故只引用 badge_frutiger_* 即可。
extern const lv_font_t badge_frutiger_13;  // 落款拉丁 13px,fallback = badge_han_heavy_11
extern const lv_font_t badge_frutiger_20;  // "致 朋友" 拉丁 20px,fallback = badge_han_heavy_20
extern const lv_font_t badge_han_heavy_11; // 落款汉字 11px(张子威和朋友们)
extern const lv_font_t badge_han_heavy_20; // 赠言汉字 20px(致朋友千里送鹅毛礼轻情意重、，)

// --- 图片(assets/images) ---
extern const lv_image_dsc_t wss_logo_red;    // 83x56  红色锁定 logo
extern const lv_image_dsc_t wss_logo_red_lg; // 94x64  降级方案用放大版
extern const lv_image_dsc_t emblem_pixel;    // 120x120 榫卯徽记像素风
extern const lv_image_dsc_t medal_silver;    // 120x120 银牌像素风
extern const lv_image_dsc_t portrait_sample; // 84x84  头像方形裁切
extern const lv_image_dsc_t bg_page3;        // 240x320 赠言页背景(图案 1:1 裁切)

#if BADGE_HAVE_SMT_LOGO
extern const lv_image_dsc_t smt_logo;    // 59x18 白底组合标(P1,浅底页)
extern const lv_image_dsc_t smt_logo_a8; // 59x18 仅 alpha,配 image_recolor 用于深色页
#endif

#ifdef __cplusplus
}
#endif