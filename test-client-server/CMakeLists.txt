cmake_minimum_required(VERSION 3.15)

# 项目名称
project(TcpClientServer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置编译选项
if(MSVC)
    # 启用多处理器编译
    add_compile_options(/MP)
    # 警告级别
    add_compile_options(/W3)
    # UTF-8编码
    add_compile_options(/utf-8)
    # 取消特定警告
    add_compile_options(/wd4996)  # 弃用API警告
    # 静态编译选项 - 静态链接 C++ 运行时库
    add_compile_options(/MT$<$<CONFIG:Debug>:d>)
    # 链接器选项
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} /INCREMENTAL:NO /NODEFAULTLIB:msvcrtd.lib /NODEFAULTLIB:msvcp140d.lib")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /INCREMENTAL:NO /NODEFAULTLIB:msvcrt.lib /NODEFAULTLIB:msvcp140.lib")
else()
    # GCC/Clang 警告级别
    add_compile_options(-Wall -Wextra)
    # 静态编译选项
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/include
)

# 链接库目录
link_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/bin/debug/x64
    ${CMAKE_CURRENT_SOURCE_DIR}/../sdk/lib/release/x64
)

# TCP Server
add_executable(tcp-server
    tcp-server/main.cpp
    tcp-server/tcp_server.cpp
    tcp-server/vigem_driver.cpp
    tcp-server/common.h
    tcp-server/tcp_server.h
    tcp-server/vigem_driver.h
)

# TCP Client
add_executable(tcp-client
    tcp-client/main.cpp
    tcp-client/tcp_client.cpp
    tcp-client/xbox_controller.cpp
    tcp-client/common.h
    tcp-client/tcp_client.h
    tcp-client/xbox_controller.h
)

# 链接库
if(MSVC)
    # Windows系统库
    target_link_libraries(tcp-server PRIVATE
        Ws2_32.lib
        setupapi.lib
        ViGEmClient.lib
    )

    target_link_libraries(tcp-client PRIVATE
        Ws2_32.lib
        setupapi.lib
        ViGEmClient.lib
        XInput.lib
    )

    # 设置输出目录
    set_target_properties(tcp-server PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
        LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
    )

    set_target_properties(tcp-client PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
        RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
        LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
        LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
        ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug
        ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release
    )
else()
    # 其他平台可能需要不同的库链接
    target_link_libraries(tcp-server PRIVATE
        pthread
        Ws2_32
        setupapi
        ViGEmClient
    )

    target_link_libraries(tcp-client PRIVATE
        pthread
        Ws2_32
        setupapi
        ViGEmClient
    )
endif()

# 调试信息（可选）
if(CMAKE_BUILD_TYPE MATCHES Debug)
    if(MSVC)
        target_compile_options(tcp-server PRIVATE /Zi)
        target_compile_options(tcp-client PRIVATE /Zi)
        target_link_options(tcp-server PRIVATE /DEBUG)
        target_link_options(tcp-client PRIVATE /DEBUG)
    else()
        target_compile_options(tcp-server PRIVATE -g)
        target_compile_options(tcp-client PRIVATE -g)
    endif()
endif()

# 拷贝 ViGEmClient.dll 到输出目录
function(copy_vigem_dll target config)
    if(MSVC)
        # 根据配置选择对应的 DLL 路径
        if("${config}" STREQUAL "Debug")
            set(VIGEM_DLL_PATH "${CMAKE_SOURCE_DIR}/../sdk/bin/debug/x64/ViGEmClient.dll")
        else()
            # Release 版本也使用 Debug 的 DLL（因为没有找到 Release 的 DLL）
            set(VIGEM_DLL_PATH "${CMAKE_SOURCE_DIR}/../sdk/bin/debug/x64/ViGEmClient.dll")
        endif()

        # 添加自定义命令来拷贝 DLL
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${VIGEM_DLL_PATH}"
                $<TARGET_FILE_DIR:${target}>
            COMMENT "Copying ViGEmClient.dll to $<TARGET_FILE_DIR:${target}>"
        )
    endif()
endfunction()

# 为两个目标添加 DLL 拷贝命令
copy_vigem_dll(tcp-server Debug)
copy_vigem_dll(tcp-client Debug)
copy_vigem_dll(tcp-server Release)
copy_vigem_dll(tcp-client Release)

message(STATUS "CMake configuration complete.")
message(STATUS "Build commands:")
message(STATUS "  Debug:   cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug -A x64")
message(STATUS "           cmake --build build --config Debug")
message(STATUS "  Release: cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -A x64")
message(STATUS "           cmake --build build --config Release")
message(STATUS "")
message(STATUS "Executables and ViGEmClient.dll will be in: ${CMAKE_BINARY_DIR}/bin/<Configuration>/")
