.section .sys.version, "ax"

.global lib_json_c_version

lib_json_c_version:

#ifdef CONFIG_RELEASE_ENABLE

#else
.asciz __FILE__
.asciz "JSON_C-*modified*-@20220727-$cb4de2d"
#endif
