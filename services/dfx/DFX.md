# DFX 层详细设计
 
## 概述
 
DFX 层提供 soc_perf 模块的诊断和可观测性能力，包括 HiTrace 追踪链、调试信息输出等功能，支持问题诊断和性能分析。
 
## 目录结构
 
```
services/dfx/
├── include/
│   └── socperf_hitrace_chain.h  # HiTrace 追踪链
└── src/
    └── socperf_hitrace_chain.cpp # 追踪链实现
```
 
## 核心组件
 
### SocPerfHiTraceChain
 
#### 功能描述
HiTrace 追踪链管理类，负责调频请求的追踪链创建、更新和销毁，支持跨进程追踪。
 
#### 核心职责
- 追踪链创建和管理
- 追踪点记录
- 追踪信息输出
- 性能数据采集
 
#### 主要接口
 
##### 追踪链管理接口
```cpp
void BeginTrace(const std::string& name);
void EndTrace();
void UpdateTrace(const std::string& info);
```
 
##### 追踪点记录接口
```cpp
void RecordTracePoint(const std::string& point);
void RecordTraceValue(const std::string& key, const std::string& value);
```
 
#### 核心数据结构
- `traceId_`: 追踪链 ID
- `traceName_`: 追踪链名称
- `tracePoints_`: 追踪点记录
- `traceValues_`: 追踪值记录
 
#### 追踪流程
 
##### 调频请求追踪流程
1. 调频请求开始：创建追踪链
2. 记录请求参数（cmdId、msg）
3. 记录处理过程（权限验证、模式匹配、仲裁）
4. 记录执行结果（执行的动作、耗时）
5. 调频请求结束：关闭追踪链
 
##### 追踪链生命周期
1. `BeginTrace`: 创建追踪链，分配 traceId
2. `RecordTracePoint`: 记录追踪点
3. `UpdateTrace`: 更新追踪信息
4. `EndTrace`: 关闭追踪链，输出追踪信息
 
## 追踪信息
 
### 追踪点类型
- `REQUEST_START`: 请求开始
- `PERMISSION_CHECK`: 权限验证
- `MODE_MATCH`: 模式匹配
- `ARBITRATION`: 仲裁
- `EXECUTION`: 执行
- `REQUEST_END`: 请求结束
 
### 追踪信息格式
```
TraceId: 12345
TraceName: PerfRequest
TracePoints:
  - REQUEST_START: cmdId=10000, msg=app_launch
  - PERMISSION_CHECK: token_id=xxx, result=true
  - MODE_MATCH: mode=default, match_cmd_id=10000
  - ARBITRATION: candidates=[1000, 1500, 2000], result=2000
  - EXECUTION: cpu_freq=2000, gpu_freq=800
  - REQUEST_END: duration=5ms
```
 
## 性能分析
 
### 性能指标
- 请求处理时间
- 权限验证时间
- 模式匹配时间
- 仲裁时间
- 执行时间
 
### 性能瓶颈分析
通过追踪链分析，可以识别：
- 权限验证是否过慢
- 模式匹配是否复杂
- 仲裁逻辑是否高效
- 执行动作是否耗时
 
## 调试支持
 
### 调试信息输出
- 追踪链信息输出到日志
- 支持按 traceId 查询追踪链
- 支持按 traceName 查询追踪链
 
### 问题定位
通过追踪链信息，可以定位：
- 请求失败原因
- 性能瓶颈位置
- 异常调用链
 
## 设计原则
 
1. **低开销**: 追踪功能不影响正常性能
2. **可配置**: 支持动态开关追踪功能
3. **完整性**: 记录完整的调用链
4. **可观测性**: 提供清晰的追踪信息
 
## 依赖关系
 
- **依赖**: Common 层（日志、追踪）
- **外部依赖**: hitrace (HiTrace 框架), hilog (日志)