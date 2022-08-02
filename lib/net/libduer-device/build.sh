TOOLS_PATH=/mnt/work/workspace
export PATH=$PATH:${TOOLS_PATH}/esp/xtensa-esp32-elf/bin
export IDF_PATH=${TOOLS_PATH}/esp/esp-idf
export PATH=$PATH:${TOOLS_PATH}/android-ndk-r15c
export PATH=$PATH:${TOOLS_PATH}/esp/xtensa-lx106-elf/bin
export SDK_PATH=${TOOLS_PATH}/esp8266-rtos-sdk
export OECORE_PATH=/usr/local/oecore-x86_64
export PATH=${PATH}:${OECORE_PATH}/sysroots/x86_64-oesdk-linux/usr/bin/aarch64-poky-linux
export PATH=$PATH:${TOOLS_PATH}/mipsel-linux-gnu-4.9.3_64/bin
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${TOOLS_PATH}/mipsel-linux-gnu-4.9.3_64/lib
export MIPS_SYSROOT_PATH=${TOOLS_PATH}/mipsel-linux-gnu-4.9.3_64/mipsel-linux-gnu/sysroot

set -e

rm -rf out
source ./build/envsetup.sh
#freertos_esp32-sdkconfig
lightduerconfig 1
make
#freertos_esp8266-sdkconfig
lightduerconfig 2
make
#freertos_mw300-sdkconfig
lightduerconfig 3
make
#lightduer_storydroid-x86linuxdemoconfig
lightduerconfig 5
make
#linux_mips-democonfig
lightduerconfig 6
make
#linux_mips-sdkconfig
lightduerconfig 7
make
#linux_mtk-democonfig
lightduerconfig 8
make
#linux_x86-democonfig
lightduerconfig 9
make
#linux_x86-speexencoderconfig
lightduerconfig 10
make
#linux_x86-dcs3linuxdemoconfig
lightduerconfig 11
make
#android
#sh android_ndk.sh

HEAD_FOLDER="include"

if [ ! -d "output" ]; then
    mkdir output
fi
cp out/linux/x86/dcs3linuxdemoconfig/modules/dcs3-linux-demo/dcs3-linux-demo output
cd output
mkdir libduer-device
cd libduer-device
mkdir ${HEAD_FOLDER}

find ../../modules -name \*.h -exec cp {} ${HEAD_FOLDER} \;
cp ../../external/baidu_json/baidu_json.h ${HEAD_FOLDER}
cp ../../framework/include/* ${HEAD_FOLDER}
cp ../../framework/core/*.h ${HEAD_FOLDER}
cp ../../platform/include/* ${HEAD_FOLDER}
cp ../../out/freertos/esp32/sdkconfig/modules/duer-device/libduer-device.a .
cd ..
tar -czf libduer-device.tar.gz *
rm -r libduer-device
rm dcs3-linux-demo
cd ..

outfile=output/libduer-device.tar.gz
if [ ! -f "$outfile" ]
then
    echo "$outfile is empty"
    exit 1
fi
