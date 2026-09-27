<p align="right">
  <a href="README.zh_CN.md">简体中文</a> · <strong>English</strong>
</p>

# Assets

This directory stores reusable fonts, images, music, and sound effects, organized by asset type.

Keep each asset in the matching subdirectory and document its destination, naming, integration method, and source/license. Do not mix binary assets with Markdown documentation.

## Fonts

Store reusable font files and generated font sources in `fonts/`.

- Use descriptive names that include the family, weight, size, and format when relevant.
- Document the source, license, character range, conversion command, and expected destination.
- Check Flash and internal-RAM impact before adding a font; the ESP32-C3 has no PSRAM.
- Do not commit fonts whose license does not permit redistribution.

## Images

Store reusable source images and generated display assets in `images/`.

| File | Dimensions and format | Use and source |
| --- | --- | --- |
| [`images/home.jpg`](images/home.jpg) | 3840 × 2160, JPEG | Product hero image embedded in both project README files to foreground AI Passport and its open, maker-oriented identity. |
| [`images/readme-hardware-specs.png`](images/readme-hardware-specs.png) | 2172 × 724, PNG RGBA | Optional technical infographic retained as a reference asset; it is no longer used as the homepage hero. Generated for this repository with the built-in image generation tool on 2026-09-17; the six labels and values were checked against the documented hardware contract. |
| [`images/logo-wordmark.png`](images/logo-wordmark.png) | 1648 × 336, PNG RGBA | Transparent black wordmark extracted from the repository's original `images/logo.png`; embedded in both project README files for light backgrounds. |
| [`images/logo-wordmark-dark.png`](images/logo-wordmark-dark.png) | 1648 × 336, PNG RGBA | White version of the extracted wordmark, used by the README `<picture>` element when GitHub is in dark mode. |

- Use descriptive names and document dimensions, pixel format, conversion steps, and destination.
- Prefer formats suitable for the 240 × 320 RGB565 display and account for Flash and internal RAM.
- Preserve editable sources where licensing permits, and record the source and license.
- Never commit device QR secrets, credentials, or personal data in images.

## Music and sound effects

Store reusable music and sound-effect sources in `music/`.

- Document the source, license, sample rate, bit depth, channels, conversion command, and destination.
- Prefer 16 kHz, 16-bit mono PCM when it matches the current BSP audio path.
- Check Flash and internal-RAM cost before embedding audio; stream or chunk long recordings.
- Do not commit media without redistribution permission.

## Badge application assets

`fonts/badge_*.c` and `images/wss_logo_red*.c`, `images/emblem_pixel.c`,
`images/medal_silver.c`, `images/smt_logo.c`, `images/smt_logo_a8.c`,
`images/bg_page3.c`, and `images/portrait_sample.c` are generated LVGL 9.5
sources for the three-page badge application (`main/badge_ui.c`). They serve that
application and are not reusable upstream assets. Regenerate them with
`tools/make_badge_fonts.py` and `tools/make_badge_assets.py`; the editable source files stay
outside the repository.

The dedication page's Latin face follows the brand guide and is therefore Frutiger, a
commercial Monotype typeface: its licence does not permit redistribution, so the
generated `fonts/badge_frutiger_*.c` files are listed in `.gitignore` and are never
committed. **Neither commit them nor ship them inside a distributed firmware.** If that
licence cannot be retained, Source Sans 3 (already behind the other `badge_sans_*.c`
files here) is the stand-in.

The work-pass portrait is personal data for the same reason: `tools/make_badge_assets.py`
reads a locally supplied photo and `images/portrait_sample.c` is likewise gitignored.

| File | Dimensions and format | Use and source |
| --- | --- | --- |
| [`images/wss_logo_red.c`](images/wss_logo_red.c) | 83 × 56, RGB565 | WorldSkills 2026 red lockup, supplied with the brand package. |
| [`images/wss_logo_red_lg.c`](images/wss_logo_red_lg.c) | 94 × 64, RGB565 | Same lockup, enlarged for the top-of-card fallback layout. |
| [`images/emblem_pixel.c`](images/emblem_pixel.c) | 120 × 120, RGB565 | Pixel-art redraw of the mortise-and-tenon emblem supplied in the brief; its six colours were sampled from that source. |
| [`images/medal_silver.c`](images/medal_silver.c) | 120 × 120, RGB565 | Original pixel-art silver medal on a 30 × 30 logical grid. |
| [`images/smt_logo.c`](images/smt_logo.c) | 59 × 18, RGB565 | SMT emblem and wordmark for the "Presented by" row, supplied as a transparent PNG and snapped to the brand blue `#0096E0`. |
| [`images/smt_logo_a8.c`](images/smt_logo_a8.c) | 59 × 18, A8 | The same lockup as an alpha plane only, for the dark dedication page: LVGL fills it with the widget recolour colour, so no opaque plate is baked in. Recolour it to `#0096E0` when drawing. |
| [`images/bg_page3.c`](images/bg_page3.c) | 240 × 320, RGB565 | Dedication-page background. The supplied artwork is a seamless navy geometric pattern in landscape, so a native-resolution panel-sized window is cropped from it instead of scaling. A Gaussian blur (2px radius) is baked in at generation time to push the line work behind the dedication text; the ESP32-C3 has no PSRAM, so runtime blurring would need a snapshot plus a second full-screen buffer, and baking adds no Flash cost. |
| [`images/portrait_sample.c`](images/portrait_sample.c) | 84 × 84, RGB565 | Portrait plate for the work-pass page. Personal data: listed in `.gitignore` and never committed; supply your own photo as `portrait-source.jpeg` and rerun `tools/make_badge_assets.py` to build locally. |
| [`fonts/badge_sans_11.c`](fonts/badge_sans_11.c), [`badge_sans_14.c`](fonts/badge_sans_14.c), [`badge_sans_18.c`](fonts/badge_sans_18.c), [`badge_sans_bold_22.c`](fonts/badge_sans_bold_22.c) | 4 bpp bitmap | Source Sans 3 (SIL OFL 1.1); Latin U+0020–U+007E plus U+00B7 and U+2014. |
| [`fonts/badge_frutiger_13.c`](fonts/badge_frutiger_13.c), [`badge_frutiger_20.c`](fonts/badge_frutiger_20.c) | 4 bpp bitmap | **Frutiger Bold (Monotype, commercial)**; Latin U+0020–U+007E plus U+2014, used by the dedication page (13px sign-off and the 20px greeting line). Generated locally from a copy of the typeface and **not redistributable**: listed in `.gitignore` and never committed. |
| [`fonts/badge_han_heavy_11.c`](fonts/badge_han_heavy_11.c), [`badge_han_heavy_20.c`](fonts/badge_han_heavy_20.c) | 4 bpp bitmap | Source Han Sans Heavy (SIL OFL 1.1). The 11px face carries only the seven Han characters of the sign-off; the 20px face carries the dedication's Han characters plus U+FF0C. They are selected directly for the Chinese verse and serve as the `fallback` of the two Frutiger subsets, so Han characters in mixed strings resolve to them — the "deliberate fallback chain" the conventions require. The generator unifies each pair's `line_height`/`base_line` (22/4 at 20px, 14/3 at 13px) so Han and Latin share a baseline. |
