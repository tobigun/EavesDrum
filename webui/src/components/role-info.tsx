// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { updateConfig, useConfig } from "@/config";
import { connection } from "@/connection/connection";
import { SettingEntryContainer } from "@/pages/settings/setting-entry";
import { Box, IconButton, Input, MenuItem, Select, SelectChangeEvent, SelectProps } from "@mui/material";
import { useShallow } from "zustand/shallow";

import RenameIcon from "@mui/icons-material/Edit";
import RenameDoneIcon from "@mui/icons-material/Done";
import RenameCancelIcon from "@mui/icons-material/Close";
import { useRef, useState } from "react";

export function RoleInfo({ padRole, padIndex }: {
  padRole: string,
  padIndex?: number
}) {
  const roles = useConfig(useShallow(config => Object.keys(config.mappings)));
  const roleName = useConfig(config => config.mappings[padRole]?.name);
  const roleNames = roles.map(role => useConfig.getState().mappings[role]?.name);
  
  const handleSelectChange = (event: SelectChangeEvent) => {
    const newRole = event.target.value;
    connection.sendSetPadConfigCommand(padIndex!, { role: newRole });
  };

  function handleRoleRename(name: string) {
    updateConfig(config => config.mappings[padRole] = {
      ...config.mappings[padRole],
      name: name
    });
    connection.sendSetRoleMappingsCommand(padRole, {name: name});
  }

  return <SettingEntryContainer name='Role'>
    <EditableSelect value={padRole} editableValue={roleName ?? padRole} disabled={padIndex === undefined} size='small'
      onChange={handleSelectChange}
      onRename={handleRoleRename}
      sx={{color: roles.includes(padRole) ? null : 'red'}}>
      {
        !roles.includes(padRole) ? <MenuItem value={padRole} sx={{ color: 'red' }}>{padRole}</MenuItem> : null
      }
      {
        roles.map((role, index) => <MenuItem key={role} value={role}>
          {roleNames[index] ? roleNames[index] : role}
          &nbsp;
          <Box component='em' color='grey' fontSize='0.8em'>(ID: {role})</Box>
        </MenuItem>)
      }
    </EditableSelect>
  </SettingEntryContainer>;
}

function EditableSelect(props: SelectProps<string> & {
  editableValue: string,
  onRename?: (name: string) => void
}) {
  const [editMode, setEditMode] = useState(false);
  const inputRef = useRef<HTMLInputElement>(null);
  const editModeSupported = props.onRename !== undefined;

  function handleRenameClick(event: any) {
    event.stopPropagation();
    setEditMode(true);
  }
  
  function handleRenameDone(event: any) {
    event.stopPropagation();
    setEditMode(false);

    const newName = inputRef?.current?.value;
    if (newName && newName !== "") {
      props.onRename?.(newName);
    }
  }

  function handleRenameCancelled(event: any) {
    event.stopPropagation();
    setEditMode(false);
  }

  return <>
    {
      (editModeSupported && editMode)
        ? <Input inputRef={inputRef} defaultValue={props.editableValue} onClick={(event) => event.stopPropagation()} />
        : <Select {...props}>{props.children}</Select>
    }
    {
      !editModeSupported ? null : ( editMode ?
        <>
          <IconButton component="span" size="small" onClick={handleRenameDone} color="primary" >
            <RenameDoneIcon />
          </IconButton>
          <IconButton component="span" size="small" onClick={handleRenameCancelled} color="primary" >
            <RenameCancelIcon />
          </IconButton>
        </>
        :
        <IconButton component="span" size="small" onClick={handleRenameClick} color="primary" >
          <RenameIcon />
        </IconButton>
      )
    }
  </>;
}

