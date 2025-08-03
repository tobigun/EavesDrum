// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { memo, useCallback, useState } from "react";

import { AppBar as MuiAppBar, Box, Toolbar } from "@mui/material";

import { Tab, Tabs } from "@components/tab";
import Logo from "@/image/logo.svg?react";
import { IconBar } from "./icon-bar";
import { ConfigUploadDialog } from "@components/file-upload";

export const AppBar = memo(MyAppBar);

function MyAppBar({ tabIndex, setTabIndex, tabNames }: {
  tabIndex: number,
  setTabIndex: (index: number) => void,
  tabNames: string[]
}) {
  const handleChangeTab = useCallback((_event: React.SyntheticEvent, newTabIndex: number) => {
    setTabIndex(newTabIndex);
  }, [setTabIndex]);

  const [fileUploadDialogOpen, setFileUploadDialogOpen] = useState(false);

  return (
    <MuiAppBar position="static">
      <Toolbar sx={{ "&.MuiToolbar-root": { minHeight: 0 } }} onDragEnter={() => setFileUploadDialogOpen(true)}>        
        <Box height={30} width={150} display='flex' paddingRight={1}>
          <Logo />
        </Box>
        
        <Box>
          <Tabs value={tabIndex} textColor="secondary" indicatorColor="secondary" onChange={handleChangeTab}>
            {
              tabNames.map(name =>
                <Tab key={name} label={name} />
              )
            }
          </Tabs>
        </Box>
        
        <Box flexGrow={1} />
        
        <IconBar setFileUploadDialogOpen={setFileUploadDialogOpen} />

        <ConfigUploadDialog
          open={fileUploadDialogOpen}
          onClose={() => setFileUploadDialogOpen(false)} />

      </Toolbar>
    </MuiAppBar>
  );
}
