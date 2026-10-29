cmake_minimum_required(VERSION 3.10)
project(ccpp_coverage_planner LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
  add_compile_options(/W4)
else()
  add_compile_options(-Wall -Wextra)
endif()

# The algorithm itself has no ROS dependencies. Keep catkin as the default.
option(CCPP_STANDALONE "Build core planner and synthetic demo without ROS" OFF)
if(CCPP_STANDALONE)
  add_library(ccpp_grid_planner src/grid_planner.cpp)
  target_include_directories(ccpp_grid_planner PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>)
  set_target_properties(ccpp_grid_planner PROPERTIES EXPORT_NAME grid_planner)
  add_executable(ccpp_demo examples/standalone_demo.cpp)
  target_link_libraries(ccpp_demo ccpp_grid_planner)
  enable_testing()
  foreach(scene rectangle obstacles skip disconnected stride maze)
    add_test(NAME demo_${scene} COMMAND ccpp_demo --scenario ${scene})
  endforeach()
  add_test(NAME core_invariants COMMAND ccpp_demo --self-test)
  option(CCPP_BUILD_UPSTREAM_TESTS "Also build upstream GTest tests (matching compiler ABI required)" OFF)
  if(CCPP_BUILD_UPSTREAM_TESTS)
    set(CCPP_GTEST_SOURCE_DIR "" CACHE PATH "Optional local GoogleTest source checkout")
    if(CCPP_GTEST_SOURCE_DIR)
      set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
      add_subdirectory("${CCPP_GTEST_SOURCE_DIR}" "${CMAKE_BINARY_DIR}/_gtest" EXCLUDE_FROM_ALL)
    else()
      find_package(GTest REQUIRED)
    endif()
    add_executable(test_grid_planner test/test_grid_planner.cpp)
    target_link_libraries(test_grid_planner ccpp_grid_planner GTest::gtest_main)
    add_test(NAME upstream_grid_planner COMMAND test_grid_planner)
  endif()
  add_executable(test_core_boundaries test/test_core_boundaries.cpp)
  target_link_libraries(test_core_boundaries ccpp_grid_planner)
  foreach(case invalid_shape no_free_cells single_cell chunk_ack thin_obstacle unknown_barrier)
    add_test(NAME boundary_${case} COMMAND test_core_boundaries ${case})
  endforeach()
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)
  configure_package_config_file(cmake/ccpp_coverage_plannerConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/ccpp_coverage_plannerConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ccpp_coverage_planner)
  install(TARGETS ccpp_grid_planner EXPORT ccpp_coverage_plannerTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(TARGETS ccpp_demo RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  install(EXPORT ccpp_coverage_plannerTargets NAMESPACE ccpp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ccpp_coverage_planner)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ccpp_coverage_plannerConfig.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ccpp_coverage_planner)
  install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
  install(DIRECTORY docs assets tools examples
    DESTINATION ${CMAKE_INSTALL_DATADIR}/ccpp_coverage_planner)
  install(FILES LICENSE README.md README_EN.md README_zh-CN.md CITATION.cff CONTRIBUTING.md CODE_OF_CONDUCT.md SECURITY.md CHANGELOG.md
    DESTINATION ${CMAKE_INSTALL_DATADIR}/ccpp_coverage_planner)
  return()
endif()

find_package(catkin REQUIRED COMPONENTS
  geometry_msgs
  message_generation
  nav_msgs
  roscpp
  sensor_msgs
  std_msgs
  tf2
  tf2_ros
  xmlrpcpp
)

add_service_files(
  FILES
  PlanCoverage.srv
)

generate_messages(
  DEPENDENCIES
  geometry_msgs
  nav_msgs
)

catkin_package(
  INCLUDE_DIRS include
  LIBRARIES ccpp_grid_planner
  CATKIN_DEPENDS geometry_msgs message_runtime nav_msgs roscpp sensor_msgs std_msgs tf2 tf2_ros xmlrpcpp
)

include_directories(include ${catkin_INCLUDE_DIRS})

add_library(ccpp_grid_planner
  src/grid_planner.cpp
)
add_dependencies(ccpp_grid_planner ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})
target_link_libraries(ccpp_grid_planner ${catkin_LIBRARIES})

add_executable(ccpp_planner_node
  src/ccpp_planner_node.cpp
)
add_dependencies(ccpp_planner_node ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})
target_link_libraries(ccpp_planner_node ccpp_grid_planner ${catkin_LIBRARIES})

if(CATKIN_ENABLE_TESTING)
  find_package(rostest REQUIRED)
  catkin_install_python(PROGRAMS test/test_demo_integration.py
    DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})
  catkin_install_python(PROGRAMS test/test_service_contract.py
    DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION})
  add_rostest(test/demo_integration.test)
  add_rostest(test/service_contract.test)
  add_executable(test_core_boundaries test/test_core_boundaries.cpp)
  target_link_libraries(test_core_boundaries ccpp_grid_planner)
  add_test(NAME core_boundaries COMMAND test_core_boundaries)
  catkin_add_gtest(test_grid_planner test/test_grid_planner.cpp)
  if(TARGET test_grid_planner)
    target_link_libraries(test_grid_planner ccpp_grid_planner ${GTEST_MAIN_LIBRARIES} ${catkin_LIBRARIES})
  endif()
endif()

install(TARGETS ccpp_grid_planner ccpp_planner_node
  ARCHIVE DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  LIBRARY DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)
install(DIRECTORY include/${PROJECT_NAME}/
  DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION}
)
catkin_install_python(PROGRAMS scripts/demo_world.py
  DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)
install(FILES README.md README_EN.md README_zh-CN.md LICENSE CHANGELOG.md CONTRIBUTING.md CITATION.cff CODE_OF_CONDUCT.md SECURITY.md
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION})
install(DIRECTORY launch config rviz docs assets tools examples
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
)

install(DIRECTORY test/ DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/test
  FILES_MATCHING PATTERN "*.test")
