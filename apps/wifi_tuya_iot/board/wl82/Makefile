
# make 编译并下载
# make VERBOSE=1 显示编译详细过程
# make clean 清除编译临时文件
#
# 注意： Linux 下编译方式：
#     1. 从 http://pkgman.jieliapp.com/doc/all 处找到下载链接
#     2. 下载后，解压到 /opt/jieli 目录下，保证
#       /opt/jieli/common/bin/clang 存在（注意目录层次）
#     3. 确认 ulimit -n 的结果足够大（建议大于8096），否则链接可能会因为打开文件太多而失败
#       可以通过 ulimit -n 8096 来设置一个较大的值
#

# 工具路径设置
ifeq ($(OS), Windows_NT)
# Windows 下工具链位置
TOOL_DIR := C:/JL/pi32/bin
CC    := clang.exe
CXX   := clang.exe
LD    := pi32v2-lto-wrapper.exe
AR    := pi32v2-lto-ar.exe
MKDIR := mkdir_win -p
RM    := rm -rf

SYS_LIB_DIR := C:/JL/pi32/pi32v2-lib/r3
SYS_INC_DIR := C:/JL/pi32/pi32v2-include
EXT_CFLAGS  := # Windows 下不需要 -D__SHELL__
export PATH:=$(TOOL_DIR);$(PATH)

## 后处理脚本
FIXBAT          := ..\..\..\..\tools\utils\fixbat.exe # 用于处理 utf8->gbk 编码问题
POST_SCRIPT     := ..\..\..\..\cpu\wl82\tools\download.bat
RUN_POST_SCRIPT := $(POST_SCRIPT)
else
# Linux 下工具链位置
TOOL_DIR := /opt/jieli/pi32v2/bin
CC    := clang
CXX   := clang++
LD    := lto-wrapper
AR    := lto-ar
MKDIR := mkdir -p
RM    := rm -rf
export OBJDUMP := $(TOOL_DIR)/objdump
export OBJCOPY := $(TOOL_DIR)/objcopy
export OBJSIZEDUMP := $(TOOL_DIR)/objsizedump

SYS_LIB_DIR := $(TOOL_DIR)/../lib/r3
SYS_INC_DIR := $(TOOL_DIR)/../include
EXT_CFLAGS  := -D__SHELL__ # Linux 下需要这个保证正确处理 download.c
export PATH:=$(TOOL_DIR):$(PATH)

## 后处理脚本
FIXBAT          := touch # Linux下不需要处理 bat 编码问题
POST_SCRIPT     := ../../../../cpu/wl82/tools/download.sh
RUN_POST_SCRIPT := bash $(POST_SCRIPT)
endif

CC  := $(TOOL_DIR)/$(CC)
CXX := $(TOOL_DIR)/$(CXX)
LD  := $(TOOL_DIR)/$(LD)
AR  := $(TOOL_DIR)/$(AR)
# 输出文件设置
OUT_ELF   := ../../../../cpu/wl82/tools/sdk.elf
OBJ_FILE  := $(OUT_ELF).objs.txt
# 编译路径设置
BUILD_DIR := objs
# 工程路径前缀
ROOT_PREFIX := ../../../..

# 编译参数设置
CFLAGS := \
	-target pi32v2 \
	-mcpu=r3 \
	-integrated-as \
	-flto \
	-Wuninitialized \
	-Wno-invalid-noreturn \
	-fno-common \
	-integrated-as \
	-Oz \
	-g \
	-flto \
	-fallow-pointer-null \
	-fprefer-gnu-section \
	-Wno-shift-negative-value \
	-Wframe-larger-than=2560 \
	-mllvm -pi32v2-large-program=true \
	-fms-extensions \
	-w \


# C++额外的编译参数
CXXFLAGS :=


# 宏定义
DEFINES := \
	-DSUPPORT_MS_EXTENSIONS \
	-DCONFIG_RELEASE_ENABLE \
	-DCONFIG_CPU_WL82 \
	-DCONFIG_JLFAT_ENABLE \
	-DCONFIG_FREE_RTOS_ENABLE \
	-DCONFIG_THREAD_ENABLE \
	-DCONFIG_AUDIO_ONCHIP \
	-D__GCC_PI32V2__ \
	-DCONFIG_NEW_ECC_ENABLE \
	-DCONFIG_EQ_SUPPORT_ASYNC \
	-DEQ_CORE_V1 \
	-DCONFIG_SFC_ENABLE \
	-DCONFIG_NO_SDRAM_ENABLE \
	-DCONFIG_NET_ENABLE=1 \
	-DCONFIG_BT_ENABLE=1 \
	-DEVENT_HANDLER_NUM_CONFIG=2 \
	-DEVENT_TOUCH_ENABLE_CONFIG=0 \
	-DEVENT_POOL_SIZE_CONFIG=256 \
	-DCONFIG_EVENT_KEY_MAP_ENABLE=0 \
	-DTIMER_POOL_NUM_CONFIG=15 \
	-DAPP_ASYNC_POOL_NUM_CONFIG=0 \
	-DVFS_ENABLE=1 \
	-DUSE_SDFILE_NEW=1 \
	-DSDFILE_STORAGE=1 \
	-DVFS_FILE_POOL_NUM_CONFIG=8 \
	-DVM_MAX_PAGE_ALIGN_SIZE_CONFIG=64*1024 \
	-DVM_MAX_SECTOR_ALIGN_SIZE_CONFIG=64*1024 \
	-DVM_ITEM_MAX_NUM=256 \
	-DCONFIG_TWS_ENABLE \
	-DCONFIG_BTCTRLER_TASK_DEL_ENABLE \
	-DCONFIG_LMP_CONN_SUSPEND_ENABLE \
	-DCONFIG_LMP_REFRESH_ENCRYPTION_KEY_ENABLE \
	-DCONFIG_UPDATA_ENABLE \
	-DCONFIG_OTA_UPDATA_ENABLE \
	-DCONFIG_ITEM_FORMAT_VM \
	-D__LD__ \


DEFINES += $(EXT_CFLAGS) # 额外的一些定义

# 头文件搜索路径
INCLUDES := \
	-I../../../../include_lib \
	-I../../../../include_lib/driver \
	-I../../../../include_lib/driver/device \
	-I../../../../include_lib/driver/cpu/wl82 \
	-I../../../../include_lib/system \
	-I../../../../include_lib/system/generic \
	-I../../../../include_lib/btctrler \
	-I../../../../include_lib/btctrler/port/wl82 \
	-I../../../../include_lib/update \
	-I../../../../include_lib/btstack/third_party/common \
	-I../../../../include_lib/btstack/third_party/rcsp \
	-I../../../../include_lib/utils \
	-I../../../../include_lib/utils/syscfg \
	-I../../../../include_lib/utils/event \
	-I../../../../include_lib/media \
	-I../../../../include_lib/media/cpu/wl82 \
	-I../../../../include_lib/btstack \
	-I../../../../apps \
	-I../../../../apps/wifi_tuya_iot/include \
	-I../../../../apps/common \
	-I../../../../apps/common/include \
	-I../../../../apps/wifi_tuya_iot \
	-I../../../../include_lib/net \
	-I../../../../include_lib/net/mbedtls_2_2_1 \
	-I../../../../include_lib/net/lwip_2_1_3/lwip/src/include \
	-I../../../../include_lib/net/lwip_2_1_3/lwip/src/include/lwip \
	-I../../../../include_lib/net/lwip_2_1_3/lwip/port \
	-I../../../../include_lib/net/lwip_2_1_3/lwip/app \
	-I../../../../include_lib/net/lwip_2_1_3/lwip/app/ctp \
	-I../../../../include_lib/net/lwip_2_1_3/lwip/app/cdp \
	-I../../../../include_lib/system/os \
	-I../../../../apps/common/config/include \
	-I../../../../apps/common/ble/include \
	-I../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_os_adapter/include \
	-I../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_os_adapter/include/driver \
	-I../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_os_adapter/include/system \
	-I../../../../apps/wifi_tuya_iot/tuya/sdk/include \
	-I../../../../apps/wifi_tuya_iot/tuya/apps/template_demo \
	-I../../../../apps/wifi_tuya_iot/tuya/apps/template_demo/include \
	-I$(SYS_INC_DIR) \


# 需要编译的 .c 文件
c_SRC_FILES := \
	../../../../apps/common/config/bt_profile_config.c \
	../../../../apps/common/config/log_config/lib_btctrler_config.c \
	../../../../apps/common/config/log_config/lib_btstack_config.c \
	../../../../apps/common/config/log_config/lib_driver_config.c \
	../../../../apps/common/config/log_config/lib_system_config.c \
	../../../../apps/common/config/log_config/lib_update_config.c \
	../../../../apps/common/config/user_cfg.c \
	../../../../apps/common/debug/debug.c \
	../../../../apps/common/debug/debug_user.c \
	../../../../apps/common/key/adkey.c \
	../../../../apps/common/key/key_driver.c \
	../../../../apps/common/net/assign_macaddr.c \
	../../../../apps/common/net/config_network.c \
	../../../../apps/common/net/iperf_test.c \
	../../../../apps/common/net/platform_cfg.c \
	../../../../apps/common/net/wifi_conf.c \
	../../../../apps/common/system/init.c \
	../../../../apps/common/system/system_reset_reason.c \
	../../../../apps/common/system/version.c \
	../../../../apps/common/update/net_update.c \
	../../../../apps/common/update/update.c \
	../../../../apps/wifi_tuya_iot/app_main.c \
	../../../../apps/wifi_tuya_iot/board/wl82/board.c \
	../../../../apps/wifi_tuya_iot/bt_ble/ble.c \
	../../../../apps/wifi_tuya_iot/bt_ble/le_net_cfg_tuya.c \
	../../../../apps/wifi_tuya_iot/tuya/apps/template_demo/src/light_system.c \
	../../../../apps/wifi_tuya_iot/tuya/apps/template_demo/src/light_tools.c \
	../../../../apps/wifi_tuya_iot/tuya/apps/template_demo/src/soc_flash.c \
	../../../../apps/wifi_tuya_iot/tuya/apps/template_demo/src/user_flash.c \
	../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_common/tuya_main.c \
	../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_os_adapter/src/adapter_platform.c \
	../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_os_adapter/src/driver/tuya_drv_i2c.c \
	../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_os_adapter/src/driver/tuya_drv_pin.c \
	../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_os_adapter/src/driver/tuya_drv_rtc.c \
	../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_os_adapter/src/driver/tuya_drv_uart.c \
	../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_os_adapter/src/driver/tuya_os_adapt_bt.c \
	../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_os_adapter/src/driver/tuya_os_adapt_ota.c \
	../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_os_adapter/src/driver/tuya_os_adapt_storge.c \
	../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_os_adapter/src/driver/tuya_os_adapt_wifi.c \
	../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_os_adapter/src/system/tuya_os_adapt_memory.c \
	../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_os_adapter/src/system/tuya_os_adapt_mutex.c \
	../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_os_adapter/src/system/tuya_os_adapt_network.c \
	../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_os_adapter/src/system/tuya_os_adapt_output.c \
	../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_os_adapter/src/system/tuya_os_adapt_queue.c \
	../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_os_adapter/src/system/tuya_os_adapt_semaphore.c \
	../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_os_adapter/src/system/tuya_os_adapt_system.c \
	../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_os_adapter/src/system/tuya_os_adapt_thread.c \
	../../../../apps/wifi_tuya_iot/tuya/platforms/tuya_os_adapter/src/tuya_os_init.c \
	../../../../apps/wifi_tuya_iot/wifi/wifi_tuya_task.c \
	../../../../cpu/wl82/debug.c \
	../../../../cpu/wl82/iic.c \
	../../../../cpu/wl82/key/adc_api.c \
	../../../../cpu/wl82/port_waked_up.c \
	../../../../cpu/wl82/setup.c \


# 需要编译的 .S 文件
S_SRC_FILES := \
	../../../../include_lib/btctrler/version.z.S \
	../../../../include_lib/btstack/version.z.S \
	../../../../include_lib/driver/version.z.S \
	../../../../include_lib/media/version.z.S \
	../../../../include_lib/net/version.z.S \
	../../../../include_lib/server/version.z.S \
	../../../../include_lib/system/version.z.S \
	../../../../include_lib/update/version.z.S \
	../../../../include_lib/utils/version.z.S \
	../../../../lib/net/version.z.S \
	../../../../lib/server/version.z.S \
	../../../../lib/utils/version.z.S \


# 需要编译的 .s 文件
s_SRC_FILES :=


# 需要编译的 .cpp 文件
cpp_SRC_FILES :=


# 需要编译的 .cc 文件
cc_SRC_FILES :=


# 需要编译的 .cxx 文件
cxx_SRC_FILES :=


# 链接参数
LFLAGS := \
	--plugin-opt=-pi32v2-always-use-itblock=false \
	--plugin-opt=-enable-ipra=true \
	--plugin-opt=-pi32v2-merge-max-offset=4096 \
	--plugin-opt=-pi32v2-enable-simd=true \
	--plugin-opt=mcpu=r3 \
	--plugin-opt=-global-merge-on-const \
	--plugin-opt=-inline-threshold=5 \
	--plugin-opt=-inline-max-allocated-size=32 \
	--plugin-opt=-inline-normal-into-special-section=true \
	--plugin-opt=-dont-used-symbol-list=malloc,free,sprintf,printf,puts,putchar \
	--plugin-opt=save-temps \
	--plugin-opt=-pi32v2-enable-rep-memop \
	--plugin-opt=-warn-stack-size=2560 \
	--sort-common \
	--dont-complain-call-overflow \
	--plugin-opt=-used-symbol-file=../../../../cpu/wl82/sdk_used_list.used \
	--plugin-opt=-enable-movable-region=true \
	--plugin-opt=-movable-region-section-prefix=.movable.slot. \
	--plugin-opt=-movable-region-stub-section-prefix=.movable.stub. \
	--plugin-opt=-movable-region-prefix=.movable.region. \
	--plugin-opt=-movable-region-stub-prefix=__movable_stub_ \
	--plugin-opt=-movable-region-stub-swi-number=-2 \
	--plugin-opt=-movable-region-map-file-list=../../../../apps/common/movable/funcname.txt \
	--plugin-opt=-movable-region-section-map-file-list=../../../../apps/common/movable/section.txt \
	--plugin-opt=-movable-region-exclude-func-file-list=../../../../apps/common/movable/exclude.txt \
	--plugin-opt=-pi32v2-large-program=true \
	--start-group \
	../../../../cpu/wl82/liba/cpu.a \
	../../../../cpu/wl82/liba/fs.a \
	../../../../cpu/wl82/liba/event.a \
	../../../../cpu/wl82/liba/system.a \
	../../../../cpu/wl82/liba/cfg_tool.a \
	../../../../cpu/wl82/liba/common_lib.a \
	../../../../cpu/wl82/liba/update.a \
	../../../../apps/wifi_tuya_iot/tuya/sdk/lib/libtuya_iot.a \
	../../../../cpu/wl82/liba/btctrler.a \
	../../../../cpu/wl82/liba/btstack.a \
	../../../../cpu/wl82/liba/crypto_toolbox_Osize.a \
	../../../../cpu/wl82/liba/lib_ccm_aes.a \
	../../../../cpu/wl82/liba/lib_sig_mesh.a \
	../../../../cpu/wl82/liba/wl_rf_common.a \
	../../../../cpu/wl82/liba/hsm.a \
	../../../../cpu/wl82/liba/wpasupplicant.a \
	../../../../cpu/wl82/liba/json.a \
	../../../../cpu/wl82/liba/iperf.a \
	../../../../cpu/wl82/liba/libmbedtls_2_2_1.a \
	../../../../cpu/wl82/liba/lwip_2_1_3_sfc.a \
	../../../../cpu/wl82/liba/wl_wifi_sfc.a \
	--end-group \
	-T../../../../cpu/wl82/sdk.ld \
	-M=../../../../cpu/wl82/tools/sdk.map \
	--plugin-opt=mcpu=r3 \
	--plugin-opt=-mattr=+fprev1 \


LIBPATHS := \
	-L$(SYS_LIB_DIR) \


LIBS := \
	$(SYS_LIB_DIR)/libm.a \
	$(SYS_LIB_DIR)/libc.a \
	$(SYS_LIB_DIR)/libm.a \
	$(SYS_LIB_DIR)/libcompiler-rt.a \



c_OBJS    := $(c_SRC_FILES:%.c=%.c.o)
S_OBJS    := $(S_SRC_FILES:%.S=%.S.o)
s_OBJS    := $(s_SRC_FILES:%.s=%.s.o)
cpp_OBJS  := $(cpp_SRC_FILES:%.cpp=%.cpp.o)
cxx_OBJS  := $(cxx_SRC_FILES:%.cxx=%.cxx.o)
cc_OBJS   := $(cc_SRC_FILES:%.cc=%.cc.o)

OBJS      := $(c_OBJS) $(S_OBJS) $(s_OBJS) $(cpp_OBJS) $(cxx_OBJS) $(cc_OBJS)
DEP_FILES := $(OBJS:%.o=%.d)


OBJS      := $(addprefix $(BUILD_DIR)/, $(OBJS:$(ROOT_PREFIX)/%=%))
DEP_FILES := $(addprefix $(BUILD_DIR)/, $(DEP_FILES:$(ROOT_PREFIX)/%=%))


VERBOSE ?= 0
ifeq ($(VERBOSE), 1)
QUITE :=
else
QUITE := @
endif

# 一些旧的 make 不支持 file 函数，需要 make 的时候指定 LINK_AT=0 make
LINK_AT ?= 1

# 表示下面的不是一个文件的名字，无论是否存在 all, clean, pre_build 这样的文件
# 还是要执行命令
# see: https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean pre_build

# 不要使用 make 预设置的规则
# see: https://www.gnu.org/software/make/manual/html_node/Suffix-Rules.html
.SUFFIXES:

all: pre_build $(OUT_ELF)
	$(info +POST-BUILD)
	$(QUITE) $(RUN_POST_SCRIPT) sdk

pre_build:
	$(info +PRE-BUILD)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -E -P ../../../../cpu/wl82/sdk_used_list.c -o ../../../../cpu/wl82/sdk_used_list.used
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -E -P ../../../../apps/common/movable/section.c -o ../../../../apps/common/movable/section.txt
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -E -P ../../../../cpu/wl82/sdk_ld.c -o ../../../../cpu/wl82/sdk.ld
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -E -P ../../../../cpu/wl82/tools/download.c -o $(POST_SCRIPT)
	$(QUITE) $(FIXBAT) $(POST_SCRIPT)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -E -P ../../../../cpu/wl82/tools/isd_config_rule.c -o ../../../../cpu/wl82/tools/isd_config.ini

clean:
	$(QUITE) $(RM) $(OUT_ELF)
	$(QUITE) $(RM) $(BUILD_DIR)



ifeq ($(LINK_AT), 1)
$(OUT_ELF): $(OBJS)
	$(info +LINK $@)
	$(shell $(MKDIR) $(@D))
	$(file >$(OBJ_FILE), $(OBJS))
	$(QUITE) $(LD) -o $(OUT_ELF) @$(OBJ_FILE) $(LFLAGS) $(LIBPATHS) $(LIBS)
else
$(OUT_ELF): $(OBJS)
	$(info +LINK $@)
	$(shell $(MKDIR) $(@D))
	$(QUITE) $(LD) -o $(OUT_ELF) $(OBJS) $(LFLAGS) $(LIBPATHS) $(LIBS)
endif


$(BUILD_DIR)/%.c.o : $(ROOT_PREFIX)/%.c
	$(info +CC $<)
	@$(MKDIR) $(@D)
	@$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.S.o : $(ROOT_PREFIX)/%.S
	$(info +AS $<)
	@$(MKDIR) $(@D)
	@$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.s.o : $(ROOT_PREFIX)/%.s
	$(info +AS $<)
	@$(MKDIR) $(@D)
	@$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.cpp.o : $(ROOT_PREFIX)/%.cpp
	$(info +CXX $<)
	@$(MKDIR) $(@D)
	@$(CXX) $(CFLAGS) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.cxx.o : $(ROOT_PREFIX)/%.cxx
	$(info +CXX $<)
	@$(MKDIR) $(@D)
	@$(CXX) $(CFLAGS) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.cc.o : $(ROOT_PREFIX)/%.cc
	$(info +CXX $<)
	@$(MKDIR) $(@D)
	@$(CXX) $(CFLAGS) $(CXXFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

-include $(DEP_FILES)
