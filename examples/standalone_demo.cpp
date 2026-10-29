// Runs the unchanged upstream GridPlanner on synthetic maps. No ROS or flight IO.
#include <ccpp_coverage_planner/grid_planner.h>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <queue>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using namespace ccpp_coverage_planner;

namespace {
void require(bool value, const std::string& message) {
  if (!value) throw std::runtime_error(message);
}

struct Scenario {
  Grid grid;
  Cell start{0, 0};
  int stride = 1;
  std::vector<bool> skip;
};

Scenario makeScenario(const std::string& name) {
  Scenario scene;
  auto& grid = scene.grid;
  grid.width = 20;
  grid.height = 16;
  grid.resolution = 0.25;
  grid.occupancy.assign(grid.width * grid.height, 0);
  scene.skip.assign(grid.occupancy.size(), false);
  if (name == "obstacles" || name == "stride") {
    for (int y = 4; y <= 10; ++y)
      for (int x = 8; x <= 11; ++x) grid.occupancy[grid.index({x, y})] = 100;
  } else if (name == "maze") {
    const int walls[] = {4, 9, 14};
    const int gaps[] = {3, 12, 6};
    for (int i = 0; i < 3; ++i)
      for (int y = 0; y < grid.height; ++y)
        if (std::abs(y - gaps[i]) > 1) grid.occupancy[grid.index({walls[i], y})] = 100;
  } else if (name == "skip") {
    // A whole column is skipped as targets, but must remain traversable.
    for (int y = 0; y < grid.height; ++y) scene.skip[grid.index({9, y})] = true;
  } else if (name == "disconnected") {
    for (int y = 0; y < grid.height; ++y) grid.occupancy[grid.index({9, y})] = 100;
  } else {
    require(name == "rectangle", "unknown scenario: " + name);
  }
  if (name == "stride") scene.stride = 2;
  return scene;
}

bool clear(const Grid& grid, const Cell& a, const Cell& b) {
  if (!grid.free(a) || !grid.free(b)) return false;
  // Demo starts on its lattice; every returned edge must be axis aligned.
  if (a.x != b.x && a.y != b.y) return false;
  const int distance = std::abs(b.x - a.x) + std::abs(b.y - a.y);
  for (int i = 0; i <= distance; ++i) {
    const int dx = (b.x > a.x) - (b.x < a.x);
    const int dy = (b.y > a.y) - (b.y < a.y);
    if (!grid.free({a.x + dx * i, a.y + dy * i})) return false;
  }
  return true;
}

std::set<int> reachableTargets(const Scenario& scene) {
  std::set<int> reached;
  std::queue<Cell> pending;
  pending.push(scene.start);
  reached.insert(scene.grid.index(scene.start));
  const int step = scene.stride;
  const Cell offsets[] = {{step, 0}, {0, step}, {-step, 0}, {0, -step}};
  while (!pending.empty()) {
    const Cell current = pending.front();
    pending.pop();
    for (const auto& offset : offsets) {
      const Cell next{current.x + offset.x, current.y + offset.y};
      if (clear(scene.grid, current, next) && reached.insert(scene.grid.index(next)).second)
        pending.push(next);
    }
  }
  for (auto it = reached.begin(); it != reached.end();) {
    if (scene.skip[*it])
      it = reached.erase(it);
    else
      ++it;
  }
  return reached;
}

struct Step {
  Cell cell;
  int type;
  int chunk;
};

void writeArtifacts(const Scenario& scene, const std::vector<Step>& steps, const std::string& name,
                    const std::string& prefix, int confirmed, int chunks) {
  std::ofstream json(prefix + ".json");
  require(json.good(), "cannot open output: " + prefix + ".json");
  json << "{\n  \"provenance\": \"synthetic map; unchanged C++ GridPlanner; ideal simulated "
          "arrivals\",\n"
       << "  \"scenario\": \"" << name << "\",\n  \"width\": " << scene.grid.width
       << ", \"height\": " << scene.grid.height << ", \"stride\": " << scene.stride
       << ", \"confirmed_targets\": " << confirmed << ", \"chunks\": " << chunks
       << ",\n  \"occupancy\": [";
  for (size_t i = 0; i < scene.grid.occupancy.size(); ++i)
    json << (i ? "," : "") << static_cast<int>(scene.grid.occupancy[i]);
  json << "],\n  \"skip\": [";
  for (size_t i = 0; i < scene.skip.size(); ++i) json << (i ? "," : "") << (scene.skip[i] ? 1 : 0);
  json << "],\n  \"steps\": [\n";
  for (size_t i = 0; i < steps.size(); ++i) {
    const auto& s = steps[i];
    json << (i ? ",\n" : "") << "    [" << s.cell.x << "," << s.cell.y << "," << s.type << ","
         << s.chunk << "]";
  }
  json << "\n  ]\n}\n";
  json.close();
  require(!json.fail(), "failed to write JSON");

  std::ofstream svg(prefix + ".svg");
  require(svg.good(), "cannot open SVG output");
  const int scale = 26;
  const auto x = [&](int value) { return 35 + (value + 0.5) * scale; };
  const auto y = [&](int value) { return 85 + (scene.grid.height - value - 0.5) * scale; };
  svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"800\" height=\"570\" viewBox=\"0 0 800 "
         "570\">"
      << "<rect width=\"800\" height=\"570\" fill=\"#faf9f6\"/>"
      << "<g font-family=\"sans-serif\" fill=\"#243746\">"
      << "<text x=\"35\" y=\"35\" font-size=\"22\">CCPP / " << name << "</text>"
      << "<text x=\"35\" y=\"60\" font-size=\"12\">Synthetic map / C++ planner output / ideal "
         "simulated arrivals</text>";
  for (int row = 0; row < scene.grid.height; ++row) {
    for (int col = 0; col < scene.grid.width; ++col) {
      const int index = scene.grid.index({col, row});
      const char* color = scene.grid.occupancy[index] == 100 ? "#344454"
                          : scene.skip[index]                ? "#f2cf85"
                                                             : "#ffffff";
      svg << "<rect x=\"" << x(col) - scale / 2.0 << "\" y=\"" << y(row) - scale / 2.0
          << "\" width=\"26\" height=\"26\" fill=\"" << color << "\" stroke=\"#e1e5e8\"/>";
    }
  }
  svg << "<polyline fill=\"none\" stroke=\"#138e91\" stroke-opacity=\"0.7\" stroke-width=\"2\" "
         "points=\"";
  for (const auto& step : steps) svg << x(step.cell.x) << ',' << y(step.cell.y) << ' ';
  svg << "\"/>";
  for (const auto& step : steps)
    if (step.type == 1)
      svg << "<circle cx=\"" << x(step.cell.x) << "\" cy=\"" << y(step.cell.y)
          << "\" r=\"2.8\" fill=\"#138e91\"/>";
  svg << "<circle cx=\"" << x(scene.start.x) << "\" cy=\"" << y(scene.start.y)
      << "\" r=\"5\" fill=\"#c05735\"/>"
      << "<text x=\"585\" y=\"110\" font-size=\"16\">" << confirmed << " targets</text>"
      << "<text x=\"585\" y=\"140\" font-size=\"16\">" << chunks << " chunks</text>"
      << "<text x=\"585\" y=\"180\" font-size=\"13\">Teal: route / targets</text>"
      << "<text x=\"585\" y=\"205\" font-size=\"13\">Dark: obstacle</text>"
      << "<text x=\"585\" y=\"230\" font-size=\"13\">Amber: skip targets</text>"
      << "<text x=\"35\" y=\"545\" font-size=\"12\">Completion applies to reachable coverage "
         "lattice cells, not every physical point.</text>"
      << "</g></svg>\n";
  svg.close();
  require(!svg.fail(), "failed to write SVG");
}

void run(const std::string& name, const std::string& prefix) {
  const Scenario scene = makeScenario(name);
  const auto expected = reachableTargets(scene);
  std::vector<bool> visited = scene.skip;
  std::set<int> confirmed;
  Cell current = scene.start;
  int rank = -1;
  int chunks = 0;
  bool completed = false;
  bool used_skip_transit = false;
  std::vector<Step> steps;
  GridPlanner planner(scene.stride);
  for (int call = 0; call < 1000; ++call) {
    const auto plan = planner.plan(scene.grid, current, 0, visited, 8, rank, true);
    require(plan.success, plan.message);
    require(plan.path.size() == plan.waypoint_types.size(), "unaligned waypoint types");
    require(plan.new_cells <= 8, "chunk target limit exceeded");
    // Planning is repeatable without arrival acknowledgments; it must not visit cells.
    const auto repeated = planner.plan(scene.grid, current, 0, visited, 8, rank, true);
    require(plan.path == repeated.path && plan.waypoint_types == repeated.waypoint_types,
            "planning changed state without confirmation");
    if (plan.complete) {
      require(plan.new_cells == 0, "completed plan still has new targets");
      completed = true;
      break;
    }
    require(plan.new_cells > 0, "planner made no progress");
    ++chunks;
    int accepted = 0;
    for (size_t i = 0; i < plan.path.size(); ++i) {
      const Cell next = plan.path[i];
      require(clear(scene.grid, current, next), "path crosses obstacle or leaves map");
      const int index = scene.grid.index(next);
      const int type = plan.waypoint_types[i];
      require(type == 0 || type == 1, "unknown waypoint type");
      if (scene.skip[index]) {
        require(type == 0, "skip cell scheduled as coverage");
        used_skip_transit = true;
      }
      current = next;  // Ideal synthetic movement, not flight execution.
      steps.push_back({next, type, chunks});
      if (type == 1) {
        require(expected.count(index) == 1, "unexpected coverage target");
        require(confirmed.insert(index).second, "coverage target scheduled twice");
        visited[index] = true;  // Confirm only after the synthetic move.
        const int rows = (scene.grid.height + scene.stride - 1) / scene.stride;
        const int column = next.x / scene.stride;
        const int row = next.y / scene.stride;
        rank = column * rows + (column % 2 == 0 ? row : rows - 1 - row);
        ++accepted;
      }
    }
    require(accepted == plan.new_cells, "new-cell count differs from confirmations");
  }
  require(completed, "completion exceeded bounded call limit");
  require(confirmed == expected, "reachable coverage targets missing");
  if (name == "skip") require(used_skip_transit, "skip corridor was not exercised");
  if (!prefix.empty())
    writeArtifacts(scene, steps, name, prefix, static_cast<int>(confirmed.size()), chunks);
  std::cout << name << ": PASS; " << confirmed.size() << " confirmed targets, " << chunks
            << " chunks, " << steps.size() << " path points; synthetic arrivals only\n";
}
}  // namespace

int main(int argc, char** argv) {
  try {
    std::string scenario = "obstacles", output;
    bool self_test = false;
    for (int i = 1; i < argc; ++i) {
      const std::string arg = argv[i];
      if (arg == "--scenario" && i + 1 < argc)
        scenario = argv[++i];
      else if (arg == "--output" && i + 1 < argc)
        output = argv[++i];
      else if (arg == "--self-test")
        self_test = true;
      else if (arg == "--help") {
        std::cout << "ccpp_demo [--scenario rectangle|obstacles|skip|disconnected|stride|maze] "
                     "[--output PREFIX] [--self-test]\n";
        return 0;
      } else
        throw std::runtime_error("invalid argument: " + arg);
    }
    if (self_test) {
      const auto bad = GridPlanner(1).plan(Grid{}, Cell{}, 0);
      require(!bad.success, "invalid grid was accepted");
      for (const auto& name : {"rectangle", "obstacles", "skip", "disconnected", "stride", "maze"})
        run(name, "");
    } else
      run(scenario, output);
    return 0;
  } catch (const std::exception& error) {
    std::cerr << "CCPP demo failed: " << error.what() << '\n';
    return 1;
  }
}
