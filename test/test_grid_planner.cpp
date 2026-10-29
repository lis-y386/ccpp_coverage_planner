#include <ccpp_coverage_planner/grid_planner.h>
#include <gtest/gtest.h>

#include <algorithm>

namespace ccpp_coverage_planner {
namespace {

Grid freeGrid(int width, int height) {
  Grid grid;
  grid.width = width;
  grid.height = height;
  grid.resolution = 0.1;
  grid.occupancy.assign(width * height, 0);
  return grid;
}

TEST(GridPlanner, CoversRectangle) {
  const Grid grid = freeGrid(5, 4);
  const PlanResult result = GridPlanner(1).plan(grid, Cell{0, 0}, 0);
  EXPECT_TRUE(result.success) << result.message;
  EXPECT_EQ(20, result.new_cells);
  EXPECT_EQ(20u, static_cast<size_t>(std::count(
                     result.waypoint_types.begin(),
                     result.waypoint_types.end(), 1)));
}

TEST(GridPlanner, UsesAlternatingBoustrophedonColumns) {
  const Grid grid = freeGrid(3, 4);
  const PlanResult result = GridPlanner(1).plan(grid, Cell{0, 0}, 0);
  ASSERT_TRUE(result.success) << result.message;
  ASSERT_GE(result.path.size(), 8u);
  EXPECT_EQ(0, result.path[0].x);
  EXPECT_EQ(0, result.path[0].y);
  EXPECT_EQ(0, result.path[3].x);
  EXPECT_EQ(3, result.path[3].y);
  EXPECT_EQ(1, result.path[4].x);
  EXPECT_EQ(3, result.path[4].y);
  EXPECT_EQ(1, result.path[7].x);
  EXPECT_EQ(0, result.path[7].y);
}

TEST(GridPlanner, UsesAStarToLeaveDeadEnd) {
  Grid grid = freeGrid(5, 3);
  // T shape: a horizontal corridor and a branch from its middle.
  std::fill(grid.occupancy.begin(), grid.occupancy.end(), -1);
  for (int x = 0; x < 5; ++x) grid.occupancy[x] = 0;
  grid.occupancy[1 * grid.width + 2] = 0;
  grid.occupancy[2 * grid.width + 2] = 0;

  const PlanResult result = GridPlanner(1).plan(grid, Cell{0, 0}, 0);
  ASSERT_TRUE(result.success) << result.message;
  EXPECT_EQ(7, result.new_cells);
  EXPECT_GT(result.path.size(), 7u);  // A visited corridor cell is revisited.
}

TEST(GridPlanner, DoesNotCrossObstacle) {
  Grid grid = freeGrid(7, 3);
  for (int y = 0; y < grid.height; ++y) grid.occupancy[y * grid.width + 3] = 100;
  const PlanResult result = GridPlanner(1).plan(grid, Cell{0, 1}, 0);
  ASSERT_TRUE(result.success) << result.message;
  EXPECT_EQ(9, result.new_cells);
  for (const Cell& cell : result.path) EXPECT_LT(cell.x, 3);
}

TEST(GridPlanner, SkipsPresetCellsButMayUseThemAsTransit) {
  const Grid grid = freeGrid(5, 1);
  std::vector<bool> unavailable(grid.occupancy.size(), false);
  unavailable[1] = true;
  unavailable[2] = true;
  unavailable[3] = true;

  const PlanResult result =
      GridPlanner(1).plan(grid, Cell{0, 0}, 0, unavailable);
  ASSERT_TRUE(result.success) << result.message;
  EXPECT_EQ(2, result.new_cells);
  ASSERT_EQ(result.path.size(), result.waypoint_types.size());
  for (size_t i = 0; i < result.path.size(); ++i) {
    const int x = result.path[i].x;
    if (x >= 1 && x <= 3) EXPECT_EQ(0, result.waypoint_types[i]);
  }
  const Cell middle_transit{2, 0};
  EXPECT_NE(result.path.end(), std::find(result.path.begin(), result.path.end(),
                                         middle_transit));
}

TEST(GridPlanner, ConnectsAnUnalignedStartCell) {
  const Grid grid = freeGrid(7, 7);
  const PlanResult result = GridPlanner(3).plan(grid, Cell{2, 1}, 0);
  EXPECT_TRUE(result.success) << result.message;
  EXPECT_FALSE(result.path.empty());
}

TEST(GridPlanner, KeepsVisitedStateAndLimitsTheNextChunk) {
  const Grid grid = freeGrid(6, 2);
  std::vector<bool> visited(grid.occupancy.size(), false);
  visited[0] = true;
  visited[1] = true;
  const PlanResult result =
      GridPlanner(1).plan(grid, Cell{0, 0}, 0, visited, 3);
  ASSERT_TRUE(result.success) << result.message;
  EXPECT_EQ(3, result.new_cells);
  EXPECT_FALSE(result.complete);
  EXPECT_EQ(2u, std::count(result.coverage.begin(), result.coverage.end(), 50));
}

TEST(GridPlanner, ContinuesAfterLastConfirmedSweepRank) {
  const Grid grid = freeGrid(3, 4);
  std::vector<bool> visited(grid.occupancy.size(), false);
  visited[1 * grid.width + 0] = true;
  visited[2 * grid.width + 0] = true;
  visited[3 * grid.width + 0] = true;
  const PlanResult result =
      GridPlanner(1).plan(grid, Cell{0, 3}, 1, visited, 1, 3);
  ASSERT_TRUE(result.success) << result.message;
  ASSERT_EQ(result.path.size(), result.waypoint_types.size());
  auto coverage = std::find(result.waypoint_types.begin(),
                            result.waypoint_types.end(), 1);
  ASSERT_NE(result.waypoint_types.end(), coverage);
  const size_t index = std::distance(result.waypoint_types.begin(), coverage);
  EXPECT_EQ((Cell{1, 3}), result.path[index]);
}

TEST(GridPlanner, StartsNewSweepFromNearestSafeCorner) {
  const Grid grid = freeGrid(5, 5);
  const PlanResult result =
      GridPlanner(1).plan(grid, Cell{2, 2}, 1, {}, 1, -1, true);
  ASSERT_TRUE(result.success) << result.message;
  ASSERT_EQ(result.path.size(), result.waypoint_types.size());
  auto coverage = std::find(result.waypoint_types.begin(),
                            result.waypoint_types.end(), 1);
  ASSERT_NE(result.waypoint_types.end(), coverage);
  const size_t index = std::distance(result.waypoint_types.begin(), coverage);
  EXPECT_EQ((Cell{0, 0}), result.path[index]);
}

TEST(GridPlanner, MarksAStarConnectorsAsTransitOnly) {
  Grid grid = freeGrid(5, 3);
  std::fill(grid.occupancy.begin(), grid.occupancy.end(), -1);
  for (int x = 0; x < 5; ++x) grid.occupancy[x] = 0;
  grid.occupancy[1 * grid.width + 2] = 0;
  grid.occupancy[2 * grid.width + 2] = 0;
  const PlanResult result = GridPlanner(1).plan(grid, Cell{0, 0}, 0);
  ASSERT_TRUE(result.success) << result.message;
  ASSERT_EQ(result.path.size(), result.waypoint_types.size());
  EXPECT_EQ(static_cast<size_t>(result.new_cells),
            static_cast<size_t>(std::count(result.waypoint_types.begin(),
                                           result.waypoint_types.end(), 1)));
  EXPECT_NE(result.waypoint_types.end(),
            std::find(result.waypoint_types.begin(),
                      result.waypoint_types.end(), 0));
}

TEST(GridPlanner, ReportsCompleteWhenAllReachableCellsWereVisited) {
  const Grid grid = freeGrid(3, 2);
  std::vector<bool> visited(grid.occupancy.size(), true);
  const PlanResult result =
      GridPlanner(1).plan(grid, Cell{0, 0}, 0, visited, 2);
  ASSERT_TRUE(result.success) << result.message;
  EXPECT_TRUE(result.complete);
  EXPECT_EQ(0, result.new_cells);
}

}  // namespace
}  // namespace ccpp_coverage_planner
