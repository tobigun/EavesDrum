// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { useConfig } from "@/config";
import { connection } from "@/connection/connection";
import { SettingEntryContainer } from "@/pages/settings/setting-entry";
import { Box, MenuItem, Select, SelectChangeEvent } from "@mui/material";
import { useShallow } from "zustand/shallow";

export function RoleInfo({ padRole, padIndex }: {
  padRole: string,
  padIndex?: number
}) {
  const roles = useConfig(useShallow(config => Object.keys(config.mappings)));
  const mappingNames = roles.map(role => useConfig.getState().mappings[role]?.name);
  
  const handleSelectChange = (event: SelectChangeEvent) => {
    const role = event.target.value;
    connection.sendSetPadConfigCommand(padIndex!, { role: role });
  };

  return <SettingEntryContainer name='Role'>
    <Select value={padRole} disabled={padIndex === undefined} size='small'
      onChange={handleSelectChange}
      sx={{color: roles.includes(padRole) ? null : 'red'}}>
      {
        !roles.includes(padRole) ? <MenuItem value={padRole} sx={{ color: 'red' }}>{padRole}</MenuItem> : null
      }
      {
        roles.map((role, index) => <MenuItem key={role} value={role}>
          {mappingNames[index] ? mappingNames[index] : role}
          &nbsp;
          <Box component='em' color='grey' fontSize='0.8em'>(ID: {role})</Box>
        </MenuItem>)
      }
    </Select>
  </SettingEntryContainer>;
}
