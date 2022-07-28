.section .sys.version, "ax"

.global lib_wolfmqtt_version

lib_wolfmqtt_version:

#ifdef CONFIG_RELEASE_ENABLE

#else
.asciz __FILE__
.asciz "WOLFMQTT-*modified*-@20220727-$cb4de2d"
#endif
