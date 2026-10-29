# Troubleshooting

| Symptom | Check |
|---|---|
| ROS absent | Use standalone mode or the offline replay. ROS tests remain Not Verified. |
| Noetic APT/rosdep fails | Check EOL distro inclusion, repositories, keys and image availability. Do not disable signature validation. |
| GTest link errors | Match compiler ABI; do not link MSVC GTest to MinGW. Local source builds are supported. |
| Runtime DLL missing | Supply matching compiler runtime on PATH or appropriate static linker flags. |
| no valid point cloud has been converted yet | Verify cloud topic, XYZ fields, frame and TF. |
| no odometry received | Check odometry; explicit starts still need odometry to establish the local FREE window. |
| start is outside the safe free region | Inspect actual pose, frames, hull shrink, observed window and inflation. |
| start cell cannot connect to the coverage lattice | Inspect stride/corridor width and intermediate cells. |
| unknown area remains but no reachable free frontier exists | Check map observation and reachability; do not report completion. |
| Repeated targets | Send real type-1 arrivals and wait for state updates. Planning/odometry do not acknowledge. |
| Multi-robot name collisions | Absolute /ccpp/* interfaces need explicit remaps or isolated masters. |
| HTML source on GitHub | Download and open locally; hosting is prepared but not published. |
| Viewer/data mismatch | Regenerate from C++ and verify manifest; never hand-draw successful output. |
| Two strict xfails | H02/H03 are known defects, not successful rejection checks. XPASS requires review. |
| Format gate touches upstream files | Scope is maintained examples/new tests only; preserve original hashes. |
| Cppcheck cannot load std.cfg | Use a complete tool distribution. A relocated binary needs adjacent cfg files and matching runtime DLLs; see validation attempts. |

The coverage grid uses -1 unknown/outside, 0 FREE/unconfirmed, 50 visited, 75 preset skip and 100 blocked. These are planner states, not ground-truth sensor coverage.

Report exact OS/toolchain, snapshot, command, small map/YAML, start and response. Remove credentials and unnecessary logs.
