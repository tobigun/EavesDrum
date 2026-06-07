// Copyright (c) 2026 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { useCallback, useContext } from "react";

import { MenuItem, Select, SelectChangeEvent } from "@mui/material";

import { DecayType, updateConfig, useConfig } from "@config";
import { connection } from "@/connection/connection";
import { SettingEntryContainer } from "./setting-entry";
import { SettingsValueAccessor } from "./settings-pad";
import { ConnectionStateContext } from "@/connection/connection-state";

export function SettingDecayTypeEntry({ label, padIndex, valueAccessor }: {
    label: string,
    padIndex: number,
    valueAccessor: SettingsValueAccessor
}) {
  const connected = useContext(ConnectionStateContext);
  
  const value = useConfig(config => valueAccessor.getValue(config.pads[padIndex].settings));
  
  const onDecayTypeChanged = useCallback((event: SelectChangeEvent) => {
    const value = event.target.value as DecayType;
    updateConfig(config => valueAccessor.setValue(config.pads[padIndex].settings, value));
    connection.sendSetPadSettingsCommand(padIndex, valueAccessor.setValue({}, value));
  }, [valueAccessor, padIndex]);
  
  return (
    <SettingEntryContainer name={label}>
      <Select disabled={!connected} value={value} size='small'
        onChange={onDecayTypeChanged}
      >
        <MenuItem value={DecayType.Linear}>Linear</MenuItem>
      </Select>
    </SettingEntryContainer>
  );
}
  