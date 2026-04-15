# ViGEmBus 测试程序

这是一个 TCP 通信的测试程序，包含：

- **tcp-client**: 捕获真实 Xbox 手柄事件
- **tcp-server**: 接收消息并转发给 ViGEmBus 驱动

## 架构图

```
┌─────────────────┐         TCP         ┌─────────────────┐
│ 真实 Xbox 手柄  │─────────────────────>│ 虚拟 Xbox 手柄  │
│   (tcp-client)   │ 127.0.0.1:12345  │  (ViGEmBus)     │
└─────────────────┘                     └─────────────────┘
```

## 使用步骤

1. 首先安装并运行 ViGEmBus 驱动
2. 运行 tcp-server (服务端)
3. 运行 tcp-client (客户端)
4. 连接真实的 Xbox 手柄
5. 游戏会看到虚拟手柄的输入！

## 项目结构

```
test-client-server/
├── TestClientServer.sln          # 解决方案文件
├── tcp-client/
│   ├── tcp-client.vcxproj        # 客户端项目
│   ├── main.cpp                   # 客户端主程序
│   ├── xbox_controller.cpp/.h     # Xbox 手柄捕获
│   ├── tcp_client.cpp/.h         # TCP 客户端
│   └── common.h                   # 共享定义
└── tcp-server/
    ├── tcp-server.vcxproj        # 服务端项目
    ├── main.cpp                   # 服务端主程序
    ├── vigem_driver.cpp/.h        # ViGEm 驱动接口
    ├── tcp_server.cpp/.h         # TCP 服务端
    └── common.h                   # 共享定义 (同客户端)
```

## 编译步骤

  一键静态编译: 双击 build-static.cmd 即可自动编译 Debug 和 Release 版本

  手动编译:
  cd build-static
  cmake --build . --config Debug    # 编译 Debug 版
  cmake --build . --config Release  # 编译 Release 版

## 注意事项

- 需要先安装 ViGEmBus 驱动
- 客户端需要真实的 Xbox 手柄连接
- 默认端口 12345，可在 common.h 中修改
