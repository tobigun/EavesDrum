// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { Alert, MenuItem, Select, Stack } from "@mui/material";
import { getPadByIndex, isPadPinConnectedToMux, useConfig } from "@config";

export function MuxMonitorInputCheck() {
  const monitoredPadIndex = useConfig(config => config.monitor.padIndex);
  const monitoredPadName = getPadByIndex(monitoredPadIndex)?.name ?? 'None';
  const isMux = isPadPinConnectedToMux(0, monitoredPadIndex);

  return (
    <Stack direction='row' paddingLeft={5}>
      <Select disabled={true} value={monitoredPadName} size='small'>
        <MenuItem value={monitoredPadName}>{monitoredPadName}</MenuItem>
      </Select>
      <Alert severity={isMux ? "success" : "error"}>
        { isMux ? "Input OK" : "Input is not a multiplexer input! Select another input." }
      </Alert>
    </Stack>
  );
}        
