// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { Switch } from "@mui/material";
import { updateConfig, useConfig } from "../config/config";
import { SettingEntryContainer } from "./setting-entry";
import { SettingsValueAccessor } from "./settings-pad";
import { connection } from "../connection/connection";

export function SettingEnableEntry({ label, padRole, valueAccessor }: {
    label: string,
    padRole: string,
    valueAccessor: SettingsValueAccessor
  }) {
  const connected = true; // TODO useContext(ConnectionStateContext);
  const checkedValue = useConfig(config => valueAccessor.getValue(config.settings[padRole]));
  
  function onChange(_event: unknown, newEnabled: boolean) {
    updateConfig(config => valueAccessor.setValue(config.settings[padRole], newEnabled));
    connection.sendSetPadSettingsCommand(padRole, valueAccessor.setValue({}, newEnabled));
  }
  
  return (
    <SettingEntryContainer name={label}>
      <Switch checked={checkedValue} onChange={onChange} disabled={!connected} />
    </SettingEntryContainer>
  );
}
  