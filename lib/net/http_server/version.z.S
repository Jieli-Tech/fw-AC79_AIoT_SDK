.section .sys.version, "ax"

.global lib_http_server_version

lib_http_server_version:

#ifdef CONFIG_RELEASE_ENABLE

#else
.asciz __FILE__
.asciz "HTTP_SERVER-*modified*-@20220727-$cb4de2d"
#endif
