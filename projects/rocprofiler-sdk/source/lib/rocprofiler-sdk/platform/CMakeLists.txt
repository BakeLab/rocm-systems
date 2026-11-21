set(ROCPROFILER_PLATFORM_HEADERS agent.hpp)

target_sources(rocprofiler-sdk-object-library PRIVATE ${ROCPROFILER_PLATFORM_HEADERS})

if(WIN32)
    add_subdirectory(windows)
else()
    add_subdirectory(gnulinux)
endif()

if(ROCPROFILER_BUILD_TESTS)
    add_subdirectory(tests)
endif()
