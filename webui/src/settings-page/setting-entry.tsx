// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { Stack } from "@mui/material";
import { PropsWithChildren } from "react";
import { EntryContainer } from "../components/card";

export function SettingEntryContainer(props: PropsWithChildren<{ name: string }>) {
  return (
    <EntryContainer name={props.name} labelWidth={'14em'}>
      <Stack direction='row' gap={2} minWidth={350} flexGrow={1}>
        {props.children}
      </Stack>
    </EntryContainer>
  );
}

export interface SettingEntryProps {
  label: string,
  padRole: string
}
