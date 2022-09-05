set(APP_NAME Arducam_Opencv_Demo)

cmake_minimum_required(VERSION 2.8.12)

project(${APP_NAME})

add_definitions(-D__PC__)

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "-std=c++17 ${CMAKE_CXX_FLAGS}")
    message(STATUS "optional:-std=c++17")   
endif(CMAKE_COMPILER_IS_GNUCXX)

find_package( OpenCV REQUIRED )

add_executable( ${APP_NAME}
    src/Arducam.cpp
    src/ArduCam_Demo.cpp
    src/Utils.cpp
)

function(download_file url filename)
    message(STATUS "Download to ${filename} ...")
    file(DOWNLOAD ${url} ${filename})
endfunction(download_file)

function(extract_file filename extract_dir)
    message(STATUS "Extract to ${extract_dir} ...")
    set(temp_dir ${CMAKE_CURRENT_SOURCE_DIR}/tmp_for_extract.dir)
    if(EXISTS ${temp_dir})
        file(REMOVE_RECURSE ${temp_dir})
    endif()
    file(MAKE_DIRECTORY ${temp_dir})
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar -zxvf ${filename} WORKING_DIRECTORY ${temp_dir} RESULT_VARIABLE tar_result)
    
    file(GLOB contents "${temp_dir}/*")
    list(LENGTH contents n)
    if(NOT n EQUAL 1 OR NOT IS_DIRECTORY "${contents}")
        set(contents "${temp_dir}")
    endif()
    get_filename_component(contents ${contents} ABSOLUTE)
    file(INSTALL "${contents}/" DESTINATION ${extract_dir})
    
    file(REMOVE_RECURSE ${temp_dir})
endfunction(extract_file)

if (WIN32) 
    download_file("https://github.com/ArduCAM/arducam_ppa/releases/download/v1.0/libs_arducam.zip" "${CMAKE_CURRENT_SOURCE_DIR}/libs_arducam.zip")
    extract_file("${CMAKE_CURRENT_SOURCE_DIR}/libs_arducam.zip" "${CMAKE_CURRENT_SOURCE_DIR}/libs")
    set(ArduCam_INCLUDE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/libs/arducam/include")
    set(ArduCam_LIBS "${CMAKE_CURRENT_SOURCE_DIR}/libs/arducam/lib")
    include_directories(
        ./src
        ${ArduCam_INCLUDE_DIRS}
        ${OpenCV_INCLUDE_DIRS}
    )
    
    if (${CMAKE_GENERATOR_PLATFORM} STREQUAL "x64")
        target_link_directories(${APP_NAME} PUBLIC ${ArduCam_LIBS}/x64)
    else(${CMAKE_GENERATOR_PLATFORM} STREQUAL "Win32")
        target_link_directories(${APP_NAME} PUBLIC ${ArduCam_LIBS}/Win32)
    endif()

    target_link_libraries(
        ${APP_NAME} 
        ArduCamLib
        arducam_config_parser
        ${OpenCV_LIBS}
    )
endif(WIN32)

if (UNIX)
    include_directories(
        ./src
        ${OpenCV_INCLUDE_DIRS}
    )

    target_link_libraries(
        ${APP_NAME} 
        -lArduCamLib 
        -larducam_config_parser
        -lpthread
        ${OpenCV_LIBS}
    )
endif(UNIX)
