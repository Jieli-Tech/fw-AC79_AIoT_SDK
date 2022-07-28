# AC79 系列WiFi&蓝牙 AIoT固件程序



快速开始
------------

欢迎使用杰理AC79开源项目，在开始进入项目之前，请详细阅读以下芯片介绍获得对AC79系列芯片大概认识，SDK固件包不含开发文档，进行开发之前详细阅读SDK开发文档:[https://doc.zh-jieli.com/AC79/zh-cn/release_v1.0.3/index.html](https://doc.zh-jieli.com/AC79/zh-cn/release_v1.0.3/index.html)，为用户提供完善的开发例程，帮助开发者快速顺利地使用AC79系列芯片进行方案开发。



芯片概述
------------

杰理AC79系列是一款低成本高集成度WiFi  802.11b/g/n以及双模蓝牙V2.1到V5.0组合的音视频多媒体系统级Soc。内部集成了主频高达320MHz的双核浮点DSP处理器，自带D-cache、I-cache为各类方案提供了强大的运算能力；并完整支持了单天线40MHz BW WiFi 802.11b/g/n  AP和STA各种通讯模式；通过内部集成PTA共存分时设计模块，使得WiFi/蓝牙V2.1/蓝牙V5.0可同时工作，实现灵活和高性能的无线传输能力；芯片同时集成ADC/DAC接口作为音频处理资源、摄像头ISC接口作为视频处理资源、RGB推屏接口作为UI显示资源，方便实现各种高集成度的音视频多媒体处理方案，同时自带PMU模块提供多种低功耗工作模式，能使用LDO或者DCDC供电模式满足不同方案供电管理需求。



芯片应用场景
------------

- 儿童绘本故事机
- 点读笔/扫描笔/翻译笔
- WiFi蓝牙智能音箱
- 可视门铃/视频门锁/楼宇智能
- WiFi监控摄像头IP Camera
- WiFi可视美容仪
- 蓝牙/USB扫码枪
- 婴儿监护器
- 宠物喂食机
- WiFi摄像头玩具
- 智能家居/物联网设备



芯片软硬件资源介绍
------------

### CPU

- 双核DSP，最高主频320Mhz，支持单精度浮点以及数学运算加速引擎，带I-cache、D-cache、MMU功能，片上集成了共578K字节SRAM，部分封装支持2/8M字节SDRAM

  

### 外设

- GPIO、IIC、SPI、SDIO、PWM、MCPWM、UART、USB1.1、USB2.0、ADC、TIMER、IR接收、电容触摸按键、GPCNT、RTC

  

### MATH

- 支持硬件FFT、IFFT、矩阵运算

- 支持硬件AES128/256

- 支持硬件SHA128/256

- 支持硬件随机数

- 支持硬件CRC16

  

### 蓝牙

- 符合蓝牙V5.0+BR+EDR+BLE规范

- 支持蓝牙微微网和散射网

- 满足class2和class3发射功率要求

- 支持 GFSK 和 π/4 DQPSK 所有包类型

- 提供+15dbm发射功率

- 接收器灵敏度为 -93dBm

  

### WiFi

- 支持 IEEE 802.11b/g/n

- 802.11n支持 MCS0~ MCS7、20MHz/40MHz 带宽

- 支持800ns 和 400ns 保护间隔

- 支持AP模式、STA模式、monitor配网模式

- AP模式支持多个基站接入

- STA模式支持保存多个连接网络，匹配信号最好的网络去连接

- 支持STA模式冷启动快连

- 支持power save mode省电模式

- 支持Open System、WEP、WPA-PSK/WPA2-PSK + TKIP/AES/CCMP加密方式

- 支持脱离802.11协议，直接利用底层RF收发数据包

- 支持连接CMW270等测试仪测试板子RF性能

- 发射功率: 
  DSSS 1M/S		17  dBm
  MCS0			     16  dBm
  MCS7			     12	dBm

- 接收灵敏度:
  DSSS 1M/S		-95  dBm
  MCS0			     -91  dBm
  MCS7			     -72  dBm

  

### 音频

- 集成DAC、MIC、LINEIN、IIS、PDM、SPDIF音频硬件模块，其中IIS模块最高支持8个通道同时工作，可单独设置成输入或者输出、支持16/24bit数据位宽
  PDMLINK模块支持同时接入4路16bit数据位宽的数字麦，ADC支持4个通道同时工作，每通道皆可支持配置成MIC或者LINEIN
- 支持SBC、MSBC、CVSD、AAC、MP2、MP3、ADPCM、AMR、OPUS、SPX、WAV、PCM音频编码格式，编码数据源支持MIC、LINEIN、IIS、PDMLINK、SPDIF和虚拟数据源
- 支持SBC、MSBC、CVSD、AAC、ADPCM、AMR、APE、DTS、FLAC、M4A、MP1、MP2、MP3、OPUS、SPX、WAV、WMA、PCM音频解码格式，解码数据源支持FLASH、SD卡、U盘、LINEIN、外挂FM模块、网络URL、经典蓝牙、虚拟数据源、客户自定义解密数据源
- 音效处理支持混响、回声、电音、变声变调、变速、移频、啸波抑制、EQ、DRC、回声消除、传统降噪、神经网络降噪
- 语音识别支持活动语音检测VAD、单/双mic的打断唤醒ASR功能
- 部分解码格式支持断点播放、快进快退、定点播放、AB点复读播放



### 视频

- 支持DVP-1/2/4/8bit、 BT656图像传感器接口的YUV sensor，最大支持720P分辨率
- JPEG编码最大支持720P@30fps@AVI封装
- 支持任意尺寸JPEG单张编解码
- 支持SPI接口摄像头
- 支持一路DVP摄像头 + 一路SPI摄像头， 可同时输出YUV，一路联动JPEG图传或录卡，另一路做光流算法
- 支持图像拼接功能，图像分辨率为176*128，帧率可高达60帧
- 支持软件对摄像头YUV帧缩放、裁剪



### 显示

- 支持SPI、EMI、PAP、RGB888(8bit)/RGB666(6bit) 推屏接口，其中RGB推屏可达480\*272@15fps，320\*240@30fps
- 杰理UI工具可支持屏触摸、软件显示图层、软件旋转图像、音标显示、自定义合成显示区域、支持不同大小字体同时显示，支持SD卡加载UI资源文件
- 支持播放JPG、AVI、GIF文件



###  网络协议栈

- 基础协议支持:lwip、mbedtls、http/https、websocket、coap、nopoll、curl、mqtt、ftp、uip、iperf

- AI云平台支持:图灵、百度云、腾讯云、中国电信智能家居平台、涂鸦、阿里云、华为hilink、天猫精灵、亚马逊平台、思必弛、玩瞳

  

### 固件升级

- 支持U盘/SD卡单备份升级
- 支持U盘/SD卡/WIFI双备份升级
- 支持代码双备份+资源部分备份+资源部分固定方式升级



### WiFi测试盒

- 支持发射功率、灵敏度、频率偏差参数测试，频率偏差校正
- 支持UI人机交互
- 支持双模块同步测试
- 支持传导、空中两种测试方式
- 支持样本标定与样本筛选
- 支持CDROM，存储上位机软件及使用文档
- 支持上位机参数配置、本地固件升级、出厂校准
- 支持辅助通信（UART）



### SDK中间件

- FAT文件系统
- 数据存储记忆
- FreeRtos/Pthread API
- 循环CBUF
- 帧LBUF



参考资源
------------

* 芯片数据手册&原理图 : [doc/datasheet/AC791N规格书](./doc/datasheet/AC791N规格书)
* SDK 发布版本信息 : [AC79NN_SDK_发布版本信息](https://doc.zh-jieli.com/AC79/zh-cn/release_v1.0.3/other/version/index.html)
* 钉钉技术支持群号 : 31275808
