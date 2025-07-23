#include "pm_utils.h"

/**
 * @brief 重置LVGL对象的样式：边框宽度、圆角半径、内边距均设为0
 *
 * @param obj 目标LVGL对象
 */
void pm_reset_style(lv_obj_t *obj)
{
  lv_obj_set_style_border_width(obj, 0, LV_STATE_DEFAULT); // 取消边框
  lv_obj_set_style_radius(obj, 0, LV_STATE_DEFAULT);       // 取消圆角
  lv_obj_set_style_pad_all(obj, 0, LV_STATE_DEFAULT);      // 取消所有内边距
}