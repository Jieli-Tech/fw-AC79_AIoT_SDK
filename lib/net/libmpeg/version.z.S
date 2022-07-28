.section .sys.version, "ax"

.global lib_mpeg_version

lib_mpeg_version:

#ifdef CONFIG_RELEASE_ENABLE

#else
.asciz __FILE__
.asciz "MPEG-*modified*-@20220727-$cb4de2d"
#endif
