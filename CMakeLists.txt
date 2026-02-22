cmake_minimum_required(VERSION 3.10)

project(taskcpp CXX)

include(CheckIncludeFileCXX)

check_include_file_cxx(any HAS_ANY)
check_include_file_cxx(string_view HAS_STRING_VIEW)
check_include_file_cxx(coroutine HAS_COROUTINE)
if (NOT "${CMAKE_CXX_STANDARD}" STREQUAL "")
    # Do nothing
elseif (HAS_ANY AND HAS_STRING_VIEW AND HAS_COROUTINE)
    set(CMAKE_CXX_STANDARD 20)
elseif (HAS_ANY AND HAS_STRING_VIEW)
    set(CMAKE_CXX_STANDARD 17)
else ()
    set(CMAKE_CXX_STANDARD 14)
endif ()
#set(CMAKE_CXX_STANDARD 23)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_executable(${PROJECT_NAME} main.cc)

# ##############################################################################
# If you include the drogon source code locally in your project, use this method
# to add drogon 
# add_subdirectory(drogon) 
# target_link_libraries(${PROJECT_NAME} PRIVATE drogon)
#
# and comment out the following lines
find_package(Drogon CONFIG REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Drogon::Drogon)

# ##############################################################################

# add boost
# find_package(Boost CONFIG REQUIRED)
# target_link_libraries(${PROJECT_NAME} PRIVATE Boost::boost)

if (CMAKE_CXX_STANDARD LESS 17)
    message(FATAL_ERROR "c++17 or higher is required")
elseif (CMAKE_CXX_STANDARD LESS 20)
    message(STATUS "use c++17")
else ()
    message(STATUS "use c++20")
endif ()

aux_source_directory(controllers CTL_SRC)
# file(GLOB_RECURSE CTL_SRC "controllers/*.cc")
aux_source_directory(filters FILTER_SRC)
aux_source_directory(plugins PLUGIN_SRC)
aux_source_directory(models MODEL_SRC)
aux_source_directory(utils UTILS_SRC)
file(GLOB_RECURSE PLUGINS_SRC "plugins/*.cc")

drogon_create_views(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/views
        ${CMAKE_CURRENT_BINARY_DIR})
# use the following line to create views with namespaces.
# drogon_create_views(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/views
#                     ${CMAKE_CURRENT_BINARY_DIR} TRUE)
# use the following line to create views with namespace CHANGE_ME prefixed
# and path namespaces.
# drogon_create_views(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/views
#                     ${CMAKE_CURRENT_BINARY_DIR} TRUE CHANGE_ME)

# bcrypt
find_library(BCRYPT_LIBRARY
        NAMES bcrypt libbcrypt
        HINTS
        # /usr/local/opt/libbcrypt/lib    # Homebrew (Intel)
        # /opt/homebrew/opt/libbcrypt/lib # Homebrew (Apple Silicon)
        # /usr/local/lib                  # 默认安装路径
        /usr/local/include/bcrypt
)
find_path(BCRYPT_INCLUDE_DIR
        NAMES bcrypt.h
        HINTS
        /usr/local/include/bcrypt
)


set(BOOST_ASIO_DIR "/opt/homebrew/Cellar/asio/1.36.0/include")


if (NOT BCRYPT_LIBRARY)
    message(FATAL_ERROR "未找到 libbcrypt 库，请先安装: brew install libbcrypt")
endif ()
if (NOT BCRYPT_INCLUDE_DIR)
    message(FATAL_ERROR "未找到 bcrypt.h 头文件")
endif ()
message(STATUS "找到 libbcrypt: ${BCRYPT_LIBRARY}")
message(STATUS "找到 bcrypt.h: ${BCRYPT_INCLUDE_DIR}")


target_include_directories(${PROJECT_NAME}
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/models
        ${CMAKE_CURRENT_SOURCE_DIR}/jwt
        ${BCRYPT_INCLUDE_DIR}
        ${BOOST_ASIO_DIR})

target_link_libraries(taskcpp PRIVATE ${BCRYPT_LIBRARY})

list(APPEND PRJ_COMPILE_FEATURES cxx_std_20)

target_compile_features(${PROJECT_NAME}
        PRIVATE
        ${PRJ_COMPILE_FEATURES}
)

target_sources(${PROJECT_NAME}
        PRIVATE
        ${SRC_DIR}
        ${CTL_SRC}
        ${FILTER_SRC}
        ${PLUGIN_SRC}
        ${MODEL_SRC}
        ${UTILS_SRC}
        ${PLUGINS_SRC}
)

# ##############################################################################
# uncomment the following line for dynamically loading views 
# set_property(TARGET ${PROJECT_NAME} PROPERTY ENABLE_EXPORTS ON)

# ##############################################################################

add_subdirectory(test)

file(COPY config.yaml DESTINATION ${CMAKE_BINARY_DIR})