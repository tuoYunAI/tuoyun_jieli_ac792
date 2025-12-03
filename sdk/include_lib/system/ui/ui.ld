	. = ALIGN(4);
        ui_module_event_handler_begin = .;
        KEEP(*(.ui_module_event_handler))
        ui_module_event_handler_end = .;

        ui_screen_action_handler_begin = .;
        KEEP(*(.ui_screen_action_handler))
        ui_screen_action_handler_end = .;

        ui_varable_reg_tab_begin = .;
        KEEP(*(.ui_varable_reg_tab))
        ui_varable_reg_tab_end = .;

        ui_key_event_handler_begin = .;
        KEEP(*(.ui_key_event_handler))
        ui_key_event_handler_end = .;
