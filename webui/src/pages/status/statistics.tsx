// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { useCallback, useEffect, useState } from 'react';

import Box from '@mui/material/Box';

import { connection, DrumCommand } from '@connection';
import { IconButton, Stack, Typography } from '@mui/material';
import ReloadIcon from '@mui/icons-material/Sync';
import { InfoBox } from '@components/info-box';

interface StatisticsJson {
    updateCountPer30s?: number;
}

interface StatisticsInfo {
    lastRetrievalDate: Date;
    statsJson: StatisticsJson;
}

function requestStats() {
  connection.sendCommand(DrumCommand.getStats);
}

export function StatisticsTable() {
  const [statsInfo, setStatsInfo] = useState<StatisticsInfo>();

  useEffect(() => {
    const onStatsJsonListenerHandle = connection.registerOnJsonDataListener('stats', (statsJson: StatisticsJson) => {
      setStatsInfo({
        lastRetrievalDate: new Date(),
        statsJson: statsJson
      });
    });
    
    requestStats();

    return () => {
      connection.unregisterListener(onStatsJsonListenerHandle);
    };
  }, []);

  const handleReload = useCallback(() => {
    requestStats();
  }, []);

  let lastRetrieval = "<n/a>";
  let pollingInfo = "<n/a>";
  if (statsInfo) {
    lastRetrieval = statsInfo.lastRetrievalDate.toISOString();
    if (statsInfo.statsJson.updateCountPer30s) {
      const measurementIntervalSec = 30;
      const updateCount = statsInfo.statsJson.updateCountPer30s;
      const updatesPerSecond = Math.round(updateCount / measurementIntervalSec);
      const pollingIntervalUs = Math.round(measurementIntervalSec * 1000 * 1000 / updateCount);
      pollingInfo = `${pollingIntervalUs}µs (${updatesPerSecond} polls/s)`;
    }
  }

  return (
    <Stack direction='row'>
      <InfoBox>
        <Typography gridColumn='span 2' fontSize='80%' fontStyle='italic'>Last update: {lastRetrieval}</Typography>
        {
          !statsInfo ? null :
            <>
              <Box>Sensor Polling Interval:</Box><Box>{pollingInfo}</Box>
            </>
        }
      </InfoBox>
      <IconButton onClick={handleReload}>
        <ReloadIcon/>
      </IconButton>
    </Stack>
  );
}
