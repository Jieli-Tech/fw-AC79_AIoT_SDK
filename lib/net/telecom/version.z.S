.section .sys.version, "ax"

.global lib_telecom_version

lib_telecom_version:

#ifdef CONFIG_RELEASE_ENABLE

#else
.asciz __FILE__
.asciz "TELECOM-*modified*-@20220727-$cb4de2d"
#endif
