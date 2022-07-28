.section .sys.version, "ax"

.global lib_device_vad_version

lib_device_vad_version:

#ifdef CONFIG_RELEASE_ENABLE

#else
.asciz __FILE__
.asciz "LIBVAD-*modified*-@20220727-$cb4de2d"
#endif
