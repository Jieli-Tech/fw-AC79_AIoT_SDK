.section .sys.version, "ax"

.global lib_iperf_version

lib_iperf_version:

#ifdef CONFIG_RELEASE_ENABLE

#else
.asciz __FILE__
.asciz "IPERF-*modified*-@20220727-$cb4de2d"
#endif
