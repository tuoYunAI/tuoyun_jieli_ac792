
    _vfs_ops_begin = .;
    PROVIDE(vfs_ops_begin = .);
        *(.vfs_operations)
    _vfs_ops_end = .;
    PROVIDE(vfs_ops_end = .);

    _initcall_begin = .;
    PROVIDE(initcall_begin = .);
        *(.initcall)
    _initcall_end = .;
    PROVIDE(initcall_end = .);

    _early_initcall_begin = .;
    PROVIDE(early_initcall_begin = .);
       *(.early.initcall)
    _early_initcall_end = .;
    PROVIDE(early_initcall_end = .);

    _late_initcall_begin = .;
    PROVIDE(late_initcall_begin = .);
       *(.late.initcall)
    _late_initcall_end = .;
    PROVIDE(late_initcall_end = .);

    _platform_initcall_begin = .;
    PROVIDE(platform_initcall_begin = .);
       *(.platform.initcall)
    _platform_initcall_end = .;
    PROVIDE(platform_initcall_end = .);

    _module_initcall_begin = .;
    PROVIDE(module_initcall_begin = .);
       *(.module.initcall)
    _module_initcall_end = .;
    PROVIDE(module_initcall_end = .);

    _sys_event_handler_begin = .;
    PROVIDE(sys_event_handler_begin = .);
       *(.sys_event.4.handler)
       *(.sys_event.3.handler)
       *(.sys_event.2.handler)
       *(.sys_event.1.handler)
       *(.sys_event.0.handler)
    _sys_event_handler_end = .;
    PROVIDE(sys_event_handler_end = .);

    _app_event_handler_begin = .;
    PROVIDE(app_event_handler_begin = .);
       *(.app_event_handler)
    _app_event_handler_end = .;
    PROVIDE(app_event_handler_end = .);

    _syscfg_ops_begin = .;
    PROVIDE(syscfg_ops_begin = .);
       *(.syscfg.2.ops)
       *(.syscfg.1.ops)
       *(.syscfg.0.ops)
    _syscfg_ops_end = .;
    PROVIDE(syscfg_ops_end = .);

    _vm_spi_target_begin = .;
    PROVIDE(vm_spi_target_begin = .);
        *(.vm_spi_target)
    _vm_spi_target_end = .;
    PROVIDE(vm_spi_target_end = .);

    _vm_spi_mutex_target_begin = .;
    PROVIDE(vm_spi_mutex_target_begin = .);
        *(.vm_spi_mutex_target)
    _vm_spi_mutex_target_end = .;
    PROVIDE(vm_spi_mutex_target_end = .);

    _key_driver_ops_begin = .;
    PROVIDE(key_driver_ops_begin = .);
        *(.key_operation)
    _key_driver_ops_end = .;
    PROVIDE(key_driver_ops_end = .);

    _server_info_begin = .;
    PROVIDE(server_info_begin = .);
       *(.server_info)
    _server_info_end = .;
    PROVIDE(server_info_end = .);

    _cmd_interface_begin = .;
    PROVIDE(cmd_interface_begin = .);
       *(.eff_cmd)
    _cmd_interface_end = .;
    PROVIDE(cmd_interface_end = .);

    _snap_interface_begin = .;
    PROVIDE(snap_interface_begin = .);
       *(.snap_parm)
    _snap_interface_end = .;
    PROVIDE(snap_interface_end = .);

    _device_node_begin = .;
    PROVIDE(device_node_begin = .);
        *(.device)
    _device_node_end = .;
    PROVIDE(device_node_end = .);

    _ai_sdk_api_begin = .;
    PROVIDE(ai_sdk_api_begin = .);
        *(.ai_sdk)
    _ai_sdk_api_end = .;
    PROVIDE(ai_sdk_api_end = .);

    _tool_interface_begin = .;
    PROVIDE(tool_interface_begin = .);
        *(.tool_interface)
    _tool_interface_end = .;
    PROVIDE(tool_interface_end = .);

    _camera_dev_begin = .;
    PROVIDE(camera_dev_begin = .);
        *(.camera_dev)
    _camera_dev_end = .;
    PROVIDE(camera_dev_end = .);

    _eth_phy_device_begin = .;
    PROVIDE(eth_phy_device_begin = .);
        *(.eth_phy_device)
    _eth_phy_device_end = .;
    PROVIDE(eth_phy_device_end = .);

    _camera1_dev_begin = .;
    PROVIDE(camera1_dev_begin = .);
        *(.camera1_dev)
    _camera1_dev_end = .;
    PROVIDE(camera1_dev_end = .);

    _iic_device_begin = .;
    PROVIDE(iic_device_begin = .);
        *(.iic)
    _iic_device_end = .;
    PROVIDE(iic_device_end = .);

    _video_pkg_sys_begin = .;
    PROVIDE(video_pkg_sys_begin = .);
        *(.video_pkg_sys)
    _video_pkg_sys_end = .;
    PROVIDE(video_pkg_sys_end = .);

    _video_pkg_begin = .;
    PROVIDE(video_pkg_begin = .);
        *(.video_pkg)
    _video_pkg_end = .;
    PROVIDE(video_pkg_end = .);

    _plugin_begin = .;
    PROVIDE(plugin_begin = .);
        *(.pipeline_plugin)
    _plugin_end = .;
    PROVIDE(plugin_end = .);

    _tp_driver_begin = .;
    PROVIDE(tp_driver_begin = .);
     *(.tp_driver)
    _tp_driver_end = .;
    PROVIDE(tp_driver_end = .);

    wireless_trans_ops_begin = .;
    PROVIDE(wireless_trans_ops_begin = .);
    *(.wireless_trans)
    wireless_trans_ops_end = .;
    PROVIDE(wireless_trans_ops_end = .);

    wireless_data_callback_func_begin = .;
    PROVIDE(wireless_data_callback_func_begin = .);
    *(.wireless_call_sync)
    wireless_data_callback_func_end = .;
    PROVIDE(wireless_data_callback_func_end = .);

    wireless_custom_data_trans_stub_begin = .;
    PROVIDE(wireless_custom_data_trans_stub_begin = .);
    *(.wireless_custom_data_trans_stub)
    wireless_custom_data_trans_stub_end = .;
    PROVIDE(wireless_custom_data_trans_stub_end = .);

