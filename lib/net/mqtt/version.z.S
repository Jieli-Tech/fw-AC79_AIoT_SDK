.section .sys.version, "ax"

.global lib_mqtt_version

lib_mqtt_version:

#ifdef CONFIG_RELEASE_ENABLE

#else
.asciz __FILE__
.asciz "MQTT-*modified*-@20220727-$cb4de2d"
#endif
