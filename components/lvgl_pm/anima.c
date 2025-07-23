#include "anima.h"
#include "pm.h"

#include "lvgl.h"
#include <stdlib.h>

#define POPUP_TOP_HEIGHT 15

// 全局动画对象（lvgl动画结构体）
lv_anim_t appear_anima;     // 页面出现动画
lv_anim_t disAppear_anima;  // 页面消失动画

/**
 * @brief 动画数据结构，传递动画所需的回调和页面信息
 */
typedef struct _lv_pm_anima_data {
  lv_pm_page_t *pm_page;              // 页面对象指针
  lv_pm_anima_complete_cb cb;         // 动画完成回调
  lv_pm_open_options_t options;       // 页面打开选项
} lv_pm_anima_data;

/**
 * @brief X方向平移动画回调
 * @param var 页面对象
 * @param v   x坐标
 */
static void translateX_anima_cb(void *var, int32_t v)
{
  lv_obj_set_x(var, v);
}

/**
 * @brief Y方向平移动画回调
 * @param var 页面对象
 * @param v   y坐标
 */
static void translateY_anima_cb(void *var, int32_t v)
{
  lv_obj_set_y(var, v);
}

/**
 * @brief 动画结束回调，调用页面动画完成回调，并释放动画数据
 * @param anim 动画对象
 */
static void anima_ready_cb(lv_anim_t *anim)
{
  lv_pm_anima_data *cb_data = (lv_pm_anima_data *)anim->user_data;
  cb_data->cb(cb_data->pm_page, cb_data->options);
  free(anim->user_data);
}

/**
----------------------------------------------------------------------------------------------------------
  slide animation
----------------------------------------------------------------------------------------------------------
*/

/**
 * @brief 页面滑入动画（出现）
 * @param anima_data 动画数据
 *
 * 从左或右滑动页面进入
 */
static void _pm_slide_appear(lv_pm_anima_data *anima_data)
{
  lv_coord_t width = lv_disp_get_hor_res(NULL);

  lv_anim_init(&appear_anima);
  lv_anim_set_user_data(&appear_anima, (void *)anima_data);
  lv_anim_set_var(&appear_anima, anima_data->pm_page->page);

  if (anima_data->pm_page->_back) {
    // 返回时从左滑入
    lv_anim_set_values(&appear_anima, -width, 0);
  } else {
    // 正常打开从右滑入
    lv_anim_set_values(&appear_anima, width, 0);
  }

  lv_anim_set_path_cb(&appear_anima, lv_anim_path_ease_out);
  lv_anim_set_time(&appear_anima, 500);
  lv_anim_set_repeat_count(&appear_anima, 1);
  lv_anim_set_exec_cb(&appear_anima, translateX_anima_cb);
  lv_anim_set_ready_cb(&appear_anima, anima_ready_cb);
  lv_anim_start(&appear_anima);
}

/**
 * @brief 页面滑出动画（消失）
 * @param anima_data 动画数据
 *
 * 向左或向右滑动页面离开
 */
static void _pm_slide_disAppear(lv_pm_anima_data *anima_data)
{
  lv_coord_t width = lv_disp_get_hor_res(NULL);

  lv_anim_init(&disAppear_anima);
  lv_anim_set_user_data(&disAppear_anima, (void *)anima_data);
  lv_anim_set_var(&disAppear_anima, anima_data->pm_page->page);

  if (anima_data->pm_page->_back) {
    // 返回时向右滑出
    lv_anim_set_values(&disAppear_anima, 0, width);
  } else {
    // 正常关闭向左滑出
    lv_anim_set_values(&disAppear_anima, 0, -width);
  }

  lv_anim_set_time(&disAppear_anima, 500);
  lv_anim_set_repeat_count(&disAppear_anima, 1);
  lv_anim_set_exec_cb(&disAppear_anima, translateX_anima_cb);
  lv_anim_set_ready_cb(&disAppear_anima, anima_ready_cb);
  lv_anim_set_path_cb(&disAppear_anima, lv_anim_path_ease_out);
  lv_anim_start(&disAppear_anima);
}

/** ------------------------------------slide animation end-------------------------------------------- */


/**
----------------------------------------------------------------------------------------------------------
  popup animation
----------------------------------------------------------------------------------------------------------
*/

/**
 * @brief popup页面出现动画（从下往上弹出）
 * @param anima_data 动画数据
 */
static void _pm_popup_appear(lv_pm_anima_data *anima_data)
{
  lv_coord_t height = lv_disp_get_ver_res(NULL);

  lv_anim_init(&appear_anima);
  lv_anim_set_user_data(&appear_anima, (void *)anima_data);
  lv_anim_set_var(&appear_anima, anima_data->pm_page->page);

  if (anima_data->pm_page->_back) {
    // 返回弹窗时小幅度弹出
    lv_anim_set_values(&appear_anima, 5, 0);
    lv_obj_set_style_radius(anima_data->pm_page->page, 0, LV_STATE_DEFAULT);
  } else {
    // 正常弹窗从屏幕底部弹出到顶部
    lv_anim_set_values(&appear_anima, height, POPUP_TOP_HEIGHT);
    lv_obj_set_style_radius(anima_data->pm_page->page, 10, LV_STATE_DEFAULT);
  }

  lv_anim_set_path_cb(&appear_anima, lv_anim_path_ease_out);
  lv_anim_set_time(&appear_anima, 500);
  lv_anim_set_repeat_count(&appear_anima, 1);
  lv_anim_set_exec_cb(&appear_anima, translateY_anima_cb);
  lv_anim_set_ready_cb(&appear_anima, anima_ready_cb);
  lv_anim_start(&appear_anima);
}

/**
 * @brief popup页面消失动画（向下弹回）
 * @param anima_data 动画数据
 */
static void _pm_popup_disAppear(lv_pm_anima_data *anima_data)
{
  lv_coord_t height = lv_disp_get_ver_res(NULL);

  lv_anim_init(&disAppear_anima);
  lv_anim_set_user_data(&disAppear_anima, (void *)anima_data);
  lv_anim_set_var(&disAppear_anima, anima_data->pm_page->page);

  if (anima_data->pm_page->_back) {
    // 返回时弹窗从顶部回到底部
    lv_anim_set_values(&disAppear_anima, POPUP_TOP_HEIGHT, height);
    lv_obj_set_style_radius(anima_data->pm_page->page, 0, LV_STATE_DEFAULT);
  } else {
    // 正常关闭时微量下移再消失
    lv_anim_set_values(&disAppear_anima, 0, 5);
    lv_obj_set_style_radius(anima_data->pm_page->page, 10, LV_STATE_DEFAULT);
  }

  lv_anim_set_time(&disAppear_anima, 500);
  lv_anim_set_repeat_count(&disAppear_anima, 1);
  lv_anim_set_exec_cb(&disAppear_anima, translateY_anima_cb);
  lv_anim_set_ready_cb(&disAppear_anima, anima_ready_cb);
  lv_anim_set_path_cb(&disAppear_anima, lv_anim_path_ease_out);
  lv_anim_start(&disAppear_anima);
}

/** ------------------------------------popup animation end-------------------------------------------- */

/**
 * @brief 页面出现动画启动入口
 * @param pm_page 页面对象
 * @param behavior 打开选项
 * @param cb 动画完成回调
 *
 * 根据动画类型执行不同动画
 */
void _pm_anima_appear(lv_pm_page_t *pm_page, lv_pm_open_options_t *behavior, lv_pm_anima_complete_cb cb)
{
  if (behavior == NULL || behavior->animation == LV_PM_ANIMA_NONE) {
    cb(pm_page, *behavior);
    return;
  }

  lv_pm_anima_data *anima_data = (lv_pm_anima_data *)malloc(sizeof(lv_pm_anima_data));
  if (anima_data == NULL) {
    cb(pm_page, *behavior);
    return;
  }

  anima_data->pm_page = pm_page;
  anima_data->cb = cb;
  anima_data->options = *behavior;

  switch (behavior->animation)
  {
  case LV_PM_ANIMA_SLIDE:
    _pm_slide_appear(anima_data);
    break;
  case LV_PM_ANIMA_POPUP:
    _pm_popup_appear(anima_data);
    break;
  default:
    cb(pm_page, *behavior);
    break;
  }
}

/**
 * @brief 页面消失动画启动入口
 * @param pm_page 页面对象
 * @param behavior 打开选项
 * @param cb 动画完成回调
 *
 * 根据动画类型执行不同动画
 */
void _pm_anima_disAppear(lv_pm_page_t *pm_page, lv_pm_open_options_t *behavior, lv_pm_anima_complete_cb cb)
{
  if (behavior == NULL || behavior->animation == LV_PM_ANIMA_NONE) {
    cb(pm_page, *behavior);
    return;
  }

  lv_pm_anima_data *anima_data = (lv_pm_anima_data *)malloc(sizeof(lv_pm_anima_data));
  if (anima_data == NULL) {
    cb(pm_page, *behavior);
    return;
  }

  anima_data->pm_page = pm_page;
  anima_data->cb = cb;
  anima_data->options = *behavior;

  switch (behavior->animation)
  {
  case LV_PM_ANIMA_SLIDE:
    _pm_slide_disAppear(anima_data);
    break;
  case LV_PM_ANIMA_POPUP:
    _pm_popup_disAppear(anima_data);
    break;
  default:
    cb(pm_page, *behavior);
    break;
  }
}