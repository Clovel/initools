# 
#                     Copyright (C) 2020 Clovis Durand
# 
# -----------------------------------------------------------------------------

# Header files --------------------------------------------
file(GLOB PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/*.hpp
)

file(GLOB_RECURSE MODULE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp
)

set(HEADERS
    ${PUBLIC_HEADERS}
    ${MODULE_HEADERS}
)

# Source files --------------------------------------------
file(GLOB SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
)

# Link directories ----------------------------------------

# Target definition ---------------------------------------
add_library(${CMAKE_PROJECT_NAME} SHARED
    ${SOURCES}
)
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES
    PUBLIC_HEADER "${PUBLIC_HEADERS}"
)

#----------------------------------------------------------------------------
# The installation is prepended by the CMAKE_INSTALL_PREFIX variable
install(TARGETS ${CMAKE_PROJECT_NAME}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    PUBLIC_HEADER DESTINATION include
)
