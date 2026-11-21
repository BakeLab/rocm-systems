#
# Miscellaneous cmake functions for rocprofiler-sdk
#

include_guard(GLOBAL)

function(rocprofiler_sdk_get_gfx_architectures _VAR)
    cmake_parse_arguments(ARG "ECHO" "PREFIX;DELIM" "" ${ARGN})

    if(NOT DEFINED ARG_DELIM)
        set(ARG_DELIM ", ")
    endif()

    set(CMAKE_MESSAGE_INDENT "[${PROJECT_NAME}]${ARG_PREFIX} ")

    find_program(
        rocminfo_EXECUTABLE
        NAMES rocminfo
        HINTS ${rocprofiler-sdk_ROOT_DIR} ${rocm_version_DIR} ${ROCM_PATH} /opt/rocm
        PATHS ${rocprofiler-sdk_ROOT_DIR} ${rocm_version_DIR} ${ROCM_PATH} /opt/rocm
        PATH_SUFFIXES bin)

    if(rocminfo_EXECUTABLE)
        execute_process(
            COMMAND ${rocminfo_EXECUTABLE}
            RESULT_VARIABLE rocminfo_RET
            OUTPUT_VARIABLE rocminfo_OUT
            ERROR_VARIABLE rocminfo_ERR
            OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_STRIP_TRAILING_WHITESPACE)

        if(rocminfo_RET EQUAL 0)
            set(rocminfo_SUCCEEDED ON)
            string(REGEX MATCHALL "gfx([0-9A-Fa-f]+)" rocminfo_GFXINFO "${rocminfo_OUT}")
            list(REMOVE_DUPLICATES rocminfo_GFXINFO)
        else()
            message(
                AUTHOR_WARNING
                    "${rocminfo_EXECUTABLE} returned ${rocminfo_RET}\nstderr:\n${rocminfo_ERR}\nstdout:\n${rocminfo_OUT}"
                )
        endif()
    endif()

    # Fallback for environments where rocminfo is unavailable or cannot enumerate a GPU at
    # configure time (containers without /dev/kfd, cross-compile).
    if("${rocminfo_GFXINFO}" STREQUAL "" AND NOT "${GPU_TARGETS}" STREQUAL "")
        set(rocminfo_GFXINFO "${GPU_TARGETS}")
        message(STATUS "using explicit GPU_TARGETS=${GPU_TARGETS}")
    endif()

    # Preserve the prior behavior when rocminfo cannot run and no fallback was provided.
    if(rocminfo_SUCCEEDED OR rocminfo_GFXINFO)
        set(${_VAR}
            "${rocminfo_GFXINFO}"
            PARENT_SCOPE)

        if(ARG_ECHO)
            string(REPLACE ";" "${ARG_DELIM}" _GFXINFO_ECHO "${rocminfo_GFXINFO}")
            message(STATUS "System architectures: ${_GFXINFO_ECHO}")
        endif()
    endif()
endfunction()

# Reports whether the KFD device node (/dev/kfd) is present.
#
# The answer describes the machine running cmake, which is not necessarily the machine
# running ctest: ROCm is routinely configured inside a container that does not pass
# /dev/kfd through even though the bare-metal host does have it. Set
# ROCPROFILER_SDK_ASSUME_KFD to say so explicitly (ON to keep the KFD-gated tests and the
# unscaled timeouts, OFF to force the no-KFD behavior); it overrides the probe entirely.
# Deliberately not cached, so a configure-host change is picked up by re-running cmake
# rather than needing the build tree wiped.
function(rocprofiler_sdk_kfd_available _VAR)
    if(DEFINED ROCPROFILER_SDK_ASSUME_KFD)
        set(_KFD_AVAILABLE "${ROCPROFILER_SDK_ASSUME_KFD}")
    elseif(EXISTS "/dev/kfd")
        set(_KFD_AVAILABLE ON)
    else()
        set(_KFD_AVAILABLE OFF)
    endif()
    set(${_VAR}
        "${_KFD_AVAILABLE}"
        PARENT_SCOPE)
endfunction()

# In case the underlying architecture does not support PC sampling, this function will
# tell us whether the PC sampling is disabled
function(rocprofiler_sdk_pc_sampling_disabled _VAR)
    cmake_parse_arguments(ARG "ECHO" "PREFIX" "" ${ARGN})

    set(CMAKE_MESSAGE_INDENT "[${PROJECT_NAME}]${ARG_PREFIX} ")

    rocprofiler_sdk_get_gfx_architectures(rocprofiler-sdk-tests-gfx-info ECHO)
    list(GET rocprofiler-sdk-tests-gfx-info 0 pc-sampling-gpu-0-gfx-info)

    if("${pc-sampling-gpu-0-gfx-info}" MATCHES "^gfx90a$"
       OR "${pc-sampling-gpu-0-gfx-info}" MATCHES "^gfx94[0-9]$"
       OR "${pc-sampling-gpu-0-gfx-info}" MATCHES "^gfx95[0-9]$"
       OR "${pc-sampling-gpu-0-gfx-info}" MATCHES "^gfx12[0-9][0-9]$")
        # PC sampling is enabled on this architecture.
        set(${_VAR}
            FALSE
            PARENT_SCOPE)
        if(ARG_ECHO)
            message(STATUS "PC Sampling is enabled for ${pc-sampling-gpu-0-gfx-info}")
        endif()
    else()
        # PC sampling is disabled on this architecture.
        set(${_VAR}
            TRUE
            PARENT_SCOPE)
        if(ARG_ECHO)
            message(STATUS "PC Sampling is disabled for ${pc-sampling-gpu-0-gfx-info}")
        endif()
    endif()
endfunction()

# In case the underlying architecture does not support stochastic PC sampling, this
# function will tell us whether the PC sampling is disabled
function(rocprofiler_sdk_pc_sampling_stochastic_disabled _VAR)
    cmake_parse_arguments(ARG "ECHO" "PREFIX" "" ${ARGN})

    set(CMAKE_MESSAGE_INDENT "[${PROJECT_NAME}]${ARG_PREFIX} ")

    rocprofiler_sdk_get_gfx_architectures(rocprofiler-sdk-tests-gfx-info ECHO)
    list(GET rocprofiler-sdk-tests-gfx-info 0 pc-sampling-gpu-0-gfx-info)

    if("${pc-sampling-gpu-0-gfx-info}" MATCHES "^gfx94[0-9]$"
       OR "${pc-sampling-gpu-0-gfx-info}" MATCHES "^gfx95[0-9]$"
       OR "${pc-sampling-gpu-0-gfx-info}" MATCHES "^gfx1250$")
        # PC sampling is enabled on this architecture.
        set(${_VAR}
            FALSE
            PARENT_SCOPE)
        if(ARG_ECHO)
            message(STATUS "PC Sampling is enabled for ${pc-sampling-gpu-0-gfx-info}")
        endif()
    else()
        # PC sampling is disabled on this architecture.
        set(${_VAR}
            TRUE
            PARENT_SCOPE)
        if(ARG_ECHO)
            message(STATUS "PC Sampling is disabled for ${pc-sampling-gpu-0-gfx-info}")
        endif()
    endif()
endfunction()

# Checks whether triple buffer is implemented for architecture: MI3xx and gfx12
function(rocprofiler_sdk_sqtt_triple_buffer_disabled _VAR)
    cmake_parse_arguments(ARG "ECHO" "PREFIX" "" ${ARGN})

    set(CMAKE_MESSAGE_INDENT "[${PROJECT_NAME}]${ARG_PREFIX} ")

    rocprofiler_sdk_get_gfx_architectures(rocprofiler-sdk-tests-gfx-info ECHO)
    list(GET rocprofiler-sdk-tests-gfx-info 0 gpu-0-gfx-info)

    if("${gpu-0-gfx-info}" MATCHES "^gfx(9[4-5][0-9]|12[0-9][0-9])$")
        set(${_VAR}
            FALSE
            PARENT_SCOPE)
    else()
        set(${_VAR}
            TRUE
            PARENT_SCOPE)
    endif()
endfunction()

function(rocprofiler_sdk_spm_disabled _VAR)
    cmake_parse_arguments(ARG "ECHO" "PREFIX" "" ${ARGN})

    set(CMAKE_MESSAGE_INDENT "[${PROJECT_NAME}]${ARG_PREFIX} ")

    rocprofiler_sdk_get_gfx_architectures(rocprofiler-sdk-tests-gfx-info ECHO)
    list(GET rocprofiler-sdk-tests-gfx-info 0 spm-gpu-0-gfx-info)

    if("${spm-gpu-0-gfx-info}" MATCHES "^gfx94[0-9]$")
        # spm is enabled on this architecture.
        set(${_VAR}
            FALSE
            PARENT_SCOPE)
        if(ARG_ECHO)
            message(STATUS "SPM is enabled for ${spm-gpu-0-gfx-info}")
        endif()
    else()
        # SPM is disabled on this architecture.
        set(${_VAR}
            TRUE
            PARENT_SCOPE)
        if(ARG_ECHO)
            message(STATUS "SPM is disabled for ${spm-gpu-0-gfx-info}")
        endif()
    endif()
endfunction()
