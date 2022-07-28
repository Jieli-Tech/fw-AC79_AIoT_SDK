.section .sys.version, "ax"

.global lib_mongoose_version

lib_mongoose_version:

#ifdef CONFIG_RELEASE_ENABLE

#else
.asciz __FILE__
.asciz "MONGOOSE-*modified*-@20220727-$cb4de2d"
#endif
