# 中文字体生成指导

```bash
nvm use 22
npm i lv_font_conv -g
lv_font_conv --font SourceHanSansSC-VF.ttf --size 16 --format lvgl --bpp 1 -o ./lv_font_hansans_16.c --range 0x20-0x7F,0x4E00-0x9FFF

lv_font_conv --font SourceHanSansSC-VF.ttf --size 16 --format lvgl --bpp 1 -o ./lv_font_hansans_16.c --range 0x00-0x7F,0x3000-0x9FFF,0xFF00-0xFFEF --lv-fallback lv_font_montserrat_16


