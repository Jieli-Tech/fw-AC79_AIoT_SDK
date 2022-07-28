.section .sys.version, "ax"

.global lib_wt_version

lib_wt_version:

#ifdef CONFIG_RELEASE_ENABLE

#else
.asciz __FILE__
.asciz "WT-*modified*-@20220727-$cb4de2d"
#endif
