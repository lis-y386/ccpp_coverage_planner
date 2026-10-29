#!/usr/bin/env python3
"""Synthetic map and ideal arrival simulator. Publishes no flight commands."""
import threading
import time

import rospy
from geometry_msgs.msg import Point, PointStamped, PoseStamped
from nav_msgs.msg import OccupancyGrid, Odometry, Path
from sensor_msgs import point_cloud2
from sensor_msgs.msg import PointCloud2
from std_msgs.msg import Header
from ccpp_coverage_planner.srv import PlanCoverage, PlanCoverageRequest


class DemoWorld:
    def __init__(self):
        self.frame = "map"
        self.position = (0.125, 0.125, 1.0)
        self.grid = None
        self.lock = threading.Lock()
        self.trail = Path(header=Header(frame_id=self.frame))
        self.odom_pub = rospy.Publisher("/ccpp_demo/odom", Odometry, queue_size=1)
        self.cloud_pub = rospy.Publisher("/ccpp_demo/obstacles", PointCloud2, queue_size=1, latch=True)
        self.trail_pub = rospy.Publisher("/ccpp_demo/executed_path", Path, queue_size=1, latch=True)
        self.reached_pub = rospy.Publisher("/ccpp/reached_point", PointStamped, queue_size=10)
        self.grid_sub = rospy.Subscriber("/ccpp/coverage_grid", OccupancyGrid, self.on_grid, queue_size=1)
        self.timer = rospy.Timer(rospy.Duration(0.1), self.publish_world)
        self.delay = float(rospy.get_param("~step_delay", 0.04))
        if self.delay < 0.0:
            raise ValueError("step_delay must be nonnegative")
        self.timeout = float(rospy.get_param("~timeout", 120.0))
        if self.timeout <= 0:
            raise ValueError("timeout must be positive")
        self.deadline = time.monotonic() + self.timeout
        rospy.set_param("~finished", False)
        rospy.set_param("~failed", False)
        rospy.set_param("~confirmed_targets", 0)

    def on_grid(self, message):
        with self.lock:
            self.grid = message

    def publish_world(self, _event):
        now = rospy.Time.now()
        odom = Odometry(header=Header(stamp=now, frame_id=self.frame))
        odom.child_frame_id = "demo_body"
        odom.pose.pose.position = Point(*self.position)
        odom.pose.pose.orientation.w = 1.0
        self.odom_pub.publish(odom)
        points = [(0.125 + x * 0.25, 0.125 + y * 0.25, 1.0)
                  for y in range(4, 11) for x in range(8, 12)]
        self.cloud_pub.publish(point_cloud2.create_cloud_xyz32(
            Header(stamp=now, frame_id=self.frame), points))

    def check_deadline(self):
        if rospy.is_shutdown():
            raise RuntimeError("ROS shutdown before demo completion")
        if time.monotonic() > self.deadline:
            raise RuntimeError("demo timed out waiting for planner or coverage acknowledgment")

    def wait_until(self, predicate):
        while not predicate():
            self.check_deadline()
            rospy.sleep(0.02)

    def grid_value(self, point):
        with self.lock:
            grid = self.grid
        if grid is None:
            return None
        x = int((point.x - grid.info.origin.position.x) // grid.info.resolution)
        y = int((point.y - grid.info.origin.position.y) // grid.info.resolution)
        if x < 0 or y < 0 or x >= grid.info.width or y >= grid.info.height:
            return None
        return grid.data[y * grid.info.width + x]

    def run(self):
        rospy.wait_for_service("/ccpp/plan_coverage", timeout=self.timeout)
        planner = rospy.ServiceProxy("/ccpp/plan_coverage", PlanCoverage)
        self.wait_until(lambda: self.grid_value(Point(*self.position)) == 0)
        self.wait_until(lambda: self.reached_pub.get_num_connections() > 0)
        confirmed = 0
        for chunk in range(1, 1001):
            self.check_deadline()
            # Explicit start avoids a race with the synthetic odometry subscriber.
            request = PlanCoverageRequest(use_current_pose=False,
                                          start=Point(*self.position), initial_heading=0)
            response = planner(request)
            if not response.success:
                raise RuntimeError(response.message)
            if response.finished:
                rospy.set_param("~finished", True)
                rospy.loginfo("SYNTHETIC CCPP demo complete: %d confirmed targets, %d chunks; ideal arrivals only",
                              confirmed, chunk - 1)
                return
            if not response.path.poses or len(response.path.poses) != len(response.waypoint_types):
                raise RuntimeError("empty path or unaligned waypoint types")
            for pose, kind in zip(response.path.poses, response.waypoint_types):
                self.check_deadline()
                if kind not in (0, 1):
                    raise RuntimeError("unknown waypoint type")
                point = pose.pose.position
                self.position = (point.x, point.y, point.z)  # Ideal simulated move.
                self.publish_world(None)
                stamped = PoseStamped(header=Header(stamp=rospy.Time.now(), frame_id=self.frame), pose=pose.pose)
                self.trail.poses.append(stamped)
                self.trail.header.stamp = stamped.header.stamp
                self.trail_pub.publish(self.trail)
                if self.delay:
                    rospy.sleep(self.delay)
                if kind == 1:
                    self.reached_pub.publish(PointStamped(header=stamped.header, point=point))
                    self.wait_until(lambda: self.grid_value(point) == 50)
                    confirmed += 1
                    rospy.set_param("~confirmed_targets", confirmed)
        raise RuntimeError("demo exceeded 1000 chunks")


if __name__ == "__main__":
    rospy.init_node("ccpp_demo_world")
    try:
        world = DemoWorld()
        world.run()
    except (RuntimeError, ValueError, rospy.ROSException) as error:
        rospy.set_param("~failed", True)
        rospy.logerr("Synthetic demo failed: %s", error)
        raise SystemExit(1)
    rospy.spin()
