#!/usr/bin/env python3
"""ROS end-to-end demonstration check; run by rostest in an isolated ROS master."""
import time
import unittest
import rospy
import rostest
from nav_msgs.msg import OccupancyGrid, Path


class DemoIntegration(unittest.TestCase):
    def test_full_demo(self):
        deadline = time.monotonic() + 70.0
        while not rospy.is_shutdown() and time.monotonic() < deadline:
            self.assertFalse(rospy.get_param("/ccpp_demo_world/failed", False))
            if rospy.get_param("/ccpp_demo_world/finished", False):
                break
            rospy.sleep(0.05)
        self.assertTrue(rospy.get_param("/ccpp_demo_world/finished", False), "demo never completed")
        self.assertGreater(rospy.get_param("/ccpp_demo_world/confirmed_targets", 0), 0)
        coverage = rospy.wait_for_message("/ccpp/coverage_grid", OccupancyGrid, timeout=5.0)
        self.assertIn(50, coverage.data, "no confirmed targets")
        self.assertIn(75, coverage.data, "skip region missing")
        self.assertIn(100, coverage.data, "obstacles missing")
        trail = rospy.wait_for_message("/ccpp_demo/executed_path", Path, timeout=5.0)
        self.assertGreater(len(trail.poses), 10)
        for pose in trail.poses:
            point = pose.pose.position
            x = int((point.x - coverage.info.origin.position.x) // coverage.info.resolution)
            y = int((point.y - coverage.info.origin.position.y) // coverage.info.resolution)
            self.assertTrue(0 <= x < coverage.info.width and 0 <= y < coverage.info.height)
            self.assertNotIn(coverage.data[y * coverage.info.width + x], (-1, 100))


if __name__ == "__main__":
    rospy.init_node("ccpp_demo_integration_test")
    rostest.rosrun("ccpp_coverage_planner", "synthetic_coverage_completion", DemoIntegration)
