	_app_begin = .;
	PROVIDE(app_begin = .);
	   *(.app)
	_app_end = .;
	PROVIDE(app_end = .);

	_static_hi_timer_begin = .;
	PROVIDE(static_hi_timer_begin = .);
	   *(.hi_timer)
	_static_hi_timer_end = .;
	PROVIDE(static_hi_timer_end = .);

	_sys_cpu_timer_begin = .;
	PROVIDE(sys_cpu_timer_begin = .);
	   *(.sys_cpu_timer)
	_sys_cpu_timer_end = .;
	PROVIDE(sys_cpu_timer_end = .);

	_gsensor_dev_begin = .;
	PROVIDE(gsensor_dev_begin = .);
	    *(.gsensor_dev)
	_gsensor_dev_end = .;
	PROVIDE(gsensor_dev_end = .);

	_clock_critical_handler_begin = .;
	PROVIDE(clock_critical_handler_begin = .);
	   *(.clock_critical_txt)
	_clock_critical_handler_end = .;
	PROVIDE(clock_critical_handler_end = .);
