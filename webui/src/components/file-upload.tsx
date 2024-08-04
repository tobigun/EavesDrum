// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { PropsWithChildren, useCallback, useState } from "react";
import { parse } from "yaml";
import { connection, DrumCommand } from "../connection/connection";
import { Alert, Box, Dialog, DialogProps, Snackbar, SnackbarCloseReason, Stack, Typography } from "@mui/material";
import { UploadFile } from "@mui/icons-material";
import { useDropzone } from "react-dropzone";
import { PadRole } from "../config/config";

export enum ConfigFilter {
  Settings,
  Mappings
}

export interface ConfigDropProps {
  filter?: ConfigFilter,
  padRole?: string
}

function applyConfig(configNode: any, filter?: ConfigFilter, padRole?: PadRole) : boolean {
  let success = false;
  if (filter === undefined || filter === ConfigFilter.Settings) {
    const settingsNode = configNode['settings'];
    if (padRole) {
      const padSettingsNode = settingsNode?.[padRole] ?? configNode['preset'];
      if (padSettingsNode) {
        connection.sendSetPadSettingsCommand(padRole, padSettingsNode);
        success = true;
      }
    } else if (settingsNode) {
      connection.sendSetSettingsCommand(settingsNode);
      success = true;
    }
  }
      
  if (filter === undefined || filter === ConfigFilter.Mappings) {
    const mappingsNode = configNode['mappings'];
    if (padRole) {
      const padMappingsNode = mappingsNode?.[padRole];
      if (padMappingsNode) {
        connection.sendSetPadMappingsCommand(padRole, padMappingsNode);
        success = true;
      }
    } else if (mappingsNode) {
      connection.sendSetMappingsCommand(mappingsNode);
      success = true;
    }
  }

  return success;
}

function handleConfigDrop(file: File, dropProps: ConfigDropProps, setErrorText: (message: string) => void) {
  const reader = new FileReader();
  reader.onabort = () => setErrorText('File reading was aborted');
  reader.onerror = () => setErrorText('File reading has failed');
  reader.onload = () => {
    try {
      const yaml = reader.result as string;
      const configNode = parse(yaml);
      if (!applyConfig(configNode, dropProps.filter, dropProps.padRole)) {
        setErrorText('No valid config entry found');
      } else {
        connection.sendCommand(DrumCommand.getConfig);
      }
    } catch(error) {
      if (typeof error === "string") {
        setErrorText(error as string);
      } else if (error instanceof Error) {
        setErrorText((error as Error).message);
      }        
      console.log(error);
    }
  };
  reader.readAsText(file);
}

const acceptedConfigTypes = { 'application/yaml': ['.yaml', '.yml'] };

export function ConfigDropOverlay(props: {
    dropProps: ConfigDropProps
} & PropsWithChildren) {
  const [drag, setDrag] = useState(false);
  const [errorText, setErrorText] = useState<string>();

  const onDrop = useCallback((acceptedFiles: File[]) => {
    handleConfigDrop(acceptedFiles[0], props.dropProps, setErrorText);
    setDrag(false);
  }, [props.dropProps, setErrorText]);
  
  const {getRootProps, getInputProps} = useDropzone({
    onDrop,
    accept: acceptedConfigTypes,
    onDragEnter: () => setDrag(true),
    onDragLeave: () => setDrag(false)
  });
  
  return (
    <>
      <Box {...getRootProps()} position='relative'>
        <input {...getInputProps()} disabled={true} />
        <div>
          { props.children }
        </div>
        <div className='droparea' style={{ display: drag ? 'flex' : 'none' }}>
          <Stack display='flex' alignItems='center' justifyContent='center' padding={2} width='80%' height='80%' borderRadius='2px' border='2px dashed #bdbdbd'>
            <Typography>Drop a mapping file (.yaml) here</Typography>
            <UploadFile sx={{ fontSize: '5em' }}/>
          </Stack>
        </div>
      </Box>
      <ErrorSnackbar errorText={errorText} setErrorText={setErrorText} />
    </>
  );
}
function ErrorSnackbar({ errorText, setErrorText } : {
    errorText: string | undefined,
    setErrorText: (text: string | undefined) => void
}) {
  const handleClose = useCallback((_event?: React.SyntheticEvent | Event, reason?: SnackbarCloseReason) => {
    if (reason === 'clickaway') {
      //return;
    }
    setErrorText(undefined);
  }, [setErrorText]);

  return (
    <Snackbar open={!!errorText} onClose={handleClose}>
      <Alert onClose={handleClose} severity="error" sx={{ width: '100%' }} >
        <Box sx={{ whiteSpace: "pre-wrap", fontFamily: 'Monospace' }}>{errorText}</Box>
      </Alert>
    </Snackbar>
  );
}

export function ConfigUploadDialog(props: DialogProps) {
  const [errorText, setErrorText] = useState<string>();

  const onDrop = useCallback((acceptedFiles: File[]) => {
    const file = acceptedFiles[0];
    handleConfigDrop(file, {}, setErrorText);
    props.onClose?.({}, "backdropClick");
  }, [props]);
  
  const {getRootProps, getInputProps} = useDropzone({
    onDrop,
    accept: acceptedConfigTypes
  });
  
  return (
    <>
      <Dialog {...props}
        fullWidth={true} maxWidth='xl'
        slotProps={{ paper: { sx: { alignSelf: "flex-start", margin: 0, minHeight: '80%', maxHeight: '80%' } }}}
      >
        <Box {...getRootProps({className: 'dropzone'})} >
          <input {...getInputProps()} />
          <Typography>Drop a config file (.yaml) here, or click to select file</Typography>
          <UploadFile sx={{ fontSize: '5em' }}/>
        </Box>
      </Dialog>    
      <ErrorSnackbar errorText={errorText} setErrorText={setErrorText} />
    </>
  );
}
  