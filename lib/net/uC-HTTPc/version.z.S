.section .sys.version, "ax"

.global lib_uc_httpc_version

lib_uc_httpc_version:

#ifdef CONFIG_RELEASE_ENABLE

#else
.asciz __FILE__
.asciz "UC_HTTPC-*modified*-@20220727-$cb4de2d"
#endif
