set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")

set(SUB_SRC_DIRS
    base
    acceptor
    channel
    epoller
    eventloop
    logger
    timer
    tcp
    buffer
    http
)

set(SOURCES "")
set(INCLUDE_DIRS "")
foreach(D ${SUB_SRC_DIRS})
    set(DIR_PATH "${SRC_DIR}/${D}")
    message(STATUS ${DIR_PATH})
    file(GLOB D_SOURCES "${DIR_PATH}/*.cpp")
    list(APPEND SOURCES ${D_SOURCES})
    list(APPEND INCLUDE_DIRS ${DIR_PATH})
endforeach()


add_library(MyObjects ${SOURCES})
target_include_directories(MyObjects PUBLIC ${INCLUDE_DIRS})

add_executable(http_server main.cpp)
target_link_libraries(http_server PRIVATE MyObjects)
# set_target_properties(http_server PROPERTIES
#     RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}   # 单独输出到bin.
# )

# 为所有头文件在src/include/目录下创建符号链接. 
set(LINK_DIR "${SRC_DIR}/include")
file(MAKE_DIRECTORY ${LINK_DIR})
set(SYMLINKS "")
foreach(INC_DIR ${INCLUDE_DIRS})
    file(GLOB HEADER_FILES "${INC_DIR}/*.h")
    foreach(HEADER ${HEADER_FILES})
        get_filename_component(HEADER_NAME ${HEADER} NAME)  # basename文件名
        set(SYMLINK_PATH "${LINK_DIR}/${HEADER_NAME}")

        # 使用 cmake -E create_symlink 生成符号链接（跨平台）
        add_custom_command(
            OUTPUT ${SYMLINK_PATH}
            COMMAND ${CMAKE_COMMAND} -E remove -f ${SYMLINK_PATH}  # 移除旧链接
            COMMAND ${CMAKE_COMMAND} -E create_symlink ${HEADER} ${SYMLINK_PATH}
            COMMENT "Creating symlink: ${SYMLINK_PATH} -> ${HEADER}"
            VERBATIM
        )
        list(APPEND SYMLINKS ${SYMLINK_PATH})
        # file(CREATE_LINK ${HEADER} ${SYMLINK_PATH} SYMBOLIC)
        # message(STATUS "Created symlink: ${SYMLINK_PATH} -> ${HEADER}")
    endforeach()
endforeach()

add_custom_target(create_symlinks ALL DEPENDS ${SYMLINKS})