// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { Box, Button, Dialog, DialogActions, DialogContent, DialogContentText, DialogTitle, IconButton, Popover } from "@mui/material";
import { ReactNode, useCallback, useContext, useState } from "react";
import { VersionInfo } from "./version-info";
import { Config, useConfig } from "@config";
import { stringify } from 'yaml';

import DownloadIcon from '@mui/icons-material/SaveAlt';
import SaveIcon from '@mui/icons-material/SdCard';
import RestoreIcon from '@mui/icons-material/Undo';
import UploadFile from '@mui/icons-material/UploadFile';
import InfoIcon from '@mui/icons-material/InfoOutline';
import { connection, DrumCommand } from "@/connection/connection";
import { ConnectionStateContext } from "@/connection/connection-state";

const iconSize = 'large';
const iconColor = 'rgb(0, 0, 0)';

export function IconBar({ setFileUploadDialogOpen }: {
  setFileUploadDialogOpen: (open: boolean) => void
}) {
  const connected = useContext(ConnectionStateContext);
  const isDirty = useConfig(config => config.isDirty ?? false);
  
  const [restoreDialogOpen, setRestoreDialogOpen] = useState(false);

  const handleSaveConfig = useCallback(() => {
    connection.sendCommand(DrumCommand.saveConfig);
  }, []);

  const handleRestoreDialogAccept = useCallback(() => {
    connection.sendCommand(DrumCommand.restoreConfig);
    setRestoreDialogOpen(false);
  }, []);

  const handleDownloadCurrentConfig = () => {
    const configState = useConfig.getState();

    // ignore sections that are UI specific (like isDirty or version)
    const config : Config = {
      general: configState.general,
      mux: configState.mux,
      connectors: configState.connectors,
      pads: configState.pads,
      mappings: configState.mappings
    };

    const configContent = stringify(config);
    const schema = "# yaml-language-server: $schema=./config.jsonc\n";

    // simulate a click on an anchor element
    const element = document.createElement("a");
    const downloadFile = new Blob([schema, configContent], {type: 'application/json'});
    element.href = URL.createObjectURL(downloadFile);
    element.download = "config.yaml";
    document.body.appendChild(element);
    element.click(); // required for firefox
  };

  return (
    <Box display='flex'>
      <IconButton onClick={handleSaveConfig} title='Save configuration' disabled={!isDirty} size={iconSize} sx={{ color: iconColor }}>
        <SaveIcon />
      </IconButton>
          
      <IconButton onClick={() => setRestoreDialogOpen(true)} title='Restore last saved configuration' disabled={!isDirty} size={iconSize} sx={{ color: iconColor }}>
        <RestoreIcon />
      </IconButton>          
      <Dialog open={restoreDialogOpen} onClose={() => setRestoreDialogOpen(false)}>
        <DialogTitle id="alert-dialog-title">Restore config?</DialogTitle>
        <DialogContent>
          <DialogContentText id="alert-dialog-description">
                This will revert all unsaved settings and mappings to the previously saved state.
          </DialogContentText>
        </DialogContent>
        <DialogActions>
          <Button variant="outlined" onClick={() => setRestoreDialogOpen(false)}>Abort</Button>
          <Button variant="contained" onClick={handleRestoreDialogAccept} autoFocus>Ok</Button>
        </DialogActions>
      </Dialog>

      <IconButton onClick={handleDownloadCurrentConfig} title='Download current configuration' size={iconSize} sx={{ color: iconColor }}>
        <DownloadIcon />
      </IconButton>

      <IconButton onClick={() => setFileUploadDialogOpen(true)} title='Apply config file' size={iconSize} sx={{ color: iconColor }}>
        <UploadFile />
      </IconButton>

      <VersionInfoPopover iconButton={(onClick) =>
        <IconButton onClick={onClick} title='Info' size={iconSize} sx={{ color: iconColor }}>
          <InfoIcon />
        </IconButton>
      } />
          

      <ConnectionIndicator connected={connected} />
    </Box>
  );
}


function ConnectionIndicator({ connected }: {
  connected: boolean
}) {
  return (
    <Box title={connected ? 'Connected' : 'Disconnected'} sx={{ width: '100%', display: 'flex', pl: '10px', alignItems: 'center' }}>
      {connected ? '🟢' : '🔴'}
    </Box>
  );
}

function VersionInfoPopover({iconButton} : {iconButton: (onClick: (event: React.MouseEvent<HTMLButtonElement>) => void) => ReactNode}) {
  const [versionInfoAnchorEl, setVersionInfoAnchorEl] = useState<HTMLButtonElement | null>(null);
  
  const onShowVersionInfo = useCallback((event: React.MouseEvent<HTMLButtonElement>) => {
    setVersionInfoAnchorEl(event.currentTarget);
  }, []);
  
  const onCloseVersionInfo = useCallback(() => {
    setVersionInfoAnchorEl(null);
  }, []);

  return <>
    {iconButton(onShowVersionInfo)}
    <Popover
      open={versionInfoAnchorEl !== null}
      anchorEl={versionInfoAnchorEl}
      onClose={onCloseVersionInfo}
      anchorOrigin={{ vertical: 'bottom', horizontal: 'center' }}
    >
      <VersionInfo />
    </Popover>
  </>;
}
