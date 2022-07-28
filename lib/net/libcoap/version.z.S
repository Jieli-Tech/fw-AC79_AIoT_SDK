.section .sys.version, "ax"

.global lib_coap_version

lib_coap_version:

#ifdef CONFIG_RELEASE_ENABLE

#else
.asciz __FILE__
.asciz "COAP-*modified*-@20220727-$cb4de2d"
#endif
