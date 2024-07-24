#include <rtthread.h>
#include <lvgl.h>
#include <lv_port_indev.h>

#define DBG_TAG "LVGL.demo"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

extern void lv_port_disp_init(void);
extern void lv_port_indev_init(void);

static struct rt_thread lvgl_thread;

static lv_obj_t *label;

static rt_uint8_t lvgl_thread_stack[PKG_LVGL_THREAD_STACK_SIZE];

static void btn_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    if (code == LV_EVENT_CLICKED)
    {
        static uint8_t cnt = 0;
        cnt++;
        /*Get the first child of the button which is the label and change its text*/
        lv_obj_t *label = lv_obj_get_child(btn, 0);
        lv_label_set_text_fmt(label, "Button: %d", cnt);
    }
}

/**
 * Create a button with a label and react on click event.
 */
void lv_example_get_started_1(void)
{
    lv_obj_t *btn = lv_btn_create(lv_scr_act()); /*Add a button the current screen*/
                                                 // lv_obj_set_pos(btn, 10, 10);                            /*Set its position*/
    lv_obj_center(btn);
    lv_obj_set_size(btn, 120, 50);                              /*Set its size*/
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL); /*Assign a callback to the button*/

    lv_obj_t *label = lv_label_create(btn); /*Add a label to the button*/
    lv_label_set_text(label, "Button");     /*Set the labels text*/
    lv_obj_center(label);
}

static void slider_event_cb(lv_event_t *e)
{
    lv_obj_t *slider = lv_event_get_target(e);

    /*Refresh the text*/
    lv_label_set_text_fmt(label, "%" LV_PRId32, lv_slider_get_value(slider));

    lv_obj_align_to(label, slider, LV_ALIGN_OUT_TOP_MID, 0, -15); /*Align top of the slider*/
}

/**
 * Create a slider and write its value on a label.
 */
void lv_example_get_started_3(void)
{
    /*Create a slider in the center of the display*/
    lv_obj_t *slider = lv_slider_create(lv_scr_act());
    lv_obj_set_width(slider, 200);                                              /*Set the width*/
    lv_obj_center(slider);                                                      /*Align to the center of the parent (screen)*/
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL); /*Assign an event function*/

    /*Create a label below the slider*/
    label = lv_label_create(lv_scr_act());
    lv_label_set_text(label, "0");
    lv_obj_align_to(label, slider, LV_ALIGN_OUT_TOP_MID, 0, -15); /*Align top of the slider*/
}

static void lvgl_thread_entry(void *parameter)
{

    lv_init();

    lv_port_disp_init();

    lv_port_indev_init();

    // 同时使用会冲突
    //  lv_example_get_started_1(); // 小按钮，按一下加一
    lv_example_get_started_3(); // 滑块

    /* handle the tasks of LVGL */
    while (1)
    {
        lv_task_handler();
        rt_thread_mdelay(30);
    }
}

int lvgl_thread_init(void)
{
    rt_err_t err;

    err = rt_thread_init(&lvgl_thread, "LVGL",
                         lvgl_thread_entry,
                         RT_NULL,
                         &lvgl_thread_stack[0],
                         sizeof(lvgl_thread_stack),
                         PKG_LVGL_THREAD_PRIO,
                         10);
    if (err != RT_EOK)
    {
        LOG_E("Failed to create LVGL thread");
        return -1;
    }
    rt_thread_startup(&lvgl_thread);

    return 0;
}
INIT_ENV_EXPORT(lvgl_thread_init);
