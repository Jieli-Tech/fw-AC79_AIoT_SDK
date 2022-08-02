
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
       *(.sys_event.*)
    _sys_event_handler_end = .;
    PROVIDE(sys_event_handler_end = .);

    _syscfg_ops_begin = .;
    PROVIDE(syscfg_ops_begin = .);
       *(.syscfg.2.ops)
       *(.syscfg.1.ops)
       *(.syscfg.0.ops)
    _syscfg_ops_end = .;
    PROVIDE(syscfg_ops_end = .);

    _server_info_begin = .;
    PROVIDE(server_info_begin = .);
       *(.server_info)
    _server_info_end = .;
    PROVIDE(server_info_end = .);

    _bus_device_begin = .;
    PROVIDE(bus_device_begin = .);
       *(.bus_device)
    _bus_device_end = .;
    PROVIDE(bus_device_end = .);

    _sys_power_hal_ops_begin = .;
    PROVIDE(sys_power_hal_ops_begin = .);
       *(.sys_power_hal_ops)
    _sys_power_hal_ops_end = .;
    PROVIDE(sys_power_hal_ops_end = .);

    _device_node_begin = .;
    PROVIDE(device_node_begin = .);
        *(.device)
    _device_node_end = .;
    PROVIDE(device_node_end = .);

    lp_target_begin = .;
    PROVIDE(lp_target_begin = .);
        *(.lp_target)
    lp_target_end = .;
    PROVIDE(lp_target_end = .);

    _ai_sdk_api_begin = .;
    PROVIDE(ai_sdk_api_begin = .);
        *(.ai_sdk)
    _ai_sdk_api_end = .;
    PROVIDE(ai_sdk_api_end = .);

    config_target_begin = .;
    PROVIDE(config_target_begin = .);
        *(.config_target)
    config_target_end = .;
    PROVIDE(config_target_end = .);

    _lcd_device_begin = .;
    PROVIDE(lcd_device_begin = .);
        *(.lcd)
    _lcd_device_end = .;
    PROVIDE(lcd_device_end = .);

    _camera_dev_begin = .;
    PROVIDE(camera_dev_begin = .);
        *(.camera_dev)
    _camera_dev_end = .;
    PROVIDE(camera_dev_end = .);

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

