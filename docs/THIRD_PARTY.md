# Dependency, code and asset provenance

This is an engineering inventory, not legal clearance or a complete historical ownership audit. Package-level BSD-3-Clause does not relicense external dependencies.

| Item | Provenance / declared license | Distribution boundary / status |
|---|---|---|
| Preserved CCPP sources | drone/ccpp_dev at 8bc54d2; package.xml declares BSD-3-Clause | Original hashes retained. Owner must confirm authorship and any incorporated code before public release. |
| Local demo/tool/docs additions | Created for this project | Included under package BSD-3-Clause. No third-party paper figures, photos or datasets copied. |
| Banner, architecture, badges, scene SVGs and GIF | tools/render_assets.py from validated native JSON | Original generated artwork; assets/PROVENANCE.json records source and output hashes. System font files are not bundled. |
| ROS dependencies | Resolved from package.xml | Separately distributed ROS packages keep their own licenses. Do not call the entire ROS dependency stack BSD. |
| xmlrpcpp | Upstream [package.xml declares LGPL-2.1](https://raw.githubusercontent.com/ros/ros_comm/noetic-devel/utilities/xmlrpcpp/package.xml) | External linked ROS dependency; inspect the actual installed library/source, linking method and obligations before distributing binaries/images. No blanket compatibility/legal claim. |
| GoogleTest v1.14.0 | [Official license](https://github.com/google/googletest/blob/v1.14.0/LICENSE), BSD-style 3-clause | Local source under ignored .maintenance/vendor; not copied into public source payload. Preserve notices if redistributing test binaries/source. |
| Playwright 1.62.1 | npm package lock metadata: Apache-2.0; optional fsevents MIT | Developer tooling only; not required by HTML and not bundled into it. npm lock includes integrity values. |
| Pillow / PyYAML / pytest / clang-format | Optional requirements-dev.txt | Not vendored into runtime/source payload. Check their actual installed notices when distributing a development environment. |
| Cppcheck | Existing local tool, copied only into ignored maintenance area for configuration troubleshooting | Not part of public package. Its tool license does not describe the analyzed code's license. |
| Docker ROS base image / APT packages | ros:noetic-ros-base-focal and resolved packages | Mutable, not built locally. Inspect the actual digest and package notices before redistributing an image. |

## References versus implementation

The [ROS Navigation repository](https://github.com/ros-planning/navigation) and [Nav2 repository](https://github.com/ros-navigation/navigation2) were inspected for public technical presentation patterns: concise entry points, separate architecture/API docs and clear installation paths. No code, artwork, branding or claims of affiliation were copied.

[GitHub citation guidance](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/customizing-your-repository/about-citation-files) informed software citation metadata. There is no DOI, paper, public independent repository URL, release date or institution invented in CITATION.cff.

Algorithms are described from this project's actual source. No third-party algorithm citation is asserted as the origin of this implementation without historical evidence.

## Public-release gate

Owner confirmation of upstream rights remains **Blocked**. When distributing compiled ROS binaries or containers, inventory resolved dependencies and their notices/linking obligations separately. Hash matching proves identity to a source snapshot, not authorship or license compliance.
