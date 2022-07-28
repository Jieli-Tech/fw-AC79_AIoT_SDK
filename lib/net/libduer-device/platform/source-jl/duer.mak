include $(CLEAR_VAR)

MODULE_PATH := $(BASE_DIR)/platform/source-linux

LOCAL_MODULE := port-linux

LOCAL_STATIC_LIBRARIES := framework cjson connagent coap voice_engine platform

LOCAL_SRC_FILES := $(wildcard $(MODULE_PATH)/*.c)

LOCAL_INCLUDES :=

include $(BUILD_STATIC_LIB)
