# 来源与修改范围

- 上游仓库：https://github.com/lis-y386/drone
- 分支：`ccpp_dev`
- 固定提交：`8bc54d2bdb257b3f2f357b1709350e28d25f35cc`
- 提交时间：2026-10-20 19:20:32 +08:00
- 包路径：`src/ccpp_coverage_planner`
- 提取日期：2026-10-25（Asia/Shanghai）
- 维护者身份依据：该提交作者 `lis-y386 <ybmq02@163.com>`。

本整理从隔离克隆的远端提交中按 git blob 原始字节提取十个包文件，没有使用飞行工程中未提交的文件。`upstream_manifest.json` 保存每个原文件的 SHA256。`tools/verify_provenance.py` 检查必须保持不变的核心文件，并列出有意修改的包装文件。

保持不变：`src/grid_planner.cpp`、`src/ccpp_planner_node.cpp`、`include/ccpp_coverage_planner/grid_planner.h`、`srv/PlanCoverage.srv`、`test/test_grid_planner.cpp`。

有意修改：README；CMake 加入可选独立构建、Python 安装和演示测试；package.xml 使用真实上游作者信息并补演示运行依赖；默认 YAML 换成合成地图参数；launch 直接尊重指定 YAML，避免固定话题默认值覆盖自定义配置。

新增：标准 BSD-3-Clause 文本、独立 C++ 演示、合成 ROS 世界、RViz 配置、交互 HTML 生成器、来源检查脚本、测试、容器和 CI 配置、使用与验证文档。

远端原始默认配置的 `resolution=0.12`、`coverage_stride=8` 意味着间距为 0.96 m；原注释中的 0.60 m 不正确。独立版改用 0.25 × 2 = 0.50 m 的合成演示参数，不携带原比赛边界、航点或飞控配置。

远端版本的 initial_heading 参数被核心忽略，也没有螺旋模式或跳过区回访功能；这些差异在 README 明示。未把本地分支的新算法合并进来。

许可沿用上游 package.xml 的 BSD-3-Clause 声明，本次补入对应文本；本次记录并不构成对仓库全部历史代码权属的审计。此目录只包含该包，没有复制 EGO、PX4、YOLO、模型、飞行日志或整个 drone 工程。没有新建远端仓库、推送或发布。

## Engineering preparation, 2026-10-26

Added six-scene actual-C++ replay, generated assets, explicit boundary defects, installed-library consumer, developer/community docs and validation gates. Original protected sources and service remain unchanged. See ENGINEERING_AUDIT.md, VALIDATION.md and RELEASE_READINESS.md.
