cmake_minimum_required(VERSION 3.16)
project(shengwei_written_test LANGUAGES C CXX CUDA)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_SOURCE_DIR}/include)



find_package(Eigen3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIRS})
find_package(PCL REQUIRED COMPONENTS
    common
    io
    filters
    kdtree
    octree
    search
    surface
    features
    registration
    segmentation
    visualization
)
include_directories(${PCL_INCLUDE_DIRS})
add_definitions(${PCL_DEFINITIONS})

# CUDA
find_package(CUDA QUIET)
if(CUDA_FOUND)
    enable_language(CUDA)
    include_directories(${CUDA_INCLUDE_DIRS})
    add_definitions(-DHAS_CUDA)
    set(CMAKE_CUDA_ARCHITECTURES 89) # 个人使用的是rtx 4060 -> 89
    set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -arch=sm_89")
    message(STATUS "CUDA found: ${CUDA_VERSION}, arch=sm_89 (RTX 4060)")
else()
    message(STATUS "CUDA not found, building CPU-only")
endif()


# ---- Q1: MapManager 测试 ----
add_executable(Q1_answer_test
    src/test/Q1_answer_test.cpp
    src/map_manager.cpp
)

# ---- Q2: Graph 测试 ----
add_executable(Q2_answer_test
    src/test/Q2_answer_test.cpp
    src/graph.cpp
)

target_link_libraries(Q1_answer_test ${PCL_LIBRARIES})
target_link_libraries(Q2_answer_test ${PCL_LIBRARIES})

if(CUDA_FOUND)
    target_link_libraries(Q1_answer_test
        ${CUDA_CUBLAS_LIBRARIES}
        ${CUDA_CUFFT_LIBRARIES}
    )
    target_link_libraries(Q2_answer_test
        ${CUDA_CUBLAS_LIBRARIES}
        ${CUDA_CUFFT_LIBRARIES}
    )
endif()
