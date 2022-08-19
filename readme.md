## Learning Live（学习直播）
### 这是个什么项目？
--- 
这个一个直播项目，其包含客户端以及服务器实现。客户端暂时只会支持windows平台（只是界面显示部分代码与平台相关），客户端可以录制以及播放声音与视频，服务器运行在Linux。

### 为什么有这个项目
--- 
直播领域最火的项目莫过于webrtc了。webrtc的源码，仅仅windows平台，就由1000多个Visual Studio项目组成，其代码不仅庞大而且难于理解。直播涉及的知识实在是太多了，但在这个项目中，我们仅仅选择音视频直播中最基础，同时也是最核心的知识来实现我们的功能。对于直播中锦上添花的一些技术，这个项目并不会涉及。同时，弱网对抗是工业音视频项目中必备的功能，也是最核心的竞争力，我们这里也不会涉及。我们的目标仅仅是要实现一个能够进行音视频直播的学习项目，它只会涉及音视频直播必不可少的技术。

### 项目结构
--- 
* common 基础工具的定义
* audio 音频相关，包含音频采集与播放等
* endec 音频编解码相关
* pc_client 客户端相关，windows客户端，使用Qt+opengl实现
* netrate 带宽预测、拥塞控制相关
* qos 接收与解析流媒体，实现jitter buffer功能等
* rtp_base rtp底层实现
* video 视频解码相关（rtp解包及h264解码）
* windows_capture_test windows平台的视频采集
* endec 音视频编解码相关

### 开发计划
0. 实现流媒体的发送与接收【已经完成】
1. 抗抖动及弱网优化
2. 实现网络带宽预测+码控功能

### 当前开发计划
* 一个随机丢包发生器
* 实现视频包的NACK功能

### 目标
打造一个足够简单，功能刚刚够用的音视频直播项目，一个新手也能轻易运行，并且也能看懂代码的项目。

## 功能设计
1. p2p功能，假设两个对端是无条件可连通的(接收端必须是可连通的)，因此，客户端可以直接将直播数据发送到对端；发送音视频数据之前并没有信令的交互；

## 编译相关
### portaudio [链接](http://portaudio.com/docs/v19-doxydocs/compile_windows.html)

### 单独编译的模块
1. portaudio(portaudio-master.zip)
2. libjpeg-turbo-2.1.3(libjpeg-turbo-2.1.3.zip)
3. jsoncpp(jsoncpp-1.9.5.zip)
4. libyuv(libyuv.zip)
5. uvnet(https://github.com/tlexy/uvnet)

### 编译与运行环境
vs2019

### cmake
1. 在相应项目建立build目录
2. 进入build目录，运行cmake -G "Visual Studio 16 2019" ..