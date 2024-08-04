// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { useCallback, useEffect, useState } from "react";

import Box from "@mui/material/Box";
import Grid from '@mui/material/Grid';
import Stack from "@mui/material/Stack";
import Typography from "@mui/material/Typography";

import RecordIcon from '@mui/icons-material/FiberManualRecord';

import { connection, DrumCommand } from '../../connection/connection';
import { getPadSettingsByIndex, PadType, updateConfig, useConfig } from "../../config/config";
import { Card, CardSize, PanelButton, PanelToggleButton } from "../../components/card";
import { MonitorMessage, MonitorMessageInfo } from "./monitor-message";
import { LatencyTestWizard } from "./latency-test";
import { SignalGraph } from "./signal-graph/signal-graph";
import { HitGraph } from "./hit-graph";
import { CircularProgress } from "@mui/material";
import { numRecentMessageInfos, RecentHitsGraph } from "./recent-graph";
import { MonitorMode } from "./monitor-mode";
import { CalibrationWizard } from "./calibration";

const logMonitorMessages = true;
const backgroundColor = 'rgb(52, 73, 94)';
export const recordButtonColor = 'rgb(202, 13, 13)';

export function MonitorCard() {
  const spacing = 1;

  const latencyTestActiveInBackend = useConfig(config => config.latencyTest);
  const triggeredByAllPads = useConfig(config => config.monitor.triggeredByAllPads);
  const monitoredPadIndex = useConfig(config => config.monitor.padIndex);
  const monitoredPad = monitoredPadIndex !== undefined ? useConfig.getState().pads[monitoredPadIndex] : null;

  const [expanded, setExpanded] = useState(false);
  const [startLatencyTest, setStartLatencyTest] = useState(false);
  const [showCalibration, setShowCalibration] = useState(false);

  const onExpandedChange = (_event: React.SyntheticEvent, isExpanded: boolean) => {
    setExpanded(isExpanded);
  };

  const onTrigger = useCallback(() => {
    connection.sendCommand(DrumCommand.triggerMonitor, {});
  }, []);

  function closeLatencyTest() {
    setStartLatencyTest(false);
    connection.sendLatencyTestOffCommand();
  }

  const onChangeShowAllHits = useCallback(() => {
    const newValue = !triggeredByAllPads;
    updateConfig(config => config.monitor.triggeredByAllPads = newValue);
    connection.sendCommand(DrumCommand.setMonitor, {triggeredByAllPads: newValue});
  }, [triggeredByAllPads]);

  // we have to keep the latency test view alive if it is still active in the backend (e.g. if F5 is pressed)
  const showLatencyTest = startLatencyTest || latencyTestActiveInBackend;
  const wizardEnabled = showLatencyTest || showCalibration;

  return (
    <Box sx={{ margin: spacing / 2 }}>
      <Card name='Monitor' size={CardSize.large} color={backgroundColor}
        expanded={expanded || wizardEnabled}
        onChange={onExpandedChange}
        titleDecorators={
          monitoredPad &&
          <Stack direction='row' alignItems='center' spacing={0}>
            <RecordIcon sx={{ color: recordButtonColor }} />
            <Typography color='primary' variant="h6">{monitoredPad.name}</Typography>
          </Stack>
        }
        edgeDecorators={<>
          { monitoredPad &&
            <PanelButton title='Trigger' onClick={onTrigger} disabled={wizardEnabled}>
              Trigger
            </PanelButton>
          }
          <PanelButton title='Measures latency from trigger to soundcard output'
            disabled={showCalibration}
            onClick={() => setStartLatencyTest(true)}
          >
            Check&nbsp;Latency
          </PanelButton>
          <PanelButton title='Calibrates the DC voltage offset of the multiplexers'
            disabled={showLatencyTest}
            onClick={() => setShowCalibration(true)}
          >
            Calibrate
          </PanelButton>
          <PanelToggleButton title='Monitor also shows hits (without signal graph) of non-monitored pads'
            value='triggered-by-all'
            disabled={wizardEnabled}
            selected={triggeredByAllPads}
            onChange={onChangeShowAllHits}
          >
            Show&nbsp;&nbsp;all&nbsp;Pads
          </PanelToggleButton>
        </>}
      >
        {
          showLatencyTest ? <LatencyTestWizard closeLatencyTest={closeLatencyTest} />
            : showCalibration ? <CalibrationWizard closeCalibration={() => setShowCalibration(false)} />
              : <Monitor />
        }
      </Card>
    </Box>
  );
}

export function Monitor({mode = MonitorMode.Default, showHitGraph = true} : {
  mode?: MonitorMode,
  showHitGraph?: boolean
}) {
  const [selectedMessageInfo, setSelectedMessageInfo] = useState<MonitorMessageInfo>();
  const [recentMessageInfos, setRecentMessageInfos] = useState<MonitorMessageInfo[]>([]);
  const monitoredPad = useConfig(config => config.monitor.padIndex);

  const triggeredByAllPadsConfig = useConfig(config => config.monitor.triggeredByAllPads);
  const triggeredByAllPads = triggeredByAllPadsConfig && mode === MonitorMode.Default;

  useEffect(() => {
    const handle = connection.registerOnBinaryDataListener(monitorData => {
      const message = new MonitorMessage(monitorData);
      const messageInfo = new MonitorMessageInfo(message);
      setSelectedMessageInfo(messageInfo);

      if (showHitGraph) {
        if (logMonitorMessages) {
          console.log(JSON.stringify(message));
        }
        
        setRecentMessageInfos(oldSlots => [messageInfo, ...oldSlots.slice(0, numRecentMessageInfos - 1)]);
      }
    });

    return () => {
      connection.unregisterListener(handle);
    };
  }, [showHitGraph]);

  if (!selectedMessageInfo) {
    const waitForHit = monitoredPad !== undefined || triggeredByAllPads;
    let waitMessage = 'Waiting for hit ...';
    if (monitoredPad !== undefined) {
      const settings = getPadSettingsByIndex(monitoredPad);
      if (settings.padType === PadType.Pedal) {
        waitMessage = 'Waiting for pedal movement ...';
      }
    }
    return <Stack alignItems='center' justifyContent='center' height='100%'>
      {waitForHit ? <CircularProgress /> : null}
      <Typography fontSize='28px'>
        {waitForHit ? waitMessage : 'No pad selected for monitoring'}
      </Typography>
    </Stack>;
  } else {
    const height = '40vh';
    return <Grid container>
      <Grid size={{ xs: 12, sm: 12, md: 9, xl: 9.5 }} height={height}>
        <SignalGraph mode={mode} messageInfo={selectedMessageInfo}/>
      </Grid>
      {
        !showHitGraph ? null : <>
          <Grid size={{ xs: 9, sm: 11, md: 2, xl: 2 }} height={height} display='flex' alignItems='center' justifyContent='center'>
            <HitGraph messageInfo={selectedMessageInfo}/>
          </Grid>
          <Grid size={{ xs: 3, sm: 1, md: 1, xl: 0.5 }} height={height} display='flex' alignItems='center' justifyContent='right'>
            <RecentHitsGraph selectedMessageInfo={selectedMessageInfo} setSelectedMessageInfo={setSelectedMessageInfo} recentMessageInfos={recentMessageInfos}/>
          </Grid>
        </>
      }
    </Grid>;
  }
}
