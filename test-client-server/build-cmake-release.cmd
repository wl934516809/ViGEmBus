@echo off
echo ================================================
echo 正在使用 CMake 构建 test-client-server 项目...
echo ================================================
echo.

REM 配置 Visual Studio 2019 环境
call "D:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat"

REM 进入项目目录
cd /d "%~dp0"

REM 创建构建目录
if not exist "build" mkdir build

REM 配置 CMake（Release 版本）
echo 正在配置 CMake（Release 版本）...
cmake -B build -S . -G "Visual Studio 16 2019" -A x64 -DCMAKE_BUILD_TYPE=Release

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [91mCMake 配置失败！[0m
    pause
    exit /b 1
)

REM 构建项目（Release 版本）
echo.
echo 正在构建项目（Release 版本）...
cmake --build build --config Release

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [91m构建失败！[0m
    pause
    exit /b 1
)

REM 检查输出文件
echo.
echo ================================================
echo [92m构建成功！[0m
echo ================================================
echo.
echo 可执行文件位置:
echo   build\bin\Release\tcp-server.exe
echo   build\bin\Release\tcp-client.exe
echo.

REM 检查是否生成了可执行文件
if exist "build\bin\Release\tcp-server.exe" (
    echo tcp-server.exe 已成功生成！
) else (
    echo [93m警告: tcp-server.exe 未找到！[0m
)

if exist "build\bin\Release\tcp-client.exe" (
    echo tcp-client.exe 已成功生成！
) else (
    echo [93m警告: tcp-client.exe 未找到！[0m
)

echo.
pause
