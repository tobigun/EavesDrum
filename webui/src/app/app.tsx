// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { useState } from 'react';

import Box from '@mui/material/Box';

import { AppBar } from './app-bar';
import { ConnectionStateProvider } from '@connection';
import { StatusPage } from "@pages/status/status-page";
import { MappingsPage } from "@pages/mappings/mappings-page";
import { SettingsPage } from "@pages/settings/settings-page";
import { TabContentPage } from '@components/tab';

import './app.css';

export function App() {
  const [tabIndex, setTabIndex] = useState(0);

  return (
    <Box sx={{ flexGrow: 1 }}>
      <ConnectionStateProvider>
        <AppBar tabIndex={tabIndex} setTabIndex={setTabIndex} tabNames={[
          'Settings',
          'Mappings',
          'Status'
        ]} />
        <TabContentPage value={tabIndex} index={0}>
          <SettingsPage />
        </TabContentPage>
        <TabContentPage value={tabIndex} index={1}>
          <MappingsPage />
        </TabContentPage>
        <TabContentPage value={tabIndex} index={2}>
          <StatusPage />
        </TabContentPage>
      </ConnectionStateProvider>
    </Box>
  );
}
