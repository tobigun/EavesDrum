// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { Switch } from "@mui/material";
import { updateConfig, useConfig } from "@config";
import { SettingEntryContainer } from "./setting-entry";
import { SettingsValueAccessor } from "./settings-pad";
import { connection } from "@/connection/connection";
import { useContext } from "react";
import { ConnectionStateContext } from "@/connection/connection-state";

export function SettingEnableEntry({ label, padIndex, valueAccessor }: {
    label: string,
    padIndex: number,
    valueAccessor: SettingsValueAccessor
  }) {
  const connected = useContext(ConnectionStateContext);
  const checkedValue = useConfig(config => valueAccessor.getValue(config.pads[padIndex].settings));
  
  function onChange(_event: unknown, newEnabled: boolean) {
    updateConfig(config => valueAccessor.setValue(config.pads[padIndex].settings, newEnabled));
    connection.sendSetPadSettingsCommand(padIndex, valueAccessor.setValue({}, newEnabled));
  }
  
  return (
    <SettingEntryContainer name={label}>
      <Switch checked={checkedValue} onChange={onChange} disabled={!connected} />
    </SettingEntryContainer>
  );
}
  