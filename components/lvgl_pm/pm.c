#include "pm.h"
#include "anima.h"
#include "pm_utils.h"

#include <stdlib.h>
#include <stdio.h>

// 定义全局变量
uint8_t lv_pm_history_len;
lv_pm_page_t *lv_pm_router[LV_PM_PAGE_NUM];
uint8_t lv_pm_history[LV_PM_PAGE_NUM];

/**
 * @brief 页面出现动画完成的回调函数
 *
 * @param pm_page 当前页面结构体指针
 * @param options 页面打开选项
 *
 * 如果页面实现了 didAppear 回调，则调用它
 */
static void _appear_complete_cb(lv_pm_page_t *pm_page, lv_pm_open_options_t options) {
    if (pm_page->didAppear) {
        pm_page->didAppear(pm_page->page);
    }
}

/**
 * @brief 返回时页面出现动画完成的回调函数
 *
 * @param pm_page 当前页面结构体指针
 * @param options 页面打开选项
 *
 * 与 _appear_complete_cb 类似，也是调用 didAppear 回调
 */
static void _back_appear_complete_cb(lv_pm_page_t *pm_page, lv_pm_open_options_t options) {
    if (pm_page->didAppear) {
        pm_page->didAppear(pm_page->page);
    }
}

/**
 * @brief 页面消失动画完成的回调函数
 *
 * @param pm_page 当前页面结构体指针
 * @param options 页面打开选项
 *
 * 1. 如果不是弹窗动画，则隐藏页面
 * 2. 如果页面实现了 didDisappear 回调，则调用它
 * 3. 如果目标是自己（SELF），卸载并清理页面对象
 */
static void _disAppear_complete_cb(lv_pm_page_t *pm_page, lv_pm_open_options_t options) {
    if (options.animation != LV_PM_ANIMA_POPUP) {
        lv_obj_add_flag(pm_page->page, LV_OBJ_FLAG_HIDDEN);
    }
    if (pm_page->didDisappear) {
        pm_page->didDisappear(pm_page->page);
    }
    if (options.target == LV_PM_TARGET_SELF) {
        pm_page->unLoad(pm_page->page);
        lv_obj_clean(pm_page->page);
    }
}

/**
 * @brief 返回时页面消失动画完成的回调函数
 *
 * @param pm_page 当前页面结构体指针
 * @param options 页面打开选项
 *
 * 1. 隐藏页面
 * 2. 如果页面实现了 didDisappear 回调，则调用它
 * 3. 卸载并清理页面对象
 */
static void _back_disAppear_complete_cb(lv_pm_page_t *pm_page, lv_pm_open_options_t options) {
    lv_obj_add_flag(pm_page->page, LV_OBJ_FLAG_HIDDEN);
    if (pm_page->didDisappear) {
        pm_page->didDisappear(pm_page->page);
    }
    pm_page->unLoad(pm_page->page);
    lv_obj_clean(pm_page->page);
}

/**
 * @brief 页面管理模块初始化
 * @return 总是返回0，表示成功
 * 1. 清空历史页面长度
 * 2. 将路由数组置为0
 * 3. 获取当前屏幕对象，关闭滚动条
 */
uint8_t lv_pm_init() {
    lv_pm_history_len = 0;
    for (uint8_t i = 0; i < LV_PM_PAGE_NUM; i++) {
        lv_pm_router[i] = 0;
    }

    lv_obj_t *screen = lv_scr_act();
    // 关闭滚动条
    lv_obj_set_scrollbar_mode(screen, LV_SCROLLBAR_MODE_OFF);

    return 0;
}

/**
 * @brief 创建一个新的页面对象并添加到路由表
 *
 * @param id 页面ID
 * @return 指向新页面结构体的指针，失败返回NULL
 *
 * 1. 申请并初始化页面结构体内存
 * 2. 创建一个lvgl页面对象，设置其风格及尺寸
 * 3. 将页面对象加入路由表
 */
lv_pm_page_t *lv_pm_create_page(uint8_t id) {
    lv_pm_page_t *pm_page = (lv_pm_page_t *) malloc(sizeof(lv_pm_page_t));
    if (pm_page == NULL) {
        return NULL;
    }
    memset(pm_page, 0, sizeof(lv_pm_page_t));

    lv_pm_router[id] = pm_page; // 将页面对象添加到当前页面ID对应的路由表
    lv_obj_t *page = lv_obj_create(lv_scr_act());
    // 重置样式，如圆角等
    pm_reset_style(page);
    // 默认隐藏
    lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);
    // 设置页面宽高为屏幕分辨率
    lv_coord_t width = lv_disp_get_hor_res(NULL);
    lv_coord_t height = lv_disp_get_ver_res(NULL);
    lv_obj_set_width(page, width);
    lv_obj_set_height(page, height);

    pm_page->page = page; // 往页面结构体中添加页面对象指针
    return pm_page;
}

/**
 * @brief 打开指定ID的页面
 *
 * @param id 页面ID
 * @param behavior 页面打开行为参数，允许为NULL
 * @return 0成功，其余为错误码
 *
 * 1. 检查页面是否存在以及页面历史是否已满
 * 2. 将页面ID加入历史
 * 3. 更新页面选项
 * 4. 处理前一个页面的willDisappear和动画
 * 5. 调用当前页面onLoad、willAppear及出现动画
 * 6. 更新历史长度
 */
uint8_t lv_pm_open_page(uint8_t id, lv_pm_open_options_t *behavior) {
    if (lv_pm_router[id] == 0) {
        return 4; // 页面未注册
    }
    if (lv_pm_history_len == LV_PM_PAGE_NUM) {
        return 5; // 页面历史已满
    }
    lv_pm_history[lv_pm_history_len] = id; // 将页面ID加入历史
    lv_pm_page_t *pm_page = lv_pm_router[id]; // 从路由表中获取页面对象
    lv_obj_t *page = pm_page->page; // 获取页面对象指针
    // 如果存在行为参数，则更新页面对象
    if (behavior) {
        pm_page->_options = *behavior;
    }
    pm_page->_back = false; // 标记为非返回操作

    // 如果不是首次打开，处理前一个页面的消失动画
    if (lv_pm_history_len > 0) {
        uint8_t pid = lv_pm_history[lv_pm_history_len - 1]; // 获取当前页面ID
        lv_pm_page_t *prev_pm_page = lv_pm_router[pid]; // 获取当前页面对象
        lv_obj_t *prev_page = prev_pm_page->page; // 获取当前页面对象指针
        prev_pm_page->_back = false; // 标记为非返回操作
        // 如果前一个页面存在willDisappear回调，则调用它
        if (prev_pm_page->willDisappear) {
            prev_pm_page->willDisappear(prev_page);
        }
        // 执行前一个页面的消失动画，并在动画完成后调用回调cb
        _pm_anima_disAppear(prev_pm_page, &pm_page->_options, _disAppear_complete_cb);
    }

    // 加载并显示新页面
    pm_page->onLoad(page); // 页面初始化
    lv_obj_clear_flag(page, LV_OBJ_FLAG_HIDDEN); // 取消页面隐藏标记以显示页面
    // 调用willAppear生命周期回调
    if (pm_page->willAppear) {
        pm_page->willAppear(page);
    }
    // 启动页面出现动画，动画完成调用后只需cb
    _pm_anima_appear(pm_page, &pm_page->_options, _appear_complete_cb);

    // 更新历史栈
    // 如果是“替换当前页面”（target == SELF），会覆盖历史栈顶部的id
    if (behavior && behavior->target == LV_PM_TARGET_SELF) {
        if (lv_pm_history_len == 0) {
            lv_pm_history_len++;
        } else {
            lv_pm_history[lv_pm_history_len - 1] = lv_pm_history[lv_pm_history_len];
        }
    } else {
        // 否则正常增加历史长度
        lv_pm_history_len++;
    }

    return 0;
}

/**
 * @brief 返回到上一个页面
 *
 * @return 0表示成功，其余为错误码
 *
 * 1. 历史长度小于2则不能返回
 * 2. 当前页面调用willDisappear和消失动画
 * 3. 历史长度减1
 * 4. 上一个页面调用willAppear和出现动画
 */
uint8_t lv_pm_back() {
    if (lv_pm_history_len < 2) {
        return 0; // 没有上一个页面，不能返回
    }
    uint8_t pid = lv_pm_history[lv_pm_history_len - 1]; // 取出当前页面对象ID
    lv_pm_page_t *pm_page = lv_pm_router[pid]; // 根据ID从路由表中获取页面对象
    pm_page->_back = true; // 标记返回操作
    lv_obj_t *page = pm_page->page; // 获取页面指针

    // 如果存在willDisappear则调用
    if (pm_page->willDisappear) {
        pm_page->willDisappear(page);
    }
    // 在启动页面消失动画后，执行回调
    _pm_anima_disAppear(pm_page, &pm_page->_options, _back_disAppear_complete_cb);

    lv_pm_history_len--; // 历史长度减一，把当前页面"弹出"历史堆栈

    // 获取上一个页面，并准备显示
    uint8_t prev_pid = lv_pm_history[lv_pm_history_len - 1];
    lv_pm_page_t *prev_pm_page = lv_pm_router[prev_pid];
    lv_obj_t *prev_page = prev_pm_page->page;
    prev_pm_page->_back = true;

    // 上一个页面 willAppear、显示和动画
    if (prev_pm_page->willAppear) {
        prev_pm_page->willAppear(prev_page);
    }
    lv_obj_clear_flag(prev_pm_page->page, LV_OBJ_FLAG_HIDDEN); // 取消页面的"隐藏"标志，显示页面
    _pm_anima_appear(prev_pm_page, &pm_page->_options, _back_appear_complete_cb); // 动画完毕后执行回调

    return 0;
}
