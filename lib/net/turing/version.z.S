.section .sys.version, "ax"

.global lib_turing_version

lib_turing_version:

#ifdef CONFIG_RELEASE_ENABLE

#else
.asciz __FILE__
.asciz "TURING-*modified*-@20220727-$cb4de2d"
#endif
