## Learning Live（学习直播）
### 这是个什么项目？
--- 
这个一个直播项目，其包含客户端以及服务器实现。客户端暂时只会支持windows平台（只是界面显示部分代码与平台相关），客户端可以录制以及播放声音与视频，服务器运行在Linux。

### 为什么有这个项目
--- 
直播领域最火的项目莫过于webrtc了。webrtc的源码，仅仅windows平台，就由1000多个Visual Studio项目组成，其代码不仅庞大而且难于理解。直播涉及的知识实在是太多了，但在这个项目中，我们仅仅选择音视频直播中最基础，同时也是最核心的知识来实现我们的功能。对于直播中锦上添花的一些技术，这个项目并不会涉及。同时，弱网对抗是工业音视频项目中必备的功能，也是最核心的竞争力，我们这里也不会涉及。我们的目标仅仅是要实现一个能够进行音视频直播的学习项目，它只会涉及音视频直播必不可少的技术。

### 项目结构
--- 
* audio 音频相关，包含音频采集与播放等
* pc_client 客户端相关，windows客户端，使用Qt+SDL实现
* learnsvr 服务器
* net_base 封装的跨平台网络接口
* netrate 带宽预测、拥塞控制相关
* qos Qos相关
* rtp_base rtp低层实现
* video windows平台的视频采集
* endec 音视频编解码相关

### 开发及进度
项目将会是从零开始开发，欢迎大家参与。

### 目标
打造一个足够简单，功能刚刚够用的音视频直播项目，一个新手也能轻易运行，并且也能看懂代码的项目。

## 功能设计
服务器有房间的概念，一个三元组（appid,roomid, uid）唯一标识了服务器上的一个用户。服务器上的用户有两种类型：主播以及观众。主播既向服务器推送流，也向服务器拉取流。其观众仅仅是从指定房间中拉取流，不能上传流。