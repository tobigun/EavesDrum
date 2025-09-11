// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { PropsWithChildren, useCallback, useState } from "react";
import { parse } from "yaml";
import { connection, DrumCommand } from "@/connection/connection";
import { Alert, AlertColor, Box, Dialog, DialogProps, Snackbar, SnackbarCloseReason, Stack, Typography } from "@mui/material";
import { UploadFile } from "@mui/icons-material";
import { useDropzone } from "react-dropzone";
import { ConfigFilter } from "./component-enums";

export interface ConfigDropProps {
  filter?: ConfigFilter,
  padIndex?: number,
  padRole?: string
}

function applyConfig(configNode: any, dropProps: ConfigDropProps) : boolean {
  let success = false;
  const filter = dropProps.filter;

  if (filter === undefined || filter === ConfigFilter.Settings) {
    if (applyPadSettings(configNode, dropProps.padIndex)) {
      success = true;
    }
  }

  if (filter === undefined || filter === ConfigFilter.Mappings) {
    if (applyPadOrAllMappings(configNode, dropProps.padRole)) {
      success = true;
    }
  }

  return success;
}

// returns true if a config was applied
function applyPadSettings(configNode: any, padIndex?: number): boolean {
  const padSettingsNode = configNode['settings'];
  if (!padSettingsNode) {
    return false;
  }

  if (padIndex === undefined) { // config was dropped on overall config upload area
    return false; // ignore, as the pad for this config is unknown
  }

  // settings were dropped on a pad -> apply settings for selected pad only
  connection.sendSetPadSettingsCommand(padIndex, padSettingsNode);
  return true; // Note: we do not check yet if backend accepted the settings (i.e. upload might still have failed)
}

// returns true if a config was applied
function applyPadOrAllMappings(configNode: any, padRole?: string): boolean {
  const mappingsNode = configNode['mappings'];
  if (!mappingsNode) {
    return false;
  }

  if (!padRole) { // mapping was dropped on overall config upload area -> use all applicable mappings
    connection.sendSetMappingsCommand(mappingsNode, true);
    return true;
  }

  // mappings were dropped on a pad -> apply settings for role of selected pad only
  const roleMappingsNode = mappingsNode[padRole];
  if (roleMappingsNode) { // uploaded config must contain mappings for selected pad
    connection.sendSetRoleMappingsCommand(padRole, roleMappingsNode, true);
    return true; // Note: we do not check yet if backend accepted the mappings (i.e. upload might still have failed)
  }

  return false;
}


interface FileUploadMessage {
  message: string;
  severity: AlertColor
}

function handleConfigDrop(file: File, dropProps: ConfigDropProps, setMessage: (message: FileUploadMessage) => void) {
  const reader = new FileReader();
  reader.onabort = () => setMessage({message: 'File reading was aborted', severity: 'error'});
  reader.onerror = () => setMessage({message: 'File reading has failed', severity: 'error'});
  reader.onload = () => {
    try {
      const yaml = reader.result as string;
      const configNode = parse(yaml);
      if (!applyConfig(configNode, dropProps)) {
        setMessage({message: 'No valid config entry found', severity: 'error'});
      } else {
        connection.sendCommand(DrumCommand.getConfig);
        setMessage({message: 'Config applied', severity: 'success'});
      }
    } catch(error) {
      if (typeof error === "string") {
        setMessage({message: error as string, severity: 'error'});
      } else if (error instanceof Error) {
        setMessage({message: (error as Error).message, severity: 'error'});
      }        
      console.log(error);
    }
  };
  reader.readAsText(file);
}

const acceptedConfigTypes = { 'application/yaml': ['.yaml', '.yml'] };

export function CardConfigDropOverlay(props: {
    dropProps: ConfigDropProps
} & PropsWithChildren) {
  const [drag, setDrag] = useState(false);
  const [message, setMessage] = useState<FileUploadMessage>();

  const onDrop = useCallback((acceptedFiles: File[]) => {
    handleConfigDrop(acceptedFiles[0], props.dropProps, setMessage);
    setDrag(false);
  }, [props.dropProps, setMessage]);
  
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
            <Typography>
              Drop a {props.dropProps.filter === ConfigFilter.Mappings ? 'mapping' : 'setting'} file (.yaml) here
            </Typography>
            <UploadFile sx={{ fontSize: '5em' }}/>
          </Stack>
        </div>
      </Box>
      <FileUploadSnackbar message={message} setMessage={setMessage} />
    </>
  );
}

function FileUploadSnackbar({ message, setMessage } : {
    message: FileUploadMessage | undefined,
    setMessage: (message: FileUploadMessage | undefined) => void
}) {
  const handleClose = useCallback((_event?: React.SyntheticEvent | Event, reason?: SnackbarCloseReason) => {
    if (reason === 'clickaway') {
      //return;
    }
    setMessage(undefined);
  }, [setMessage]);

  return (
    <Snackbar open={!!message} onClose={handleClose}>
      <Alert onClose={handleClose} severity={message?.severity} sx={{ width: '100%' }} >
        <Box sx={{ whiteSpace: "pre-wrap", fontFamily: 'Monospace' }}>{message?.message}</Box>
      </Alert>
    </Snackbar>
  );
}

export function ConfigUploadDialog(props: DialogProps) {
  const [message, setMessage] = useState<FileUploadMessage>();

  const onDrop = useCallback((acceptedFiles: File[]) => {
    const file = acceptedFiles[0];
    handleConfigDrop(file, {}, setMessage);
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
      <FileUploadSnackbar message={message} setMessage={setMessage} />
    </>
  );
}
  