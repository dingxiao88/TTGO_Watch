/**
 * Simple clock display interface, power consumption is about 20mA
 * Written by lewishe
 * */

 #define LILYGO_TWATCH_2020_V1        // If you are using T-Watch-2020 version, please open this macro definition
#include <TTGO.h>

typedef struct {
    lv_obj_t *hour;
    lv_obj_t *minute;
    lv_obj_t *second;
} str_datetime_t;


static str_datetime_t g_data;
TTGOClass *watch = nullptr;
PCF8563_Class *rtc;
LV_IMG_DECLARE(batman_png);
LV_FONT_DECLARE(morgnite_bold_64);

void setup()
{
    Serial.begin(115200);
    watch = TTGOClass::getWatch();
    watch->begin();
    watch->eTFT->fillScreen(TFT_BLACK);
    watch->lvgl_begin();
    rtc = watch->rtc;

    // Use compile time
    rtc->check();

    watch->openBL();

    //Lower the brightness
    watch->bl->adjust(150);

    lv_obj_t *img1 = lv_img_create(lv_scr_act(), NULL);
    lv_img_set_src(img1, &batman_png);
    lv_obj_align(img1, NULL, LV_ALIGN_CENTER, 0, 0);

    static lv_style_t style;
    lv_style_copy(&style, &lv_style_pretty_color);
    style.text.font  = &morgnite_bold_64;
    style.text.color = LV_COLOR_WHITE;

    g_data.hour = lv_label_create(img1, nullptr);
    lv_label_set_style(g_data.hour, LV_LABEL_STYLE_MAIN, &style);
    lv_label_set_text(g_data.hour, "00");
    lv_obj_align(g_data.hour, img1, LV_ALIGN_IN_TOP_MID, 10, 30);

    g_data.minute = lv_label_create(img1, nullptr);
    lv_label_set_style(g_data.minute, LV_LABEL_STYLE_MAIN, &style);
    lv_label_set_text(g_data.minute, "00");
    lv_obj_align(g_data.minute, g_data.hour, LV_ALIGN_OUT_RIGHT_MID, 5, 0);

    g_data.second = lv_label_create(img1, nullptr);
    lv_label_set_style(g_data.second, LV_LABEL_STYLE_MAIN, &style);
    lv_label_set_text(g_data.second, "00");
    lv_obj_align(g_data.second, g_data.minute, LV_ALIGN_OUT_RIGHT_MID, 9, 0);

    lv_task_create([](lv_task_t *t) {

        RTC_Date curr_datetime = rtc->getDateTime();
        lv_label_set_text_fmt(g_data.second, "%02u", curr_datetime.second);
        lv_label_set_text_fmt(g_data.minute, "%02u", curr_datetime.minute);
        lv_label_set_text_fmt(g_data.hour, "%02u", curr_datetime.hour);

    }, 1000, LV_TASK_PRIO_MID, nullptr);

    // Set 20MHz operating speed to reduce power consumption
    setCpuFrequencyMhz(20);

}

void loop()
{
    lv_task_handler();
}
