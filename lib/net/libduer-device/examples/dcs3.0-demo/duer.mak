#
# Copyright (2017) Baidu Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

#
# Build for dcs3.0 linux demo
#

include $(CLEAR_VAR)

MODULE_PATH := $(BASE_DIR)/examples/dcs3.0-demo

LOCAL_MODULE := dcs3-linux-demo

selected_modules := $(call get_current_selected_modules,$(lightduer_supported_modules))

LOCAL_STATIC_LIBRARIES :=  connagent coap dcs \
    voice_engine ntp port-linux \
    framework nsdl speex mbedtls cjson

LOCAL_SRC_FILES := \
    $(MODULE_PATH)/duerapp.c \
    $(MODULE_PATH)/duerapp_profile_config.c \
    $(MODULE_PATH)/duerapp_recorder.c \
    $(MODULE_PATH)/duerapp_media.c \
    $(MODULE_PATH)/duerapp_event.c




LOCAL_INCLUDES += $(BASE_DIR)/platform/include \
    /usr/include/gstreamer-1.0 \
    /usr/include/glib-2.0/ \
    /usr/lib/i386-linux-gnu/glib-2.0/include \
    /usr/lib/i386-linux-gnu/gstreamer-1.0/include

LOCAL_LDFLAGS := -lm \
    -lrt \
    -lasound \
    -pthread \
    -lgstreamer-1.0 \
    -lgobject-2.0 \
    -lglib-2.0

include $(BUILD_EXECUTABLE)
