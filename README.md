# resourceschedule_soc_perf

#### 介绍
## SocPerf

### Interfaces

Supported SocPerf interfaces description

| Interface  | Description  |
|----------|-------|
| PerfRequest(int32_t cmdId, const std::string& msg) | Used for Performace boost freq |
| PerfRequestEx(int32_t cmdId, bool onOffTag, const std::string& msg) | Used for Performace boost freq and support ON/OFF |
| PowerLimitBoost(bool onOffTag, const std::string& msg) | Used for Power limit freq which cannot be boosted |
| ThermalLimitBoost(bool onOffTag, const std::string& msg) | Used for Thermal limit freq which cannot be boosted |
| LimitRequest(int32_t clientId, const std::vector<int32_t>& tags, const std::vector<int64_t>& configs, const std::string& msg) | Used for Power or Thermal limit freq and multiple freq items can be set together |

All interfaces are based on the key parameter cmdID, cmdID connects scenes and configs, which is used to boost freq or limit freq.
Interface with parameter onOffTag means it support ON/OFF event. Normally, these interfaces are used for long-term event,
which needs user to turn on or turn off manually.
Parameter msg is used for extra information, like client's pid and tid.

### Configs

Config files description

| File  | Description  |
|----------|-------|
| socperf_resource_config.xml | Define resource which can be modify，such as CPU/GPU/DDR/NPU |
| socperf_boost_config.xml | Config file used for Performace boost |

All xml files are different for particular products.
For specific product, all resources which could be modify are defined in socperf_resource_config.xml. Each resource has its own resID.
The cmdID in the socperf_boost_config.xml/socperf_resource_config.xml/socperf_thermal_config.xml must be different.