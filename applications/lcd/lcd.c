#include <lcd_port.h>
#include "lcd.h"
#include "board.h"
#include "stm32f429xx.h"

#include "lcdfont.h"

#define DRV_DEBUG
#define LOG_TAG "lcd"
#include <drv_log.h>

struct drv_lcd_device *g_lcd_handle;

_lcd_dev lcddev = {
    .dir = 1,
    .height = LCD_HEIGHT,
    .width = LCD_WIDTH,
};

void lcd_display_dir(uint8_t dir)
{
    g_lcd_handle->direction = dir; /* 0竖屏/ 1横屏 */
}

void lcd_flush()
{
    g_lcd_handle->parent.control(&g_lcd_handle->parent, RTGRAPHIC_CTRL_RECT_UPDATE, RT_NULL);
}

rt_err_t lcd_init(void)
{
    g_lcd_handle = (struct drv_lcd_device *)rt_device_find("lcd");
    if (g_lcd_handle == RT_NULL)
    {
        LOG_E("init lcd failed!\n");
        return RT_ERROR;
    }
    else
    {
        lcd_display_dir(1); /* 默认为竖屏 */
        return RT_EOK;
    }
}

void lcd_draw_point(uint16_t x, uint16_t y, uint32_t color)
{
#if LCD_PIXEL_FORMAT == LCD_PIXEL_FORMAT_RGB565
    if (g_lcd_handle->direction == 1) /* 横屏 */
    {
        *(uint16_t *)((uint32_t)g_lcd_handle->lcd_info.framebuffer + sizeof(uint16_t) * (LCD_WIDTH * y + x)) = color;
    }
    else /* 竖屏 */
    {
        *(uint16_t *)((uint32_t)g_lcd_handle->lcd_info.framebuffer + sizeof(uint16_t) * (LCD_WIDTH * (LCD_HEIGHT - x - 1) + y)) = color;
    }

#else

#endif
}

/**
 * @brief       在指定位置显示一个字符
 * @param       x,y  : 坐标
 * @param       chr  : 要显示的字符:" "--->"~"
 * @param       size : 字体大小 12/16/24/32
 * @param       mode : 叠加方式(1); 非叠加方式(0);
 * @param       color: 字体颜色
 * @retval      无
 */
void lcd_show_char(uint16_t x, uint16_t y, char chr, uint8_t size, uint8_t mode, uint16_t color)
{
    uint8_t temp, t1, t;
    uint16_t y0 = y;
    uint8_t csize = 0;
    uint8_t *pfont = 0;
    uint32_t g_back_color = 0xFFFFFFFF; /* 背景色 */

    csize = (size / 8 + ((size % 8) ? 1 : 0)) * (size / 2); /* 得到字体一个字符对应点阵集所占的字节数 */
    chr = chr - ' ';                                        /* 得到偏移后的值（ASCII字库是从空格开始取模，所以-' '就是对应字符的字库） */

    switch (size)
    {
    case 12:
        pfont = (uint8_t *)asc2_1206[chr]; /* 调用1206字体 */
        break;

    case 16:
        pfont = (uint8_t *)asc2_1608[chr]; /* 调用1608字体 */
        break;

    case 24:
        pfont = (uint8_t *)asc2_2412[chr]; /* 调用2412字体 */
        break;

    case 32:
        pfont = (uint8_t *)asc2_3216[chr]; /* 调用3216字体 */
        break;

    default:
        return;
    }

    for (t = 0; t < csize; t++)
    {
        temp = pfont[t]; /* 获取字符的点阵数据 */

        for (t1 = 0; t1 < 8; t1++) /* 一个字节8个点 */
        {
            if (temp & 0x80) /* 有效点,需要显示 */
            {
                lcd_draw_point(x, y, color); /* 画点出来,要显示这个点 */
            }
            else if (mode == 0) /* 无效点,不显示 */
            {
                lcd_draw_point(x, y, g_back_color); /* 画背景色,相当于这个点不显示(注意背景色由全局变量控制) */
            }

            temp <<= 1; /* 移位, 以便获取下一个位的状态 */
            y++;

            if (y >= LCD_HEIGHT)
                return; /* 超区域了 */

            if ((y - y0) == size) /* 显示完一列了? */
            {
                y = y0; /* y坐标复位 */
                x++;    /* x坐标递增 */

                if (x >= LCD_WIDTH)
                {
                    return; /* x坐标超区域了 */
                }

                break;
            }
        }
    }
}

/**
 * @brief       显示字符串
 * @param       x,y         : 起始坐标
 * @param       width,height: 区域大小
 * @param       size        : 选择字体 12/16/24/32
 * @param       p           : 字符串首地址
 * @param       color       : 字体颜色
 * @retval      无
 */
void lcd_show_string(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint8_t size, char *p, uint16_t color)
{
    uint8_t x0 = x;

    width += x;
    height += y;

    while ((*p <= '~') && (*p >= ' ')) /* 判断是不是非法字符! */
    {
        if (x >= width)
        {
            x = x0;
            y += size;
        }

        if (y >= height)
        {
            break; /* 退出 */
        }

        lcd_show_char(x, y, *p, size, 0, color);
        x += size / 2;
        p++;
    }
}

void ltdc_clear(uint32_t color)
{
    ltdc_fill(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, color);
}

void lcd_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color)
{
    ltdc_color_fill(sx, sy, ex, ey, color);
}

// g_lcd_handle->lcd_info.framebuffer 对应g_ltdc_framebuf[lcdltdc.activelayer]
// lcdltdc.pixsize对应sizeof(uint16_t)
// LCD_WIDTH 对应LCD_WIDTH
// lcdltdc.pheight对应LCD_HEIGHT

/**
 * @brief       LTDC填充矩形,DMA2D填充
 *  @note       (sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex - sx + 1) * (ey - sy + 1)
 *              注意:sx,ex,不能大于lcddev.width - 1; sy,ey,不能大于lcddev.height - 1
 * @param       sx,sy       : 起始坐标
 * @param       ex,ey       : 结束坐标
 * @param       color       : 填充的颜色
 * @retval      无
 */
void ltdc_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{
    uint32_t psx, psy, pex, pey; /* 以LCD面板为基准的坐标系,不随横竖屏变化而变化 */
    uint32_t timeout = 0;
    uint16_t offline;
    uint32_t addr;

    /* 坐标系转换 */

    if (g_lcd_handle->direction == Landscape)
    {
        psx = sx;
        psy = sy;
        pex = ex;
        pey = ey;
    }
    else /* 竖屏 */
    {
        if (ex >= LCD_HEIGHT)
        {
            ex = LCD_HEIGHT - 1; /* 限制范围 */
        }
        if (sx >= LCD_HEIGHT)
        {
            sx = LCD_HEIGHT - 1; /* 限制范围 */
        }

        psx = sy;
        psy = LCD_HEIGHT - ex - 1;
        pex = ey;
        pey = LCD_HEIGHT - sx - 1;
    }

    offline = LCD_WIDTH - (pex - psx + 1);
    addr = ((uint32_t)g_lcd_handle->lcd_info.framebuffer + sizeof(uint16_t) * (LCD_WIDTH * psy + psx));

    __HAL_RCC_DMA2D_CLK_ENABLE();                           /* 使能DM2D时钟 */
    DMA2D->CR &= ~(DMA2D_CR_START);                         /* 先停止DMA2D */
    DMA2D->CR = DMA2D_CR_MODE;                              /* 寄存器到存储器模式 */
    DMA2D->OPFCCR = LTDC_PIXFORMAT_RGB565;                  /* 设置颜色格式 */
    DMA2D->OOR = offline;                                   /* 设置行偏移  */
    DMA2D->OMAR = addr;                                     /* 输出存储器地址 */
    DMA2D->NLR = (pey - psy + 1) | ((pex - psx + 1) << 16); /* 设定行数寄存器 */
    DMA2D->OCOLR = color;                                   /* 设定输出颜色寄存器 */
    DMA2D->FGMAR = (uint32_t)color;                         /* 源地址 */
    DMA2D->CR |= DMA2D_CR_START;                            /* 启动DMA2D */

    while ((DMA2D->ISR & (DMA2D_ISR_TCIF)) == 0) /* 等待传输完成 */
    {
        timeout++;
        if (timeout > 0X1FFFFF)
            break; /* 超时退出 */
    }
    DMA2D->IFCR |= DMA2D_ISR_TCIF; /* 清除传输完成标志 */
}

void ltdc_color_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint16_t *color)
{
    uint32_t psx, psy, pex, pey; /* 以LCD面板为基准的坐标系,不随横竖屏变化而变化 */
    uint32_t timeout = 0;
    uint16_t offline;
    uint32_t addr;

    /* 坐标系转换 */

    if (g_lcd_handle->direction == Landscape)
    {
        psx = sx;
        psy = sy;
        pex = ex;
        pey = ey;
    }
    else /* 竖屏 */
    {
        psx = sy;
        psy = LCD_HEIGHT - ex - 1;
        pex = ey;
        pey = LCD_HEIGHT - sx - 1;
    }

    offline = LCD_WIDTH - (pex - psx + 1);
    addr = ((uint32_t)g_lcd_handle->lcd_info.framebuffer + sizeof(uint16_t) * (LCD_WIDTH * psy + psx));

    RCC->AHB1ENR |= 1 << 23; /* 使能DM2D时钟 */

    DMA2D->CR = 0 << 16;                                    /* 存储器到存储器模式 */
    DMA2D->FGPFCCR = LTDC_PIXFORMAT_RGB565;                 /* 设置颜色格式 */
    DMA2D->FGOR = 0;                                        /* 前景层行偏移为0 */
    DMA2D->OOR = offline;                                   /* 设置行偏移 */
    DMA2D->CR &= ~(1 << 0);                                 /* 先停止DMA2D */
    DMA2D->FGMAR = (uint32_t)color;                         /* 源地址 */
    DMA2D->OMAR = addr;                                     /* 输出存储器地址 */
    DMA2D->NLR = (pey - psy + 1) | ((pex - psx + 1) << 16); /* 设定行数寄存器 */
    DMA2D->CR |= 1 << 0;                                    /* 启动DMA2D */

    while ((DMA2D->ISR & (1 << 1)) == 0) /* 等待传输完成 */
    {
        timeout++;

        if (timeout > 0X1FFFFF)
            break; /* 超时退出 */
    }

    DMA2D->IFCR |= 1 << 1; /* 清除传输完成标志 */
}

/**
 * @brief       在指定区域内填充单个颜色
 * @param       (sx,sy),(ex,ey):填充矩形对角坐标,区域大小为:(ex - sx + 1) * (ey - sy + 1)
 * @param       color:  要填充的颜色(32位颜色,方便兼容LTDC)
 * @retval      无
 */
void lcd_fill(uint16_t sx, uint16_t sy, uint16_t ex, uint16_t ey, uint32_t color)
{
    uint16_t i, j;
    uint16_t xlen = 0;

    ltdc_fill(sx, sy, ex, ey, color);
}

void lcd_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint16_t color)
{
    if ((len == 0) || (x > lcddev.width) || (y > lcddev.height))
    {
        return;
    }

    lcd_fill(x, y, x + len - 1, y, color);
}

/**
 * @brief       填充实心圆
 * @param       x,y  : 圆中心坐标
 * @param       r    : 半径
 * @param       color: 圆的颜色
 * @retval      无
 */
void lcd_fill_circle(uint16_t x, uint16_t y, uint16_t r, uint16_t color)
{
    uint32_t i;
    uint32_t imax = ((uint32_t)r * 707) / 1000 + 1;
    uint32_t sqmax = (uint32_t)r * (uint32_t)r + (uint32_t)r / 2;
    uint32_t xr = r;

    lcd_draw_hline(x - r, y, 2 * r, color);

    for (i = 1; i <= imax; i++)
    {
        if ((i * i + xr * xr) > sqmax)
        {
            /* draw lines from outside */
            if (xr > imax)
            {
                lcd_draw_hline(x - i + 1, y + xr, 2 * (i - 1), color);
                lcd_draw_hline(x - i + 1, y - xr, 2 * (i - 1), color);
            }

            xr--;
        }

        /* draw lines from inside (center) */
        lcd_draw_hline(x - xr, y + i, 2 * xr, color);
        lcd_draw_hline(x - xr, y - i, 2 * xr, color);
    }
}

void lcd_clear(uint16_t color)
{
    uint32_t index = 0;
    uint32_t totalpoint = lcddev.width;

    ltdc_clear(color);
}