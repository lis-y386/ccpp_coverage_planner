# Installation and reproduction

## Compatibility

| Path | Requirements | Evidence |
|---|---|---|
| Offline replay | Modern browser; local file or static host | Actual Chrome checks; no Python/ROS required |
| Native core | C++14, CMake 3.10+ | Windows MinGW GCC 13.2 tested |
| Linux native | C++14, CMake, Make/Ninja | CI prepared; not locally run |
| ROS | Ubuntu 20.04, Noetic, Python 3.8, catkin | Legacy target; Not Verified locally |
| Docker | Linux containers / Docker Engine | Prepared; Docker absent locally |
| MSVC/macOS/ARM | Matching C++14 environment | Not Verified |

[REP-3](https://github.com/ros-infrastructure/rep/blob/master/rep-0003.rst) defines the Noetic Focal/C++14/Python3 target. [Noetic EOL was May 31, 2025](https://discourse.ros.org/t/ros-noetic-end-of-life-may-31-2025/43160). EOL rosdep data, APT keys and image availability may affect reproduction. This project does not supply upstream ROS security maintenance.

## Native build

From the repository directory:

```bash
cmake -S . -B build -DCCPP_STANDALONE=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/ccpp_demo --scenario maze --output docs/data/maze
```

Windows PowerShell + MinGW on PATH:

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCCPP_STANDALONE=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
.\build\ccpp_demo.exe --scenario obstacles --output docs/demo_obstacles
```

Both g++ and MinGW make are required. Explicit CMAKE_CXX_COMPILER and CMAKE_MAKE_PROGRAM help with multiple toolchains. Local validation used -DCMAKE_EXE_LINKER_FLAGS=-static to avoid runtime DLL dependencies. Multi-config generators need --config Release, ctest -C Release and the corresponding Release executable; MSVC is not locally verified.

Install and consume with the same compiler ABI:

```bash
cmake --install build --prefix "$PWD/.maintenance/install"
cmake -S test/install_consumer -B build-consumer -DCMAKE_PREFIX_PATH="$PWD/.maintenance/install"
cmake --build build-consumer --parallel
./build-consumer/consumer
```

The exported target is ccpp::grid_planner.

## ROS dependencies and build

Required declared packages include geometry_msgs, nav_msgs, roscpp, sensor_msgs, std_msgs, tf2, tf2_ros, xmlrpcpp and message generation/runtime. rospy supplies the demonstration and RViz the viewer. rostest is needed with catkin testing.

Place this directory directly at ~/ccpp_ws/src/ccpp_coverage_planner. Do not copy the flight repository or nest another ROS package.

```bash
source /opt/ros/noetic/setup.bash
rosdep update --include-eol-distros --rosdistro noetic
cd ~/ccpp_ws
rosdep install --from-paths src --ignore-src --rosdistro noetic -y
catkin_make -DCMAKE_BUILD_TYPE=Release
source devel/setup.bash
roslaunch ccpp_coverage_planner demo.launch
```

rosdep must already be installed and initialized. Run rosdep init only once if its default source list is absent. This maintenance did not change host APT/ROS installations.

```bash
roslaunch ccpp_coverage_planner demo.launch rviz:=false step_delay:=0.0
rosparam get /ccpp_demo_world/finished
```

Use an isolated master. Restart to reset; no reset API exists.

Installed-space check:

```bash
cd ~/ccpp_ws
catkin_make install
source install/setup.bash
rospack find ccpp_coverage_planner
roslaunch ccpp_coverage_planner demo.launch rviz:=false
```

The package path should point into install/share. This path remains Not Verified locally.

## Developer checks

Use an isolated Python environment for optional requirements-dev.txt. Python tools target 3.8+. Pillow is required only for regenerating GIF/images.

```bash
python -m pip install -r requirements-dev.txt
python tools/verify_provenance.py
python tools/check_project.py
python -m pytest -q
python tools/check_quality.py --format --static
```

To run the original GTest with a compiler-compatible installed library:

```bash
cmake -S . -B build -DCCPP_STANDALONE=ON -DCCPP_BUILD_UPSTREAM_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Alternatively pass CCPP_GTEST_SOURCE_DIR pointing to a local GoogleTest source checkout. v1.14.0 was used locally. No CMake network download occurs. Set CCPP_BOUNDARY_BINARY if the boundary executable is not at build/test_core_boundaries. Known defects are strict xfails: XPASS deliberately requires review.

Regenerate records and presentation:

```bash
python tools/generate_demo.py --binary build/ccpp_demo
python tools/make_preview.py
python tools/render_assets.py
```

Optional browser developer checks (Node.js 20+; CI uses Node 22):

```bash
npm ci --ignore-scripts
npx playwright install chromium
npm run test:browser
```

CCPP_BROWSER_EXECUTABLE and CCPP_PLAYWRIGHT_MODULE can select existing tools. The resulting HTML needs none of them.

## ROS tests and container

```bash
cd ~/ccpp_ws
source devel/setup.bash
catkin_make run_tests_ccpp_coverage_planner
catkin_test_results build/test_results
```

These exercise real nodes: original GTest, synthetic completion and negative/confirmation service contracts. They have not been run on this Windows host.

Prepared commands on a Linux Docker host, from this directory:

```bash
docker build --target tested -t ccpp-local-test .
docker build --target runtime -t ccpp-local-demo .
docker run --rm ccpp-local-demo
```

The tested stage builds, runs devel tests, installs and runs an install-space rostest. The runtime stage launches a headless synthetic demo. No image is pushed. After success, record the image digest and resolved packages: the default tag is mutable and not locally verified.
