// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { useCallback, useContext } from "react";

import { MenuItem, Select, SelectChangeEvent } from "@mui/material";

import { CurveType, updateConfig, useConfig } from "../config/config";
import { connection, ConnectionStateContext } from "../connection/connection";
import { SettingEntryContainer } from "./setting-entry";
import { SettingsValueAccessor } from "./settings-pad";

export function SettingCurveTypeEntry({ label, padRole, valueAccessor }: {
    label: string,
    padRole: string,
    valueAccessor: SettingsValueAccessor
}) {
  const connected = useContext(ConnectionStateContext);
  
  const value = useConfig(config => valueAccessor.getValue(config.settings[padRole]));
  
  const onCurveTypeChanged = useCallback((event: SelectChangeEvent) => {
    const value = event.target.value as CurveType;
    updateConfig(config => valueAccessor.setValue(config.settings[padRole], value));
    connection.sendSetPadSettingsCommand(padRole, valueAccessor.setValue({}, value));
  }, [valueAccessor, padRole]);
  
  return (
    <SettingEntryContainer name={label}>
      <Select disabled={!connected} value={value} size='small'
        onChange={onCurveTypeChanged}
      >
        <MenuItem value={CurveType.Linear}>Linear</MenuItem>
        <MenuItem value={CurveType.Exp1}>Exponential 1</MenuItem>
        <MenuItem value={CurveType.Exp2}>Exponential 2</MenuItem>
        <MenuItem value={CurveType.Log1}>Logarithmic 1</MenuItem>
        <MenuItem value={CurveType.Log2}>Logarithmic 2</MenuItem>
      </Select>
    </SettingEntryContainer>
  );
}
  