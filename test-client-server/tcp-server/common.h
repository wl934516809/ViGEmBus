
#ifndef COMMON_H
#define COMMON_H

#include <Windows.h>
#include <stdint.h>
#include <string.h>

#define DEFAULT_PORT "12345"
#define DEFAULT_SERVER_ADDR "10.86.51.110"
#define MAX_BUFFER_SIZE 2048

typedef uint32_t MessageType;
#define MSG_HEARTBEAT 0x00000001
#define MSG_XBOX_STATE 0x00000002
#define MSG_XBOX_VIBRATION 0x00000003
#define MSG_DISCONNECT 0xFFFFFFFF

typedef struct _XboxControllerState
{
    uint16_t wButtons;
    uint8_t  bLeftTrigger;
    uint8_t  bRightTrigger;
    int16_t  sThumbLX;
    int16_t  sThumbLY;
    int16_t  sThumbRX;
    int16_t  sThumbRY;
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

static inline int IsValidMessage(const MessageHeader* header)
{
    return header && header->magic == MESSAGE_MAGIC;
}

static inline void FillMessageHeader(MessageHeader* header, MessageType type, uint32_t length)
{
    if (header)
    {
        header->magic = MESSAGE_MAGIC;
        header->type = type;
        header->length = length;
    }
}

#endif
