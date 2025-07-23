#ifndef PM_H
#define PM_H

#include "lvgl.h"
#include <stdint.h>

/**
 * 页面管理模块版本号定义
 */
#define LV_PM_MAJOR 0
#define LV_PM_MINOR 1
#define LV_PM_PATCH 1

/**
 * 生命周期回调类型，参数为页面对象指针
 */
typedef void (*lv_pm_lifecycle)(lv_obj_t *page);

/**
 * 页面动画类型枚举
 */
enum LV_PM_PAGE_ANIMA {
  LV_PM_ANIMA_NONE = 0,        // 无动画
  LV_PM_ANIMA_SLIDE = 1,       // 滑动动画
  LV_PM_ANIMA_SLIDE_SCALE = 2, // 缩放滑动动画
  LV_PM_ANIMA_POPUP = 3        // 弹窗动画
};

/**
 * 动画方向枚举
 */
enum LV_PM_ANIMA_DIR {
  LV_PM_ANIMA_TOP = 0,    // 向上
  LV_PM_ANIMA_RIGHT = 1,  // 向右
  LV_PM_ANIMA_BOTTOM = 2, // 向下
  LV_PM_ANIMA_LEFT = 3    // 向左
};

/**
 * 页面打开目标方式枚举
 */
enum LV_PM_OPEN_TARGET {
  LV_PM_TARGET_NEW = 0,   // 新开页面
  LV_PM_TARGET_SELF = 1,  // 替换当前页面
  LV_PM_TARGET_RESET = 2  // 关闭所有页面并新开
};

/**
 * 页面打开选项结构体
 */
typedef struct _lv_pm_open_options_t
{
  enum LV_PM_PAGE_ANIMA animation; // 动画类型
  enum LV_PM_OPEN_TARGET target;   // 打开目标方式
  enum LV_PM_ANIMA_DIR direction;  // 动画方向
} lv_pm_open_options_t;

/**
 * 页面信息结构体
 */
typedef struct _lv_pm_page_t
{
  lv_obj_t *page;                 // 页面对象指针
  lv_pm_lifecycle onLoad;         // 页面加载时回调
  lv_pm_lifecycle willAppear;     // 页面出现前回调
  lv_pm_lifecycle didAppear;      // 页面出现后回调
  lv_pm_lifecycle willDisappear;  // 页面消失前回调
  lv_pm_lifecycle didDisappear;   // 页面消失后回调
  lv_pm_lifecycle unLoad;         // 页面卸载时回调
  lv_pm_open_options_t _options;  // 当前页面的打开选项
  bool _back;                     // 是否为返回操作
} lv_pm_page_t;

/**
 * 页面历史相关全局变量
 */
extern uint8_t lv_pm_history_len; ///< 页面历史长度

#ifndef LV_PM_PAGE_NUM
#define LV_PM_PAGE_NUM 10 ///< 最多支持的页面数量
#endif

extern lv_pm_page_t *lv_pm_router[LV_PM_PAGE_NUM]; ///< 页面路由表
extern uint8_t lv_pm_history[LV_PM_PAGE_NUM];      ///< 页面历史记录

/**
 * @brief 页面管理初始化
 * @return 0 成功
 */
uint8_t lv_pm_init();

/**
 * @brief 创建页面并添加到路由表
 * @param id 页面ID
 * @return 页面结构体指针或NULL
 */
lv_pm_page_t *lv_pm_create_page(uint8_t id);

/**
 * @brief 打开页面
 * @param id 页面ID
 * @param behavior 打开行为参数，可为NULL
 * @return 0 成功，其他为错误码
 */
uint8_t lv_pm_open_page(uint8_t id, lv_pm_open_options_t *behavior);

/**
 * @brief 返回上一个页面
 * @return 0 成功，其他为错误码
 */
uint8_t lv_pm_back();

#endif