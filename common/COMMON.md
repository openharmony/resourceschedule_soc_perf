# Common 层详细设计
 
## 概述
 
公共层提供本模块的公共工具，包括日志宏、缓存和追踪工具，为其他层提供通用支持。
 
## 目录结构
 
```
common/
└── include/
    ├── res_sched_log.h              # 日志宏定义
    ├── lru_cache.h                  # LRU 缓存工具
    └── res_sched_trace.h            # 追踪工具
```
 
## 核心组件
 
### res_sched_log.h
 
#### 功能描述
提供统一的日志宏定义，支持不同级别的日志输出，便于调试和问题定位。
 
#### 主要宏定义
- `SOC_PERF_LOGE`: 错误日志级别
- `SOC_PERF_LOGW`: 警告日志级别
- `SOC_PERF_LOGI`: 信息日志级别
- `SOC_PERF_LOGD`: 调试日志级别
 
#### 使用示例
```cpp
SOC_PERF_LOGE("Failed to init SocPerf config");
SOC_PERF_LOGD("Success to Create All threadWrap threads");
```
 
### lru_cache.h
 
#### 功能描述
提供 LRU (Least Recently Used) 缓存实现，用于缓存频繁访问的数据，提高性能。
 
#### 核心类
- `SocPerfLRUCache<K, V>`: LRU 缓存模板类
 
#### 主要方法
- `Put(key, value)`: 插入键值对
- `Get(key)`: 获取值
- `Remove(key)`: 移除键值对
- `Clear()`: 清空缓存
- `Size()`: 获取缓存大小
 
#### 使用场景
- 权限缓存：在 SocPerfServer 中用于缓存访问令牌权限验证结果
- 配置缓存：缓存频繁访问的配置项
 
### res_sched_trace.h
 
#### 功能描述
提供追踪工具，用于性能分析和调试，支持 HiTrace 追踪链。
 
#### 主要功能
- 追踪点记录
- 性能数据采集
- 调试信息输出
 
#### 使用场景
- 调频请求追踪
- 系统事件追踪
- 性能瓶颈分析
 
## 设计原则
 
1. **可复用性**: 提供通用工具，避免代码重复
2. **性能优化**: LRU 缓存减少重复计算和访问
3. **可观测性**: 日志和追踪提供系统可观测性
4. **线程安全**: 关键组件支持多线程访问
 
## 依赖关系
 
- **被依赖**: Core 层、Server 层、Client 层
- **外部依赖**: hilog (日志), hitrace (追踪)