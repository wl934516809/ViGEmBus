@echo off
echo ================================================
echo 正在使用 CMake 静态编译 test-client-server 项目...
echo ================================================
echo.

REM 配置 Visual Studio 2019 环境
call "D:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

REM 进入项目目录
cd /d "%~dp0"

REM 清理之前的构建
if exist "build-static" (
    echo 清理旧的构建目录...
    rmdir /s /q build-static
    if exist "build-static" (
        echo [93m警告: 无法完全清理 build-static 目录！[0m
        echo 尝试手动删除 build-static 文件夹。
    )
)

REM 创建构建目录
mkdir build-static

REM 配置 CMake（Debug 版本，静态编译）
echo 正在配置 CMake（静态编译 - Debug 版本）...
cmake -B build-static -S . -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Debug

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [91mCMake 配置失败！[0m
    pause
    exit /b 1
)

REM 构建项目（Debug 版本）
echo.
echo 正在构建项目（静态编译 - Debug 版本）...
cmake --build build-static --config Debug

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [91m构建失败！[0m
    pause
    exit /b 1
)

echo.
echo ================================================
echo [92m静态编译成功！[0m
echo ================================================
echo.
echo 可执行文件位置:
echo   build-static\bin\Debug\tcp-server.exe
echo   build-static\bin\Debug\tcp-client.exe
echo.

REM 检查是否生成了可执行文件
if exist "build-static\bin\Debug\tcp-server.exe" (
    echo tcp-server.exe 已成功静态编译！
) else (
    echo [93m警告: tcp-server.exe 未找到！[0m
)

if exist "build-static\bin\Debug\tcp-client.exe" (
    echo tcp-client.exe 已成功静态编译！
) else (
    echo [93m警告: tcp-client.exe 未找到！[0m
)

echo.
echo 现在尝试构建 Release 静态版本...
echo.

REM 配置 CMake（Release 版本，静态编译）
echo 正在配置 CMake（静态编译 - Release 版本）...
cmake -B build-static -S . -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [91mCMake Release 配置失败！[0m
    pause
    exit /b 1
)

REM 构建项目（Release 版本）
echo.
echo 正在构建项目（静态编译 - Release 版本）...
cmake --build build-static --config Release

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [91mRelease 构建失败！[0m
    pause
    exit /b 1
)

echo.
echo ================================================
echo [92mRelease 静态编译成功！[0m
echo ================================================
echo.
echo 可执行文件位置:
echo   build-static\bin\Release\tcp-server.exe
echo   build-static\bin\Release\tcp-client.exe
echo.

REM 检查是否生成了可执行文件
if exist "build-static\bin\Release\tcp-server.exe" (
    echo tcp-server.exe (Release) 已成功静态编译！
) else (
    echo [93m警告: tcp-server.exe (Release) 未找到！[0m
)

if exist "build-static\bin\Release\tcp-client.exe" (
    echo tcp-client.exe (Release) 已成功静态编译！
) else (
    echo [93m警告: tcp-client.exe (Release) 未找到！[0m
)

echo.
pause
