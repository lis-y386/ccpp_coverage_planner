# ROS1 API

Source: unchanged src/ccpp_planner_node.cpp and srv/PlanCoverage.srv. The node name is ccpp_planner; parameters are private. Outputs and /ccpp inputs are absolute names. A namespace alone does not isolate multiple planners.

## Service: /ccpp/plan_coverage

Type: ccpp_coverage_planner/PlanCoverage.

| Request | Type | Meaning |
|---|---|---|
| use_current_pose | bool | Use latest odometry instead of start |
| start | geometry_msgs/Point | Actual start in target_frame; output z uses flight_altitude |
| initial_heading | int32 | 0/+X, 1/+Y, 2/-X, 3/-Y; reserved and ignored by current core |

Invalid heading requests fall back to default_initial_heading, but the core ignores heading. The service comment's +X fallback corresponds to the default parameter, not a new heading-controlled mode.

| Response | Type | Meaning |
|---|---|---|
| success | bool | Planner accepted input and produced a valid response |
| finished | bool | Reachable confirmed coverage complete with no non-skipped UNKNOWN remaining |
| message | string | Diagnostic |
| path | nav_msgs/Path | Ordered path in target_frame; empty on finished |
| waypoint_types | uint8[] | Aligned with poses: 0=start/transit, 1=coverage target |

A successful ROS RPC does not imply success=true. Always inspect both fields. The exact start is prepended by the adapter. Planning does not mark targets visited. Request the next chunk after execution/confirmation or interruption.

## Inputs

| Topic | Message | Contract |
|---|---|---|
| configured cloud_topic | sensor_msgs/PointCloud2 | float32 x/y/z obstacle-map cloud with valid frame/stamp |
| configured odom_topic | nav_msgs/Odometry | Pose converted from header.frame_id through TF |
| /ccpp/reached_point | geometry_msgs/PointStamped | Actual reached coverage point; send target_frame |
| /ccpp/skip_area | geometry_msgs/PointStamped | Mark lattice points within target_skip_radius visited |

The node trusts reached_point; it does not check physical arrival against odometry. Nonempty wrong frame IDs are rejected; use an explicit correct frame. Odometry alone never writes VISITED.

## Latched outputs

| Topic | Message | Meaning |
|---|---|---|
| /ccpp/occupancy_grid | nav_msgs/OccupancyGrid | -1 unknown/outside, 0 FREE, 100 blocked |
| /ccpp/coverage_grid | nav_msgs/OccupancyGrid | -1 unknown/outside, 0 unconfirmed, 50 visited, 75 preset skip, 100 blocked |
| /ccpp/visited_cells | nav_msgs/GridCells | Confirmed lattice points |
| /ccpp/preset_skip_cells | nav_msgs/GridCells | Skip mask |
| /ccpp/forbidden_cells | nav_msgs/GridCells | Expanded forbidden mask |
| /ccpp/path | nav_msgs/Path | Latest chunk; replaced by the next plan |

Publisher queue size is 1 and outputs are latched. CCPP forbidden masks do not automatically update external controllers or EGO maps.

## Parameters

The table below lists **bundled synthetic YAML values**, not deployment recommendations or every constructor fallback. All parameters load at startup; restart after changes.

| Parameter | Synthetic YAML value | Meaning |
|---|---|---|
| target_frame | "map" | Output/map coordinate frame |
| cloud_topic | "/ccpp_demo/obstacles" | Obstacle map input |
| odom_topic | "/ccpp_demo/odom" | Odometry input |
| hull_xy | [0.0, 0.0, 5.0, 0.0, 5.0, 4.0, 0.0, 4.0] | Search polygon in target_frame |
| z_min | 0.5 | Obstacle slice lower Z, meters |
| z_max | 1.5 | Obstacle slice upper Z, meters |
| flight_altitude | 1.0 | Output path Z, meters |
| resolution | 0.25 | Fine-grid meters/cell |
| inflation_radius | 0.25 | Additional obstacle inflation, meters |
| boundary_clearance | 0.125 | Hull shrink, meters |
| coverage_stride | 2 | Lattice interval in fine cells |
| chunk_cells | 8 | Coverage-target budget |
| start_from_corner | true | Choose a nearest safe corner at mission start |
| visited_radius | 0.1 | Actual acknowledgment radius, meters |
| target_skip_radius | 0.5 | Dynamic skip radius, meters |
| preset_skip_rectangles | [[1.0, 0.0, 1.5, 1.0]] | Targets excluded; transit allowed |
| preset_forbidden_rectangles | [[3.5, 2.5, 4.0, 3.0]] | Targets/transit blocked |
| forbidden_clearance | 0.125 | Rectangle expansion, meters |
| local_update_range_x | 6.0 | Observed-window X half-width, meters |
| local_update_range_y | 6.0 | Observed-window Y half-width, meters |
| persistent_obstacles | true | Keep observed obstacles |
| min_points_per_cell | 1 | Occupied-cell count threshold |
| tf_timeout | 0.1 | TF lookup timeout, seconds |
| allow_latest_tf_fallback | false | Allow latest-TF retry |
| auto_replan | false | Publish plan on map changes; no execution |
| default_initial_heading | 0 | Reserved; core ignores heading |


Rectangles use [xmin,ymin,xmax,ymax], normalized endpoint order and target_frame. Skip excludes goals but permits A*; forbidden blocks both after forbidden_clearance. The hull is a polygon of flattened x/y pairs.

Lattice spacing is resolution × coverage_stride; lattice indices start at 0,0. Narrow fine-grid corridors can fail lattice connectivity. chunk_cells limits new targets, not total poses or flight distance.

Inflation rounds upward to whole fine-grid cells. It is additional to input-cloud inflation. visited_radius can confirm multiple lattice nodes if configured too large.

The observation window is centered at current odometry with local_update_range_x/y as half-widths. Absence of occupied points there becomes FREE, so the upstream map must satisfy complete-window observation. No visibility ray tracing or input-age timeout exists.

## Demonstration launch controls

demo.launch: rviz (default true), step_delay (0.04 seconds of illustrative per-point delay), timeout (120 seconds). These are demonstration settings, not core algorithm parameters.

The demo publishes /ccpp_demo/odom, /ccpp_demo/obstacles and /ccpp_demo/executed_path. Private diagnostic parameters finished, failed and confirmed_targets belong to ccpp_demo_world, not a new planner service.

## State and failures

State is in memory; restart clears it. No reset/save/load service is present. Known FREE islands outside the start-connected lattice component may not block completion; non-skipped UNKNOWN does. If no reachable FREE frontier exists while UNKNOWN remains, the response fails.

Common messages: no odometry received; no valid point cloud has been converted yet; start is outside the safe free region; start cell cannot connect to the coverage lattice; unknown area remains but no reachable free frontier exists. See TROUBLESHOOTING.md.

No coverage_pattern selector, spiral mode or skipped-rectangle revisit parameter exists.
