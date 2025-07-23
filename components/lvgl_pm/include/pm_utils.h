#ifndef PM_UTILS_H
#define PM_UTILS_H

#include "lvgl.h"

/**
 * @brief 重置LVGL对象的样式（边框宽度、圆角半径、内边距）为0
 * @param obj 需要重置样式的LVGL对象指针
 */
void pm_reset_style(lv_obj_t *obj);

#endif