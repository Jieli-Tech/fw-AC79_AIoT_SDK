.section .sys.version, "ax"

.global lib_mbedtls_2_2_1_version

lib_mbedtls_2_2_1_version:

#ifdef CONFIG_RELEASE_ENABLE

#else
.asciz __FILE__
.asciz "MBEDTLS_2_2_1-*modified*-@20220727-$cb4de2d"
#endif
