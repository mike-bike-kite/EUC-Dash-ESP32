/****************************************************************************
 *   Copyright  2020  Jesper Ortlund
 ****************************************************************************/
/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <Arduino.h>
#include "gui/mainbar/mainbar.h"
#include "fulldash_tile.h"
#include "hardware/pmu.h"
#include "hardware/blectl.h"
#include "hardware/Kingsong.h"
#include "hardware/dashboard.h"
#include "hardware/wheelctl.h"
#include "hardware/motor.h"

//task declarations
lv_task_t *dash_task = nullptr;
lv_task_t *time_task = nullptr;

// Function declarations
static void lv_dash_task(lv_task_t *dash_task);
static void lv_time_task(lv_task_t *time_task);
static void overlay_event_cb(lv_obj_t * obj, lv_event_t event);

void updateTime();
void stop_time_task();
void stop_dash_task();

/*
   Declare LVGL Dashboard objects and styles
*/
static lv_obj_t *fulldash_cont = NULL;
static lv_style_t *style;
static lv_style_t arc_style;

// Arc gauges and labels
//arc background styles
static lv_style_t arc_warn_style;
static lv_style_t arc_crit_style;
//Speed
static lv_obj_t *speed_arc = NULL;
static lv_obj_t *speed_warn_arc = NULL;
static lv_obj_t *speed_crit_arc = NULL;
static lv_obj_t *speed_label = NULL;
static lv_style_t speed_indic_style;
static lv_style_t speed_main_style;
static lv_style_t speed_label_style;
//Battery
static lv_obj_t *batt_arc = NULL;
static lv_obj_t *batt_warn_arc = NULL;
static lv_obj_t *batt_crit_arc = NULL;
static lv_obj_t *batt_label = NULL;
static lv_style_t batt_indic_style;
static lv_style_t batt_main_style;
static lv_style_t batt_label_style;
//Current
static lv_obj_t *current_arc = NULL;
static lv_obj_t *current_warn_arc = NULL;
static lv_obj_t *current_crit_arc = NULL;
static lv_obj_t *current_label = NULL;
static lv_style_t current_indic_style;
static lv_style_t current_main_style;
static lv_style_t current_label_style;
//Temperature
static lv_obj_t *temp_arc = NULL;
static lv_obj_t *temp_label = NULL;
static lv_style_t temp_indic_style;
static lv_style_t temp_main_style;
static lv_style_t temp_label_style;
//Max min avg bars
static lv_obj_t *speed_avg_bar = NULL;
static lv_obj_t *speed_max_bar = NULL;
static lv_obj_t *batt_max_bar = NULL;
static lv_obj_t *batt_min_bar = NULL;
static lv_obj_t *current_max_bar = NULL;
static lv_obj_t *current_regen_bar = NULL;
static lv_obj_t *temp_max_bar = NULL;
//Max avg, regen and min bars
static lv_style_t max_bar_indic_style;
static lv_style_t min_bar_indic_style; //also for avg speed
static lv_style_t regen_bar_indic_style;
static lv_style_t bar_main_style;
//Dashclock objects and styles
static lv_obj_t *dashtime = NULL;
static lv_obj_t *wbatt = NULL;
static lv_obj_t *trip = NULL;
static lv_style_t trip_label_style;
static lv_style_t dashtime_style;
//alert objects and styles
static lv_obj_t *batt_alert = NULL;
static lv_obj_t *current_alert = NULL;
static lv_obj_t *temp_alert = NULL;
static lv_style_t alert_style;

//Overlay objects and styles
static lv_obj_t *overlay_bar = NULL;
static lv_obj_t *overlay_line = NULL;
static lv_style_t overlay_style;

//End LV objects and styles

int speed_arc_start = 160;
int speed_arc_end = 20;
int batt_arc_start = 35;
int batt_arc_end = 145;
int current_arc_start = 130;
int current_arc_end = 230;
int temp_arc_start = 310;
int temp_arc_end = 50;
int arclinew = 15; // line width of arc gauges
int arc_spacing = 7;
bool rev_batt_arc = true;
bool rev_current_arc = false;

//int display_xres = lv_disp_get_hor_res( NULL );
//int display_yres = lv_disp_get_ver_res( NULL );

int out_arc_x = 240;
int out_arc_y = 240;

int in_arc_x = out_arc_x - (2 * (arclinew + arc_spacing));
int in_arc_y = out_arc_y - (2 * (arclinew + arc_spacing));

uint32_t fulldash_tile_num;

/*************************************
 *  Define LVGL default object styles
 ************************************/

void lv_define_styles_1(void)
{
    //style template
    lv_style_copy(&arc_style, style);
    //lv_style_init(&arc_style);
    lv_style_set_line_rounded(&arc_style, LV_STATE_DEFAULT, false);
    lv_style_set_line_width(&arc_style, LV_STATE_DEFAULT, arclinew);
    lv_style_set_bg_opa(&arc_style, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    lv_style_set_border_width(&arc_style, LV_STATE_DEFAULT, 0);
    lv_style_set_border_opa(&arc_style, LV_STATE_DEFAULT, LV_OPA_TRANSP);

    //General styles
    lv_style_copy(&arc_warn_style, &arc_style);
    lv_style_copy(&arc_crit_style, &arc_style);

    //Speed arc and label
    lv_style_copy(&speed_indic_style, &arc_style);
    lv_style_copy(&speed_main_style, &arc_style);
    lv_style_set_line_color(&speed_main_style, LV_STATE_DEFAULT, speed_bg_clr);

    lv_style_init(&speed_label_style);
    lv_style_set_text_color(&speed_label_style, LV_STATE_DEFAULT, speed_bg_clr);
    lv_style_set_text_font(&speed_label_style, LV_STATE_DEFAULT, &DIN1451_m_cond_120);

    // Battery Arc and label
    lv_style_copy(&batt_indic_style, &arc_style);
    lv_style_copy(&batt_main_style, &arc_style);
    lv_style_set_line_color(&batt_main_style, LV_STATE_DEFAULT, batt_bg_clr);

    lv_style_init(&batt_label_style);
    lv_style_set_text_font(&batt_label_style, LV_STATE_DEFAULT, &DIN1451_m_cond_66);

    // Current Arc and label
    lv_style_copy(&current_indic_style, &arc_style);
    lv_style_copy(&current_main_style, &arc_style);
    lv_style_set_line_color(&current_main_style, LV_STATE_DEFAULT, current_bg_clr);

    lv_style_init(&current_label_style);
    lv_style_set_text_font(&current_label_style, LV_STATE_DEFAULT, &DIN1451_m_cond_44);

    // Temperature Arc
    lv_style_copy(&temp_indic_style, &arc_style);
    lv_style_copy(&temp_main_style, &arc_style);
    lv_style_set_line_color(&temp_main_style, LV_STATE_DEFAULT, temp_bg_clr);

    lv_style_init(&temp_label_style);
    lv_style_set_text_font(&temp_label_style, LV_STATE_DEFAULT, &DIN1451_m_cond_44);

    //Bar background -- transparent
    lv_style_copy(&bar_main_style, &arc_style);
    lv_style_set_line_opa(&bar_main_style, LV_STATE_DEFAULT, LV_OPA_TRANSP);
    //Max bar
    lv_style_copy(&max_bar_indic_style, &arc_style);
    lv_style_set_line_color(&max_bar_indic_style, LV_STATE_DEFAULT, max_bar_clr);
    //min bar
    lv_style_copy(&min_bar_indic_style, &arc_style);
    lv_style_set_line_color(&min_bar_indic_style, LV_STATE_DEFAULT, min_bar_clr);
    //regen bar
    lv_style_copy(&regen_bar_indic_style, &arc_style);
    lv_style_set_line_color(&regen_bar_indic_style, LV_STATE_DEFAULT, regen_bar_clr);

    // Clock and watch battery
    lv_style_init(&dashtime_style);
    lv_style_set_text_color(&dashtime_style, LV_STATE_DEFAULT, watch_info_colour);
    lv_style_set_text_font(&dashtime_style, LV_STATE_DEFAULT, &DIN1451_m_cond_28);

    //Trip meter
    lv_style_init(&trip_label_style);
    lv_style_set_text_color(&trip_label_style, LV_STATE_DEFAULT, current_fg_clr);
    lv_style_set_text_font(&trip_label_style, LV_STATE_DEFAULT, &DIN1451_m_cond_44);

    //alerts
    lv_style_copy(&alert_style, style);
    lv_style_set_bg_opa(&alert_style, LV_STATE_DEFAULT, LV_OPA_TRANSP);

    //overlay
    lv_style_copy(&overlay_style, style);
    lv_style_set_bg_color(&overlay_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_line_color(&overlay_style, LV_STATE_DEFAULT, LV_COLOR_RED);
    lv_style_set_line_width(&overlay_style, LV_STATE_DEFAULT, 20);
    lv_style_set_line_opa(&overlay_style, LV_STATE_DEFAULT, LV_OPA_20);
    lv_style_set_bg_opa(&overlay_style, LV_STATE_DEFAULT, LV_OPA_20);

} //End Define LVGL default object styles

/***************************
    Create Dashboard objects
 ***************************/

void lv_speed_arc_1(void)
{
    float tiltback_speed = wheelctl_get_data(WHEELCTL_TILTBACK);
    float current_speed = wheelctl_get_data(WHEELCTL_SPEED);

    speed_arc = lv_arc_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(speed_arc, LV_OBJ_PART_MAIN);
    lv_obj_add_style(speed_arc, LV_ARC_PART_INDIC, &speed_indic_style);
    lv_obj_add_style(speed_arc, LV_OBJ_PART_MAIN, &speed_main_style);
    lv_arc_set_bg_angles(speed_arc, speed_arc_start, speed_arc_end);
    lv_arc_set_range(speed_arc, 0, tiltback_speed + 5);
    lv_arc_set_value(speed_arc, current_speed);
    lv_obj_set_size(speed_arc, out_arc_x, out_arc_y);
    lv_obj_align(speed_arc, NULL, LV_ALIGN_CENTER, 0, 0);

    //Max bar
    speed_max_bar = lv_arc_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(speed_max_bar, LV_OBJ_PART_MAIN);
    lv_obj_add_style(speed_max_bar, LV_ARC_PART_INDIC, &max_bar_indic_style);
    lv_obj_add_style(speed_max_bar, LV_OBJ_PART_MAIN, &bar_main_style);
    lv_arc_set_bg_angles(speed_max_bar, speed_arc_start, speed_arc_end);
    lv_arc_set_range(speed_max_bar, 0, tiltback_speed + 5);
    lv_obj_set_size(speed_max_bar, out_arc_x, out_arc_y);
    lv_obj_align(speed_max_bar, NULL, LV_ALIGN_CENTER, 0, 0);

    //avg bar
    speed_avg_bar = lv_arc_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(speed_avg_bar, LV_OBJ_PART_MAIN);
    lv_obj_add_style(speed_avg_bar, LV_ARC_PART_INDIC, &min_bar_indic_style);
    lv_obj_add_style(speed_avg_bar, LV_OBJ_PART_MAIN, &bar_main_style);
    lv_arc_set_bg_angles(speed_avg_bar, speed_arc_start, speed_arc_end);
    lv_arc_set_range(speed_avg_bar, 0, tiltback_speed + 5);
    lv_obj_set_size(speed_avg_bar, out_arc_x, out_arc_y);
    lv_obj_align(speed_avg_bar, NULL, LV_ALIGN_CENTER, 0, 0);

    //Label
    speed_label = lv_label_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(speed_label, LV_OBJ_PART_MAIN);

    lv_obj_add_style(speed_label, LV_LABEL_PART_MAIN, &speed_label_style);
    char speedstring[4];
    if (current_speed > 10)
    {
        dtostrf(current_speed, 2, 0, speedstring);
    }
    else
    {
        dtostrf(current_speed, 1, 0, speedstring);
    }
    lv_label_set_text(speed_label, speedstring);
    lv_label_set_align(speed_label, LV_LABEL_ALIGN_CENTER);
    lv_obj_align(speed_label, speed_arc, LV_ALIGN_CENTER, 0, -5);
    mainbar_add_slide_element(speed_label);
}

void lv_batt_arc_1(void)
{
    /*Create battery gauge arc*/

    //Arc
    batt_arc = lv_arc_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(batt_arc, LV_OBJ_PART_MAIN);
    lv_obj_add_style(batt_arc, LV_ARC_PART_INDIC, &batt_indic_style);
    lv_obj_add_style(batt_arc, LV_OBJ_PART_MAIN, &batt_main_style);
    lv_arc_set_type(batt_arc, LV_ARC_TYPE_REVERSE);
    lv_arc_set_bg_angles(batt_arc, batt_arc_start, batt_arc_end);
    lv_arc_set_angles(batt_arc, batt_arc_start, batt_arc_end);
    lv_arc_set_range(batt_arc, 0, 100);
    lv_obj_set_size(batt_arc, out_arc_x, out_arc_y);
    lv_obj_align(batt_arc, NULL, LV_ALIGN_CENTER, 0, 0);

    //max bar
    batt_max_bar = lv_arc_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(batt_max_bar, LV_OBJ_PART_MAIN);
    lv_obj_add_style(batt_max_bar, LV_ARC_PART_INDIC, &max_bar_indic_style);
    lv_obj_add_style(batt_max_bar, LV_OBJ_PART_MAIN, &bar_main_style);
    lv_arc_set_bg_angles(batt_max_bar, batt_arc_start, batt_arc_end);
    lv_arc_set_range(batt_max_bar, 0, 100);
    lv_obj_set_size(batt_max_bar, out_arc_x, out_arc_y);
    lv_obj_align(batt_max_bar, NULL, LV_ALIGN_CENTER, 0, 0);

    //min bar
    batt_min_bar = lv_arc_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(batt_min_bar, LV_OBJ_PART_MAIN);
    lv_obj_add_style(batt_min_bar, LV_ARC_PART_INDIC, &min_bar_indic_style);
    lv_obj_add_style(batt_min_bar, LV_OBJ_PART_MAIN, &bar_main_style);
    lv_arc_set_bg_angles(batt_min_bar, batt_arc_start, batt_arc_end);
    lv_arc_set_range(batt_min_bar, 0, 100);
    lv_obj_set_size(batt_min_bar, out_arc_x, out_arc_y);
    lv_obj_align(batt_min_bar, NULL, LV_ALIGN_CENTER, 0, 0);

    //Label

    batt_label = lv_label_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(batt_label, LV_OBJ_PART_MAIN);
    lv_obj_add_style(batt_label, LV_OBJ_PART_MAIN, &batt_label_style);
    char battstring[4];
    dtostrf(wheelctl_get_data(WHEELCTL_BATTPCT), 2, 0, battstring);
    lv_label_set_text(batt_label, battstring);
    lv_label_set_align(batt_label, LV_LABEL_ALIGN_CENTER);
    lv_obj_align(batt_label, batt_arc, LV_ALIGN_CENTER, 0, 75);
}

void lv_current_arc_1(void)
{
    //Create current gauge arc
    byte maxcurrent = wheelctl_get_constant(WHEELCTL_CONST_MAXCURRENT);
    float current_current = wheelctl_get_data(WHEELCTL_CURRENT);

    //Arc
    current_arc = lv_arc_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(current_arc, LV_OBJ_PART_MAIN);
    lv_obj_add_style(current_arc, LV_ARC_PART_INDIC, &current_indic_style);
    lv_obj_add_style(current_arc, LV_OBJ_PART_MAIN, &current_main_style);
    lv_arc_set_bg_angles(current_arc, current_arc_start, current_arc_end);
    lv_arc_set_angles(current_arc, current_arc_start, current_arc_end);
    lv_arc_set_range(current_arc, 0, maxcurrent);
    lv_obj_set_size(current_arc, in_arc_x, in_arc_y);
    lv_obj_align(current_arc, NULL, LV_ALIGN_CENTER, 0, 0);

    //Max bar
    current_max_bar = lv_arc_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(current_max_bar, LV_OBJ_PART_MAIN);
    lv_obj_add_style(current_max_bar, LV_ARC_PART_INDIC, &max_bar_indic_style);
    lv_obj_add_style(current_max_bar, LV_OBJ_PART_MAIN, &bar_main_style);
    lv_arc_set_bg_angles(current_max_bar, current_arc_start, current_arc_end);
    lv_arc_set_range(current_max_bar, 0, maxcurrent);
    lv_obj_set_size(current_max_bar, in_arc_x, in_arc_y);
    lv_obj_align(current_max_bar, NULL, LV_ALIGN_CENTER, 0, 0);

    //regen bar
    current_regen_bar = lv_arc_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(current_regen_bar, LV_OBJ_PART_MAIN);
    lv_obj_add_style(current_regen_bar, LV_ARC_PART_INDIC, &regen_bar_indic_style);
    lv_obj_add_style(current_regen_bar, LV_OBJ_PART_MAIN, &bar_main_style);
    lv_arc_set_bg_angles(current_regen_bar, current_arc_start, current_arc_end);
    lv_arc_set_range(current_regen_bar, 0, maxcurrent);
    lv_obj_set_size(current_regen_bar, in_arc_x, in_arc_y);
    lv_obj_align(current_regen_bar, NULL, LV_ALIGN_CENTER, 0, 0);

    //Label
    current_label = lv_label_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(current_label, LV_OBJ_PART_MAIN);
    lv_obj_add_style(current_label, LV_OBJ_PART_MAIN, &current_label_style);
    char currentstring[4];
    dtostrf(current_current, 2, 0, currentstring);
    lv_label_set_text(current_label, currentstring);
    lv_label_set_align(current_label, LV_LABEL_ALIGN_CENTER);
    lv_obj_align(current_label, current_arc, LV_ALIGN_CENTER, -64, 0);
}

void lv_temp_arc_1(void)
{
    /*Create temprature gauge arc*/
    byte crit_temp = wheelctl_get_constant(WHEELCTL_CONST_CRITTEMP);
    float current_temp = wheelctl_get_data(WHEELCTL_TEMP);
    //Arc
    temp_arc = lv_arc_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(temp_arc, LV_OBJ_PART_MAIN);
    lv_obj_add_style(temp_arc, LV_ARC_PART_INDIC, &temp_indic_style);
    lv_obj_add_style(temp_arc, LV_OBJ_PART_MAIN, &temp_main_style);
    lv_arc_set_type(temp_arc, LV_ARC_TYPE_REVERSE);
    lv_arc_set_bg_angles(temp_arc, temp_arc_start, temp_arc_end);
    lv_arc_set_angles(temp_arc, temp_arc_start, temp_arc_end);
    lv_arc_set_range(temp_arc, 0, (crit_temp + 10));
    lv_arc_set_value(temp_arc, ((crit_temp + 10) - current_temp));
    lv_obj_set_size(temp_arc, in_arc_x, in_arc_y);
    lv_obj_align(temp_arc, NULL, LV_ALIGN_CENTER, 0, 0);
    mainbar_add_slide_element(temp_arc);

    //Max bar
    temp_max_bar = lv_arc_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(temp_max_bar, LV_OBJ_PART_MAIN);
    lv_obj_add_style(temp_max_bar, LV_ARC_PART_INDIC, &max_bar_indic_style);
    lv_obj_add_style(temp_max_bar, LV_OBJ_PART_MAIN, &bar_main_style);
    lv_arc_set_bg_angles(temp_max_bar, temp_arc_start, temp_arc_end);
    lv_arc_set_range(temp_max_bar, 0, (crit_temp + 10));
    lv_obj_set_size(temp_max_bar, in_arc_x, in_arc_y);
    lv_obj_align(temp_max_bar, NULL, LV_ALIGN_CENTER, 0, 0);
    mainbar_add_slide_element(temp_max_bar);

    //Label
    temp_label = lv_label_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(temp_label, LV_OBJ_PART_MAIN);
    lv_obj_add_style(temp_label, LV_OBJ_PART_MAIN, &temp_label_style);
    char tempstring[4];
    dtostrf(current_temp, 2, 0, tempstring);
    lv_label_set_text(temp_label, tempstring);
    lv_label_set_align(temp_label, LV_LABEL_ALIGN_CENTER);
    lv_obj_align(temp_label, temp_arc, LV_ALIGN_CENTER, 64, 0);
}

void lv_dashtime(void)
{
    dashtime = lv_label_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(dashtime, LV_OBJ_PART_MAIN);
    lv_obj_add_style(dashtime, LV_OBJ_PART_MAIN, &dashtime_style);
    lv_obj_align(dashtime, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
    wbatt = lv_label_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(wbatt, LV_OBJ_PART_MAIN);
    lv_obj_add_style(wbatt, LV_OBJ_PART_MAIN, &dashtime_style);
    lv_obj_align(wbatt, NULL, LV_ALIGN_IN_BOTTOM_RIGHT, 0, -25);
    trip = lv_label_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(trip, LV_OBJ_PART_MAIN);
    lv_obj_add_style(trip, LV_OBJ_PART_MAIN, &trip_label_style);
    lv_obj_align(trip, NULL, LV_ALIGN_IN_TOP_MID, 0, 25);
}

void lv_alerts(void) 
{
    batt_alert = lv_img_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(batt_alert, LV_OBJ_PART_MAIN);
    lv_obj_set_size(batt_alert, 128, 128);
    lv_obj_add_style(batt_alert, LV_OBJ_PART_MAIN, &alert_style);
    //lv_img_set_src(batt_alert, &batt_alert_128px);
    lv_obj_set_hidden(batt_alert, true);
    lv_obj_align(batt_alert, NULL, LV_ALIGN_IN_BOTTOM_LEFT, 0, 0);

    current_alert = lv_img_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(current_alert, LV_OBJ_PART_MAIN);
    lv_obj_set_size(current_alert, 128, 128);
    lv_obj_add_style(current_alert, LV_OBJ_PART_MAIN, &alert_style);
    //lv_img_set_src(current_alert, &current_alert_128px);
    lv_obj_set_hidden(current_alert, true);
    lv_obj_align(current_alert, NULL, LV_ALIGN_IN_TOP_LEFT, 0, 0);

    temp_alert = lv_img_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(temp_alert, LV_OBJ_PART_MAIN);
    lv_obj_set_size(temp_alert, 128, 128);
    lv_obj_add_style(temp_alert, LV_OBJ_PART_MAIN, &alert_style);
    //lv_img_set_src(temp_alert, &temp_alert_128px);
    lv_obj_set_hidden(temp_alert, true);
    lv_obj_align(temp_alert, NULL, LV_ALIGN_IN_TOP_RIGHT, 0, 0);
}

void lv_overlay(void)
{
    static lv_point_t line_points[] = {{0, lv_disp_get_ver_res(NULL)}, {lv_disp_get_hor_res(NULL), 0}};

    overlay_bar = lv_bar_create(fulldash_cont, NULL);
    lv_obj_reset_style_list(overlay_bar, LV_OBJ_PART_MAIN);
    lv_obj_set_size(overlay_bar, lv_disp_get_hor_res(NULL), lv_disp_get_ver_res(NULL));
    lv_obj_add_style(overlay_bar, LV_OBJ_PART_MAIN, &overlay_style);
    lv_obj_align(overlay_bar, NULL, LV_ALIGN_CENTER, 0, 0);
    mainbar_add_slide_element(overlay_bar);
    lv_obj_set_event_cb( overlay_bar, overlay_event_cb );

    overlay_line = lv_line_create(overlay_bar, NULL);
    lv_line_set_points(overlay_line, line_points, 2);
    lv_obj_reset_style_list(overlay_line, LV_OBJ_PART_MAIN);
    lv_obj_add_style(overlay_line, LV_OBJ_PART_MAIN, &overlay_style);
    lv_obj_align(overlay_line, NULL, LV_ALIGN_CENTER, 0, 0);
    mainbar_add_slide_element(overlay_line);
    lv_obj_set_event_cb( overlay_line, overlay_event_cb );

} //End Create Dashboard objects

static void overlay_event_cb(lv_obj_t * obj, lv_event_t event) {
    switch( event ) {
        case( LV_EVENT_LONG_PRESSED ):  Serial.println("long press on overlay");
        motor_vibe(5, true);
    }
}

int value2angle(int arcstart, int arcstop, float minvalue, float maxvalue, float arcvalue, bool reverse)
{
    int rAngle;
    int arcdegrees;
    if (arcstop < arcstart)
    {
        arcdegrees = (arcstop + 360) - arcstart;
    }
    else
    {
        arcdegrees = arcstop - arcstart;
    }
    if (reverse)
    {
        rAngle = arcstop - (arcvalue * arcdegrees / (maxvalue - minvalue));
    }
    else
    {
        rAngle = arcstart + (arcvalue * arcdegrees / (maxvalue - minvalue));
    }
    if (rAngle >= 360)
    {
        rAngle = rAngle - 360;
    }
    else if (rAngle < 0)
    {
        rAngle = rAngle + 360;
    }
    return rAngle;
}

/***************************************************************
   Dashboard GUI Update Functions, called via the task handler
   runs every 250ms
 ***************************************************************/

static void lv_speed_update(void)
{
    float tiltback_speed = wheelctl_get_data(WHEELCTL_TILTBACK);
    float current_speed = wheelctl_get_data(WHEELCTL_SPEED);
    float warn_speed = wheelctl_get_data(WHEELCTL_ALARM3);
    float top_speed = wheelctl_get_data(WHEELCTL_TOPSPEED);
    if (top_speed < tiltback_speed + 5) top_speed = tiltback_speed + 5;

    if (current_speed >= tiltback_speed)
    {
        lv_style_set_line_color(&speed_indic_style, LV_STATE_DEFAULT, LV_COLOR_RED);
        lv_style_set_text_color(&speed_label_style, LV_STATE_DEFAULT, LV_COLOR_RED);
    }
    else if (current_speed >= warn_speed)
    {
        lv_style_set_line_color(&speed_indic_style, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
        lv_style_set_text_color(&speed_label_style, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
    }
    else
    {
        lv_style_set_line_color(&speed_indic_style, LV_STATE_DEFAULT, speed_fg_clr);
        lv_style_set_text_color(&speed_label_style, LV_STATE_DEFAULT, speed_fg_clr);
    }

    lv_obj_add_style(speed_arc, LV_ARC_PART_INDIC, &speed_indic_style);
    lv_arc_set_range(speed_arc, 0, (tiltback_speed + 5));
    lv_arc_set_value(speed_arc, current_speed);

    int ang_max = value2angle(speed_arc_start, speed_arc_end, 0, (tiltback_speed + 5), wheelctl_get_data(WHEELCTL_TOPSPEED), false);
    int ang_max2 = ang_max + 3;
    if (ang_max2 >= 360)
    {
        ang_max2 = ang_max2 - 360;
    }
    lv_arc_set_angles(speed_max_bar, ang_max, ang_max2);

    int ang_avg = value2angle(speed_arc_start, speed_arc_end, 0, (tiltback_speed + 5), wheelctl_get_min_data(WHEELCTL_SPEED), false);
    int ang_avg2 = ang_avg + 3;
    if (ang_avg2 >= 360)
    {
        ang_avg2 = ang_avg2 - 360;
    }
    lv_arc_set_angles(speed_avg_bar, ang_avg, ang_avg2);

    lv_obj_add_style(speed_label, LV_LABEL_PART_MAIN, &speed_label_style);
    float converted_speed = current_speed;
    if (dashboard_get_config(DASHBOARD_IMPDIST))
    {
        converted_speed = current_speed / 1.6;
    }
    char speedstring[4];
    if (converted_speed > 10)
    {
        dtostrf(converted_speed, 2, 0, speedstring);
    }
    else
    {
        dtostrf(converted_speed, 1, 0, speedstring);
    }
    lv_label_set_text(speed_label, speedstring);
    lv_label_set_align(speed_label, LV_LABEL_ALIGN_CENTER);
    lv_obj_align(speed_label, fulldash_cont, LV_ALIGN_CENTER, 0, -3);
}

void lv_batt_update(void)
{
    float current_battpct = wheelctl_get_data(WHEELCTL_BATTPCT);

    if (current_battpct < 10)
    {
        lv_style_set_line_color(&batt_indic_style, LV_STATE_DEFAULT, LV_COLOR_RED);
        lv_style_set_text_color(&batt_label_style, LV_STATE_DEFAULT, LV_COLOR_RED);
    }
    else if (current_battpct < wheelctl_get_constant(WHEELCTL_CONST_BATTWARN))
    {
        lv_style_set_line_color(&batt_indic_style, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
        lv_style_set_text_color(&batt_label_style, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
    }
    else
    {
        lv_style_set_line_color(&batt_indic_style, LV_STATE_DEFAULT, batt_fg_clr);
        lv_style_set_text_color(&batt_label_style, LV_STATE_DEFAULT, batt_fg_clr);
    }
    lv_obj_add_style(batt_arc, LV_ARC_PART_INDIC, &batt_indic_style);

    // draw batt arc

    lv_arc_set_value(batt_arc, (100 - current_battpct));

    int ang_max = value2angle(batt_arc_start, batt_arc_end, 0, 100, wheelctl_get_max_data(WHEELCTL_BATTPCT), rev_batt_arc);
    int ang_max2 = ang_max + 3;
    if (ang_max2 >= 360)
    {
        ang_max2 = ang_max2 - 360;
    }
    lv_arc_set_angles(batt_max_bar, ang_max, ang_max2);

    int ang_min = value2angle(batt_arc_start, batt_arc_end, 0, 100, wheelctl_get_min_data(WHEELCTL_BATTPCT), rev_batt_arc);
    int ang_min2 = ang_min + 3;
    if (ang_min2 >= 360)
    {
        ang_min2 = ang_min2 - 360;
    }
    lv_arc_set_angles(batt_min_bar, ang_min, ang_min2);

    lv_obj_add_style(batt_label, LV_OBJ_PART_MAIN, &batt_label_style);
    char battstring[4];
    if (current_battpct > 10)
    {
        dtostrf(current_battpct, 2, 0, battstring);
    }
    else
    {
        dtostrf(current_battpct, 1, 0, battstring);
    }
    lv_label_set_text(batt_label, battstring);
    lv_label_set_align(batt_label, LV_LABEL_ALIGN_CENTER);
    lv_obj_align(batt_label, fulldash_cont, LV_ALIGN_CENTER, 0, 75);
}

void lv_current_update(void)
{
    // Set warning and alert colour
    byte maxcurrent = wheelctl_get_constant(WHEELCTL_CONST_MAXCURRENT);
    float current_current = wheelctl_get_data(WHEELCTL_CURRENT);
    float amps = current_current;
    
    if (current_current > (maxcurrent * 0.75))
    {
        lv_style_set_line_color(&current_indic_style, LV_STATE_DEFAULT, LV_COLOR_RED);
        lv_style_set_text_color(&current_label_style, LV_STATE_DEFAULT, LV_COLOR_RED);
    }
    else if (current_current > (maxcurrent * 0.5))
    {
        lv_style_set_line_color(&current_indic_style, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
        lv_style_set_text_color(&current_label_style, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
    }
    else if (current_current < 0)
    {
        lv_style_set_line_color(&current_indic_style, LV_STATE_DEFAULT, speed_fg_clr);
        lv_style_set_text_color(&current_label_style, LV_STATE_DEFAULT, speed_fg_clr);
        amps = (current_current * -1);
    }
    else
    {
        lv_style_set_line_color(&current_indic_style, LV_STATE_DEFAULT, current_fg_clr);
        lv_style_set_text_color(&current_label_style, LV_STATE_DEFAULT, current_fg_clr);
    }
    lv_obj_add_style(current_arc, LV_ARC_PART_INDIC, &current_indic_style);

    lv_arc_set_value(current_arc, amps);

    int ang_max = value2angle(current_arc_start, current_arc_end, 0, maxcurrent, wheelctl_get_max_data(WHEELCTL_CURRENT), rev_current_arc);
    int ang_max2 = ang_max + 3;
    if (ang_max2 >= 360)
    {
        ang_max2 = ang_max2 - 360;
    }
    lv_arc_set_angles(current_max_bar, ang_max, ang_max2);

    int ang_regen = value2angle(current_arc_start, current_arc_end, 0, maxcurrent, wheelctl_get_min_data(WHEELCTL_CURRENT), rev_current_arc);
    int ang_regen2 = ang_regen + 3;
    if (ang_regen2 >= 360)
    {
        ang_regen2 = ang_regen2 - 360;
    }
    lv_arc_set_angles(current_regen_bar, ang_regen, ang_regen2);

    lv_obj_add_style(current_label, LV_OBJ_PART_MAIN, &current_label_style);
    char currentstring[4];
    dtostrf(current_current, 2, 0, currentstring);
    lv_label_set_text(current_label, currentstring);
    lv_label_set_align(current_label, LV_LABEL_ALIGN_CENTER);
    lv_obj_align(current_label, fulldash_cont, LV_ALIGN_CENTER, -64, 0);
}

void lv_temp_update(void)
{
    byte crit_temp = wheelctl_get_constant(WHEELCTL_CONST_CRITTEMP);
    float current_temp = wheelctl_get_data(WHEELCTL_TEMP);
    // Set warning and alert colour
    if (current_temp > crit_temp)
    {
        lv_style_set_line_color(&temp_indic_style, LV_STATE_DEFAULT, LV_COLOR_RED);
        lv_style_set_text_color(&temp_label_style, LV_STATE_DEFAULT, LV_COLOR_RED);
    }
    else if (current_temp > wheelctl_get_constant(WHEELCTL_CONST_WARNTEMP))
    {
        lv_style_set_line_color(&temp_indic_style, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
        lv_style_set_text_color(&temp_label_style, LV_STATE_DEFAULT, LV_COLOR_YELLOW);
    }
    else
    {
        lv_style_set_line_color(&temp_indic_style, LV_STATE_DEFAULT, temp_fg_clr);
        lv_style_set_text_color(&temp_label_style, LV_STATE_DEFAULT, temp_fg_clr);
    }
    lv_obj_add_style(temp_arc, LV_ARC_PART_INDIC, &temp_indic_style);
    lv_arc_set_value(temp_arc, ((crit_temp + 10) - current_temp));

    int ang_max = value2angle(temp_arc_start, temp_arc_end, 0, (crit_temp + 10), wheelctl_get_max_data(WHEELCTL_TEMP), true);
    int ang_max2 = ang_max + 3;
    if (ang_max2 >= 360)
    {
        ang_max2 = ang_max2 - 360;
    }
    lv_arc_set_angles(temp_max_bar, ang_max, ang_max2);

    lv_obj_add_style(temp_label, LV_OBJ_PART_MAIN, &temp_label_style);
    char tempstring[4];
    float converted_temp = current_temp;
    if (dashboard_get_config(DASHBOARD_IMPTEMP))
    {
        converted_temp = (current_temp * 1.8) + 32;
    }
    dtostrf(converted_temp, 2, 0, tempstring);
    lv_label_set_text(temp_label, tempstring);
    lv_label_set_align(temp_label, LV_LABEL_ALIGN_CENTER);
    lv_obj_align(temp_label, fulldash_cont, LV_ALIGN_CENTER, 64, 0);
} // update

void lv_overlay_update()
{
    if (blectl_cli_getconnected())
    {
        lv_style_set_line_opa(&overlay_style, LV_STATE_DEFAULT, LV_OPA_TRANSP);
        lv_style_set_bg_opa(&overlay_style, LV_STATE_DEFAULT, LV_OPA_TRANSP);
        lv_obj_add_style(overlay_bar, LV_OBJ_PART_MAIN, &overlay_style);
        lv_obj_add_style(overlay_line, LV_OBJ_PART_MAIN, &overlay_style);
    }
    else
    {
        lv_style_set_line_opa(&overlay_style, LV_STATE_DEFAULT, LV_OPA_20);
        lv_style_set_bg_opa(&overlay_style, LV_STATE_DEFAULT, LV_OPA_20);
        lv_obj_add_style(overlay_bar, LV_OBJ_PART_MAIN, &overlay_style);
        lv_obj_add_style(overlay_line, LV_OBJ_PART_MAIN, &overlay_style);
    }
}

void updateTime()
{
    // TTGOClass *ttgo = TTGOClass::getWatch();
    time_t now;
    struct tm info;
    char buf[64];
    time(&now);

    localtime_r(&now, &info);
    strftime(buf, sizeof(buf), "%H:%M", &info);
    //int watchbatt = ttgo->power->getBattPercentage();

    int32_t watchbatt = pmu_get_battery_percent();
    if (watchbatt > 99) watchbatt = 99;

    if (dashtime != nullptr)
    {
        lv_label_set_text(dashtime, buf);
        lv_obj_align(dashtime, fulldash_cont, LV_ALIGN_IN_BOTTOM_RIGHT, 0, 0);
    }
    if (wbatt != nullptr)
    {
        char wbattstring[4];
        dtostrf(watchbatt, 2, 0, wbattstring);
        lv_label_set_text(wbatt, wbattstring);
        lv_obj_align(wbatt, fulldash_cont, LV_ALIGN_IN_BOTTOM_RIGHT, 0, -25);
    }
    if (trip != nullptr)
    {
        char tripstring[6];
        float converted_trip = wheelctl_get_data(WHEELCTL_TRIP);
        if (dashboard_get_config(DASHBOARD_IMPDIST))
        {
            converted_trip = wheelctl_get_data(WHEELCTL_TRIP) / 1.6;
        }
        dtostrf(converted_trip, 2, 1, tripstring);
        lv_label_set_text(trip, tripstring);
        lv_obj_align(trip, fulldash_cont, LV_ALIGN_IN_TOP_MID, 0, 25);
    }
    //ttgo->rtc->syncToRtc();
}

/************************
   Task update functions
 ***********************/

static void lv_dash_task(lv_task_t *dash_task)
{
    if (blectl_cli_getconnected())
    {
        lv_speed_update();
        lv_batt_update();
        lv_current_update();
        lv_temp_update();
    }
    lv_overlay_update();
}

static void lv_time_task(lv_task_t *time_task)
{
    updateTime();
}

void stop_dash_task()
{
    Serial.println("check if dash is running");
    if (dash_task != nullptr)
    {
        Serial.println("shutting down dash");
        lv_task_del(dash_task);
    }
    if (time_task != nullptr)
    {
        Serial.println("shutting down dashclock");
        lv_task_del(time_task);
    }
}

uint32_t fulldash_get_tile(void)
{
    return fulldash_tile_num;
}

void fulldash_activate_cb(void)
{
    time_task = lv_task_create(lv_time_task, 2000, LV_TASK_PRIO_LOWEST, NULL);
    lv_task_ready(time_task);
    dash_task = lv_task_create(lv_dash_task, 250, LV_TASK_PRIO_LOWEST, NULL);
    lv_task_ready(dash_task);
}

void fulldash_hibernate_cb(void)
{
    lv_task_del(time_task);
    lv_task_del(dash_task);
}

void fulldash_tile_reload(void)
{
    lv_obj_del(fulldash_cont);
    fulldash_tile_setup();
}

void fulldash_tile_setup(void)
{
    fulldash_tile_num = mainbar_add_tile(1, 0, "fd tile");
    fulldash_cont = mainbar_get_tile_obj(fulldash_tile_num);
    //fulldash_cont = mainbar_get_tile_obj( mainbar_add_tile( 1, 0, "fulldash tile" ) );
    style = mainbar_get_style();
    Serial.println("setting up dashboard");
    lv_define_styles_1();
    lv_speed_arc_1();
    lv_batt_arc_1();
    lv_current_arc_1();
    lv_temp_arc_1();
    lv_dashtime();
    lv_overlay();

    mainbar_add_tile_activate_cb(fulldash_tile_num, fulldash_activate_cb);
    mainbar_add_tile_hibernate_cb(fulldash_tile_num, fulldash_hibernate_cb);
}