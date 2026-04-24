# Interfaces 层详细设计
 
## 概述
 
Interfaces 层提供 soc_perf 模块的对外接口，包括客户端接口定义、动作类型定义和 IDL 接口定义，是模块与外部交互的入口。
 
## 目录结构
 
```
interfaces/
└── inner_api/socperf_client/
    ├── include/
    │   ├── socperf_client.h         # 客户端接口
    │   └── socperf_action_type.h    # 动作类型定义
    ├── src/
    │   └── socperf_client.cpp       # 客户端实现
    └── ISocPerf.idl                 # IDL 接口定义
```
 
## 核心组件
 
### socperf_client.h
 
#### 功能描述
提供客户端接口，采用单例模式，通过 IPC 与服务端通信，支持服务死亡监听和自动重连。
 
#### 核心类
- `SocPerfClient`: 客户端单例类
 
#### 主要接口
 
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
 
#### 核心特性
- **单例模式**: `GetInstance()` 获取全局唯一实例
- **服务死亡监听**: `SocPerfDeathRecipient` 监听服务端状态
- **自动重连**: 服务重启后自动重新连接
- **PID/TID 信息**: 自动添加调用者进程和线程信息
 
### socperf_action_type.h
 
#### 功能描述
定义调频动作类型和资源类型，用于配置文件和接口参数。
 
#### 主要定义
- `ActionType`: 动作类型枚举（CPU、GPU、DDR、NPU 等）
- `ResActionItem`: 资源动作项结构
- `Actions`: 动作集合结构
- `Action`: 单个动作结构
 
### ISocPerf.idl
 
#### 功能描述
定义 IPC 接口，用于客户端和服务端之间的通信。
 
#### 主要接口
与 `socperf_client.h` 中的接口一一对应，通过 IDL 编译器生成 IPC 代码。
 
## 设计原则
 
1. **单例模式**: 客户端采用单例，确保全局唯一
2. **IPC 通信**: 通过 Binder IPC 与服务端通信
3. **自动重连**: 服务死亡后自动重连，提高可靠性
4. **权限验证**: 客户端不验证权限，由服务端统一验证
5. **线程安全**: 使用互斥锁保护共享数据
 
## 核心流程
 
### 客户端初始化流程
1. 调用 `GetInstance()` 获取单例
2. 连接服务端（SAMGR）
3. 注册服务死亡监听
4. 初始化完成
 
### 调频请求流程
1. 客户端调用 `PerfRequest` 接口
2. 添加 PID/TID 信息
3. 通过 IPC 发送到服务端
4. 等待服务端处理
5. 返回结果
 
### 服务死亡处理流程
1. 服务端死亡
2. `OnRemoteDied` 被调用
3. 清理客户端状态
4. 等待服务重启
5. 自动重新连接
 
## 依赖关系
 
- **依赖**: Common 层（日志、追踪）
- **外部依赖**: ipc (IPC 通信), samgr (服务管理), hilog (日志)