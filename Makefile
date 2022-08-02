# 总的 Makefile，用于调用目录下各个子工程对应的 Makefile
# 注意： Linux 下编译方式：
# 1. 从 http://pkgman.jieliapp.com/doc/all 处找到下载链接
# 2. 下载后，解压到 /opt/jieli 目录下，保证
#   /opt/jieli/common/bin/clang 存在（注意目录层次）
# 3. 确认 ulimit -n 的结果足够大（建议大于8096），否则链接可能会因为打开文件太多而失败
#   可以通过 ulimit -n 8096 来设置一个较大的值
# 支持的目标
# make ac790n_wifi_ipc
# make ac791n_wifi_ipc
# make ac790n_wifi_story_machine
# make ac791n_wifi_story_machine
# make ac790n_wifi_camera
# make ac791n_wifi_camera
# make ac791n_wifi_tuya_iot
# make ac790n_scan_box
# make ac791n_scan_box
# make ac791n_demo_demo_devkitboard
# make ac790n_demo_demo_video
# make ac791n_demo_demo_video
# make ac790n_demo_demo_ble
# make ac791n_demo_demo_ble
# make ac790n_demo_demo_edr
# make ac791n_demo_demo_edr
# make ac790n_demo_demo_ui
# make ac791n_demo_demo_ui
# make ac790n_demo_demo_audio
# make ac791n_demo_demo_audio
# make ac790n_demo_demo_hello
# make ac791n_demo_demo_hello
# make ac790n_demo_demo_wifi
# make ac791n_demo_demo_wifi

.PHONY: all clean ac790n_wifi_ipc ac791n_wifi_ipc ac790n_wifi_story_machine ac791n_wifi_story_machine ac790n_wifi_camera ac791n_wifi_camera ac791n_wifi_tuya_iot ac790n_scan_box ac791n_scan_box ac791n_demo_demo_devkitboard ac790n_demo_demo_video ac791n_demo_demo_video ac790n_demo_demo_ble ac791n_demo_demo_ble ac790n_demo_demo_edr ac791n_demo_demo_edr ac790n_demo_demo_ui ac791n_demo_demo_ui ac790n_demo_demo_audio ac791n_demo_demo_audio ac790n_demo_demo_hello ac791n_demo_demo_hello ac790n_demo_demo_wifi ac791n_demo_demo_wifi clean_ac790n_wifi_ipc clean_ac791n_wifi_ipc clean_ac790n_wifi_story_machine clean_ac791n_wifi_story_machine clean_ac790n_wifi_camera clean_ac791n_wifi_camera clean_ac791n_wifi_tuya_iot clean_ac790n_scan_box clean_ac791n_scan_box clean_ac791n_demo_demo_devkitboard clean_ac790n_demo_demo_video clean_ac791n_demo_demo_video clean_ac790n_demo_demo_ble clean_ac791n_demo_demo_ble clean_ac790n_demo_demo_edr clean_ac791n_demo_demo_edr clean_ac790n_demo_demo_ui clean_ac791n_demo_demo_ui clean_ac790n_demo_demo_audio clean_ac791n_demo_demo_audio clean_ac790n_demo_demo_hello clean_ac791n_demo_demo_hello clean_ac790n_demo_demo_wifi clean_ac791n_demo_demo_wifi

all: ac790n_wifi_ipc ac791n_wifi_ipc ac790n_wifi_story_machine ac791n_wifi_story_machine ac790n_wifi_camera ac791n_wifi_camera ac791n_wifi_tuya_iot ac790n_scan_box ac791n_scan_box ac791n_demo_demo_devkitboard ac790n_demo_demo_video ac791n_demo_demo_video ac790n_demo_demo_ble ac791n_demo_demo_ble ac790n_demo_demo_edr ac791n_demo_demo_edr ac790n_demo_demo_ui ac791n_demo_demo_ui ac790n_demo_demo_audio ac791n_demo_demo_audio ac790n_demo_demo_hello ac791n_demo_demo_hello ac790n_demo_demo_wifi ac791n_demo_demo_wifi
	@echo +ALL DONE

clean: clean_ac790n_wifi_ipc clean_ac791n_wifi_ipc clean_ac790n_wifi_story_machine clean_ac791n_wifi_story_machine clean_ac790n_wifi_camera clean_ac791n_wifi_camera clean_ac791n_wifi_tuya_iot clean_ac790n_scan_box clean_ac791n_scan_box clean_ac791n_demo_demo_devkitboard clean_ac790n_demo_demo_video clean_ac791n_demo_demo_video clean_ac790n_demo_demo_ble clean_ac791n_demo_demo_ble clean_ac790n_demo_demo_edr clean_ac791n_demo_demo_edr clean_ac790n_demo_demo_ui clean_ac791n_demo_demo_ui clean_ac790n_demo_demo_audio clean_ac791n_demo_demo_audio clean_ac790n_demo_demo_hello clean_ac791n_demo_demo_hello clean_ac790n_demo_demo_wifi clean_ac791n_demo_demo_wifi
	@echo +CLEAN DONE

ac790n_wifi_ipc:
	$(MAKE) -C apps/wifi_ipc/board/wl80 -f Makefile

clean_ac790n_wifi_ipc:
	$(MAKE) -C apps/wifi_ipc/board/wl80 -f Makefile clean

ac791n_wifi_ipc:
	$(MAKE) -C apps/wifi_ipc/board/wl82 -f Makefile

clean_ac791n_wifi_ipc:
	$(MAKE) -C apps/wifi_ipc/board/wl82 -f Makefile clean

ac790n_wifi_story_machine:
	$(MAKE) -C apps/wifi_story_machine/board/wl80 -f Makefile

clean_ac790n_wifi_story_machine:
	$(MAKE) -C apps/wifi_story_machine/board/wl80 -f Makefile clean

ac791n_wifi_story_machine:
	$(MAKE) -C apps/wifi_story_machine/board/wl82 -f Makefile

clean_ac791n_wifi_story_machine:
	$(MAKE) -C apps/wifi_story_machine/board/wl82 -f Makefile clean

ac790n_wifi_camera:
	$(MAKE) -C apps/wifi_camera/board/wl80 -f Makefile

clean_ac790n_wifi_camera:
	$(MAKE) -C apps/wifi_camera/board/wl80 -f Makefile clean

ac791n_wifi_camera:
	$(MAKE) -C apps/wifi_camera/board/wl82 -f Makefile

clean_ac791n_wifi_camera:
	$(MAKE) -C apps/wifi_camera/board/wl82 -f Makefile clean

ac791n_wifi_tuya_iot:
	$(MAKE) -C apps/wifi_tuya_iot/board/wl82 -f Makefile

clean_ac791n_wifi_tuya_iot:
	$(MAKE) -C apps/wifi_tuya_iot/board/wl82 -f Makefile clean

ac790n_scan_box:
	$(MAKE) -C apps/scan_box/board/wl80 -f Makefile

clean_ac790n_scan_box:
	$(MAKE) -C apps/scan_box/board/wl80 -f Makefile clean

ac791n_scan_box:
	$(MAKE) -C apps/scan_box/board/wl82 -f Makefile

clean_ac791n_scan_box:
	$(MAKE) -C apps/scan_box/board/wl82 -f Makefile clean

ac791n_demo_demo_devkitboard:
	$(MAKE) -C apps/demo/demo_DevKitBoard/board/wl82 -f Makefile

clean_ac791n_demo_demo_devkitboard:
	$(MAKE) -C apps/demo/demo_DevKitBoard/board/wl82 -f Makefile clean

ac790n_demo_demo_video:
	$(MAKE) -C apps/demo/demo_video/board/wl80 -f Makefile

clean_ac790n_demo_demo_video:
	$(MAKE) -C apps/demo/demo_video/board/wl80 -f Makefile clean

ac791n_demo_demo_video:
	$(MAKE) -C apps/demo/demo_video/board/wl82 -f Makefile

clean_ac791n_demo_demo_video:
	$(MAKE) -C apps/demo/demo_video/board/wl82 -f Makefile clean

ac790n_demo_demo_ble:
	$(MAKE) -C apps/demo/demo_ble/board/wl80 -f Makefile

clean_ac790n_demo_demo_ble:
	$(MAKE) -C apps/demo/demo_ble/board/wl80 -f Makefile clean

ac791n_demo_demo_ble:
	$(MAKE) -C apps/demo/demo_ble/board/wl82 -f Makefile

clean_ac791n_demo_demo_ble:
	$(MAKE) -C apps/demo/demo_ble/board/wl82 -f Makefile clean

ac790n_demo_demo_edr:
	$(MAKE) -C apps/demo/demo_edr/board/wl80 -f Makefile

clean_ac790n_demo_demo_edr:
	$(MAKE) -C apps/demo/demo_edr/board/wl80 -f Makefile clean

ac791n_demo_demo_edr:
	$(MAKE) -C apps/demo/demo_edr/board/wl82 -f Makefile

clean_ac791n_demo_demo_edr:
	$(MAKE) -C apps/demo/demo_edr/board/wl82 -f Makefile clean

ac790n_demo_demo_ui:
	$(MAKE) -C apps/demo/demo_ui/board/wl80 -f Makefile

clean_ac790n_demo_demo_ui:
	$(MAKE) -C apps/demo/demo_ui/board/wl80 -f Makefile clean

ac791n_demo_demo_ui:
	$(MAKE) -C apps/demo/demo_ui/board/wl82 -f Makefile

clean_ac791n_demo_demo_ui:
	$(MAKE) -C apps/demo/demo_ui/board/wl82 -f Makefile clean

ac790n_demo_demo_audio:
	$(MAKE) -C apps/demo/demo_audio/board/wl80 -f Makefile

clean_ac790n_demo_demo_audio:
	$(MAKE) -C apps/demo/demo_audio/board/wl80 -f Makefile clean

ac791n_demo_demo_audio:
	$(MAKE) -C apps/demo/demo_audio/board/wl82 -f Makefile

clean_ac791n_demo_demo_audio:
	$(MAKE) -C apps/demo/demo_audio/board/wl82 -f Makefile clean

ac790n_demo_demo_hello:
	$(MAKE) -C apps/demo/demo_hello/board/wl80 -f Makefile

clean_ac790n_demo_demo_hello:
	$(MAKE) -C apps/demo/demo_hello/board/wl80 -f Makefile clean

ac791n_demo_demo_hello:
	$(MAKE) -C apps/demo/demo_hello/board/wl82 -f Makefile

clean_ac791n_demo_demo_hello:
	$(MAKE) -C apps/demo/demo_hello/board/wl82 -f Makefile clean

ac790n_demo_demo_wifi:
	$(MAKE) -C apps/demo/demo_wifi/board/wl80 -f Makefile

clean_ac790n_demo_demo_wifi:
	$(MAKE) -C apps/demo/demo_wifi/board/wl80 -f Makefile clean

ac791n_demo_demo_wifi:
	$(MAKE) -C apps/demo/demo_wifi/board/wl82 -f Makefile

clean_ac791n_demo_demo_wifi:
	$(MAKE) -C apps/demo/demo_wifi/board/wl82 -f Makefile clean
