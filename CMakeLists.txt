cmake_minimum_required(VERSION 3.24)
project(SO2_P2 CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# raylib (okno z wizualizacja) pobierany automatycznie przy pierwszej konfiguracji
include(FetchContent)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
        raylib
        GIT_REPOSITORY https://github.com/raysan5/raylib.git
        GIT_TAG 5.5
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(raylib)

find_package(Threads REQUIRED)

add_executable(SO2_P2
        src/main.cpp

        src/core/types.h
        src/core/patient.h
        src/core/sleep_util.h
        src/core/config.cpp
        src/core/config.h
        src/core/configuration_watcher.cpp
        src/core/configuration_watcher.h
        src/core/hospital.cpp
        src/core/hospital.h

        src/flow/arrival_dispatcher.cpp
        src/flow/arrival_dispatcher.h
        src/flow/registrar.cpp
        src/flow/registrar.h
        src/flow/triage.cpp
        src/flow/triage.h
        src/flow/department.cpp
        src/flow/department.h
        src/flow/doctor.cpp
        src/flow/doctor.h

        src/sync/safe_queue.cpp
        src/sync/safe_queue.h
        src/sync/resource_manager.cpp
        src/sync/resource_manager.h
        src/sync/deadlock_detector.cpp
        src/sync/deadlock_detector.h

        src/output/monitor.cpp
        src/output/monitor.h
        src/output/metrics_collector.cpp
        src/output/metrics_collector.h
        src/output/patient_logger.cpp
        src/output/patient_logger.h

        src/view/hospital_view.cpp
        src/view/hospital_view.h
)

target_include_directories(SO2_P2 PRIVATE src)
target_link_libraries(SO2_P2 PRIVATE raylib Threads::Threads)
target_compile_definitions(SO2_P2 PRIVATE SOR_DEFAULT_CONFIG="${CMAKE_SOURCE_DIR}/config/config.txt")

# MinGW: biblioteki GCC i C++ wbudowane w .exe (pelne -static psuje linkowanie z winpthread)
if (MINGW)
    target_link_options(SO2_P2 PRIVATE -static-libgcc -static-libstdc++)
endif ()
