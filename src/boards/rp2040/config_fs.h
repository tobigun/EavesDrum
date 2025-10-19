#pragma once

#include <LittleFS.h>

// Config FS is placed in flash right after the EEPROM area (and the read-only FS)
extern uint8_t _EEPROM_start[];
static constexpr size_t EEPROM_SIZE = 4096;
static uint8_t* CONFIG_FS_start = &_EEPROM_start[EEPROM_SIZE];

FS ConfigFS = FS(FSImplPtr(new littlefs_impl::LittleFSImpl(CONFIG_FS_start, CONFIG_FS_SIZE, 256, 4096, 16)));
