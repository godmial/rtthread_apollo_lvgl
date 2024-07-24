#ifndef APPLICATIONS_LCD_H_
#define APPLICATIONS_LCD_H_

#include "stdint.h"
#include <rtthread.h>

#define LTDC_PIXFORMAT_ARGB8888 0X00 /* ARGB8888格式 */
#define LTDC_PIXFORMAT_RGB888 0X01   /* RGB888格式 */
#define LTDC_PIXFORMAT_RGB565 0X02   /* RGB565格式 */
#define LTDC_PIXFORMAT_ARGB1555 0X03 /* ARGB1555格式 */
#define LTDC_PIXFORMAT_ARGB4444 0X04 /* ARGB4444格式 */
#define LTDC_PIXFORMAT_L8 0X05       /* L8格式 */
#define LTDC_PIXFORMAT_AL44 0X06     /* AL44格式 */
#define LTDC_PIXFORMAT_AL88 0X07     /* AL88格式 */

#define WHITE 0xFFFF
#define BLACK 0x0000
#define RED 0xF800
#define GREEN 0x07E0
#define BLUE 0x001F
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define CYAN 0x07FF

/* 非常用颜色 */
#define BROWN 0xBC40      /* 棕色 */
#define BRRED 0xFC07      /* 棕红色 */
#define GRAY 0x8430       /* 灰色 */
#define DARKBLUE 0x01CF   /* 深蓝色 */
#define LIGHTBLUE 0x7D7C  /* 浅蓝色 */
#define GRAYBLUE 0x5458   /* 灰蓝色 */
#define LIGHTGREEN 0x841F /* 浅绿色 */
#define LGRAY 0xC618      /* 浅灰色(PANNEL),窗体背景色 */
#define LGRAYBLUE 0xA651  /* 浅灰蓝色(中间层颜色) */
#define LBBLUE 0x2B12     /* 浅棕蓝色(选择条目的反色) */

typedef struct
{
    uint16_t width;   /* LCD 宽度 */
    uint16_t height;  /* LCD 高度 */
    uint16_t id;      /* LCD ID */
    uint8_t dir;      /* 横屏还是竖屏控制：0，竖屏；1，横屏。 */
    uint16_t wramcmd; /* 开始写gram指令 */
    uint16_t setxcmd; /* 设置x坐标指令 */
    uint16_t setycmd; /* 设置y坐标指令 */
} _lcd_dev;

extern _lcd_dev lcddev; /* 管理LCD重要参数 */
typedef enum
{
    Portrait = 0, // 竖屏
    Landscape = 1 // 横屏
} dir_e;

rt_err_t lcd_init(void);

void lcd_flush();

void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color);

void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color);

void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color);

void ltdc_clear(uint32_t color);
void ltdc_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color);
void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color);
void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color);
void lcd_clear(uint16_t color);

#endif
