#include <ccpp_coverage_planner/PlanCoverage.h>
#include <ccpp_coverage_planner/grid_planner.h>

#include <geometry_msgs/Point.h>
#include <geometry_msgs/PointStamped.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/TransformStamped.h>
#include <nav_msgs/GridCells.h>
#include <nav_msgs/OccupancyGrid.h>
#include <nav_msgs/Odometry.h>
#include <nav_msgs/Path.h>
#include <ros/ros.h>
#include <sensor_msgs/PointCloud2.h>
#include <sensor_msgs/point_cloud2_iterator.h>
#include <tf2/LinearMath/Quaternion.h>
#include <tf2/LinearMath/Transform.h>
#include <tf2/LinearMath/Vector3.h>
#include <tf2_ros/transform_listener.h>
#include <XmlRpcValue.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>

namespace ccpp_coverage_planner {
namespace {

struct Point2 {
  double x = 0.0;
  double y = 0.0;
};

struct AxisAlignedRectangle {
  double xmin = 0.0;
  double ymin = 0.0;
  double xmax = 0.0;
  double ymax = 0.0;
};

double numberFromXml(const XmlRpc::XmlRpcValue& value) {
  if (value.getType() == XmlRpc::XmlRpcValue::TypeInt) {
    return static_cast<int>(value);
  }
  if (value.getType() == XmlRpc::XmlRpcValue::TypeDouble) {
    return static_cast<double>(value);
  }
  throw std::runtime_error("CCPP region entries must be numbers");
}

std::vector<AxisAlignedRectangle> rectanglesFromXml(
    const XmlRpc::XmlRpcValue& value, const std::string& parameter_name) {
  if (value.getType() != XmlRpc::XmlRpcValue::TypeArray) {
    throw std::runtime_error("~" + parameter_name + " must be a list");
  }
  std::vector<AxisAlignedRectangle> rectangles;
  for (int i = 0; i < value.size(); ++i) {
    const XmlRpc::XmlRpcValue& entry = value[i];
    if (entry.getType() != XmlRpc::XmlRpcValue::TypeArray || entry.size() != 4) {
      throw std::runtime_error("each ~" + parameter_name +
                               " entry must be [xmin, ymin, xmax, ymax]");
    }
    const double x1 = numberFromXml(entry[0]);
    const double y1 = numberFromXml(entry[1]);
    const double x2 = numberFromXml(entry[2]);
    const double y2 = numberFromXml(entry[3]);
    AxisAlignedRectangle rectangle;
    rectangle.xmin = std::min(x1, x2);
    rectangle.ymin = std::min(y1, y2);
    rectangle.xmax = std::max(x1, x2);
    rectangle.ymax = std::max(y1, y2);
    if (rectangle.xmax - rectangle.xmin <= 1e-9 ||
        rectangle.ymax - rectangle.ymin <= 1e-9) {
      throw std::runtime_error("~" + parameter_name +
                               " rectangles must have non-zero area");
    }
    rectangles.push_back(rectangle);
  }
  return rectangles;
}

bool pointInRectangles(double x, double y,
                       const std::vector<AxisAlignedRectangle>& rectangles,
                       double clearance = 0.0) {
  for (const AxisAlignedRectangle& rectangle : rectangles) {
    if (x >= rectangle.xmin - clearance &&
        x <= rectangle.xmax + clearance &&
        y >= rectangle.ymin - clearance &&
        y <= rectangle.ymax + clearance) {
      return true;
    }
  }
  return false;
}

bool pointInPolygon(double x, double y, const std::vector<Point2>& polygon) {
  bool inside = false;
  for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
    const Point2& pi = polygon[i];
    const Point2& pj = polygon[j];
    const bool crosses = ((pi.y > y) != (pj.y > y)) &&
                         (x < (pj.x - pi.x) * (y - pi.y) /
                                      (pj.y - pi.y + 1e-12) +
                                  pi.x);
    if (crosses) inside = !inside;
  }
  return inside;
}

double distanceToSegment(double x, double y, const Point2& a, const Point2& b) {
  const double vx = b.x - a.x;
  const double vy = b.y - a.y;
  const double length_squared = vx * vx + vy * vy;
  if (length_squared <= 1e-12) return std::hypot(x - a.x, y - a.y);
  const double projection =
      std::max(0.0, std::min(1.0, ((x - a.x) * vx + (y - a.y) * vy) /
                                      length_squared));
  return std::hypot(x - (a.x + projection * vx),
                    y - (a.y + projection * vy));
}

tf2::Transform toTf(const geometry_msgs::Transform& transform) {
  tf2::Quaternion rotation(transform.rotation.x, transform.rotation.y,
                           transform.rotation.z, transform.rotation.w);
  tf2::Vector3 translation(transform.translation.x, transform.translation.y,
                           transform.translation.z);
  return tf2::Transform(rotation, translation);
}

}  // namespace

class CcppPlannerNode {
 public:
  CcppPlannerNode()
      : nh_(), private_nh_("~"), tf_listener_(tf_buffer_) {
    loadParameters();
    initializeGrid();

    occupancy_pub_ = nh_.advertise<nav_msgs::OccupancyGrid>(
        "/ccpp/occupancy_grid", 1, true);
    coverage_pub_ =
        nh_.advertise<nav_msgs::OccupancyGrid>("/ccpp/coverage_grid", 1, true);
    visited_pub_ =
        nh_.advertise<nav_msgs::GridCells>("/ccpp/visited_cells", 1, true);
    preset_skip_pub_ = nh_.advertise<nav_msgs::GridCells>(
        "/ccpp/preset_skip_cells", 1, true);
    forbidden_pub_ = nh_.advertise<nav_msgs::GridCells>(
        "/ccpp/forbidden_cells", 1, true);
    path_pub_ = nh_.advertise<nav_msgs::Path>("/ccpp/path", 1, true);
    cloud_sub_ = nh_.subscribe(cloud_topic_, 1, &CcppPlannerNode::cloudCallback,
                               this);
    odom_sub_ = nh_.subscribe(odom_topic_, 10, &CcppPlannerNode::odomCallback,
                              this);
    reached_sub_ = nh_.subscribe("/ccpp/reached_point", 10,
                                 &CcppPlannerNode::reachedCallback, this);
    skip_area_sub_ = nh_.subscribe("/ccpp/skip_area", 10,
                                   &CcppPlannerNode::skipAreaCallback, this);
    plan_service_ = nh_.advertiseService(
        "/ccpp/plan_coverage", &CcppPlannerNode::planCallback, this);

    publishOccupancy();
    publishCoverage();
    ROS_INFO_STREAM("CCPP planner ready: cloud=" << cloud_topic_
                    << ", odom=" << odom_topic_ << ", frame=" << target_frame_
                    << ", grid=" << grid_.width << "x" << grid_.height
                    << ", spacing=" << grid_.resolution * coverage_stride_
                    << " m, preset skip rectangles=" << skip_rectangles_.size()
                    << ", forbidden rectangles=" << forbidden_rectangles_.size()
                    << ". VISITED requires /ccpp/reached_point confirmation. "
                       "It only plans and never commands the aircraft.");
  }

 private:
  void loadParameters() {
    private_nh_.param("target_frame", target_frame_, std::string("map"));
    private_nh_.param("cloud_topic", cloud_topic_,
                      std::string("/cloud_registered"));
    private_nh_.param("odom_topic", odom_topic_,
                      std::string("/mavros/odometry/out"));
    private_nh_.param("z_min", z_min_, 0.72);
    private_nh_.param("z_max", z_max_, 1.30);
    private_nh_.param("flight_altitude", flight_altitude_, 1.20);
    private_nh_.param("resolution", resolution_, 0.12);
    private_nh_.param("inflation_radius", inflation_radius_, 0.36);
    private_nh_.param("boundary_clearance", boundary_clearance_,
                      inflation_radius_);
    private_nh_.param("coverage_stride", coverage_stride_, 3);
    private_nh_.param("chunk_cells", chunk_cells_, 8);
    private_nh_.param("start_from_corner", start_from_corner_, true);
    private_nh_.param("visited_radius", visited_radius_, 0.24);
    private_nh_.param("target_skip_radius", target_skip_radius_, 1.0);
    private_nh_.param("forbidden_clearance", forbidden_clearance_, 0.30);
    private_nh_.param("local_update_range_x", local_update_range_x_, 4.0);
    private_nh_.param("local_update_range_y", local_update_range_y_, 4.0);
    private_nh_.param("persistent_obstacles", persistent_obstacles_, true);
    private_nh_.param("min_points_per_cell", min_points_per_cell_, 2);
    private_nh_.param("tf_timeout", tf_timeout_, 0.10);
    private_nh_.param("allow_latest_tf_fallback", allow_latest_tf_fallback_,
                      true);
    private_nh_.param("auto_replan", auto_replan_, false);
    private_nh_.param("default_initial_heading", default_initial_heading_, 0);

    XmlRpc::XmlRpcValue hull;
    if (!private_nh_.getParam("hull_xy", hull) ||
        hull.getType() != XmlRpc::XmlRpcValue::TypeArray || hull.size() < 6 ||
        hull.size() % 2 != 0) {
      throw std::runtime_error("~hull_xy must contain at least three x,y pairs");
    }
    for (int i = 0; i < hull.size(); i += 2) {
      hull_.push_back(Point2{numberFromXml(hull[i]), numberFromXml(hull[i + 1])});
    }
    XmlRpc::XmlRpcValue rectangles;
    if (private_nh_.getParam("preset_skip_rectangles", rectangles)) {
      skip_rectangles_ =
          rectanglesFromXml(rectangles, "preset_skip_rectangles");
    }
    if (private_nh_.getParam("preset_forbidden_rectangles", rectangles)) {
      forbidden_rectangles_ =
          rectanglesFromXml(rectangles, "preset_forbidden_rectangles");
    }
    if (resolution_ <= 0.0 || coverage_stride_ <= 0 || z_min_ >= z_max_ ||
        inflation_radius_ < 0.0 || boundary_clearance_ < 0.0 ||
        min_points_per_cell_ <= 0 || chunk_cells_ <= 0 ||
        visited_radius_ < 0.0 || target_skip_radius_ < 0.0 ||
        forbidden_clearance_ < 0.0 ||
        local_update_range_x_ <= 0.0 ||
        local_update_range_y_ <= 0.0) {
      throw std::runtime_error("invalid CCPP numeric parameter");
    }
  }

  void initializeGrid() {
    double min_x = std::numeric_limits<double>::infinity();
    double min_y = std::numeric_limits<double>::infinity();
    double max_x = -std::numeric_limits<double>::infinity();
    double max_y = -std::numeric_limits<double>::infinity();
    for (const Point2& point : hull_) {
      min_x = std::min(min_x, point.x);
      min_y = std::min(min_y, point.y);
      max_x = std::max(max_x, point.x);
      max_y = std::max(max_y, point.y);
    }
    grid_.resolution = resolution_;
    grid_.origin_x = min_x;
    grid_.origin_y = min_y;
    grid_.width = std::max(1, static_cast<int>(std::ceil((max_x - min_x) /
                                                         resolution_)));
    grid_.height = std::max(1, static_cast<int>(std::ceil((max_y - min_y) /
                                                          resolution_)));
    grid_.occupancy.assign(grid_.width * grid_.height, -1);
    safe_mask_.assign(grid_.occupancy.size(), false);
    preset_skip_mask_.assign(grid_.occupancy.size(), false);
    forbidden_mask_.assign(grid_.occupancy.size(), false);

    for (int y = 0; y < grid_.height; ++y) {
      for (int x = 0; x < grid_.width; ++x) {
        const double world_x = grid_.origin_x + (x + 0.5) * resolution_;
        const double world_y = grid_.origin_y + (y + 0.5) * resolution_;
        if (!pointInPolygon(world_x, world_y, hull_)) continue;
        double edge_distance = std::numeric_limits<double>::infinity();
        for (size_t i = 0; i < hull_.size(); ++i) {
          edge_distance = std::min(
              edge_distance,
              distanceToSegment(world_x, world_y, hull_[i],
                                hull_[(i + 1) % hull_.size()]));
        }
        if (edge_distance + 1e-9 >= boundary_clearance_) {
          const int index = y * grid_.width + x;
          safe_mask_[index] = true;
          preset_skip_mask_[index] =
              pointInRectangles(world_x, world_y, skip_rectangles_);
          forbidden_mask_[index] = pointInRectangles(
              world_x, world_y, forbidden_rectangles_, forbidden_clearance_);
        }
      }
    }
    observed_free_.assign(grid_.occupancy.size(), false);
    visited_.assign(grid_.occupancy.size(), false);
    persistent_raw_obstacles_.assign(grid_.occupancy.size(), false);
    rebuildOccupancy();
  }

  bool lookupTransform(const std::string& source_frame, const ros::Time& stamp,
                       tf2::Transform* transform) {
    if (source_frame.empty() || source_frame == target_frame_) {
      transform->setIdentity();
      return true;
    }
    try {
      const auto stamped = tf_buffer_.lookupTransform(
          target_frame_, source_frame, stamp, ros::Duration(tf_timeout_));
      *transform = toTf(stamped.transform);
      return true;
    } catch (const tf2::TransformException& exact_error) {
      if (!allow_latest_tf_fallback_) {
        ROS_WARN_THROTTLE(2.0, "Cannot transform %s to %s: %s",
                          source_frame.c_str(), target_frame_.c_str(),
                          exact_error.what());
        return false;
      }
      try {
        const auto latest = tf_buffer_.lookupTransform(
            target_frame_, source_frame, ros::Time(0),
            ros::Duration(tf_timeout_));
        *transform = toTf(latest.transform);
        ROS_WARN_THROTTLE(5.0,
                          "Using latest TF for %s -> %s because stamped TF "
                          "was unavailable",
                          source_frame.c_str(), target_frame_.c_str());
        return true;
      } catch (const tf2::TransformException& latest_error) {
        ROS_WARN_THROTTLE(2.0, "Cannot transform %s to %s: %s",
                          source_frame.c_str(), target_frame_.c_str(),
                          latest_error.what());
        return false;
      }
    }
  }

  bool worldToCell(double x, double y, Cell* cell) const {
    cell->x = static_cast<int>(std::floor((x - grid_.origin_x) / resolution_));
    cell->y = static_cast<int>(std::floor((y - grid_.origin_y) / resolution_));
    return grid_.inside(*cell);
  }

  geometry_msgs::Point cellCenter(const Cell& cell) const {
    geometry_msgs::Point point;
    point.x = grid_.origin_x + (cell.x + 0.5) * resolution_;
    point.y = grid_.origin_y + (cell.y + 0.5) * resolution_;
    point.z = flight_altitude_;
    return point;
  }

  int sweepRank(const Cell& cell) const {
    const int rows = (grid_.height + coverage_stride_ - 1) / coverage_stride_;
    const int column = cell.x / coverage_stride_;
    const int row = cell.y / coverage_stride_;
    return column * rows +
           (column % 2 == 0 ? row : rows - 1 - row);
  }

  void rebuildOccupancy() {
    grid_.occupancy.assign(grid_.occupancy.size(), -1);
    for (size_t i = 0; i < grid_.occupancy.size(); ++i) {
      if (safe_mask_[i] && observed_free_[i]) grid_.occupancy[i] = 0;
      if (safe_mask_[i] && persistent_raw_obstacles_[i]) {
        grid_.occupancy[i] = 100;
      }
    }
    const int inflation_cells =
        static_cast<int>(std::ceil(inflation_radius_ / resolution_));
    for (int y = 0; y < grid_.height; ++y) {
      for (int x = 0; x < grid_.width; ++x) {
        const Cell obstacle{x, y};
        if (!persistent_raw_obstacles_[grid_.index(obstacle)]) continue;
        for (int dy = -inflation_cells; dy <= inflation_cells; ++dy) {
          for (int dx = -inflation_cells; dx <= inflation_cells; ++dx) {
            if (dx * dx + dy * dy > inflation_cells * inflation_cells) continue;
            const Cell inflated{x + dx, y + dy};
            if (grid_.inside(inflated) && safe_mask_[grid_.index(inflated)]) {
              grid_.occupancy[grid_.index(inflated)] = 100;
            }
          }
        }
      }
    }
    for (size_t i = 0; i < grid_.occupancy.size(); ++i) {
      if (safe_mask_[i] && forbidden_mask_[i]) grid_.occupancy[i] = 100;
    }
  }

  void cloudCallback(const sensor_msgs::PointCloud2ConstPtr& message) {
    tf2::Transform transform;
    if (!lookupTransform(message->header.frame_id, message->header.stamp,
                         &transform)) {
      return;
    }

    std::vector<int> point_counts(grid_.occupancy.size(), 0);
    try {
      sensor_msgs::PointCloud2ConstIterator<float> x(*message, "x");
      sensor_msgs::PointCloud2ConstIterator<float> y(*message, "y");
      sensor_msgs::PointCloud2ConstIterator<float> z(*message, "z");
      for (; x != x.end(); ++x, ++y, ++z) {
        if (!std::isfinite(*x) || !std::isfinite(*y) || !std::isfinite(*z)) {
          continue;
        }
        const tf2::Vector3 transformed = transform * tf2::Vector3(*x, *y, *z);
        if (transformed.z() < z_min_ || transformed.z() > z_max_) continue;
        Cell cell;
        if (!worldToCell(transformed.x(), transformed.y(), &cell)) continue;
        const int index = grid_.index(cell);
        if (safe_mask_[index]) ++point_counts[index];
      }
    } catch (const std::runtime_error& error) {
      ROS_ERROR_THROTTLE(2.0, "Invalid PointCloud2: %s", error.what());
      return;
    }

    std::vector<bool> hits(grid_.occupancy.size(), false);
    for (size_t i = 0; i < point_counts.size(); ++i) {
      hits[i] = point_counts[i] >= min_points_per_cell_;
    }

    geometry_msgs::Point sensor_position;
    std::string position_error;
    if (currentPosition(&sensor_position, &position_error)) {
      for (int y = 0; y < grid_.height; ++y) {
        for (int x = 0; x < grid_.width; ++x) {
          const Cell cell{x, y};
          const int index = grid_.index(cell);
          if (!safe_mask_[index]) continue;
          const geometry_msgs::Point center = cellCenter(cell);
          if (std::abs(center.x - sensor_position.x) <= local_update_range_x_ &&
              std::abs(center.y - sensor_position.y) <= local_update_range_y_) {
            observed_free_[index] = true;
            if (!persistent_obstacles_) persistent_raw_obstacles_[index] = false;
          }
        }
      }
    } else {
      ROS_WARN_THROTTLE(2.0, "Cannot update local FREE region: %s",
                        position_error.c_str());
    }

    const std::vector<int8_t> old_occupancy = grid_.occupancy;
    for (size_t i = 0; i < hits.size(); ++i) {
      if (hits[i]) persistent_raw_obstacles_[i] = true;
    }
    rebuildOccupancy();
    map_ready_ = true;
    const bool changed = old_occupancy != grid_.occupancy;
    publishOccupancy();
    publishCoverage();
    if (changed) {
      ROS_INFO_STREAM_THROTTLE(2.0, "CCPP occupancy grid updated from "
                                       << message->width * message->height
                                       << " cloud points");
      if (auto_replan_ && odom_ready_) planFromCurrentPose();
    }
  }

  void odomCallback(const nav_msgs::OdometryConstPtr& message) {
    latest_odom_ = *message;
    odom_ready_ = true;
  }

  void reachedCallback(const geometry_msgs::PointStampedConstPtr& message) {
    if (!map_ready_) {
      ROS_WARN("Ignoring reached point before map is ready");
      return;
    }
    if (!message->header.frame_id.empty() &&
        message->header.frame_id != target_frame_) {
      ROS_ERROR_STREAM("Ignoring reached point in frame "
                       << message->header.frame_id << ", expected "
                       << target_frame_);
      return;
    }

    bool changed = false;
    for (int y = 0; y < grid_.height; y += coverage_stride_) {
      for (int x = 0; x < grid_.width; x += coverage_stride_) {
        const Cell cell{x, y};
        const geometry_msgs::Point center = cellCenter(cell);
        if (grid_.free(cell) &&
            std::hypot(center.x - message->point.x,
                       center.y - message->point.y) <=
                visited_radius_) {
          const int index = grid_.index(cell);
          if (!visited_[index]) {
            visited_[index] = true;
            sweep_after_rank_ = sweepRank(cell);
            changed = true;
          }
        }
      }
    }
    if (changed) {
      publishCoverage();
      ROS_INFO("CCPP reached point accepted: %.2f %.2f",
               message->point.x, message->point.y);
    } else {
      ROS_WARN("CCPP reached point did not match an unvisited free cell: %.2f %.2f",
               message->point.x, message->point.y);
    }
  }

  void skipAreaCallback(const geometry_msgs::PointStampedConstPtr& message) {
    if (!map_ready_) return;
    if (!message->header.frame_id.empty() &&
        message->header.frame_id != target_frame_) {
      ROS_ERROR_STREAM("Ignoring skip area in frame "
                       << message->header.frame_id << ", expected "
                       << target_frame_);
      return;
    }

    int changed = 0;
    for (int y = 0; y < grid_.height; y += coverage_stride_) {
      for (int x = 0; x < grid_.width; x += coverage_stride_) {
        const Cell cell{x, y};
        const geometry_msgs::Point center = cellCenter(cell);
        const int index = grid_.index(cell);
        if (safe_mask_[index] && !visited_[index] &&
            std::hypot(center.x - message->point.x,
                       center.y - message->point.y) <= target_skip_radius_) {
          visited_[index] = true;
          changed++;
        }
      }
    }
    if (changed > 0) {
      publishCoverage();
      ROS_INFO("CCPP skipped %d coverage cells around target %.2f %.2f",
               changed, message->point.x, message->point.y);
    }
  }

  bool currentPosition(geometry_msgs::Point* point, std::string* error) {
    if (!odom_ready_) {
      *error = "no odometry received";
      return false;
    }
    tf2::Transform transform;
    if (!lookupTransform(latest_odom_.header.frame_id,
                         latest_odom_.header.stamp, &transform)) {
      *error = "cannot transform odometry into " + target_frame_;
      return false;
    }
    const auto& source = latest_odom_.pose.pose.position;
    const tf2::Vector3 transformed =
        transform * tf2::Vector3(source.x, source.y, source.z);
    point->x = transformed.x();
    point->y = transformed.y();
    point->z = transformed.z();
    return true;
  }

  nav_msgs::OccupancyGrid makeGridMessage(const std::vector<int8_t>& data) const {
    nav_msgs::OccupancyGrid message;
    message.header.stamp = ros::Time::now();
    message.header.frame_id = target_frame_;
    message.info.map_load_time = message.header.stamp;
    message.info.resolution = resolution_;
    message.info.width = grid_.width;
    message.info.height = grid_.height;
    message.info.origin.position.x = grid_.origin_x;
    message.info.origin.position.y = grid_.origin_y;
    message.info.origin.orientation.w = 1.0;
    message.data = data;
    return message;
  }

  void publishOccupancy() { occupancy_pub_.publish(makeGridMessage(grid_.occupancy)); }

  void publishCoverage() {
    std::vector<int8_t> coverage = grid_.occupancy;
    nav_msgs::GridCells visited_cells;
    visited_cells.header.stamp = ros::Time::now();
    visited_cells.header.frame_id = target_frame_;
    visited_cells.cell_width = resolution_ * coverage_stride_;
    visited_cells.cell_height = resolution_ * coverage_stride_;
    nav_msgs::GridCells preset_skip_cells;
    preset_skip_cells.header = visited_cells.header;
    preset_skip_cells.cell_width = resolution_;
    preset_skip_cells.cell_height = resolution_;
    nav_msgs::GridCells forbidden_cells;
    forbidden_cells.header = visited_cells.header;
    forbidden_cells.cell_width = resolution_;
    forbidden_cells.cell_height = resolution_;
    for (size_t i = 0; i < coverage.size(); ++i) {
      const Cell cell{static_cast<int>(i % grid_.width),
                      static_cast<int>(i / grid_.width)};
      if (preset_skip_mask_[i] && coverage[i] != 100) {
        coverage[i] = 75;
        geometry_msgs::Point point = cellCenter(cell);
        point.z = 0.03;
        preset_skip_cells.cells.push_back(point);
      } else if (coverage[i] == 0 && visited_[i]) {
        coverage[i] = 50;
        geometry_msgs::Point point = cellCenter(cell);
        point.z = 0.03;
        visited_cells.cells.push_back(point);
      }
      if (forbidden_mask_[i]) {
        geometry_msgs::Point point = cellCenter(cell);
        point.z = 0.06;
        forbidden_cells.cells.push_back(point);
      }
    }
    coverage_pub_.publish(makeGridMessage(coverage));
    visited_pub_.publish(visited_cells);
    preset_skip_pub_.publish(preset_skip_cells);
    forbidden_pub_.publish(forbidden_cells);
  }

  bool hasUnknownSafeCell() const {
    for (size_t i = 0; i < grid_.occupancy.size(); ++i) {
      if (safe_mask_[i] && !preset_skip_mask_[i] &&
          grid_.occupancy[i] < 0) {
        return true;
      }
    }
    return false;
  }

  bool findFrontierRoute(const Cell& start, std::vector<Cell>* route) const {
    if (!grid_.free(start)) return false;
    const int count = grid_.width * grid_.height;
    std::vector<int> parent(count, -1);
    std::vector<int> distance(count, -1);
    std::queue<Cell> queue;
    const int start_index = grid_.index(start);
    parent[start_index] = start_index;
    distance[start_index] = 0;
    queue.push(start);
    int best_index = -1;
    int best_distance = -1;
    const int dx[4] = {1, 0, -1, 0};
    const int dy[4] = {0, 1, 0, -1};
    while (!queue.empty()) {
      const Cell cell = queue.front();
      queue.pop();
      const int index = grid_.index(cell);
      bool frontier = false;
      for (int direction = 0; direction < 4; ++direction) {
        const Cell next{cell.x + dx[direction], cell.y + dy[direction]};
        if (!grid_.inside(next)) continue;
        const int next_index = grid_.index(next);
        if (safe_mask_[next_index] && !preset_skip_mask_[next_index] &&
            grid_.occupancy[next_index] < 0) {
          frontier = true;
        }
        if (!grid_.free(next) || parent[next_index] >= 0) continue;
        parent[next_index] = index;
        distance[next_index] = distance[index] + 1;
        queue.push(next);
      }
      if (frontier && distance[index] > best_distance) {
        best_index = index;
        best_distance = distance[index];
      }
    }
    if (best_index < 0 || best_index == start_index) return false;
    std::vector<Cell> reverse;
    for (int index = best_index; index != start_index; index = parent[index]) {
      reverse.push_back(Cell{index % grid_.width, index / grid_.width});
    }
    std::reverse(reverse.begin(), reverse.end());
    const size_t max_steps =
        static_cast<size_t>(chunk_cells_ * coverage_stride_);
    if (reverse.size() > max_steps) reverse.resize(max_steps);
    *route = reverse;
    return true;
  }

  bool generatePlan(const geometry_msgs::Point& requested_start, int heading,
                    nav_msgs::Path* output_path, std::string* output_message,
                    bool* finished,
                    std::vector<uint8_t>* output_types) {
    *finished = false;
    output_types->clear();
    if (!map_ready_) {
      *output_message = "no valid point cloud has been converted yet";
      return false;
    }
    Cell start_cell;
    if (!worldToCell(requested_start.x, requested_start.y, &start_cell) ||
        !grid_.free(start_cell)) {
      *output_message = "start is outside the safe free region";
      return false;
    }

    std::vector<bool> unavailable = visited_;
    for (size_t i = 0; i < unavailable.size(); ++i) {
      if (preset_skip_mask_[i]) unavailable[i] = true;
    }
    GridPlanner planner(coverage_stride_);
    const PlanResult result = planner.plan(grid_, start_cell, heading,
                                           unavailable, chunk_cells_,
                                           sweep_after_rank_,
                                           start_from_corner_);
    if (!result.success) {
      *output_message = result.message;
      return false;
    }

    std::vector<Cell> planned_cells = result.path;
    std::vector<uint8_t> planned_types = result.waypoint_types;
    std::string plan_message = result.message;
    nav_msgs::Path path;
    path.header.stamp = ros::Time::now();
    path.header.frame_id = target_frame_;
    const bool unknown_remains = hasUnknownSafeCell();
    if (result.complete && result.new_cells == 0 && unknown_remains) {
      if (!findFrontierRoute(start_cell, &planned_cells)) {
        *output_message =
            "unknown area remains but no reachable free frontier exists";
        return false;
      }
      planned_types.assign(planned_cells.size(), 0);
      plan_message = "known area covered; moving toward an unknown frontier";
    } else if (result.complete && result.new_cells == 0) {
      *finished = true;
      *output_path = path;
      *output_message = "coverage complete: all reachable free cells are "
                        "confirmed visited";
      path_pub_.publish(path);
      publishCoverage();
      return true;
    }

    std::vector<geometry_msgs::Point> points;
    geometry_msgs::Point exact = requested_start;
    exact.z = flight_altitude_;
    points.push_back(exact);
    std::vector<uint8_t> point_types(1, 0);
    for (size_t cell_index = 0; cell_index < planned_cells.size();
         ++cell_index) {
      const Cell& cell = planned_cells[cell_index];
      const uint8_t type = cell_index < planned_types.size()
                               ? planned_types[cell_index]
                               : 0;
      if (cell_index == 0 && type == 0 && visited_[grid_.index(cell)]) {
        const geometry_msgs::Point center = cellCenter(cell);
        ROS_INFO("CCPP skips visited start cell: %.2f %.2f",
                 center.x, center.y);
        continue;
      }
      const geometry_msgs::Point center = cellCenter(cell);
      if (std::hypot(center.x - points.back().x, center.y - points.back().y) >
              1e-6 ||
          type == 1) {
        points.push_back(center);
        point_types.push_back(type);
      }
    }
    for (size_t i = 0; i < points.size(); ++i) {
      geometry_msgs::PoseStamped pose;
      pose.header = path.header;
      pose.pose.position = points[i];
      double yaw = 0.0;
      if (i + 1 < points.size()) {
        yaw = std::atan2(points[i + 1].y - points[i].y,
                         points[i + 1].x - points[i].x);
      } else if (i > 0) {
        yaw = std::atan2(points[i].y - points[i - 1].y,
                         points[i].x - points[i - 1].x);
      }
      pose.pose.orientation.z = std::sin(yaw * 0.5);
      pose.pose.orientation.w = std::cos(yaw * 0.5);
      path.poses.push_back(pose);
    }

    *output_path = path;
    *output_types = point_types;
    *output_message = plan_message;
    path_pub_.publish(path);
    publishCoverage();
    return true;
  }

  bool planCallback(PlanCoverage::Request& request,
                    PlanCoverage::Response& response) {
    geometry_msgs::Point start = request.start;
    if (request.use_current_pose && !currentPosition(&start, &response.message)) {
      response.success = false;
      return true;
    }
    const int heading = request.initial_heading >= 0 && request.initial_heading <= 3
                            ? request.initial_heading
                            : default_initial_heading_;
    bool finished = false;
    response.waypoint_types.clear();
    response.success = generatePlan(start, heading, &response.path,
                                    &response.message, &finished,
                                    &response.waypoint_types);
    response.finished = finished;
    if (response.success) ROS_INFO_STREAM("CCPP: " << response.message);
    else ROS_WARN_STREAM("CCPP planning rejected: " << response.message);
    return true;
  }

  void planFromCurrentPose() {
    geometry_msgs::Point start;
    std::string message;
    nav_msgs::Path path;
    std::vector<uint8_t> waypoint_types;
    bool finished = false;
    if (currentPosition(&start, &message)) {
      generatePlan(start, default_initial_heading_, &path, &message, &finished,
                   &waypoint_types);
    }
  }

  ros::NodeHandle nh_;
  ros::NodeHandle private_nh_;
  ros::Subscriber cloud_sub_;
  ros::Subscriber odom_sub_;
  ros::Subscriber reached_sub_;
  ros::Subscriber skip_area_sub_;
  ros::Publisher occupancy_pub_;
  ros::Publisher coverage_pub_;
  ros::Publisher visited_pub_;
  ros::Publisher preset_skip_pub_;
  ros::Publisher forbidden_pub_;
  ros::Publisher path_pub_;
  ros::ServiceServer plan_service_;
  tf2_ros::Buffer tf_buffer_;
  tf2_ros::TransformListener tf_listener_;

  std::string target_frame_;
  std::string cloud_topic_;
  std::string odom_topic_;
  double z_min_ = 0.0;
  double z_max_ = 0.0;
  double flight_altitude_ = 0.0;
  double resolution_ = 0.0;
  double inflation_radius_ = 0.0;
  double boundary_clearance_ = 0.0;
  double tf_timeout_ = 0.0;
  double visited_radius_ = 0.0;
  double target_skip_radius_ = 1.0;
  double forbidden_clearance_ = 0.30;
  double local_update_range_x_ = 0.0;
  double local_update_range_y_ = 0.0;
  int coverage_stride_ = 1;
  int chunk_cells_ = 8;
  bool start_from_corner_ = true;
  int min_points_per_cell_ = 1;
  int default_initial_heading_ = 0;
  bool persistent_obstacles_ = true;
  bool allow_latest_tf_fallback_ = true;
  bool auto_replan_ = false;
  bool map_ready_ = false;
  bool odom_ready_ = false;
  std::vector<Point2> hull_;
  std::vector<AxisAlignedRectangle> skip_rectangles_;
  std::vector<AxisAlignedRectangle> forbidden_rectangles_;
  Grid grid_;
  std::vector<bool> safe_mask_;
  std::vector<bool> preset_skip_mask_;
  std::vector<bool> forbidden_mask_;
  std::vector<bool> observed_free_;
  std::vector<bool> visited_;
  std::vector<bool> persistent_raw_obstacles_;
  int sweep_after_rank_ = -1;
  nav_msgs::Odometry latest_odom_;
};

}  // namespace ccpp_coverage_planner

int main(int argc, char** argv) {
  ros::init(argc, argv, "ccpp_planner");
  try {
    ccpp_coverage_planner::CcppPlannerNode node;
    ros::spin();
  } catch (const std::exception& error) {
    ROS_FATAL("Failed to start CCPP planner: %s", error.what());
    return 1;
  }
  return 0;
}
