// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { ChangeEvent, useCallback, useContext } from "react";

import { Box, MenuItem, Select, SelectChangeEvent } from "@mui/material";
import RecordIcon from '@mui/icons-material/FiberManualRecord';

import { getHeaderBackground } from "@/common";
import { Card, PanelIconToggleButton } from "@/components/card";
import { GroupChip } from "@/components/group-chip";
import { Switch } from "@/components/switch";
import { ConnectorId, getPadIndexByName, PadType, updateConfig, useConfig } from "@config";
import { connection, DrumCommand } from "@/connection/connection";
import { recordButtonColor } from "./monitor/monitor-card";
import { SettingsElements } from "./settings-pad";
import { SettingEntryContainer } from "./setting-entry";
import { PadTypeSelector } from "./pad-type-selector";
import { useShallow } from "zustand/shallow";
import { ConfigFilter } from "@/components/component-enums";
import { ConnectionStateContext } from "@/connection/connection-state";
import { RoleInfo } from "@/components/role-info";

export function SettingsCardGroup({ padIndex }: {
  padIndex: number,
}) {
  const padType = useConfig(config => config.pads[padIndex].settings.padType);
  const pedalName = useConfig(config => config.pads[padIndex].pedal);
  const pedalIndex = pedalName ? getPadIndexByName(pedalName) : undefined;

  if (padType === PadType.Pedal) {
    return null;
  }
  
  return (
    <Box>
      <SettingsCard padIndex={padIndex} padType={padType} />
      {
        pedalIndex && <SettingsCard padIndex={pedalIndex} padType={PadType.Pedal} />
      }
    </Box>
  );
}

function SettingsCard({ padIndex, padType }: {
  padIndex: number,
  padType: PadType
}) {
  const padName = useConfig(config => config.pads[padIndex].name);
  const group = useConfig(config => config.pads[padIndex].group);
  const role = useConfig(config => config.pads[padIndex].role);
  const headerBackground = getHeaderBackground(padType);
  
  function handleRename(name: string) {
    connection.sendSetPadConfigCommand(padIndex, {name: name});
  }

  return (
    <Card name={padName} defaultExpanded={false}
      headerBackground={headerBackground}
      dropProps={{filter: ConfigFilter.Settings, padIndex: padIndex}}
      titleDecorators={<GroupChip group={group} />}
      onRename={handleRename}
      edgeDecorators={
        <>
          <SettingEnabledSwitch key="enabled" padIndex={padIndex} />
          <MonitorToggleButton key="monitor" padIndex={padIndex} />
        </>
      }>
      <PadTypeSelector padIndex={padIndex}/>
      <RoleInfo padRole={role} padIndex={padIndex} />
      <ConnectorInfo padIndex={padIndex} />
      <SettingsElements padIndex={padIndex} padType={padType} />
    </Card>
  );
}

function ConnectorInfo({ padIndex }: { padIndex: number }) {
  const connectorId = useConfig(config => config.pads[padIndex].connector);
  const usedConnectors = useConfig(useShallow(config => new Set(config.pads.map(pad => pad.connector))));
  useConfig(useShallow(config => connectorId ? config.connectors[connectorId].pins.length : 0)); // only used to trigger update

  const handleSelectChange = (event: SelectChangeEvent) => {
    const newConnectorId = event.target.value;
    if (newConnectorId !== connectorId) {
      const value = (newConnectorId !== "-") ? newConnectorId : (null as unknown as ConnectorId);
      connection.sendSetPadConfigCommand(padIndex, {connector: value});
    }
  };

  return <SettingEntryContainer name='Connector'>
    <Select value={connectorId ?? "-"} size='small' onChange={handleSelectChange}>
      <MenuItem value={"-"}>&mdash;</MenuItem>
      {
        Object.keys(useConfig.getState().connectors).map(id =>
          <MenuItem key={id} value={id} disabled={id != connectorId && usedConnectors.has(id)}>
            {id} (Pins: {useConfig.getState().connectors[id].pins.length})
          </MenuItem>)
      }
    </Select>
  </SettingEntryContainer>;
}

function SettingEnabledSwitch({ padIndex }: {
  padIndex: number
}) {
  const padEnabled = useConfig(config => config.pads[padIndex].enabled);
  const connected = useContext(ConnectionStateContext);
  
  const onChange = useCallback((event: ChangeEvent<HTMLInputElement>) => {
    const enabled = event.target.checked;
    updateConfig(config => config.pads[padIndex].enabled = enabled);
    connection.sendSetPadConfigCommand(padIndex, {enabled: enabled});
  }, [padIndex]);
  
  return (
    <Switch disabled={!connected}
      checked={padEnabled}
      title={padEnabled ? 'Enabled' : 'Disabled'}
      onChange={onChange} />
  );
}
  
function MonitorToggleButton({ padIndex }: {
  padIndex: number,
}) {
  const connected = useContext(ConnectionStateContext);
  const monitoredPadIndex = useConfig(config => config.monitor?.padIndex);
  
  const isMonitored = (padIndex === monitoredPadIndex);
  
  const onChangePadMonitoring = useCallback(() => {
    const selectAsMonitor = !isMonitored;
    const newMonitoredPadIndex = selectAsMonitor ? padIndex : undefined;
    updateConfig(config => config.monitor = { ...config.monitor, padIndex: newMonitoredPadIndex });
    connection.sendCommand(DrumCommand.setMonitor, { padIndex: newMonitoredPadIndex ?? null });
  }, [padIndex, isMonitored]);
  
  return (
    <PanelIconToggleButton title={isMonitored ? 'Stop monitoring' : 'Start monitoring'} value="check" disabled={!connected}
      selected={isMonitored} selectedColor={recordButtonColor}
      onChange={onChangePadMonitoring}
    >
      <RecordIcon />
    </PanelIconToggleButton>
  );
}
  