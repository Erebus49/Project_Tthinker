# v0 与衍生版本 v1-v4 逐行差异完整报告

## 1. 范围与方法

本报告覆盖以下目录之间的对比：
- Tthinker-main-v0
- Tthinker-main-v1
- Tthinker-main-v2
- Tthinker-main-v3
- Tthinker-main-v4

对比方法：
- 使用目录级差异识别变更文件。
- 使用 numstat 统计每个源码文件的新增行/删除行。
- 提取关键文件差异区块的行号范围（hunk headers）进行逐行定位。
- 重点聚焦源码文件（.h/.cpp/.md/.sh），将可执行文件、.o、输出样例文件等作为噪声单列说明。

## 2. 总体结论（先看）

1. v1 是一次大改，核心是引入线程统计摘要文件机制，同时把运行稳定性（信号处理、异常捕获）纳入主流程。
2. v2 对 v1 的输出机制做了重构：放弃全局摘要文件，改为每线程独立输出文件，并记录每个发现团的时间戳与顶点列表。
3. v3 在 v2 基础上继续收敛输出：只保留线程级最大团大小与首次时间，不再输出每个团明细。
4. v4 在 v3 基础上新增“最大运行时长”控制（--max-runtime），并完善停止路径；同时补充批量脚本与 README 文档。
5. 你当前新增的 v4 修改（最大团后追加团成员）属于 v3->v4 输出机制的一次再增强，与演进主线一致。

## 3. 源码变更规模统计

按 v0 对比：
- v1: files=15, add=1219, del=599, net=+620
- v2: files=14, add=618, del=34, net=+584
- v3: files=14, add=634, del=46, net=+588
- v4: files=16, add=3898, del=65, net=+3833
- v4 去除 run_p4.sh 后: files=15, add=802, del=65, net=+737

解释：
- v4 的 3898 行新增主要由 app_qc/run_p4.sh 的 3096 行脚本造成。
- 若关注核心 C++ 代码，v4 的规模与 v1-v3 同量级。

## 4. 逐版本逐行差异详述

### 4.1 v0 -> v1

#### 4.1.1 系统层输出模型重构（global/comper/worker）

关键改动点：
- 引入全局停止标志与线程统计容器。
- 新增线程摘要写盘流程（thread_summary.txt 或自定义路径）。
- worker 初始化时显式初始化输出摘要容器。
- comper 线程启动时注册 thread_id 到统计系统。

定位：
- system/global.h 差异区块：
  - @@ -13,0 +14,6
  - @@ -28,0 +35,1
  - @@ -95,0 +103,166
  - @@ -148,0 +322,276
- system/worker.h 差异区块：
  - @@ -52,1 +52,9
  - @@ -73,0 +90,27
  - @@ -265,0 +309,10
- system/comper.h 差异区块：
  - @@ -388,0 +389,2
  - @@ -392,0 +395,1

关键行为证据：
- global_stop_requested 定义：v1/system/global.h:35
- 输出摘要初始化入口：v1/system/worker.h:73
- comper 绑定线程 id：v1/system/comper.h:395

#### 4.1.2 app_qc/app_kernel 入口稳定性增强

改动内容：
- run.cpp 新增 signal 处理（SIGINT/SIGTERM/SIGHUP/SIGABRT/SIGSEGV 等）。
- main 包裹 try/catch，失败时输出 Fatal error。
- 增加更多运行参数打印与计时输出。

定位：
- app_qc/run.cpp 差异区块开头：@@ -24,0 +25,2 与 @@ -28,0 +31,10
- app_kernel/run.cpp 差异区块开头：@@ -24,0 +25,2 与 @@ -27,0 +30,10

#### 4.1.3 输出语义变化（graph）

v0 的 Output1Clique 是写出完整团；v1 改为仅记录统计，不直接写团内容。
- v0: 输出 size + 顶点列表。
- v1: 调用 record_quasi_clique(nclique_size)。

定位：
- v0/app_qc/graph.h:587 附近
- v1/app_qc/graph.h:587 附近

#### 4.1.4 注意事项

- v1 对 app_kernel/kernel_app.h 显示大规模文本变动（+536/-555），忽略空白后仍有 78/97 行逻辑差异，说明不是纯格式改动。

### 4.2 v0 -> v2

#### 4.2.1 输出模型从“单摘要文件”切到“每线程结果文件”

改动内容：
- global.h 中不再围绕 THREAD_SUMMARY_FILE_PATH 与 record_quasi_clique 容器写入。
- 改为 THREAD_OUTPUT_ROOT_DIR / THREAD_OUTPUT_BENCHMARK_DIR / THREAD_OUTPUT_FILE_PREFIX 路径拼装。
- comper::start 在启动时直接打开线程输出文件。

定位：
- v2/system/global.h 大区块新增：@@ -148,0 +166,344
- v2/system/comper.h start 处：读取 build_thread_output_file_path(thread_id)

#### 4.2.2 输出粒度变化

v2 的 app_qc/graph.h 在 Output1Clique 中直接写出：
- 团大小
- 线程内相对时间（秒，%.6f）
- 团顶点列表

这意味着 v2 是“全量发现结果输出”模型。

定位：
- v2/app_qc/graph.h:587 附近

#### 4.2.3 任务生命周期健壮性

q c_app.h 中新增 ContextValue 初始化默认值、task_enqueued 防双删/泄漏、load_data 时重置 spawned_num。

定位：
- v2/app_qc/qc_app.h 差异区块：
  - @@ -41,0 +42,4
  - @@ -394,0 +401,2
  - @@ -497,0 +510,1

### 4.3 v0 -> v3

#### 4.3.1 在 v2 基础上收敛输出到“线程级最大团统计”

改动内容：
- Output1Clique 不再输出每个团明细。
- 改为更新 thread_max_clique_size 与 thread_max_clique_time。
- comper 线程退出时写一行：最大团大小 + 首次时间。

定位：
- v3/app_qc/graph.h:589 附近
- v3/system/global.h 新增 thread_max_clique_size / thread_max_clique_time
- v3/system/comper.h run 末尾新增输出

#### 4.3.2 非源码管理痕迹

v3 出现：
- patch_global.sed
- system/global.h.orig
- system/global.h.rej

这表明 v3 可能经历过补丁冲突或手工合并遗留。

### 4.4 v0 -> v4

#### 4.4.1 运行时上限控制（新增核心能力）

改动内容：
- run.cpp 支持 --max-runtime=SECONDS 或 --max-runtime SECONDS（最后参数）。
- 参数校验失败时输出 Missing value for --max-runtime。
- 将 MAX_RUNTIME_SEC 与 global_program_start_time 注入全局。
- worker 主循环按 elapsed>=MAX_RUNTIME_SEC 触发 global_stop_requested。

定位：
- v4/app_qc/run.cpp 差异主区块：@@ -32 +46,57
- v4/system/global.h:36（MAX_RUNTIME_SEC）
- v4/system/worker.h:306-313（超时触发 stop）

#### 4.4.2 输出格式和稳定性细化

改动内容：
- comper::run 进入时设置 setlocale(LC_NUMERIC, "C")，保证小数点格式统一。
- 线程退出时写线程级最大团摘要（v4 当前已被你进一步扩展为“大小 时间 团成员”）。

定位：
- v4/system/comper.h:420（setlocale）
- v4/system/comper.h:439-444（写线程摘要）

#### 4.4.3 算法流程可中断性增强

在 app_qc/qc_app.h 和 app_kernel/kernel_app.h 中增加多处 global_stop_requested 检查，避免超时后继续深搜。

定位：
- v4/app_qc/qc_app.h:233, 243, 426, 454
- v4/app_kernel/kernel_app.h 对应 Expand/compute 区块

#### 4.4.4 工程化增强

新增：
- app_qc/run_p4.sh（大脚本，3096 行）
- README 更新（参数说明与流程说明）

## 5. 关键文件逐版本增删行表（源码）

以下列出最核心文件的 numstat（v0 对比）：

### 5.1 app_qc
- run.cpp
  - v1: +66 / -17
  - v2: +54 / -5
  - v3: +54 / -5
  - v4: +110 / -16
- qc_app.h
  - v1: +15 / -1
  - v2: +13 / -0
  - v3: +13 / -0
  - v4: +22 / -1
- graph.h
  - v1: +3 / -6
  - v2: +2 / -1
  - v3: +6 / -6
  - v4: +9 / -6

### 5.2 app_kernel
- run.cpp
  - v1: +48 / -4
  - v2: +50 / -4
  - v3: +50 / -4
  - v4: +99 / -7
- kernel_app.h
  - v1: +536 / -555
  - v2: +33 / -8
  - v3: +33 / -8
  - v4: +42 / -9
- graph.h
  - v1: +1 / -0
  - v2: +2 / -1
  - v3: +6 / -8
  - v4: +9 / -7

### 5.3 system
- global.h
  - v1: +449 / -0
  - v2: +361 / -0
  - v3: +364 / -0
  - v4: +370 / -0
- worker.h
  - v1: +56 / -3
  - v2: +53 / -3
  - v3: +53 / -3
  - v4: +60 / -3
- comper.h
  - v1: +7 / -4
  - v2: +13 / -4
  - v3: +18 / -4
  - v4: +39 / -4

## 6. 非源码差异（建议忽略）

以下文件不建议作为功能差异证据：
- 可执行文件：app_qc/run, app_kernel/run, maximal_check/quasiCliques
- 目标文件：maximal_check/*.o
- 输出样例：app_qc/output 下大量 txt
- 合并残留：system/global.h.orig, system/global.h.rej

## 7. 风险与回归关注点

1. 输出协议变化频繁：v1/v2/v3/v4 的输出格式互不兼容，后处理脚本必须按版本绑定。
2. 超时退出路径：v4 新增 MAX_RUNTIME_SEC，若下游假设“总会跑完整图”，需评估中断语义。
3. locale 影响：v4 在线程中强制 LC_NUMERIC=C，可解决小数点问题，但也意味着跨模块共享 locale 语义。
4. v1 的 kernel_app.h 逻辑改动幅度显著，建议对照单测或基准输出做一次回归验真。

## 8. 建议的后续验证

1. 统一每版本最小复现实验，比较输出行协议是否符合预期。
2. 对 v4 执行带/不带 --max-runtime 的 A/B 测试，确认停止行为与结果完整性。
3. 为 app_qc/graph.h 的 Output1Clique 增加协议注释，固定输出字段顺序，避免后续版本漂移。

