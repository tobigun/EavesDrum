// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import Stack from '@mui/material/Stack';

import { useShallow } from 'zustand/shallow';

import { MonitorCard } from './monitor/monitor-card';
import { Masonry } from '../components/masonry';
import { SettingsCardGroup } from './settings-card';
import { useConfig } from '../config/config';
import { GeneralCard } from './settings-general';

export function SettingsPage() {
  const padCount = useConfig(useShallow(config => config.pads.length));

  return <>
    <MonitorCard />
    <GeneralCard />
    <Stack>
      <Masonry columns={{ xs: 1, sm: 1, md: 2, lg: 3, xl: 4 }}>
        {
          [...Array(padCount)].map((_, padIndex) =>
            <SettingsCardGroup key={padIndex} padIndex={padIndex} />
          )
        }
      </Masonry>
    </Stack>
  </>;
}
