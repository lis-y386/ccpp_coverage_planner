# Mutable legacy base tag: record its digest after a verified Linux build.
ARG ROS_IMAGE=ros:noetic-ros-base-focal
FROM ${ROS_IMAGE} AS build
SHELL ["/bin/bash", "-o", "pipefail", "-c"]
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential python3-rosdep ros-noetic-rostest \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /ws
COPY . /ws/src/ccpp_coverage_planner
RUN if [ ! -f /etc/ros/rosdep/sources.list.d/20-default.list ]; then rosdep init; fi \
    && rosdep update --include-eol-distros --rosdistro noetic
RUN source /opt/ros/noetic/setup.bash \
    && rosdep install --from-paths src --ignore-src --rosdistro noetic -y \
    && catkin_make -DCMAKE_BUILD_TYPE=Release -DCATKIN_ENABLE_TESTING=ON \
    && catkin_make install

FROM build AS tested
RUN source /opt/ros/noetic/setup.bash && source devel/setup.bash \
    && catkin_make run_tests_ccpp_coverage_planner \
    && catkin_test_results build/test_results
RUN source /opt/ros/noetic/setup.bash && source install/setup.bash \
    && test "$(rospack find ccpp_coverage_planner)" = /ws/install/share/ccpp_coverage_planner \
    && rostest ccpp_coverage_planner demo_integration.test

FROM build AS runtime
CMD ["bash", "-lc", "source /ws/install/setup.bash && exec roslaunch ccpp_coverage_planner demo.launch rviz:=false"]
