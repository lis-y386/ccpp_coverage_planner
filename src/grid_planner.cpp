#include <ccpp_coverage_planner/grid_planner.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <queue>
#include <unordered_set>

namespace ccpp_coverage_planner {
namespace {

struct QueueEntry {
  int f = 0;
  int index = -1;
  bool operator<(const QueueEntry& other) const { return f > other.f; }
};

}  // namespace

bool Grid::valid() const {
  return width > 0 && height > 0 && resolution > 0.0 &&
         occupancy.size() == static_cast<size_t>(width * height);
}

bool Grid::inside(const Cell& cell) const {
  return cell.x >= 0 && cell.y >= 0 && cell.x < width && cell.y < height;
}

int Grid::index(const Cell& cell) const { return cell.y * width + cell.x; }

bool Grid::free(const Cell& cell) const {
  return inside(cell) && occupancy[index(cell)] == 0;
}

GridPlanner::GridPlanner(int stride) : stride_(std::max(1, stride)) {}

PlanResult GridPlanner::plan(const Grid& grid, const Cell& requested_start,
                             int initial_heading,
                             const std::vector<bool>& already_visited,
                             int max_new_cells,
                             int sweep_after_rank,
                             bool start_from_corner) const {
  PlanResult result;
  (void)initial_heading;
  if (!grid.valid()) {
    result.message = "invalid occupancy grid";
    return result;
  }

  result.coverage = grid.occupancy;
  std::vector<Cell> nodes;
  std::unordered_set<int> node_indices;
  for (int y = 0; y < grid.height; y += stride_) {
    for (int x = 0; x < grid.width; x += stride_) {
      const Cell cell{x, y};
      if (grid.free(cell)) {
        nodes.push_back(cell);
        node_indices.insert(grid.index(cell));
        result.coverage[grid.index(cell)] = 0;
      }
    }
  }
  if (nodes.empty()) {
    result.message = "search region contains no free coverage nodes";
    return result;
  }

  auto clearSegment = [&](const Cell& from, const Cell& to) {
    const int dx = to.x - from.x;
    const int dy = to.y - from.y;
    const int steps = std::max(std::abs(dx), std::abs(dy));
    if (steps == 0) return grid.free(from);
    for (int step = 1; step <= steps; ++step) {
      const Cell sample{
          static_cast<int>(std::lround(from.x + dx * step /
                                                   static_cast<double>(steps))),
          static_cast<int>(std::lround(from.y + dy * step /
                                                   static_cast<double>(steps)))};
      if (!grid.free(sample)) return false;
    }
    return true;
  };

  Cell start = nodes.front();
  double best_distance = std::numeric_limits<double>::infinity();
  for (const Cell& node : nodes) {
    const double dx = static_cast<double>(node.x - requested_start.x);
    const double dy = static_cast<double>(node.y - requested_start.y);
    const double distance = dx * dx + dy * dy;
    if (distance < best_distance && clearSegment(requested_start, node)) {
      best_distance = distance;
      start = node;
    }
  }
  if (!std::isfinite(best_distance)) {
    result.message = "start cell cannot connect to the coverage lattice";
    return result;
  }

  const int directions[4][2] = {{stride_, 0}, {0, stride_},
                                {-stride_, 0}, {0, -stride_}};
  auto neighbor = [&](const Cell& cell, int direction) {
    return Cell{cell.x + directions[direction][0],
                cell.y + directions[direction][1]};
  };
  auto connected = [&](const Cell& from, const Cell& to) {
    return node_indices.count(grid.index(to)) > 0 && clearSegment(from, to);
  };

  // Restrict planning to the connected component containing the start.
  std::unordered_set<int> reachable;
  std::queue<Cell> frontier;
  frontier.push(start);
  reachable.insert(grid.index(start));
  while (!frontier.empty()) {
    const Cell current = frontier.front();
    frontier.pop();
    for (int direction = 0; direction < 4; ++direction) {
      const Cell next = neighbor(current, direction);
      if (!grid.inside(next) || !connected(current, next)) continue;
      const int next_index = grid.index(next);
      if (reachable.insert(next_index).second) frontier.push(next);
    }
  }

  std::unordered_set<int> visited;
  if (already_visited.size() == grid.occupancy.size()) {
    for (const int index : reachable) {
      if (already_visited[index]) {
        visited.insert(index);
        result.coverage[index] = 50;
      }
    }
  }
  const size_t confirmed_visited = visited.size();
  Cell current = start;
  result.path.push_back(current);
  result.waypoint_types.push_back(0);
  int new_cells = 0;

  auto findAStarRoute = [&](const Cell& source, const Cell& goal,
                            std::vector<Cell>* route) {
    const int source_index = grid.index(source);
    const int goal_index = grid.index(goal);
    const int count = grid.width * grid.height;
    const int infinity = std::numeric_limits<int>::max() / 4;
    std::vector<int> g_score(count, infinity);
    std::vector<int> parent(count, -1);
    std::priority_queue<QueueEntry> open;
    g_score[source_index] = 0;
    open.push({std::abs(source.x - goal.x) + std::abs(source.y - goal.y),
               source_index});

    while (!open.empty()) {
      const int current_index = open.top().index;
      open.pop();
      if (current_index == goal_index) break;
      const Cell cell{current_index % grid.width, current_index / grid.width};
      for (int direction = 0; direction < 4; ++direction) {
        const Cell next = neighbor(cell, direction);
        if (!grid.inside(next) || reachable.count(grid.index(next)) == 0 ||
            !connected(cell, next)) {
          continue;
        }
        const int next_index = grid.index(next);
        const int tentative = g_score[current_index] + 1;
        if (tentative >= g_score[next_index]) continue;
        g_score[next_index] = tentative;
        parent[next_index] = current_index;
        const int heuristic =
            (std::abs(next.x - goal.x) + std::abs(next.y - goal.y)) / stride_;
        open.push({tentative + heuristic, next_index});
      }
    }
    if (source_index != goal_index && parent[goal_index] < 0) return false;

    std::vector<Cell> reverse_route;
    for (int index = goal_index; index != source_index; index = parent[index]) {
      reverse_route.push_back(Cell{index % grid.width, index / grid.width});
    }
    std::reverse(reverse_route.begin(), reverse_route.end());
    *route = std::move(reverse_route);
    return true;
  };

  std::vector<Cell> sweep_order;
  sweep_order.reserve(reachable.size());
  int sweep_column = 0;
  for (int x = 0; x < grid.width; x += stride_, ++sweep_column) {
    std::vector<Cell> column;
    for (int y = 0; y < grid.height; y += stride_) {
      const Cell cell{x, y};
      const int index = grid.index(cell);
      if (reachable.count(index) > 0) column.push_back(cell);
    }
    if (sweep_column % 2 != 0) {
      std::reverse(column.begin(), column.end());
    }
    sweep_order.insert(sweep_order.end(), column.begin(), column.end());
  }

  auto sweepRank = [&](const Cell& cell) {
    const int rows = (grid.height + stride_ - 1) / stride_;
    const int column = cell.x / stride_;
    const int row = cell.y / stride_;
    return column * rows + (column % 2 == 0 ? row : rows - 1 - row);
  };

  size_t first_target = 0;
  if (sweep_after_rank >= 0) {
    while (first_target < sweep_order.size() &&
           sweepRank(sweep_order[first_target]) <= sweep_after_rank) {
      ++first_target;
    }
    if (first_target == sweep_order.size()) first_target = 0;
  } else if (start_from_corner) {
    const std::array<Cell, 4> corners = {
        Cell{0, 0}, Cell{0, grid.height - 1},
        Cell{grid.width - 1, 0}, Cell{grid.width - 1, grid.height - 1}};
    int best_start_distance = std::numeric_limits<int>::max();
    int best_rank = std::numeric_limits<int>::max();
    for (const Cell& corner : corners) {
      size_t nearest = sweep_order.size();
      int best_corner_distance = std::numeric_limits<int>::max();
      for (size_t i = 0; i < sweep_order.size(); ++i) {
        const Cell& cell = sweep_order[i];
        if (visited.count(grid.index(cell)) > 0) continue;
        const int corner_distance = std::abs(cell.x - corner.x) +
                                    std::abs(cell.y - corner.y);
        if (corner_distance < best_corner_distance) {
          best_corner_distance = corner_distance;
          nearest = i;
        }
      }
      if (nearest == sweep_order.size()) continue;
      const Cell& cell = sweep_order[nearest];
      const int start_distance = std::abs(cell.x - start.x) +
                                 std::abs(cell.y - start.y);
      const int rank = sweepRank(cell);
      if (start_distance < best_start_distance ||
          (start_distance == best_start_distance && rank < best_rank)) {
        best_start_distance = start_distance;
        best_rank = rank;
        first_target = nearest;
      }
    }
  } else {
    int best = std::numeric_limits<int>::max();
    for (size_t i = 0; i < sweep_order.size(); ++i) {
      if (visited.count(grid.index(sweep_order[i])) > 0) continue;
      const int distance = std::abs(sweep_order[i].x - start.x) +
                           std::abs(sweep_order[i].y - start.y);
      if (distance < best) {
        best = distance;
        first_target = i;
      }
    }
  }

  std::unordered_set<int> scheduled;
  for (size_t offset = 0; offset < sweep_order.size(); ++offset) {
    if (max_new_cells > 0 && new_cells >= max_new_cells) break;
    const Cell& target =
        sweep_order[(first_target + offset) % sweep_order.size()];
    if (visited.count(grid.index(target)) > 0) continue;
    if (scheduled.count(grid.index(target)) > 0) continue;
    std::vector<Cell> route;
    if (!findAStarRoute(current, target, &route)) {
      result.message = "A* could not connect boustrophedon sweep segments";
      return result;
    }
    if (route.empty()) route.push_back(target);
    for (size_t i = 0; i < route.size(); ++i) {
      const Cell& cell = route[i];
      const bool coverage_target = i + 1 == route.size();
      if (result.path.back() == cell) {
        if (coverage_target) result.waypoint_types.back() = 1;
      } else {
        result.path.push_back(cell);
        result.waypoint_types.push_back(coverage_target ? 1 : 0);
      }
      current = cell;
    }
    scheduled.insert(grid.index(target));
    ++new_cells;
  }

  result.success = true;
  result.complete = confirmed_visited >= reachable.size();
  result.new_cells = new_cells;
  result.message = "confirmed " + std::to_string(confirmed_visited) + "/" +
                   std::to_string(reachable.size()) + " visited, planned " +
                   std::to_string(new_cells) +
                   " new boustrophedon cells, using " +
                   std::to_string(result.path.size()) + " path points";
  return result;
}

}  // namespace ccpp_coverage_planner
