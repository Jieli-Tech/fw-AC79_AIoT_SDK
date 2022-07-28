
.section .sys.version, "ax"

.global lib_nopoll_version

lib_nopoll_version:

#ifdef CONFIG_RELEASE_ENABLE

#else
.asciz __FILE__
.asciz "LIBNOPOLL-*modified*-@20220727-$cb4de2d"
#endif
