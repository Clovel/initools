# 
#                     Copyright (C) 2020 Clovis Durand
# 
# -----------------------------------------------------------------------------

# Definitions ---------------------------------------------
add_definitions(-DEXAMPLE)

# Source files --------------------------------------------
file(GLOB_RECURSE EXAMPLE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

# Target definition ---------------------------------------
add_executable(${CMAKE_PROJECT_NAME}-example
    ${EXAMPLE_SOURCES}
)
add_dependencies(${CMAKE_PROJECT_NAME}-example
    ${CMAKE_PROJECT_NAME}
)
target_link_libraries(${CMAKE_PROJECT_NAME}-example
    ${CMAKE_PROJECT_NAME}
)
