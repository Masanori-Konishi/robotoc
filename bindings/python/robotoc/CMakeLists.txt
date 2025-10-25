set(ROBOTOC_PYTHON_VERSION ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR})
if(DEFINED CATKIN_PACKAGE_PYTHON_DESTINATION)
  message(STATUS "Installing robotoc python bindings to catkin destination")
  set(ROBOTOC_PYTHON_BINDINGS_LIBDIR ${CATKIN_PACKAGE_PYTHON_DESTINATION})
else()
  message(STATUS "Installing robotoc python bindings to default destination")
  set(ROBOTOC_PYTHON_BINDINGS_LIBDIR
      ${CMAKE_INSTALL_LIBDIR}/python${ROBOTOC_PYTHON_VERSION}/site-packages/robotoc)
endif()
message(STATUS "ROBOTOC_PYTHON_BINDINGS_LIBDIR: ${ROBOTOC_PYTHON_BINDINGS_LIBDIR}")

# create symlink for robotoc package root __init__.py in catkin devel space
## robotoc/__init__.py
set(_devel_pkg_dir "${CATKIN_DEVEL_PREFIX}/${ROBOTOC_PYTHON_BINDINGS_LIBDIR}")
file(MAKE_DIRECTORY "${_devel_pkg_dir}")
file(CREATE_LINK
  "${CMAKE_CURRENT_SOURCE_DIR}/__init__.py"
  "${_devel_pkg_dir}/__init__.py"
  SYMBOLIC)

# create symlink for each subpackage in catkin devel space
## e.g. robotoc/robot/__init__.py
## e.g. robotoc/utils/trajectory_viewer.py
macro(pybind11_add_robotoc_subpackage MODULE_DIR)
  set(_devel_module_dir "${_devel_pkg_dir}/${MODULE_DIR}")

  # create subpackage directory
  file(MAKE_DIRECTORY "${_devel_module_dir}")

  # symlink all .py files in the subpackage directory
  file(GLOB _PY_FILES_${MODULE_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/*.py")
  foreach(_py ${_PY_FILES_${MODULE_DIR}})
    get_filename_component(_name "${_py}" NAME)
    file(CREATE_LINK "${_py}" "${_devel_module_dir}/${_name}" SYMBOLIC)
  endforeach()
endmacro()

# create pybind11 module
## e.g. robotoc/robot/contact_status_xxxx.so
macro(pybind11_add_robotoc_module MODULE_DIR MODULE)
  pybind11_add_module(
    ${MODULE} 
    SHARED 
    ${MODULE}.cpp
  )

  # export .so to catkin devel space
  set(_devel_module_dir "${_devel_pkg_dir}/${MODULE_DIR}")
  set_target_properties(${MODULE} PROPERTIES
    PREFIX "" # does not include 'lib' prefix for python module
    LIBRARY_OUTPUT_DIRECTORY "${_devel_module_dir}"
  )

  target_link_libraries(
    ${MODULE} 
    PRIVATE
    robotoc::robotoc
  )
  install(
    TARGETS ${MODULE} 
    COMPONENT python
    DESTINATION ${ROBOTOC_PYTHON_BINDINGS_LIBDIR}/${MODULE_DIR}
  )
endmacro()

macro(install_robotoc_python_files CURRENT_MODULE_DIR)
  file(GLOB PYTHON_FILES_${CURRENT_MODULE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/*.py)
  install(
    FILES ${PYTHON_FILES_${CURRENT_MODULE_DIR}} 
    DESTINATION ${ROBOTOC_PYTHON_BINDINGS_LIBDIR}/${CURRENT_MODULE_DIR}
  )
endmacro()

add_subdirectory(robot)
add_subdirectory(core)
add_subdirectory(cost)
add_subdirectory(constraints)
# add_subdirectory(dynamics)
add_subdirectory(ocp)
add_subdirectory(sto)
add_subdirectory(planner)
add_subdirectory(riccati)
add_subdirectory(line_search)
add_subdirectory(solver)
add_subdirectory(mpc)
add_subdirectory(utils)

install(
  FILES __init__.py 
  DESTINATION ${ROBOTOC_PYTHON_BINDINGS_LIBDIR}/
)