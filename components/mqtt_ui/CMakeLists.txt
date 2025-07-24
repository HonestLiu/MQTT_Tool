idf_component_register(
    SRCS
        "screens/ui_homeScreen.c"
        "screens/ui_ConnestScreen.c"
        "screens/ui_PubicScreen.c"
        "screens/ui_SubScreen.c"
        "ui.c"
        "components/ui_comp_hook.c"
        "ui_helpers.c"
        "ui_events.c"
    INCLUDE_DIRS
        .
    REQUIRES lvgl ui_interface
)