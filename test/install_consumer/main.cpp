#include <ccpp_coverage_planner/grid_planner.h>

int main() {
  ccpp_coverage_planner::Grid grid;
  grid.width = 2;
  grid.height = 2;
  grid.resolution = 0.25;
  grid.occupancy.assign(4, 0);
  const auto result = ccpp_coverage_planner::GridPlanner(1).plan(grid, {0, 0}, 0);
  return result.success && result.new_cells == 4 ? 0 : 1;
}
