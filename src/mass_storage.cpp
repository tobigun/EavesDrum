// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#if ENABLE_MASS_STORAGE

#include <SingleFileDrive.h>

class MassStorage() {
public:
  void enable()
  {
    // singleFileDrive.onPlug(onPlugged);
    // singleFileDrive.onUnplug(onUnplugged);
    // singleFileDrive.onDelete(onDeleteDB);
    singleFileDrive.begin(CONFIG_FILE, CONFIG_FILE);
  }
  
  void disable()
  {
    singleFileDrive.end();
  }  
};

#endif