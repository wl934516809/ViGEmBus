
#ifndef COMMON_H
#define COMMON_H

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>
#include <stdint.h>
#include <string.h>
#include <iostream>

// 日志宏定义 - 根据编译模式控制输出
#if defined(_DEBUG) || defined(DEBUG)
    #define LOG_INFO(...)    do { std::cout << __VA_ARGS__ << std::endl; } while(false)
    #define LOG_WARNING(...) do { std::cerr << "WARNING: " << __VA_ARGS__ << std::endl; } while(false)
    #define LOG_ERROR(...)   do { std::cerr << "ERROR: " << __VA_ARGS__ << std::endl; } while(false)
    #define LOG_DEBUG(...)   do { std::cout << "DEBUG: " << __VA_ARGS__ << std::endl; } while(false)
    #define LOG_IO(...)      do { std::cout << __VA_ARGS__ << std::endl; } while(false)  // IO 相关日志，Debug 版本输出，Release 版本不输出
#else
    #define LOG_INFO(...)    do { } while(false)
    #define LOG_WARNING(...) do { } while(false)
    #define LOG_ERROR(...)   do { std::cerr << "ERROR: " << __VA_ARGS__ << std::endl; } while(false)
    #define LOG_DEBUG(...)   do { } while(false)
    #define LOG_IO(...)      do { } while(false)  // IO 相关日志，Release 版本禁止输出
#endif

#define DEFAULT_PORT "12345"
#define DEFAULT_SERVER_ADDR "127.0.0.1"
#define MAX_BUFFER_SIZE 2048

typedef uint32_t MessageType;
#define MSG_HEARTBEAT 0x00000001
#define MSG_XBOX_STATE 0x00000002
#define MSG_XBOX_VIBRATION 0x00000003
#define MSG_DISCONNECT 0xFFFFFFFF

typedef struct _XboxControllerState
{
    uint16_t wButtons;
    uint8_t bLeftTrigger;
    uint8_t bRightTrigger;
    int16_t sThumbLX;
    int16_t sThumbLY;
    int16_t sThumbRX;
    int16_t sThumbRY;
} XboxControllerState;

typedef struct _XboxVibrationState
{
    uint16_t wLeftMotorSpeed;
    uint16_t wRightMotorSpeed;
} XboxVibrationState;

typedef struct _MessageHeader
{
    uint32_t magic;
    MessageType type;
    uint32_t length;
} MessageHeader;

typedef struct _NetworkMessage
{
    MessageHeader header;
    union
    {
        XboxControllerState controller;
        XboxVibrationState vibration;
    } data;
} NetworkMessage;

#define MESSAGE_MAGIC 0x47454D58

static inline int IsValidMessage(const MessageHeader *header)
{
    return header && header->magic == MESSAGE_MAGIC;
}

static inline void FillMessageHeader(MessageHeader *header, MessageType type, uint32_t length)
{
    if (header)
    {
        header->magic = MESSAGE_MAGIC;
        header->type = type;
        header->length = length;
    }
}

#endif
