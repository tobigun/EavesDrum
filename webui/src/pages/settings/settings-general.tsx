// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { useContext } from "react";
import { MAX_GATE_TIME_MS, updateConfig, useConfig } from "@config";
import { createFractionConverter } from "./converter";
import { connection, ConnectionStateContext, DrumCommand } from "@connection";
import { Box, Stack } from "@mui/material";
import { NumberInput } from "@components/number-input";
import { Card, CardSize } from "@components/card";

export function GeneralCard() {
  const spacing = 1;
  return (
    <Box sx={{ margin: spacing / 2 }}>
      <Card name='General' size={CardSize.large}
        headerBackground={'linear-gradient(rgb(91, 73, 73), 60%, rgb(68, 72, 87));'} defaultExpanded={false}
      >
        <GateTime />
      </Card>
    </Box>
  );
}

export function GateTime() {
  const gateTimeMs = useConfig(config => config.general.gateTimeMs);
  const connected = useContext(ConnectionStateContext);

  const converter = createFractionConverter(1000);

  const handleValueChange = (value: number) => {    
    if (Number.isFinite(value)) {
      const newValue = converter.toConfig(value);
      const newGateTimeMs = Math.max(0, Math.min(newValue, MAX_GATE_TIME_MS));
      if (newGateTimeMs !== gateTimeMs) {        
        updateConfig(config => { config.general.gateTimeMs = newGateTimeMs; });
        connection.sendCommand(DrumCommand.setGeneral, {
          gateTimeMs: newGateTimeMs
        });
      }
    }
  };

  return (
    <Stack direction={'row'} spacing={1}>
      <Box>Gate Time (s):</Box>
      <NumberInput disabled={!connected} value={converter.fromConfig(gateTimeMs)}
        onValueChange={(value) => value !== null && handleValueChange(value)}
        size='small'
        step={0.1}
        width='5em'
        min={0} max={converter.fromConfig(MAX_GATE_TIME_MS)} />
    </Stack>
  );
}
