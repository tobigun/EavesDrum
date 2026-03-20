// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { useContext, useState, useEffect } from "react";
import { MidiOutputMode, updateConfig, useConfig } from "@config";
import { connection, DrumCommand } from "@/connection/connection";
import { Box, Stack, Select, MenuItem, Button, darken, useTheme, List, ListItem, ListItemText, ListItemButton, ListItemIcon, IconButton } from "@mui/material";
import StarIcon from '@mui/icons-material/Star';
import DeleteIcon from '@mui/icons-material/Delete';
import { ConnectionStateContext } from "@/connection/connection-state";
import { GeneralSetting } from "./settings-general";

enum BleConnectionStatus {
  Disconnected = "disconnected",
  Connecting = "connecting",
  Connected = "connected",
}

export function MidiOutputModeSelect() {
  const midiOutputMode = useConfig(config => config.general?.midiOutputMode);
  const supportedMidiOutputModes = useConfig(config => config._info?.midiOutputModes) ?? [];
  const blePairing = useConfig(config => config.general?.blePairing);
  const connected = useContext(ConnectionStateContext);
  const theme = useTheme();
  const [scannedDevices, setScannedDevices] = useState<{name: string; address: string}[]>([]);
  const [isScanning, setIsScanning] = useState(false);
  const [selectedBleDevice, setSelectedBleDevice] = useState<string>("");
  const [bleConnectionStatus, setBleConnectionStatus] = useState<BleConnectionStatus>(BleConnectionStatus.Disconnected);

  // listen for BLE device list messages coming from the websocket
  useEffect(() => {
    const handleBleDevices = (devices: any) => {
      if (Array.isArray(devices)) {
        setScannedDevices(devices);
      }
    };

    const handleBleStatus = (status: any) => {
      const bleStatus: BleConnectionStatus = status["status"] ?? BleConnectionStatus.Disconnected;
      const scanning: boolean = status["scanning"] ?? false;
      setBleConnectionStatus(bleStatus);
      setIsScanning(scanning);
    };

    const scanResulthandle = connection.registerOnJsonDataListener("bleDevices", handleBleDevices);
    const statusHandle = connection.registerOnJsonDataListener("bleStatus", handleBleStatus);
    const onConnectionChangeListenerHandle = connection.registerOnChangeListener(connected => {
      if (connected) {
        connection.sendCommand(DrumCommand.getBleStatus, {});
      }
    });

    return () => {
      connection.unregisterListener(scanResulthandle);
      connection.unregisterListener(statusHandle);
      connection.unregisterListener(onConnectionChangeListenerHandle);
    };
  }, []);

  const midiOutputModeLabels: Record<MidiOutputMode, string> = {
    [MidiOutputMode.UsbDevice]: "USB Client",
    [MidiOutputMode.UsbHost]: "USB Host",
    [MidiOutputMode.SerialDin]: "Serial 5-Pin DIN",
    [MidiOutputMode.BleClient]: "BLE Client",
    [MidiOutputMode.BleServer]: "BLE Server",
    [MidiOutputMode.GuitarHeroDrum]: "Guitar Hero Drum (SPI)"
  };

  const handleMidiModeChanged = (value: MidiOutputMode) => {
    if (value !== midiOutputMode) {
      updateConfig(config => config.general = {
        ...config.general!,
        midiOutputMode: value
      });
      connection.sendSetGeneralConfigCommand({
        midiOutputMode: value
      });
    }
  };

  const handlBleDeviceSelected = (device: {
    name: string; address: string
  } | undefined = undefined) => {
    setSelectedBleDevice(device?.address ?? "");
    connection.sendCommandWithDirtyFlag(DrumCommand.blePair, !device ? {} : {
      name: device.name,
      address: device.address
    }, true);
  };

  const handleScanBleDevicesClicked = () => {
    setIsScanning(true);
    setScannedDevices([]);
    connection.sendCommand(DrumCommand.scanBleDevices, {});
  };

  if (midiOutputMode === undefined) {
    return null; // or a placeholder if you prefer
  }

  return (
    <Stack spacing={1}>
      <GeneralSetting label="MIDI Output Mode:">
        <Select
          disabled={!connected}
          value={midiOutputMode}
          onChange={(e) => handleMidiModeChanged(e.target.value as MidiOutputMode)}
          size='small'
          sx={{ minWidth: '150px' }}
        >
          {supportedMidiOutputModes.map(mode => (
            <MenuItem key={mode} value={mode} selected={mode === midiOutputMode}>
              {midiOutputModeLabels[mode]}
            </MenuItem>
          ))}
        </Select>
      </GeneralSetting>
      
      {midiOutputMode === MidiOutputMode.BleClient && (
        <MidiSettingsBox label="BLE Client Settings">
          <Stack sx={{ marginBottom: 1, fontSize: '0.875rem', color: 'text.secondary' }}>
            <Box>
              <label>Paired Device:&nbsp;</label>
              <span>{blePairing?.name ?? "None"}</span>
              <span>{blePairing?.address ? <i>&nbsp;({blePairing.address})</i> : ""}</span>
              {blePairing?.address && (
                <IconButton component="span" size="small" onClick={() => handlBleDeviceSelected()} color="primary" >
                  <DeleteIcon />
                </IconButton>
              )}
            </Box>
            <Box>
              <label>Status:&nbsp;</label><span>{bleConnectionStatus}</span>
            </Box>
          </Stack>

          <Stack direction={'row'} spacing={1} alignItems={'center'}>
            <List sx={{ width: '50%', border: '1px solid', borderColor: 'divider' }}>
              {scannedDevices.length === 0 && (
                <ListItem>
                  <ListItemText sx={{ color: 'text.secondary', fontStyle: 'italic' }} primary="No devices found" />
                </ListItem>
              )}
              {scannedDevices.map((device) => (
                <ListItemButton key={device.address} onClick={() => connected && handlBleDeviceSelected(device)}>
                  <ListItemText  primary={device.name} secondary={device.address} />
                  {device.address === selectedBleDevice && (
                    <ListItemIcon>
                      <StarIcon />
                    </ListItemIcon>
                  )}
                </ListItemButton>
              ))}
            </List>
            <Button
              onClick={handleScanBleDevicesClicked}
              disabled={!connected || isScanning}
              variant="outlined"
              size="small"
              sx={{
                '&.Mui-disabled': {
                  color: darken(theme.palette.primary.main, 0.3)
                },
                marginLeft: 1
              }}
            >
              {isScanning ? "Scanning..." : "Scan Devices"}
            </Button>
          </Stack>
        </MidiSettingsBox>
      )}
    </Stack>
  );
}

function MidiSettingsBox({children, label}: {
  children: React.ReactNode;
  label: string;
}) {
  return (
    <Box
      component="fieldset"
      sx={{ 
        border: '1px solid', 
        borderColor: 'divider', 
        borderRadius: 1, 
        padding: 1, 
        backgroundColor: 'background.paper'
      }}
    >
      <Box
        component="legend"
        sx={{
          padding: '0 8px',
          fontSize: '0.875rem',
          color: 'text.secondary'
        }}
      >
        {label}
      </Box>
      {children}
    </Box>
  );
}
