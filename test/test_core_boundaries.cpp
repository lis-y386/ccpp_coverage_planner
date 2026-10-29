#include <ccpp_coverage_planner/grid_planner.h>

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

using ccpp_coverage_planner::Cell;
using ccpp_coverage_planner::Grid;
using ccpp_coverage_planner::GridPlanner;

namespace {
void check(bool condition, const std::string& message) {
  if (!condition) throw std::runtime_error(message);
}
Grid grid(int width, int height, int8_t value = 0) {
  Grid out;
  out.width = width;
  out.height = height;
  out.resolution = 0.25;
  out.occupancy.assign(width * height, value);
  return out;
}
void run(const std::string& name) {
  if (name == "invalid_shape") {
    Grid invalid;
    check(!GridPlanner(1).plan(invalid, Cell{}, 0).success, "empty grid accepted");
    invalid = grid(2, 2);
    invalid.occupancy.pop_back();
    check(!GridPlanner(1).plan(invalid, Cell{}, 0).success, "mismatched data accepted");
    invalid = grid(2, 2);
    invalid.resolution = 0;
    check(!GridPlanner(1).plan(invalid, Cell{}, 0).success, "zero resolution accepted");
    invalid.resolution = -1;
    check(!GridPlanner(1).plan(invalid, Cell{}, 0).success, "negative resolution accepted");
  } else if (name == "no_free_cells") {
    check(!GridPlanner(1).plan(grid(4, 4, 100), Cell{}, 0).success, "occupied map accepted");
    check(!GridPlanner(1).plan(grid(4, 4, -1), Cell{}, 0).success, "unknown map accepted");
  } else if (name == "single_cell") {
    auto map = grid(1, 1);
    const auto result = GridPlanner(1).plan(map, Cell{}, 0);
    check(result.success && result.new_cells == 1 && !result.complete, "single target missing");
    check(result.path.size() == 1 && result.waypoint_types[0] == 1, "single target type");
    const auto done = GridPlanner(1).plan(map, Cell{}, 0, {true});
    check(done.success && done.complete && done.new_cells == 0, "single confirmed completion");
  } else if (name == "chunk_ack") {
    const auto map = grid(5, 3);
    std::vector<bool> visited(map.occupancy.size(), false);
    const auto first = GridPlanner(1).plan(map, Cell{}, 0, visited, 1);
    check(first.success && first.new_cells == 1 && !first.complete, "chunk budget");
    const auto repeated = GridPlanner(1).plan(map, Cell{}, 0, visited, 1);
    check(first.path == repeated.path, "planning changed caller state");
    check(!visited[0], "caller mask mutated");
    visited[map.index(first.path.back())] = true;
    const auto next = GridPlanner(1).plan(map, first.path.back(), 0, visited, 1);
    check(next.success && next.new_cells == 1, "next chunk missing");
    for (size_t i = 0; i < next.path.size(); ++i)
      if (next.waypoint_types[i] == 1)
        check(!visited[map.index(next.path[i])], "confirmed target scheduled again");
  } else if (name == "thin_obstacle") {
    auto map = grid(7, 3);
    for (int y = 0; y < 3; ++y) map.occupancy[map.index({1, y})] = 100;
    const auto result = GridPlanner(2).plan(map, Cell{}, 0);
    check(result.success && result.new_cells == 2, "thin wall did not split lattice");
    for (const auto& cell : result.path) check(cell.x == 0, "path crossed intermediate obstacle");
  } else if (name == "unknown_barrier") {
    auto map = grid(5, 3);
    for (int y = 0; y < 3; ++y) map.occupancy[map.index({2, y})] = -1;
    const auto result = GridPlanner(1).plan(map, Cell{}, 0);
    check(result.success && result.new_cells == 6, "unknown cells crossed");
    for (const auto& cell : result.path) check(cell.x < 2, "unknown barrier crossed");
  } else {
    throw std::runtime_error("unknown boundary case: " + name);
  }
  std::cout << name << ": assertions passed\n";
}
}  // namespace

int main(int argc, char** argv) {
  try {
    const std::string mode = argc > 1 ? argv[1] : "all";
    if (mode == "--probe-invalid-start") {
      auto map = grid(3, 1);
      map.occupancy[1] = 100;
      const bool occupied_accepted = GridPlanner(1).plan(map, {1, 0}, 0).success;
      const bool outside_accepted = GridPlanner(1).plan(grid(3, 1), {-1, 0}, 0).success;
      std::cout << "occupied_start_accepted=" << occupied_accepted
                << " outside_start_accepted=" << outside_accepted << '\n';
      return occupied_accepted || outside_accepted ? 1 : 0;
    }
    if (mode == "--probe-nonfinite-grid") {
      auto map = grid(2, 2);
      map.resolution = std::numeric_limits<double>::infinity();
      const bool accepted = map.valid();
      std::cout << "infinite_resolution_accepted=" << accepted << '\n';
      return accepted ? 1 : 0;
    }
    if (mode == "all") {
      for (const auto& name : {"invalid_shape", "no_free_cells", "single_cell", "chunk_ack",
                               "thin_obstacle", "unknown_barrier"})
        run(name);
    } else {
      run(mode);
    }
    return 0;
  } catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
  }
}
