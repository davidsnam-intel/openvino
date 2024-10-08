# Copyright (C) 2018-2024 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
#

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_OLD ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_OLD ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY})
set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY_OLD ${CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY})
set(CMAKE_PDB_OUTPUT_DIRECTORY_OLD ${CMAKE_PDB_OUTPUT_DIRECTORY})

if(OV_GENERATOR_MULTI_CONFIG)
    set(PYTHON_BRIDGE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/$<CONFIG>/python/openvino)
else()
    set(PYTHON_BRIDGE_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/python/openvino)
endif()

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PYTHON_BRIDGE_OUTPUT_DIRECTORY})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PYTHON_BRIDGE_OUTPUT_DIRECTORY})
set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY ${PYTHON_BRIDGE_OUTPUT_DIRECTORY})
set(CMAKE_PDB_OUTPUT_DIRECTORY ${PYTHON_BRIDGE_OUTPUT_DIRECTORY})

set(TARGET_FE_NAME "openvino_mock_mo_frontend")
set(PYBIND_FE_NAME "mock_mo_python_api")

set(PYBIND_FE_SRC ${CMAKE_CURRENT_SOURCE_DIR}/mock_mo_python_api.cpp)

source_group("src" FILES ${PYBIND_FE_SRC})

if(CMAKE_CROSSCOMPILING)
    set(pybind11_min_version 2.12.0)
else()
    set(pybind11_min_version 2.12.0)
endif()
# search for FindPython3.cmake instead of legacy modules
set(PYBIND11_FINDPYTHON ON)

ov_detect_python_module_extension()

find_package(pybind11 ${pybind11_min_version} QUIET)

if(NOT pybind11_FOUND)
    add_subdirectory(${OpenVINO_SOURCE_DIR}/src/bindings/python/thirdparty/pybind11
                     ${CMAKE_CURRENT_BINARY_DIR}/pybind11_build
                     EXCLUDE_FROM_ALL)
endif()

ov_find_python3(REQUIRED)
pybind11_add_module(${PYBIND_FE_NAME} MODULE NO_EXTRAS ${PYBIND_FE_SRC})

target_link_libraries(${PYBIND_FE_NAME} PRIVATE openvino::runtime)
target_link_libraries(${PYBIND_FE_NAME} PRIVATE ${TARGET_FE_NAME})

add_dependencies(${PYBIND_FE_NAME} ${TARGET_FE_NAME})

ov_add_clang_format_target(${PYBIND_FE_NAME}_clang FOR_TARGETS ${PYBIND_FE_NAME})

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY_OLD})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY_OLD})
set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY_OLD})
set(CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_PDB_OUTPUT_DIRECTORY_OLD})

install(TARGETS ${PYBIND_FE_NAME}
        DESTINATION ${OV_CPACK_PYTHONDIR}
        COMPONENT tests EXCLUDE_FROM_ALL)
