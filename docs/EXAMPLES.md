# Examples

## Native maps

```bash
./build/ccpp_demo --scenario rectangle --output docs/data/rectangle
./build/ccpp_demo --scenario skip --output docs/data/skip
./build/ccpp_demo --scenario disconnected --output docs/data/disconnected
```

rectangle: alternating columns. skip: traverse but do not confirm an excluded column. disconnected: confirm only the start-connected component. obstacles and maze add occupied cells; stride uses two-cell spacing.

JSON records [x,y,type,chunk]. All scenes use synthetic maps and ideal acknowledgments. The viewer does not compute another route or measure planner runtime.

## ROS synthetic world

```bash
roslaunch ccpp_coverage_planner demo.launch rviz:=false step_delay:=0.04
```

A fully observed 5 × 4 m world supplies map/odometry. The simulator calls the real service, moves ideally and waits for coverage_grid to show confirmation before replanning. No physical dynamics are simulated.

## External executor

```bash
roslaunch ccpp_coverage_planner ccpp_planner.launch config:=/absolute/path/custom.yaml
rosservice call /ccpp/plan_coverage "use_current_pose: true
start: {x: 0.0, y: 0.0, z: 0.0}
initial_heading: 0"
```

The custom YAML path is user-supplied. Set actual boundary, frame, inputs, observation window and margins.

```text
request from actual current pose
if not success: stop/recover according to your mission policy
if finished: end this coverage task
require path/type arrays align
for each pose and type:
    execute motion with your own controller and collision checks
    if interrupted: do not acknowledge unachieved targets; replan
    if type == 1 and physical arrival is confirmed:
        publish reached_point in target_frame
request next chunk after confirmation processing
```

The execute/stop/physical-arrival functions are not part of this package. The node trusts confirmations rather than checking odometry.

## Zones and target handoff

```yaml
preset_skip_rectangles: [[1.0, 0.0, 1.5, 1.0]]
preset_forbidden_rectangles: [[3.5, 2.5, 4.0, 3.0]]
forbidden_clearance: 0.125
```

Rectangles use target_frame and [xmin,ymin,xmax,ymax]. Skip excludes goals but permits transit. Forbidden excludes both after expansion. CCPP masks do not constrain your external controller automatically.

A mission may interrupt motion, handle a target, publish skip_area, and replan from its actual post-action pose. There is no bundled YOLO/servo/alignment state machine. Do not invent a safe request coordinate that the robot has not reached.

## Library consumer

test/install_consumer is a minimal CMake consumer of installed ccpp::grid_planner. Direct callers must provide finite bounded geometry and a FREE in-bounds start; known upstream defects remain documented.
