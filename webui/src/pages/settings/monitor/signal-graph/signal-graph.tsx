// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import {
  Chart as ChartJS,
  Filler,
  LinearScale,
  LineElement,
  Title,
  Tooltip,
  Legend,
  PointElement,
  ChartData,
  ChartDataset,
} from 'chart.js';
import ZoomPlugin from 'chartjs-plugin-zoom';
import AnnotationPlugin from 'chartjs-plugin-annotation';
import { Line } from 'react-chartjs-2';
import { PadType, useConfig } from '@config';
import { useCallback, useState } from 'react';
import { ButtonsPlugin, yZoomButton } from './buttons';
import { getZoneName } from '@/common';
import { createSignalGraphOptions } from './options';
import { getGradient } from './gradient';
import { ChartBackgroundPlugin } from './background';
import { ZoomOutOnDoubleClickPlugin } from './zoom';
import { YAXIS_SINGLE_ID, YAXIS_MULTI_IDS } from './scales';
import { alpha } from '@mui/material';
import { chartColors } from '@theme';
import { MonitorMessage, MonitorMessageInfo } from '../monitor-message';
import { MonitorMode } from '../monitor-mode';

export type SignalChartData = ChartData<'line', number[], number>;
type SignalChartDataset = ChartDataset<'line', number[]>;

ChartJS.register([
  AnnotationPlugin,
  Filler,
  LinearScale,
  LineElement,
  PointElement,
  ZoomPlugin,
  Title,
  Tooltip,
  Legend
]);

export function SignalGraph({mode, messageInfo} : {
  mode: MonitorMode,
  messageInfo: MonitorMessageInfo
}) {
  const [scaleToSelectedRange, setScaleToSelectedRange] = useState(false);
  yZoomButton.enabled = scaleToSelectedRange;
  yZoomButton.onClick = useCallback(() => {
    setScaleToSelectedRange(!scaleToSelectedRange);
    return true;
  }, [scaleToSelectedRange]);

  const data = updateSignalGraphDatasets(scaleToSelectedRange, messageInfo);
  const options = createSignalGraphOptions(scaleToSelectedRange, data, mode, messageInfo);
  
  return <Line options={options} data={data} plugins={[
    ZoomOutOnDoubleClickPlugin,
    ChartBackgroundPlugin,
    ButtonsPlugin]}
  />;  
}

function updateSignalGraphDatasets(scaleToSelectedRange: boolean, messageInfo: MonitorMessageInfo): SignalChartData {
  if (messageInfo.message.history.length == 0) {
    return { datasets: [] };
  }

  const message = messageInfo.message;

  // as we want to have the value at hitIndex with timestamp=0, we calculate the time offset of the first entry
  let offsetTimeUs = 0;
  for (let i = message.triggerStartIndex! - 1; i >= 0; --i) {
    offsetTimeUs -= message.history[i].timeUntilPreviousUs;
  }

  const labels: number[] = [];
  for (const entry of message.history) {
    labels.push(offsetTimeUs / 1000);
    offsetTimeUs += entry.timeUntilPreviousUs;
  };

  const datasets = (message.getPadType() == PadType.Pedal)
    ? updatePedalSignalGraphDatasets(messageInfo)
    : updatePadSignalGraphDatasets(scaleToSelectedRange, message);

  return { labels, datasets };
}

function updatePadSignalGraphDatasets(scaleToSelectedRange: boolean, message: MonitorMessage): SignalChartDataset[] {
  const zones = message.getZonesCount();
  const padType = message.getPadType();
  const numCurves = zones;

  const datasets: SignalChartDataset[] = [...Array(numCurves)].map((_, curveIndex) => ({
    label: getZoneName(padType, curveIndex),
    borderColor: chartColors[curveIndex],
    yAxisID: scaleToSelectedRange ? YAXIS_MULTI_IDS[curveIndex] : YAXIS_SINGLE_ID,
    backgroundColor: (ctx: any) => getGradient(ctx, curveIndex),
    fill: true,
    data: []
  }));

  // add all values with their timestamp calculated relative to the hit's timestamp
  for (const entry of message.history) {
    const sensorValuePerZone = entry.isGap
      ? [NaN, NaN, NaN]
      : Array.from(entry.values).map(value => value * 100 / 255.);

    datasets.forEach((zoneDataset, zoneIndex) =>
      zoneDataset.data.push(sensorValuePerZone[zoneIndex]));
  };

  return datasets;
}

function updatePedalSignalGraphDatasets(messageInfo: MonitorMessageInfo): SignalChartDataset[] {
  const datasets: SignalChartDataset[] = ['Pedal (Denoised)', 'Pedal (Raw)'].map((label, curveIndex) => ({
    label: label,
    borderColor: chartColors[curveIndex],
    yAxisID: YAXIS_SINGLE_ID,
    fill: false,
    data: []
  }));
  datasets[2] = { // upper bound of tolerance range
    label: 'Detection Tolerance',
    yAxisID: YAXIS_SINGLE_ID,
    fill: '+1',
    backgroundColor: alpha(chartColors[0], 0.3),
    showLine: false,
    data: []
  };
  datasets[3] = { // lower bound of tolerance range
    label: 'hidden', // Note: 'hidden...' labels are not shown in legend
    yAxisID: YAXIS_SINGLE_ID,
    fill: false,
    showLine: false,
    data: []
  };

  const message = messageInfo.message;
  const pedal = useConfig.getState().pads[message.padIndex];
  const tolerance = (pedal.settings.moveDetectTolerance ?? NaN) * 100 / 1023;
  
  // add all values with their timestamp calculated relative to the hit's timestamp
  for (const entry of message.history) {
    const [pedalValue, rawValue] = entry.isGap
      ? [NaN, NaN]
      : Array.from(entry.values).map(value => value * 100 / 255.);

    datasets[0].data.push(pedalValue);
    datasets[1].data.push(rawValue);
    datasets[2].data.push(Math.min(pedalValue + tolerance, 100));
    datasets[3].data.push(Math.max(0, pedalValue - tolerance));
  };

  return datasets;
}
