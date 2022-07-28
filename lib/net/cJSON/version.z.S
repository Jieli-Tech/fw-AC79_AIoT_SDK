.section .sys.version, "ax"

.global lib_cjson_version

lib_cjson_version:

#ifdef CONFIG_RELEASE_ENABLE

#else
.asciz __FILE__
.asciz "CJSON-*modified*-@20220727-$cb4de2d"
#endif
