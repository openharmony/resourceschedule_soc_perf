# resourceschedule_soc_perf

#### Description
## 统一调频

### 调频接口说明

当前可支持的调频接口说明

| 接口  | 说明  |
|----------|-------|
| PerfRequest(int32_t cmdId, const std::string& msg) | 用于性能提频使用 |
| PerfRequestEx(int32_t cmdId, bool onOffTag, const std::string& msg) | 用于性能提频使用且支持ON/OFF事件 |
| PowerLimitBoost(bool onOffTag, const std::string& msg) | 用于限制boost无法突破功耗限频 |
| ThermalLimitBoost(bool onOffTag, const std::string& msg) | 用于限制boost无法突破热限频 |
| LimitRequest(int32_t clientId, const std::vector<int32_t>& tags, const std::vector<int64_t>& configs, const std::string& msg) | 用于热或功耗模块的限频且支持多项值一同设置 |

如表格所示，所有的调频接口都以cmdID为核心，将调频场景和调频参数互相关联，实现提频或者限频的功能。  
带onOffTag参数的接口表示该接口支持ON/OFF的开关调频模式，一般用于生效时间不固定的长期调频事件，需要调用者手动开启或者关闭。  
msg参数为拓展字符串信息，可承载例如调用客户端pid/tid等信息。  

### 调频配置说明

当前configs目录下的配置文件

| 配置文件  | 说明  |
|----------|-------|
| socperf_resource_config.xml | 定义产品可支持的资源配置，例如CPU/GPU/DDR/NPU等 |
| socperf_boost_config.xml | 用于性能提频的配置文件 |

各个xml配置文件都需要按产品定制，不同产品的配置不相同。  
对于指定的某产品，所有可支持配置的资源都定义在socperf_resource_config.xml内，支持单路径/多路径配置，任何资源都有唯一的resID。  
socperf_boost_config.xml使用的cmdID不能重复。  

### 调频使用举例

以点击提频事件为例。  

点击场景提频为固定生效时间，无需使用ON/OFF手动开启或关闭，故使用PerfRequest接口即可。因此需要在合适的地方调用soc_perf提供的IPC接口PerfRequest实现提频请求。  

资源调度服务对所有事件的采集有统一的入口，即资源调度框架对订阅事件的插件进行分发的机制，调频服务也依赖该机制实现对于各个需要调频的场景的感知。  

调频服务已实现为一个单独的调频插件socperf_plugin，定义在/ressched/plugins/socperf_plugin目录下。  
要实现点击场景的提频，分为点击事件插桩、事件上报给资源调度框架、框架给插件分发事件、生效调频服务四个步骤。  

第一步，  
ACE子系统仓内实现了对资源调度框架提供的可动态加载接口ReportData的封装，路径为/framework/base/ressched/ressched_report.h
并在/framework/core/gestures/click_recognizer.cpp增加了打点作为手势识别中对点击事件的判定  

第二步，  
通过动态加载libressched_client.z.so，调用资源调度框架提供的接口ReportData，ACE子系统将点击事件上报给全局资源调度子系统  

第三步，  
在资源调度框架里，点击事件类型定义在/ressched/interfaces/innerkits/ressched_client/include/res_type.h内，为RES_TYPE_CLICK_RECOGNIZE  
由于调频插件socperf_plugin在初始化过程中已完成了对该点击事件的订阅，因此框架会在接受到事件通知时将点击事件分发给调频插件  

第四步，  
调频插件socperf_plugin对于点击事件分配了cmdID：PERF_REQUEST_CMD_ID_EVENT_CLICK，路径在/ressched/plugins/socperf_plugin/src/socperf_plugin.cpp下  
通过调用调频服务提供的IPC接口PerfRequest，插件会给调频服务发送请求，实现点击提频功能。

