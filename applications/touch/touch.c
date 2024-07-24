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
        tp_scan,
        tp_adjust,
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
 * @brief       SPI写数据
 * @note        向触摸屏IC写入1 byte数据
 * @param       data: 要写入的数据
 * @retval      无
 */
static void tp_write_byte(uint8_t data)
{
    uint8_t count = 0;

    for (count = 0; count < 8; count++)
    {
        if (data & 0x80) /* 发送1 */
        {
            T_MOSI(1);
        }
        else /* 发送0 */
        {
            T_MOSI(0);
        }

        data <<= 1;
        T_CLK(0);
        delay_us(1);
        T_CLK(1); /* 上升沿有效 */
    }
}

/**
 * @brief       SPI读数据
 * @note        从触摸屏IC读取adc值
 * @param       cmd: 指令
 * @retval      读取到的数据,ADC值(12bit)
 */
static uint16_t tp_read_ad(uint8_t cmd)
{
    uint8_t count = 0;
    uint16_t num = 0;
    T_CLK(0);           /* 先拉低时钟 */
    T_MOSI(0);          /* 拉低数据线 */
    T_CS(0);            /* 选中触摸屏IC */
    tp_write_byte(cmd); /* 发送命令字 */
    delay_us(6);        /* ADS7846的转换时间最长为6us */
    T_CLK(0);
    delay_us(1);
    T_CLK(1); /* 给1个时钟，清除BUSY */
    delay_us(1);
    T_CLK(0);

    for (count = 0; count < 16; count++) /* 读出16位数据,只有高12位有效 */
    {
        num <<= 1;
        T_CLK(0); /* 下降沿有效 */
        delay_us(1);
        T_CLK(1);

        if (T_MISO)
        {
            num++;
        }
    }

    num >>= 4; /* 只有高12位有效. */
    T_CS(1);   /* 释放片选 */
    return num;
}

/* 电阻触摸驱动芯片 数据采集 滤波用参数 */
#define TP_READ_TIMES 5 /* 读取次数 */
#define TP_LOST_VAL 1   /* 丢弃值 */

/**
 * @brief       读取一个坐标值(x或者y)
 * @note        连续读取TP_READ_TIMES次数据,对这些数据升序排列,
 *              然后去掉最低和最高TP_LOST_VAL个数, 取平均值
 *              设置时需满足: TP_READ_TIMES > 2*TP_LOST_VAL 的条件
 *
 * @param       cmd : 指令
 *   @arg       0XD0: 读取X轴坐标(@竖屏状态,横屏状态和Y对调.)
 *   @arg       0X90: 读取Y轴坐标(@竖屏状态,横屏状态和X对调.)
 *
 * @retval      读取到的数据(滤波后的), ADC值(12bit)
 */
static uint16_t tp_read_xoy(uint8_t cmd)
{
    uint16_t i, j;
    uint16_t buf[TP_READ_TIMES];
    uint16_t sum = 0;
    uint16_t temp;

    for (i = 0; i < TP_READ_TIMES; i++) /* 先读取TP_READ_TIMES次数据 */
    {
        buf[i] = tp_read_ad(cmd);
    }

    for (i = 0; i < TP_READ_TIMES - 1; i++) /* 对数据进行排序 */
    {
        for (j = i + 1; j < TP_READ_TIMES; j++)
        {
            if (buf[i] > buf[j]) /* 升序排列 */
            {
                temp = buf[i];
                buf[i] = buf[j];
                buf[j] = temp;
            }
        }
    }

    sum = 0;

    for (i = TP_LOST_VAL; i < TP_READ_TIMES - TP_LOST_VAL; i++) /* 去掉两端的丢弃值 */
    {
        sum += buf[i]; /* 累加去掉丢弃值以后的数据. */
    }

    temp = sum / (TP_READ_TIMES - 2 * TP_LOST_VAL); /* 取平均值 */
    return temp;
}

/**
 * @brief       读取x, y坐标
 * @param       x,y: 读取到的坐标值
 * @retval      无
 */
static void tp_read_xy(uint16_t *x, uint16_t *y)
{
    uint16_t xval, yval;

    if (tp_dev.touchtype & 0X01) /* X,Y方向与屏幕相反 */
    {
        xval = tp_read_xoy(0X90); /* 读取X轴坐标AD值, 并进行方向变换 */
        yval = tp_read_xoy(0XD0); /* 读取Y轴坐标AD值 */
    }
    else /* X,Y方向与屏幕相同 */
    {
        xval = tp_read_xoy(0XD0); /* 读取X轴坐标AD值 */
        yval = tp_read_xoy(0X90); /* 读取Y轴坐标AD值 */
    }

    *x = xval;
    *y = yval;
}

/* 连续两次读取X,Y坐标的数据误差最大允许值 */
#define TP_ERR_RANGE 50 /* 误差范围 */

/**
 * @brief       连续读取2次触摸IC数据, 并滤波
 * @note        连续2次读取触摸屏IC,且这两次的偏差不能超过ERR_RANGE,满足
 *              条件,则认为读数正确,否则读数错误.该函数能大大提高准确度.
 *
 * @param       x,y: 读取到的坐标值
 * @retval      0, 失败; 1, 成功;
 */
static uint8_t tp_read_xy2(uint16_t *x, uint16_t *y)
{
    uint16_t x1, y1;
    uint16_t x2, y2;

    tp_read_xy(&x1, &y1); /* 读取第一次数据 */
    tp_read_xy(&x2, &y2); /* 读取第二次数据 */

    /* 前后两次采样在+-TP_ERR_RANGE内 */
    if (((x2 <= x1 && x1 < x2 + TP_ERR_RANGE) || (x1 <= x2 && x2 < x1 + TP_ERR_RANGE)) &&
        ((y2 <= y1 && y1 < y2 + TP_ERR_RANGE) || (y1 <= y2 && y2 < y1 + TP_ERR_RANGE)))
    {
        *x = (x1 + x2) / 2;
        *y = (y1 + y2) / 2;
        return 1;
    }

    return 0;
}

/******************************************************************************************/
/* 与LCD部分有关的函数, 用来校准用的 */

/**
 * @brief       画一个校准用的触摸点(十字架)
 * @param       x,y   : 坐标
 * @param       color : 颜色
 * @retval      无
 */
static void tp_draw_touch_point(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_draw_line(x - 12, y, x + 13, y, color); /* 横线 */
    lcd_draw_line(x, y - 12, x, y + 13, color); /* 竖线 */
    lcd_draw_point(x + 1, y + 1, color);
    lcd_draw_point(x - 1, y + 1, color);
    lcd_draw_point(x + 1, y - 1, color);
    lcd_draw_point(x - 1, y - 1, color);
    lcd_draw_circle(x, y, 6, color); /* 画中心圈 */
}

/**
 * @brief       画一个大点(2*2的点)
 * @param       x,y   : 坐标
 * @param       color : 颜色
 * @retval      无
 */
void tp_draw_big_point(uint16_t x, uint16_t y, uint16_t color)
{
    lcd_draw_point(x, y, color); /* 中心点 */
    lcd_draw_point(x + 1, y, color);
    lcd_draw_point(x, y + 1, color);
    lcd_draw_point(x + 1, y + 1, color);
}

/******************************************************************************************/

/**
 * @brief       触摸按键扫描
 * @param       mode: 坐标模式
 *   @arg       0, 屏幕坐标;
 *   @arg       1, 物理坐标(校准等特殊场合用)
 *
 * @retval      0, 触屏无触摸; 1, 触屏有触摸;
 */
static uint8_t tp_scan(uint8_t mode)
{
    
}

/* TP_SAVE_ADDR_BASE定义触摸屏校准参数保存在EEPROM里面的位置(起始地址)
 * 占用空间 : 13字节.
 */
#define TP_SAVE_ADDR_BASE 40

/**
 * @brief       保存校准参数
 * @note        参数保存在EEPROM芯片里面(24C02),起始地址为TP_SAVE_ADDR_BASE.
 *              占用大小为13字节
 * @param       无
 * @retval      无
 */
void tp_save_adjust_data(void)
{
    uint8_t *p = (uint8_t *)&tp_dev.xfac; /* 指向首地址 */

    /* p指向tp_dev.xfac的地址, p+4则是tp_dev.yfac的地址
     * p+8则是tp_dev.xoff的地址,p+10,则是tp_dev.yoff的地址
     * 总共占用12个字节(4个参数)
     * p+12用于存放标记电阻触摸屏是否校准的数据(0X0A)
     * 往p[12]写入0X0A. 标记已经校准过.
     */
    at24cxx_write(TP_SAVE_ADDR_BASE, p, 12);              /* 保存12个字节数据(xfac,yfac,xc,yc) */
    at24cxx_write_one_byte(TP_SAVE_ADDR_BASE + 12, 0X0A); /* 保存校准值 */
}

/**
 * @brief       获取保存在EEPROM里面的校准值
 * @param       无
 * @retval      0，获取失败，要重新校准
 *              1，成功获取数据
 */
uint8_t tp_get_adjust_data(void)
{
    uint8_t *p = (uint8_t *)&tp_dev.xfac;
    uint8_t temp = 0;

    /* 由于我们是直接指向tp_dev.xfac地址进行保存的, 读取的时候,将读取出来的数据
     * 写入指向tp_dev.xfac的首地址, 就可以还原写入进去的值, 而不需要理会具体的数
     * 据类型. 此方法适用于各种数据(包括结构体)的保存/读取(包括结构体).
     */
    at24cxx_read(TP_SAVE_ADDR_BASE, p, 12);               /* 读取12字节数据 */
    temp = at24cxx_read_one_byte(TP_SAVE_ADDR_BASE + 12); /* 读取校准状态标记 */

    if (temp == 0X0A)
    {
        return 1;
    }

    return 0;
}

/* 提示字符串 */
char *const TP_REMIND_MSG_TBL = "Please use the stylus click the cross on the screen.The cross will always move until the screen adjustment is completed.";

/**
 * @brief       提示校准结果(各个参数)
 * @param       xy[5][2]: 5个物理坐标值
 * @param       px,py   : x,y方向的比例因子(约接近1越好)
 * @retval      无
 */
static void tp_adjust_info_show(uint16_t xy[5][2], double px, double py)
{
    uint8_t i;
    char sbuf[20];

    for (i = 0; i < 5; i++) /* 显示5个物理坐标值 */
    {
        sprintf(sbuf, "x%d:%d", i + 1, xy[i][0]);
        lcd_show_string(40, 160 + (i * 20), lcddev.width, lcddev.height, 16, sbuf, RED);
        sprintf(sbuf, "y%d:%d", i + 1, xy[i][1]);
        lcd_show_string(40 + 80, 160 + (i * 20), lcddev.width, lcddev.height, 16, sbuf, RED);
    }

    /* 显示X/Y方向的比例因子 */
    lcd_fill(40, 160 + (i * 20), lcddev.width - 1, 16, WHITE); /* 清除之前的px,py显示 */
    sprintf(sbuf, "px:%0.2f", px);
    sbuf[7] = 0; /* 添加结束符 */
    lcd_show_string(40, 160 + (i * 20), lcddev.width, lcddev.height, 16, sbuf, RED);
    sprintf(sbuf, "py:%0.2f", py);
    sbuf[7] = 0; /* 添加结束符 */
    lcd_show_string(40 + 80, 160 + (i * 20), lcddev.width, lcddev.height, 16, sbuf, RED);
}

/**
 * @brief       触摸屏校准代码
 * @note        使用五点校准法(具体原理请百度)
 *              本函数得到x轴/y轴比例因子xfac/yfac及物理中心坐标值(xc,yc)等4个参数
 *              我们规定: 物理坐标即AD采集到的坐标值,范围是0~4095.
 *                        逻辑坐标即LCD屏幕的坐标, 范围为LCD屏幕的分辨率.
 *
 * @param       无
 * @retval      无
 */
void tp_adjust(void)
{
   
}

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
