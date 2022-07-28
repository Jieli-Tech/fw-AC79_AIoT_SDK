.section .sys.version, "ax"

.global lib_tmallgenie_version

lib_tmallgenie_version:

#ifdef CONFIG_RELEASE_ENABLE

#else
.asciz __FILE__
.asciz "TMALLGENIE-*modified*-@20220727-$cb4de2d"
#endif
