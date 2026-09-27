<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 资源目录（Assets）

本目录集中存放可复用的资源（字库、图片、音乐等），按资源类型分子目录管理。每个资源放在其类型对应的子目录，并记录放置路径、命名方式、集成方式与来源/许可。二进制资源（字体、图片、音频）不属于纯 markdown 文档，请勿与文档混放。涉及版权/授权的资源需注明来源与许可。

## 字库（fonts）

可复用的字库文件与生成的字库源码放在 `fonts/`。

- 命名要能反映字族、字重、字级与格式。
- 记录来源、许可、字符范围、转换命令与目标放置路径。
- 添加字库前评估 Flash 与内部 RAM 影响；ESP32-C3 无 PSRAM。
- 不提交许可不允许分发的字库。

## 图片（images）

可复用的源图与生成的显示资产放在 `images/`。

| 文件 | 尺寸与格式 | 用途与来源 |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160，JPEG | 嵌入中英文项目 README 的产品主图，突出 AI Passport 产品形象与开放、人人可创作的理念。 |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724，PNG RGBA | 保留为可选技术参考图，不再用于首页主视觉。于 2026-09-17 使用内置图像生成工具为本仓库生成；已根据文档中的硬件能力契约核对图中的六项标签与参数。 |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336，PNG RGBA | 从仓库原始 `images/logo.png` 中精确裁切并去除背景的黑色字标；用于中英文项目 README 的浅色主题。 |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336，PNG RGBA | 提取字标的白色版本；README 使用 `<picture>` 在 GitHub 深色主题下显示。 |

- 使用描述性命名，并记录尺寸、像素格式、转换步骤与目标路径。
- 优先采用适合 240 × 320 RGB565 显示的格式，并纳入 Flash 与内部 RAM 考量。
- 许可允许时保留可编辑源文件，并记录来源与许可。
- 图片中不得包含设备二维码秘密、凭证或个人数据。

## 音乐与音效（music）

可复用的音乐与音效源码放在 `music/`。

- 记录来源、许可、采样率、位深、声道、转换命令与目标路径。
- 与当前 BSP 音频路径匹配时优先采用 16 kHz、16 位单声道 PCM。
- 嵌入音频前评估 Flash 与内部 RAM 成本；长录音应流式或分块。
- 无再分发许可不提交媒体文件。

## WorldSkills 赠礼工牌资源

`fonts/badge_*.c` 与 `images/wss_logo_red*.c`、`images/emblem_pixel.c`、
`images/medal_silver.c`、`images/smt_logo.c`、`images/smt_logo_a8.c`、
`images/bg_page3.c`、`images/portrait_hengky.c` 是为一次性
WorldSkills 赠礼工牌应用（`main/badge_ui.c`）生成的 LVGL 9.5 源码。它们只服务于这次
赠礼，不属于可复用的上游资源。重新生成请使用 `tools/make_badge_fonts.py` 与
`tools/make_badge_assets.py`；可编辑的源文件保留在仓库之外。

其中赠言页的拉丁字体按品牌规范选用 Frutiger，属 Monotype 商用字体：源文件由用户
提供、只在本机使用，生成的 `fonts/badge_frutiger_*.c` 已列入 `.gitignore`，**既不要
提交、也不要随固件一起对外分发**。若无法保留该授权，可改用 Source Sans 3（本目录
其它 `badge_sans_*.c` 即由它生成）作为近似替代。

| 文件 | 尺寸与格式 | 用途与来源 |
| --- | --- | --- |
| [`images/wss_logo_red.c`](images/wss_logo_red.c) | 83 × 56，RGB565 | WorldSkills 2026 红色锁定 logo，随赠礼素材提供。 |
| [`images/wss_logo_red_lg.c`](images/wss_logo_red_lg.c) | 94 × 64，RGB565 | 同一 logo 的放大版，用于卡片顶部降级布局。 |
| [`images/emblem_pixel.c`](images/emblem_pixel.c) | 120 × 120，RGB565 | 依据赠礼素材中的榫卯徽记重绘的像素画；六种配色取自同一素材。 |
| [`images/medal_silver.c`](images/medal_silver.c) | 120 × 120，RGB565 | 原创像素风银牌，绘制于 30 × 30 逻辑网格。 |
| [`images/smt_logo.c`](images/smt_logo.c) | 59 × 18，RGB565 | SMT 徽记与字标，用于 "Presented by" 行；源自提供的透明 PNG，统一吸附到品牌蓝 `#0096E0`。 |
| [`images/smt_logo_a8.c`](images/smt_logo_a8.c) | 59 × 18，A8 | 同一组合标，只保留 alpha 平面，供深色赠言页使用：LVGL 会用控件的 recolor 颜色填充它，因此不会像 RGB565 版本那样带一块不透明底衬。绘制时需 recolor 为 `#0096E0`。 |
| [`images/bg_page3.c`](images/bg_page3.c) | 240 × 320，RGB565 | 赠言页背景。素材是横构图的无缝藏蓝几何纹样，因此按屏幕尺寸原分辨率裁切一块窗口，而不做缩放。随后在生成阶段烘焙高斯模糊（半径 2px），把细线纹样化成柔光退到文字之后；ESP32-C3 无 PSRAM，运行期模糊需要快照与第二块全屏缓冲，故不在设备上做，预烘焙也不增加 Flash 占用。 |
| [`images/portrait_hengky.c`](images/portrait_hengky.c) | 84 × 84，RGB565 | 受赠人证件照，仅为本次赠礼提供。属于个人信息，已列入 `.gitignore`、不随仓库提交；本地要构建赠礼固件时，把源图放回并重新运行 `tools/make_badge_assets.py` 生成即可。 |
| [`fonts/badge_sans_11.c`](fonts/badge_sans_11.c)、[`badge_sans_14.c`](fonts/badge_sans_14.c)、[`badge_sans_18.c`](fonts/badge_sans_18.c)、[`badge_sans_bold_22.c`](fonts/badge_sans_bold_22.c) | 4bpp 位图 | Source Sans 3（SIL OFL 1.1）；拉丁 U+0020–U+007E，另加 U+00B7 与 U+2014。 |
| [`fonts/badge_frutiger_13.c`](fonts/badge_frutiger_13.c)、[`badge_frutiger_20.c`](fonts/badge_frutiger_20.c) | 4bpp 位图 | **Frutiger Bold（Monotype，商用字体）**；拉丁 U+0020–U+007E 另加 U+2014，供赠言页使用（落款 13px、"致 Hengky" 20px）。由用户提供的字体文件在本地生成，**不得再分发**：已列入 `.gitignore`、不随仓库提交，分支公开前须删除。 |
| [`fonts/badge_han_heavy_11.c`](fonts/badge_han_heavy_11.c)、[`badge_han_heavy_20.c`](fonts/badge_han_heavy_20.c) | 4bpp 位图 | 思源黑体 Heavy（Source Han Sans Heavy，SIL OFL 1.1）。11px 只含落款的 7 个汉字，20px 含赠言汉字与 U+FF0C。它们既是赠言正文直接选用的字体，也是上面两个 Frutiger 子集的 `fallback`——中英混排时拉丁字体缺失的汉字经 `lv_font_t.fallback` 落到本字体，即规范要求的"明确 fallback 链"。生成脚本会把成对的 `line_height`/`base_line` 统一（20px 对为 22/4，13px 对为 14/3），保证汉字与拉丁落在同一基线。 |
