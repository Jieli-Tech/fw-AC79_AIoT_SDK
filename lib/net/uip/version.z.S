.section .sys.version, "ax"

.global lib_uip_version

lib_uip_version:

#ifdef CONFIG_RELEASE_ENABLE

#else
.asciz __FILE__
.asciz "UIP-*modified*-@20220727-$cb4de2d"
#endif
