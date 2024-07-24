/**
 ****************************************************************************************************
 * @file        touch.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.1
 * @date        2022-04-20
 * @brief       触摸屏 驱动代码
 * @note        支持电阻/电容式触摸屏
 *              触摸屏驱动（支持ADS7843/7846/UH7843/7846/XPT2046/TSC2046/GT9147/GT9271/FT5206等）代码
 *
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 阿波罗 F429开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.oT_PENedv.com
 * 公司网址:www.alientek.com
 * 购买地址:oT_PENedv.taobao.com
 *
 * 修改说明
 * V1.0 20220420
 * 第一次发布
 * V1.1 20230607
 * 1，新增对ST7796 3.5寸屏 GT1151的支持
 * 2，新增对ILI9806 4.3寸屏 GT1151的支持
 *
 ****************************************************************************************************
 */

#include "stdio.h"
#include "stdlib.h"
#include "lcd.h"
#include "applications\touch\touch.h"
#include "delay.h"
#include "board.h"
#include "stm32f429xx.h"
#include <stm32f4xx_hal_gpio.h>

_m_tp_dev tp_dev =
    {
        tp_init,
        NULL,
        NULL,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
};

/**
 * @brief       触摸屏初始化
 * @param       无
 * @retval      0,没有进行校准
 *              1,进行过校准
 */
uint8_t tp_init(void)
{

    tp_dev.touchtype = 0;                  /* 默认设置(电阻屏 & 竖屏) */
    tp_dev.touchtype |= lcddev.dir & 0X01; /* 根据LCD判定是横屏还是竖屏 */

    gt9xxx_init();
    tp_dev.scan = gt9xxx_scan; /* 扫描函数指向GT9147触摸屏扫描 */
    tp_dev.touchtype |= 0X80;  /* 电容屏 */
    return 0;
}
