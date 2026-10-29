#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ccpp_coverage_planner {

struct Cell {
  int x = 0;
  int y = 0;

  bool operator==(const Cell& other) const {
    return x == other.x && y == other.y;
  }
  bool operator!=(const Cell& other) const { return !(*this == other); }
};

struct Grid {
  int width = 0;
  int height = 0;
  double resolution = 0.0;
  double origin_x = 0.0;
  double origin_y = 0.0;
  // -1 outside/unknown, 0 free, 100 occupied.
  std::vector<int8_t> occupancy;

  bool valid() const;
  bool inside(const Cell& cell) const;
  int index(const Cell& cell) const;
  bool free(const Cell& cell) const;
};

struct PlanResult {
  bool success = false;
  bool complete = false;
  int new_cells = 0;
  std::string message;
  std::vector<Cell> path;
  // Aligned with path: 0 is an A* transit point, 1 is a coverage target.
  std::vector<uint8_t> waypoint_types;
  // Same size as Grid::occupancy: 0 unvisited, 50 visited, 100 obstacle,
  // and -1 outside/unknown.
  std::vector<int8_t> coverage;
};

class GridPlanner {
 public:
  explicit GridPlanner(int stride);

  PlanResult plan(const Grid& grid, const Cell& requested_start,
                  int initial_heading,
                  const std::vector<bool>& already_visited = {},
                  int max_new_cells = 0,
                  int sweep_after_rank = -1,
                  bool start_from_corner = false) const;

 private:
  int stride_;
};

}  // namespace ccpp_coverage_planner
