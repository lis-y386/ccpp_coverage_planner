#!/usr/bin/env python3
"""Real ROS node/service contracts; no mocks, no flight commands."""
import threading
import time
import unittest

import rospy
import rostest
from geometry_msgs.msg import Point, PointStamped
from nav_msgs.msg import OccupancyGrid, Odometry
from sensor_msgs import point_cloud2
from sensor_msgs.msg import PointCloud2
from std_msgs.msg import Header
from ccpp_coverage_planner.srv import PlanCoverage, PlanCoverageRequest


class ServiceContract(unittest.TestCase):
    def test_contracts(self):
        self.grid = None
        self.lock = threading.Lock()
        sub = rospy.Subscriber("/ccpp/coverage_grid", OccupancyGrid, self.on_grid)
        odom_pub = rospy.Publisher("/ccpp_demo/odom", Odometry, queue_size=1)
        cloud_pub = rospy.Publisher("/ccpp_demo/obstacles", PointCloud2, queue_size=1)
        reached_pub = rospy.Publisher("/ccpp/reached_point", PointStamped, queue_size=1)
        rospy.wait_for_service("/ccpp/plan_coverage", timeout=10)
        planner = rospy.ServiceProxy("/ccpp/plan_coverage", PlanCoverage)
        request = PlanCoverageRequest(use_current_pose=False, start=Point(0.125, 0.125, 1), initial_heading=0)
        before_map = planner(request)
        self.assertFalse(before_map.success)
        self.assertFalse(before_map.finished)
        self.assertIn("point cloud", before_map.message)
        before_odom = planner(PlanCoverageRequest(use_current_pose=True))
        self.assertFalse(before_odom.success)
        self.assertIn("odometry", before_odom.message)

        deadline = time.monotonic() + 10
        while self.value(request.start) != 0:
            self.assertLess(time.monotonic(), deadline, "map did not initialize")
            header = Header(stamp=rospy.Time.now(), frame_id="map")
            odom = Odometry(header=header)
            odom.pose.pose.position = request.start
            odom.pose.pose.orientation.w = 1
            odom_pub.publish(odom)
            cloud_pub.publish(point_cloud2.create_cloud_xyz32(header, [(100.0, 100.0, 10.0)]))
            rospy.sleep(0.05)
        for point in [Point(-1, 0, 1), Point(10, 10, 1), Point(3.75, 2.75, 1)]:
            invalid = planner(PlanCoverageRequest(use_current_pose=False, start=point))
            self.assertFalse(invalid.success)
            self.assertFalse(invalid.finished)
            self.assertIn("safe free region", invalid.message)

        first = planner(request)
        second = planner(request)
        self.assertTrue(first.success, first.message)
        self.assertFalse(first.finished)
        self.assertEqual(len(first.path.poses), len(first.waypoint_types))
        self.assertEqual(self.targets(first), self.targets(second), "planning must not acknowledge targets")
        self.assertGreater(len(self.targets(first)), 0)
        target = next(p.pose.position for p, kind in zip(first.path.poses, first.waypoint_types) if kind == 1)
        deadline = time.monotonic() + 5
        while reached_pub.get_num_connections() == 0:
            self.assertLess(time.monotonic(), deadline, "ack subscriber missing")
            rospy.sleep(0.02)
        reached_pub.publish(PointStamped(header=Header(stamp=rospy.Time.now(), frame_id="map"), point=target))
        deadline = time.monotonic() + 5
        while self.value(target) != 50:
            self.assertLess(time.monotonic(), deadline, "arrival was not acknowledged")
            rospy.sleep(0.02)
        third = planner(request)
        self.assertTrue(third.success, third.message)
        self.assertNotIn((target.x, target.y), self.targets(third))
        sub.unregister()

    @staticmethod
    def targets(response):
        return [(p.pose.position.x, p.pose.position.y)
                for p, kind in zip(response.path.poses, response.waypoint_types) if kind == 1]

    def on_grid(self, message):
        with self.lock:
            self.grid = message

    def value(self, point):
        with self.lock:
            grid = self.grid
        if grid is None:
            return None
        x = int((point.x - grid.info.origin.position.x) // grid.info.resolution)
        y = int((point.y - grid.info.origin.position.y) // grid.info.resolution)
        return grid.data[y * grid.info.width + x]


if __name__ == "__main__":
    rospy.init_node("service_contract_test")
    rostest.rosrun("ccpp_coverage_planner", "service_contract", ServiceContract)
