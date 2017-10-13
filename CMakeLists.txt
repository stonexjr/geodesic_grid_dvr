CMAKE_MINIMUM_REQUIRED(VERSION 3.1.3 FATAL_ERROR)

PROJECT(geodesic_grid_dvr)

SET(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
SET(HEADER ${SRC_DIR}/dir.h)
SET(SOURCE ${SRC_DIR}/main.cpp
           ${SRC_DIR}/dir.cpp
    )

SET(MESH_HEADER
    ${SRC_DIR}/IMesh.h
    ${SRC_DIR}/GCRMMesh.h
)
SET(MESH_SOURCE
    ${SRC_DIR}/IMesh.cpp
    ${SRC_DIR}/GCRMMesh.cpp
)

SOURCE_GROUP("mesh" FILES ${MESH_HEADER} ${MESH_SOURCE})

SET(UTILITY_HEADER ${SRC_DIR}/HostArrayTemplate.h
    ${SRC_DIR}/netcdf_read.h
    ${SRC_DIR}/NetCDFFile.h
    ${SRC_DIR}/NetCDFManager.h
    )

SET(UTILITY_SOURCE 
    ${SRC_DIR}/netcdf_read.cpp
    ${SRC_DIR}/NetCDFFile.cpp
    ${SRC_DIR}/NetCDFManager.cpp
    )

SOURCE_GROUP("utility" FILES ${UTILITY_HEADER} ${UTILITY_SOURCE})

SET(SHADER_SOURCE
    ${SRC_DIR}/shader/gcrm_triangle_mesh.vert
    ${SRC_DIR}/shader/gcrm_triangle_mesh.geom
    ${SRC_DIR}/shader/gcrm_triangle_mesh.frag
    ${SRC_DIR}/shader/gcrm_hexagon_mesh.vert
    ${SRC_DIR}/shader/gcrm_hexagon_mesh_frame.geom
    ${SRC_DIR}/shader/gcrm_hexagon_mesh_fill.geom
    ${SRC_DIR}/shader/gcrm_hexagon_mesh.frag
    ${SRC_DIR}/shader/gcrm_dvr.geom
    ${SRC_DIR}/shader/gcrm_dvr.frag
)
SOURCE_GROUP("shader" FILES ${SHADER_SOURCE})

IF(WIN32)
    # hide insecure CRT warnings, common practice
    ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
    
    IF(NOT(MSVC_VERSION LESS "1600"))
        ADD_DEFINITIONS(-D_WIN32_WINNT=0x0601)
        ADD_DEFINITIONS(-DWINVER=0x0601)
    ELSEIF(NOT(MSVC_VERSION LESS "1300"))
        ADD_DEFINITIONS(-D_WIN32_WINNT=0x0501)
        ADD_DEFINITIONS(-DWINVER=0x0501)
    ELSE()
        ADD_DEFINITIONS(-D_WIN32_WINNT=0x0500)
        ADD_DEFINITIONS(-DWINVER=0x0500)
    ENDIF()

ENDIF()

IF( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    MESSAGE( "64 bits compiler detected" )
    SET( PLATFORM_NAME "x64" )
ELSE( CMAKE_SIZEOF_VOID_P EQUAL 8 ) 
    MESSAGE( "32 bits compiler detected" )
    SET( PLATFORM_NAME "x86" )
ENDIF( CMAKE_SIZEOF_VOID_P EQUAL 8 )

IF( CMAKE_BUILD_TYPE STREQUAL "Debug" )
    MESSAGE( "Debug build" )
    SET( CONFIGURATION_NAME "Debug" )
ELSE( CMAKE_BUILD_TYPE STREQUAL "Debug" ) 
    MESSAGE( "Release build" )
    SET( CONFIGURATION_NAME "Release" )
ENDIF( CMAKE_BUILD_TYPE STREQUAL "Debug" )

#Prompt user to specify glew installation root.
FIND_PACKAGE(GLEW QUIET)
IF(GLEW_FOUND)
    MESSAGE("glew is found!")
ELSE()
    MESSAGE("\nglew cannot be automatically detected on your machine.")
    MESSAGE("Please install and manually set glew root: GLEW_ROOT_DIR in the cmake")
    SET(GLEW_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/glew" CACHE PATH
        "Please specify glew installation directory")
    SET(GLEW_INCLUDE_DIRS "${GLEW_ROOT_DIR}/include")
    SET(GLEW_LIB_DIR "${GLEW_ROOT_DIR}/lib")
    IF(WIN32)
        SET(GLEW_LIBRARIES "glew")
        SET(GLEW_LIB_DIR2 "${GLEW_LIB_DIR}/${PLATFORM_NAME}")
    ELSE()
        SET(GLEW_LIBRARIES "GLEW")
    ENDIF()
    MESSAGE("glew root dir GLEW_ROOT_DIR is set to: ${GLEW_ROOT_DIR}")
    MESSAGE("glew possible lib dir: ${GLEW_LIB_DIR}")
    MESSAGE("glew possible lib dir: ${GLEW_LIB_DIR2}")
ENDIF()

SET(INCS ${INCS} ${GLEW_INCLUDE_DIRS})
SET(LIBS ${LIBS} ${GLEW_LIBRARIES})

#Prompt user to specify freeglut installation root.
FIND_PACKAGE(FREEGLUT QUIET)
IF(FREEGLUT_FOUND)
    MESSAGE("freeglut is found!")
ELSE()
    MESSAGE("\nfreeglut cannot be automatically detected on your machine.")
    MESSAGE("Please install and manually set freeglut root: FREEGLUT_ROOT_DIR in the cmake.")
    SET(FREEGLUT_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/freeglut" CACHE PATH
        "Please specify freeglut installation directory")
    SET(FREEGLUT_INCLUDE_DIRS "${FREEGLUT_ROOT_DIR}/include")
    SET(FREEGLUT_LIB_DIR "${FREEGLUT_ROOT_DIR}/lib")
    SET(FREEGLUT_LIBRARIES "freeglut")
    IF(WIN32)
        SET(FREEGLUT_LIB_DIR2 "${FREEGLUT_LIB_DIR}/${PLATFORM_NAME}")
    ENDIF()
    MESSAGE("freeglut root dir FREEGLUT_ROOT_DIR is set to: ${FREEGLUT_ROOT_DIR}")
    MESSAGE("freeglut possible lib dir: ${FREEGLUT_LIB_DIR}")
    MESSAGE("freeglut possible lib dir: ${FREEGLUT_LIB_DIR2}")
    MESSAGE("freeglut bin dir: ${FREEGLUT_BIN_DIR}")
ENDIF()

SET(INCS ${INCS} ${FREEGLUT_INCLUDE_DIRS})
SET(LIBS ${LIBS} ${FREEGLUT_LIBRARIES})

#Prompt user to specify netcdf installation root.
FIND_PACKAGE(NETCDF QUIET)
IF(NETCDF_FOUND)
    MESSAGE("netcdf is found!")
ELSE()
    MESSAGE("\nnetcdf cannot be automatically detected on your machine.")
    MESSAGE("Please install and manually set netcdf root: NETCDF_ROOT_DIR in the cmake.")
    SET(NETCDF_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/netcdf" CACHE PATH
        "Please specify NETCDF installation directory")
    SET(NETCDF_INCLUDE_DIRS "${NETCDF_ROOT_DIR}/include")
    SET(NETCDF_LIB_DIR "${NETCDF_ROOT_DIR}/lib")
    SET(NETCDF_LIB_DIR2 "${NETCDF_LIB_DIR}/${PLATFORM_NAME}")
    SET(NETCDF_LIBRARIES "netcdf")
    MESSAGE("NETCDF root dir NETCDF_ROOT_DIR is set to: ${NETCDF_ROOT_DIR}")
    MESSAGE("NETCDF possible lib dir: ${NETCDF_LIB_DIR}")
    MESSAGE("NETCDF possible lib dir: ${NETCDF_LIB_DIR2}")
ENDIF()

SET(INCS ${INCS} ${NETCDF_INCLUDE_DIRS})
SET(LIBS ${LIBS} ${NETCDF_LIBRARIES})

FIND_PACKAGE(DAVINCI QUIET)
IF(DAVINCI_FOUND)
    MESSAGE("davinci is found!")
ELSE()
    MESSAGE("\ndavinci cannot be automatically detected on your machine.")
    MESSAGE("Please install and manually set davinci root: DAVINCI_ROOT_DIR in the cmake.")
    SET(DAVINCI_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/davinci" CACHE PATH
        "Please specify DAVINCI installation directory")
    SET(DAVINCI_INCLUDE_DIRS "${DAVINCI_ROOT_DIR}/include")
    SET(DAVINCI_LIB_DIR "${DAVINCI_ROOT_DIR}/lib")
    SET(DAVINCI_LIB_DIR2 "${DAVINCI_LIB_DIR}/${PLATFORM_NAME}")
    SET(DAVINCI_LIBRARIES "Davinci")
    MESSAGE("DAVINCI root dir: ${DAVINCI_ROOT_DIR}")
    MESSAGE("DAVINCI include dir: ${DAVINCI_INC_DIR}")
    MESSAGE("DAVINCI possible lib dir: ${DAVINCI_LIB_DIR}")
    MESSAGE("DAVINCI possible lib dir: ${DAVINCI_LIB_DIR2}")
ENDIF()

SET(INCS ${INCS} ${DAVINCI_INCLUDE_DIRS})
SET(LIBS ${LIBS} ${DAVINCI_LIBRARIES})

INCLUDE_DIRECTORIES(
${INCS}
)

LINK_DIRECTORIES(
${GLEW_LIB_DIR}
${GLEW_LIB_DIR2}
${FREEGLUT_LIB_DIR}
${FREEGLUT_LIB_DIR2}
${NETCDF_LIB_DIR}
${NETCDF_LIB_DIR2}
${DAVINCI_LIB_DIR}
${DAVINCI_LIB_DIR2}
)

ADD_EXECUTABLE(${PROJECT_NAME} ${HEADER} ${SOURCE} ${UTILITY_HEADER} ${UTILITY_SOURCE}
    ${MESH_HEADER} ${MESH_SOURCE} ${SHADER_SOURCE})

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${LIBS})

#Copy shader files to where executable is
ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_directory
   "${SRC_DIR}/shader" "${PROJECT_BINARY_DIR}/shader")

IF(WIN32)
    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_directory
       "${SRC_DIR}/shader" "${PROJECT_BINARY_DIR}/\$\(CONFIGURATION\)/shader")
ENDIF()

#Copy TF file to where executable is
ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_directory
   "${SRC_DIR}/../config" "${PROJECT_BINARY_DIR}/config")

IF(WIN32)
    ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
       COMMAND ${CMAKE_COMMAND} -E copy_directory
       "${SRC_DIR}/../config" "${PROJECT_BINARY_DIR}/\$\(CONFIGURATION\)/config")
ENDIF()

#Copy test data to where executable is
ADD_CUSTOM_COMMAND(TARGET ${PROJECT_NAME} POST_BUILD
   COMMAND ${CMAKE_COMMAND} -E copy_directory
   "${SRC_DIR}/../data" "${PROJECT_BINARY_DIR}/data")
