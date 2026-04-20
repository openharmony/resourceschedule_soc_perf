# Server 层详细设计
 
## 概述
 
Server 层是 soc_perf 模块的服务端实现，负责系统能力服务管理、IPC 接口实现、权限验证、Dump 接口等功能。
 
## 目录结构
 
```
services/server/
├── include/
│   └── socperf_server.h         # 服务端类
└── src/
    └── socperf_server.cpp       # 服务端实现
```
 
## 核心组件
 
### SocPerfServer
 
#### 功能描述
服务端类，继承自 SystemAbility 和 SocPerfStub，实现系统能力服务和 IPC 接口，提供权限验证和 Dump 功能。
 
#### 核心职责
- 系统能力服务管理
- IPC 接口实现
- 权限验证
- Dump 接口实现
- 服务生命周期管理
 
#### 主要接口
 
##### 系统能力接口
```cpp
void OnStart();
void OnStop();
```
 
##### 调频请求接口（IPC）
```cpp
ErrCode PerfRequest(int32_t cmdId, const std::string& msg);
ErrCode PerfRequestEx(int32_t cmdId, bool onOffTag, const std::string& msg);
ErrCode PowerLimitBoost(bool onOffTag, const std::string& msg);
ErrCode ThermalLimitBoost(bool onOffTag, const std::string& msg);
ErrCode LimitRequest(int32_t clientId, const std::vector<int32_t>& tags,
                     const std::vector<int64_t>& configs, const std::string& msg);
```
 
##### 状态管理接口（IPC）
```cpp
ErrCode SetRequestStatus(bool status, const std::string& msg);
ErrCode SetThermalLevel(int32_t level);
ErrCode RequestDeviceMode(const std::string& mode, bool status);
```
 
##### 统计查询接口（IPC）
```cpp
ErrCode RequestCmdIdCount(const std::string& msg, std::string& funcResult);
```
 
##### Dump 接口
```cpp
int32_t Dump(int32_t fd, const std::vector<std::u16string>& args);
```
 
#### 核心数据结构
- `socPerf`: Core 层的 SocPerf 实例
- `permissionCache_`: 权限 LRU 缓存
- `permissionCacheMutex_`: 权限缓存互斥锁
 
#### 核心流程
 
##### 服务启动流程
1. `OnStart()` 被调用
2. 初始化 SocPerf 实例
3. 调用 `SocPerf::Init()`
4. 发布服务到 SAMGR
5. 服务启动完成
 
##### IPC 请求处理流程
1. 客户端通过 IPC 发送请求
2. `HasPerfPermission()` 验证权限
3. 调用 Core 层对应接口
4. 返回结果
 
##### 权限验证流程
1. 获取调用者的访问令牌 ID
2. 检查权限缓存
3. 缓存命中：直接返回结果
4. 缓存未命中：查询权限并更新缓存
5. 返回验证结果
 
##### Dump 处理流程
1. `AllowDump()` 验证权限
2. 根据参数执行不同的 Dump 命令
3. 输出调试信息到 fd
4. 返回成功
 
## 权限管理
 
### 权限类型
- `ohos.permission.RESOURCE_SCHEDULE_AGENT`: 资源调度代理权限
 
### 权限验证机制
- 使用 LRU 缓存提高性能
- 缓存键：访问令牌 ID
- 缓存值：权限验证结果
- 缓存大小：可配置
 
### 权限验证流程
1. 获取调用者的访问令牌 ID
2. 检查权限缓存
3. 缓存命中：直接返回结果
4. 缓存未命中：查询权限并更新缓存
5. 返回验证结果
 
## Dump 命令
 
### 支持的命令
- `-h`: 显示帮助信息
- `-a`: 显示所有信息
- `-s`: 显示统计信息
- `-c`: 显示配置信息
- `-r`: 显示资源信息
 
### Dump 输出格式
```
SocPerf Dump:
- Service Status: enabled
- Thermal Level: 0
- Device Mode: default
- Boost Count: 100
```
 
## 设计原则
 
1. **系统能力服务**: 继承 SystemAbility，集成到系统服务框架
2. **IPC 通信**: 通过 IDL 生成的 Stub 实现接口
3. **权限控制**: 统一权限验证，确保系统安全
4. **性能优化**: 使用 LRU 缓存减少权限查询开销
5. **可观测性**: 提供 Dump 接口便于调试
 
## 依赖关系
 
- **依赖**: Core 层（业务逻辑）、Common 层（日志、缓存）
- **外部依赖**: safwk (系统能力框架), samgr (服务管理), ipc (IPC 通信), access_token (权限管理)