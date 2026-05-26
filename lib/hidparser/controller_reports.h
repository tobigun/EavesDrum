#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    bool dpadUp : 1;
    bool dpadDown : 1;
    bool dpadLeft : 1;
    bool dpadRight : 1;

    union
    {
        struct
        {
            bool genericButton1 : 1;
            bool genericButton2 : 1;
            bool genericButton3 : 1;
            bool genericButton4 : 1;
            bool genericButton5 : 1;
            bool genericButton6 : 1;
            bool genericButton7 : 1;
            bool genericButton8 : 1;
            bool genericButton9 : 1;
            bool genericButton10 : 1;
            bool genericButton11 : 1;
            bool genericButton12 : 1;
            bool genericButton13 : 1;
            bool genericButton14 : 1;
            bool genericButton15 : 1;
            bool genericButton16 : 1;
        };
        uint16_t genericButtons;
    };

    uint16_t genericAxisX;
    uint16_t genericAxisY;
    uint16_t genericAxisZ;
    uint16_t genericAxisRx;
    uint16_t genericAxisRy;
    uint16_t genericAxisRz;
    uint16_t genericAxisSlider;

    bool hasDPad : 1;
    bool hasAxisX : 1;
    bool hasAxisY : 1;
    bool hasAxisZ : 1;
    bool hasAxisRx : 1;
    bool hasAxisRy : 1;
    bool hasAxisRz : 1;
    bool hasAxisSlider : 1;
} __attribute__((packed)) USB_Host_Data_t;
