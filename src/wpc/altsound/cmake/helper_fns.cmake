#-------------------------------------------------------------------------------
# helper_fns.cmake
# Dave Roscoe 07/20/2023
#
# CMake requires all functions to be defined before they are called and provides
# no mechanism for forward declaration.  Grouping helper functions in a single
# .cmake file allows a simple include to bring in as many functions as we want
# while only adding a single line to the source script
#-------------------------------------------------------------------------------

function(findRootDirectory KNOWN_FILE_OR_DIRECTORY)
    # Get the current source directory
    set(CURRENT_DIR ${CMAKE_CURRENT_SOURCE_DIR})
    set(ROOT_DIR ${CURRENT_DIR})

    # Traverse upwards until the root directory is found
    while(NOT EXISTS "${ROOT_DIR}/${KNOWN_FILE_OR_DIRECTORY}")
        get_filename_component(ROOT_DIR "${ROOT_DIR}/.." ABSOLUTE)
        if(ROOT_DIR STREQUAL "/")
            message(FATAL_ERROR "Root directory not found.")
        endif()
    endwhile()

    message(STATUS "Root directory: ${ROOT_DIR}")
    set(ROOT_DIR ${ROOT_DIR} PARENT_SCOPE)
endfunction()