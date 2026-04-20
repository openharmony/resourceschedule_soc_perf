# soc_perf 模块 - AI 编码代理指南
 
## 基本信息
 
### 功能描述
 
soc_perf 是资源调度子系统的核心模块，采用客户端-服务端架构，负责 SOC 的统一调频管理。该模块通过接收调频插件的事件并进行仲裁，实现对 CPU/GPU/DDR/NPU 等资源的频率控制，支持配置驱动、线程安全、权限控制、异步处理和 DFX 追踪等特性。
 
### 核心功能
 
- **调频事件接收与仲裁**：接收调频插件的事件，进行多请求仲裁，确保调频决策的合理性
- **频率控制**：通过内核接口控制 CPU/GPU/DDR/NPU 等资源频率，实现性能与功耗的平衡
- **配置管理**：XML 驱动的配置文件，支持灵活调频策略和设备模式匹配
- **限频管理**：支持热限频和功耗限频，防止系统过热和功耗超标
- **设备模式管理**：支持不同设备模式下的调频策略，适应不同使用场景
- **性能统计**：记录调频事件统计信息，用于性能分析和问题定位
- **权限控制**：通过访问令牌验证调用权限，确保系统安全
- **DFX 追踪**：提供 HiTrace 追踪链和 Dump 接口，支持问题诊断和调试
 
## 架构及依赖
 
### 核心架构原则
 
soc_perf 采用**分层架构设计**，各层职责清晰，相互解耦：
 
- **Client 层**：客户端代理，单例管理，通过 IPC 与服务端通信
- **Server 层**：系统服务，实现 IPC 接口，进行权限验证和请求分发
- **Core 层**：核心业务逻辑，调频请求处理、设备模式管理、热级别处理
- **DFX 层**：诊断和可观测性，HiTrace 追踪链、调试信息输出
- **Common 层**：公共工具，日志宏、LRU 缓存、追踪工具
 
### 设计模式
 
- **单例模式**：客户端和配置管理采用单例模式，确保全局唯一实例
- **策略模式**：设备模式匹配采用策略模式，支持不同模式的灵活切换
- **观察者模式**：服务死亡监听采用观察者模式，自动重连服务
- **RAII**：线程封装和配置管理使用 RAII 管理资源生命周期
- **工厂模式**：配置管理采用工厂模式创建不同类型的配置对象
 
### 核心流程
 
#### 插件初始化流程
 
1. 加载 XML 配置文件
2. 创建线程封装对象
3. 初始化资源节点信息
4. 注册系统能力服务
5. 启动 IPC 监听
 
#### 事件处理流程
 
1. 客户端接收调频请求
2. 通过 IPC 发送到服务端
3. 服务端权限验证
4. 匹配设备模式和 cmdId
5. 仲裁调频动作
6. 执行频率控制
7. 更新统计信息
 
#### 调频请求流程
 
1. 接收调频请求（PerfRequest/PerfRequestEx）
2. 根据配置文件查找对应的调频动作
3. 应用热级别和限频限制
4. 仲裁候选值
5. 执行调频动作
6. 记录统计信息
 
#### 异常检测机制
 
1. 检查服务状态
2. 验证权限有效性
3. 检测配置文件合法性
4. 监控调频频率限制
5. 自动降级处理
 
## 目录结构
 
```
soc_perf/
├── common/                      # Common 层详细设计见 [COMMON.md](common/COMMON.md)
│   └── include/
│       ├── res_sched_log.h              # 日志宏定义
│       ├── lru_cache.h                  # LRU 缓存工具
│       └── res_sched_trace.h            # 追踪工具
├── interfaces/                  # Interfaces 层详细设计见 [INTERFACES.md](interfaces/INTERFACES.md)
│   └── inner_api/socperf_client/
│       ├── include/
│       │   ├── socperf_client.h         # 客户端接口
│       │   └── socperf_action_type.h    # 动作类型定义
│       ├── src/
│       │   └── socperf_client.cpp       # 客户端实现
│       └── ISocPerf.idl                 # IDL 接口定义
├── profile/                     # XML 配置文件
│   └── socperf_config.xml              # 调频配置文件
├── sa_profile/                  # 系统能力配置
│   └── 1906.json                       # 系统能力配置
├── services/                    # 服务端实现
│   ├── core/                    # Core 层详细设计见 [CORE.md](services/core/CORE.md)
│   │   ├── include/
│   │   │   ├── socperf.h                # 核心调频类
│   │   │   ├── socperf_config.h         # 配置管理类
│   │   │   ├── socperf_thread_wrap.h    # 线程封装类
│   │   │   └── socperf_common.h         # 公共定义
│   │   └── src/
│   │       ├── socperf.cpp              # 核心调频实现
│   │       ├── socperf_config.cpp       # 配置管理实现
│   │       └── socperf_thread_wrap.cpp  # 线程封装实现
│   ├── server/                  # Server 层详细设计见 [SERVER.md](services/server/SERVER.md)
│   │   ├── include/
│   │   │   └── socperf_server.h         # 服务端类
│   │   └── src/
│   │       └── socperf_server.cpp       # 服务端实现
│   └── dfx/                     # DFX 层详细设计见 [DFX.md](services/dfx/DFX.md)
│       ├── include/
│       │   └── socperf_hitrace_chain.h  # HiTrace 追踪链
│       └── src/
│           └── socperf_hitrace_chain.cpp # 追踪链实现
├── test/                        # 测试目录
│   ├── unittest/                # 单元测试
│   │   ├── socperf_server_test.cpp       # 服务端测试
│   │   ├── socperf_sub_test.cpp          # 子模块测试
│   │   ├── socperf_sub_mock_test.cpp     # Mock 测试
│   │   ├── socperf_lru_cache_test.cpp    # LRU 缓存测试
│   │   └── dfx/
│   │       └── socperf_hitrace_chain_test.cpp # 追踪链测试
│   ├── fuzztest/                # 模糊测试
│   │   ├── perfrequest_fuzzer.cpp        # PerfRequest 模糊测试
│   │   ├── perfrequestex_fuzzer.cpp      # PerfRequestEx 模糊测试
│   │   ├── limitrequest_fuzzer.cpp       # LimitRequest 模糊测试
│   │   ├── setstatus_fuzzer.cpp          # SetStatus 模糊测试
│   │   ├── setthermallevel_fuzzer.cpp    # SetThermalLevel 模糊测试
│   │   ├── powerlimitboost_fuzzer.cpp    # PowerLimitBoost 模糊测试
│   │   ├── thermallimitboost_fuzzer.cpp  # ThermalLimitBoost 模糊测试
│   │   ├── requestcmdidcount_fuzzer.cpp  # RequestCmdIdCount 模糊测试
│   │   ├── requestdevicemode_fuzzer.cpp  # RequestDeviceMode 模糊测试
│   │   ├── loadconfigxmlfile_fuzzer.cpp  # 配置文件加载模糊测试
│   │   └── socperf_fuzzer.cpp            # 综合模糊测试
│   └── testutil/                # 测试工具
│       └── socperf_test.cpp               # 测试工具
├── BUILD.gn                     # 主构建文件
├── soc_perf.gni                 # GN 配置变量
├── bundle.json                  # 组件配置
└── AGENTS.md                    # 本文件
```