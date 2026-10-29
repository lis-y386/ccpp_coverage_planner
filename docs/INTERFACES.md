# 接口与地图约定

## 服务与任务状态

`/ccpp/plan_coverage`：`ccpp_coverage_planner/PlanCoverage`，字段见 `srv/PlanCoverage.srv`。

| 字段 | 含义 |
|---|---|
| `use_current_pose` | true 时使用最近收到的里程计；否则使用 start |
| `start` | target_frame 中的实际当前位置；z 输出改为 flight_altitude |
| `initial_heading` | 0/+X、1/+Y、2/-X、3/-Y；远端当前牛耕式核心忽略此字段 |
| `success` | 规划是否成功；ROS 服务调用成功不等于此字段为 true |
| `finished` | 可达覆盖格点已确认，且没有需要探索的非跳过 UNKNOWN 区 |
| `message` | 成功说明或失败原因 |
| `path` | target_frame 中的路径；完成时为空 |
| `waypoint_types` | 与 path.poses 对齐；0 起点/中转，1 覆盖目标 |

正常流程：收到地图/里程计 → 请求短块 → 顺序执行 → 每个覆盖目标实际到达后确认 → 下一次请求。确认消息由调用方负责真实性，节点不会用里程计交叉验证已经到达。规划本身不修改 VISITED，单纯里程计更新也不标记 VISITED。

完成仅考虑起点所在的覆盖格点连通分量。已知 FREE 孤岛可能不阻止完成；UNKNOWN 仍可能阻止完成。当 UNKNOWN 存在却找不到可达 FREE frontier 时返回失败，而不是声称完成。状态只在内存中保存，重启即清空。

## 输入与输出

| 接口 | 类型 | 语义 |
|---|---|---|
| `~cloud_topic` | sensor_msgs/PointCloud2 | 含 float32 x/y/z 的局部障碍点云，需满足完整观察窗口假设 |
| `~odom_topic` | nav_msgs/Odometry | header.frame_id 中的位置，通过 TF 转到 target_frame |
| `/ccpp/reached_point` | geometry_msgs/PointStamped | 任务确认实际到达的覆盖点；frame_id 应为 target_frame |
| `/ccpp/skip_area` | geometry_msgs/PointStamped | 将 target_skip_radius 内的格点标为 VISITED |
| `/ccpp/occupancy_grid` | nav_msgs/OccupancyGrid | -1 未知/外部，0 可通行，100 障碍 |
| `/ccpp/coverage_grid` | nav_msgs/OccupancyGrid | -1 未知/外部，0 未访问，50 已访问，75 静态跳过，100 障碍/禁行 |
| `/ccpp/visited_cells` | nav_msgs/GridCells | 访问覆盖点 |
| `/ccpp/preset_skip_cells` | nav_msgs/GridCells | 静态跳过区 |
| `/ccpp/forbidden_cells` | nav_msgs/GridCells | 禁行区 |
| `/ccpp/path` | nav_msgs/Path | 最近规划块；新的规划会替换它 |

覆盖格点在栅格索引 `x % coverage_stride == 0 && y % coverage_stride == 0` 处。A* 在这些格点间按四邻接移动，检查连接段上的细栅格；它不是每次都在全部细栅格上寻找覆盖连线。狭窄通道即使细栅格可达，也可能无法连接覆盖格点。

## 地图假设

适配器先将点云经 TF 转到 target_frame，截取 z_min..z_max 高度范围，再计数、投影和膨胀。以里程计为中心，x/y 各自正负 local_update_range_x/y 是观察窗口；窗口内未被占据的合法格子变为 FREE，历史观测区保留，窗口外未观测区域为 UNKNOWN。

因此，输入通常应是上游占据地图输出，且观察窗口要和上游语义一致。这个节点没有实现基于传感器视锥和遮挡的射线自由空间推断。把它直接接到稀疏原始点云会把遮挡区域误判为空闲。

点云和里程计自带的 frame_id 必须有效；不同坐标系需有 TF。本演示全部使用 map，无需外部 TF。点云先查时间戳 TF，只有 allow_latest_tf_fallback=true 时才尝试最新 TF。当前节点没有输入过期超时保护；真实接入应由任务/适配层处理陈旧数据。

## 参数

| 参数 | 说明 |
|---|---|
| hull_xy / target_frame | 搜索多边形的 x,y 序列和统一坐标系 |
| resolution | 细栅格米/格 |
| coverage_stride | 覆盖格点间隔；名义间距 = resolution × stride |
| chunk_cells | 每次请求的覆盖目标上限；中转点和起点额外计算 |
| start_from_corner | 新任务从最近的可达角部覆盖点开始 |
| flight_altitude | 所有输出点的统一高度 |
| z_min / z_max | 障碍点云高度切片，不是飞行高度上下限 |
| inflation_radius | 输入点云已有膨胀之外的附加膨胀；向上取整为格数 |
| boundary_clearance | 格中心与多边形边界的最小距离 |
| visited_radius | reached_point 周围确认半径；过大会同时标记多个格点 |
| target_skip_radius | skip_area 周围永久跳过本轮任务的半径 |
| preset_skip_rectangles | `[xmin,ymin,xmax,ymax]`；省略覆盖目标但允许 A* 通行 |
| preset_forbidden_rectangles | 同样格式；覆盖与中转都禁止 |
| forbidden_clearance | 禁行矩形各方向扩张距离；这些区域不会写回 EGO/飞控地图 |
| local_update_range_x/y | 以当前位置为中心的观察窗口半宽，必须符合上游观测假设 |
| persistent_obstacles | true 保留历史障碍；false 在本次窗口内重建障碍，窗口外保留 |
| min_points_per_cell | 形成占据格子的最小点数 |
| auto_replan | 地图变化时自动发布规划；不是自动执行路径 |
| tf_timeout / allow_latest_tf_fallback | TF 查询等待与降级策略 |
| default_initial_heading | 非法请求朝向时的替代值；当前牛耕式核心忽略朝向 |

所有参数在启动时读取，修改 YAML 后需重启。没有 coverage_pattern 参数，也没有 skipped-rectangle revisit 参数；不应把其他分支的配置直接套过来。

## 常见问题

- `no valid point cloud has been converted yet`：确认点云话题、x/y/z 字段和 TF。
- `no odometry received`：确认里程计话题；use_current_pose=false 仍需里程计帮助建立局部 FREE 窗口。
- `start is outside the safe free region`：检查坐标系、边界缩进、障碍膨胀、当前位置和已观测范围；不能仅把请求起点替换成飞机尚未到达的合法点。
- `start cell cannot connect to the coverage lattice`：细栅格虽自由，但到覆盖格点的连线可能受阻；核对 stride 和通道宽度。
- `unknown area remains but no reachable free frontier exists`：核对观测窗口与未知区域可达性，不能据此报告任务完成。
- 重复返回相同目标：检查是否在实际到达后发送了正确坐标系的 reached_point，并等待节点处理确认。

本模块只做规划时的栅格检查。执行器必须自行决定路径有效性、飞行中避障、制动和中断处理。
