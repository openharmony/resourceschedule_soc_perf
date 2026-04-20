# Core 层详细设计
 
## 概述
 
Core 层是 soc_perf 模块的核心业务逻辑层，负责调频请求的处理、设备模式管理、热级别处理、配置管理等核心功能。
 
## 目录结构
 
```
services/core/
├── include/
│   ├── socperf.h                # 核心调频类
│   ├── socperf_config.h         # 配置管理类
│   ├── socperf_thread_wrap.h    # 线程封装类
│   └── socperf_common.h         # 公共定义
└── src/
    ├── socperf.cpp              # 核心调频实现
    ├── socperf_config.cpp       # 配置管理实现
    └── socperf_thread_wrap.cpp  # 线程封装实现
```
 
## 核心组件
 
### SocPerf
 
#### 功能描述
核心调频类，负责调频请求的入口和分发、设备模式匹配和管理、热级别处理、Boost 事件统计。
 
#### 核心职责
- 调频请求处理和分发
- 设备模式匹配和管理
- 热级别处理
- 性能统计和监控
 
#### 主要接口
 
##### 初始化接口
```cpp
bool Init();
```
 
##### 调频请求接口
```cpp
void PerfRequest(int32_t cmdId, const std::string& msg);
void PerfRequestEx(int32_t cmdId, bool onOffTag, const std::string& msg);
```
 
##### 限频控制接口
```cpp
void PowerLimitBoost(bool onOffTag, const std::string& msg);
void ThermalLimitBoost(bool onOffTag, const std::string& msg);
void LimitRequest(int32_t clientId, const std::vector<int32_t>& tags,
                 const std::vector<int64_t>& configs, const std::string& msg);
```
 
##### 状态管理接口
```cpp
void SetRequestStatus(bool status, const std::string& msg);
void SetThermalLevel(int32_t level);
void RequestDeviceMode(const std::string& mode, bool status);
```
 
##### 统计查询接口
```cpp
std::string RequestCmdIdCount(const std::string& msg);
```
 
#### 核心数据结构
- `enabled_`: 服务启用状态
- `socperfThreadWrap_`: 线程封装对象
- `limitRequest_`: 限频请求映射
- `recordDeviceMode_`: 记录的设备模式
- `thermalLvl_`: 当前热级别
- `currMode_`: 当前设备模式
- `boostCmdCount_`: Boost 命令计数
- `boostTime_`: Boost 时间统计
 
#### 核心流程
 
##### PerfRequest 流程
1. 检查服务状态和权限
2. 匹配设备模式和 cmdId
3. 获取对应的调频动作
4. 应用热级别和限频限制
5. 仲裁候选值
6. 执行调频动作
7. 更新统计信息
 
##### PerfRequestEx 流程
1. 检查服务状态和权限
2. 匹配设备模式和 cmdId
3. 根据 onOffTag 判断开启或结束
4. 开启：执行 PerfRequest 流程
5. 结束：清除对应的调频请求
6. 更新统计信息
 
### SocPerfConfig
 
#### 功能描述
配置管理类，负责 XML 配置文件解析和加载、资源节点管理、Boost 配置管理。
 
#### 核心职责
- XML 配置文件解析
- 资源节点信息管理
- Boost 配置管理
- 设备模式配置管理
 
#### 主要接口
```cpp
bool Init();
std::shared_ptr<Actions> GetActionsInfo(int32_t cmdId);
void GetResNodeInfo(std::vector<ResNode>& resNodeInfo);
```
 
#### 核心数据结构
- `resourceNodeInfo_`: 资源节点信息映射
- `configPerfActionsInfo_`: Boost 配置信息
- `sceneResourceInfo_`: 场景资源信息
- `interAction_`: 交互配置
 
#### 配置文件格式
XML 格式，包含以下主要节点：
- `ResNodeInfo`: 资源节点信息
- `PerfActions`: 性能动作配置
- `SceneResourceInfo`: 场景资源信息
- `InterAction`: 交互配置
 
### SocPerfThreadWrap
 
#### 功能描述
线程封装类，负责调频动作的线程封装、资源状态管理、候选值仲裁、配对资源仲裁。
 
#### 核心职责
- 调频动作执行
- 资源状态管理
- 候选值仲裁
- 配对资源仲裁
- 弱交互处理
 
#### 主要接口
```cpp
void InitResourceNodeInfo();
void DoFreqActionPack(std::vector<std::shared_ptr<ResActionItem>>& resActionItems);
void DoFreqAction(std::shared_ptr<ResActionItem> resActionItem);
int64_t ArbitrateCandidate(int32_t resId, const std::vector<int64_t>& candidates);
```
 
#### 核心数据结构
- `resNodeInfo_`: 资源节点信息
- `pairResInfo_`: 配对资源信息
- `resActionItems_`: 资源动作项映射
 
#### 仲裁策略
- **候选值仲裁**: 取多个候选值的最大值
- **配对资源仲裁**: 配对资源取相同值
- **弱交互处理**: 弱交互降低优先级
 
## 设计原则
 
1. **分层设计**: 职责清晰，相互解耦
2. **线程安全**: 使用互斥锁保护共享数据
3. **配置驱动**: 通过 XML 配置文件控制行为
4. **仲裁机制**: 多请求仲裁确保合理性
5. **性能优化**: 使用智能指针和缓存
 
## 依赖关系
 
- **依赖**: Common 层（日志、追踪、缓存）
- **外部依赖**: libxml2 (XML 解析), hilog (日志), hitrace (追踪)