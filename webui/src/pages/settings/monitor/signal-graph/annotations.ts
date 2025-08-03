// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { AnnotationOptions } from 'chartjs-plugin-annotation';

import { alpha } from "@mui/material";

import { YAXIS_MULTI_IDS, YAXIS_SINGLE_ID } from './scales';
import { PadType } from '@config';
import { SignalChartData } from './signal-graph';
import { chartColors, pedalThresholdColors } from '@theme';
import { MonitorMessage, MonitorMessageInfo } from '../monitor-message';
import { getZoneName } from '@/common';
import { MonitorMode } from '../monitor-mode';

export function getAnnotations(
  messageInfo: MonitorMessageInfo,
  data: SignalChartData,
  mode: MonitorMode,
  scaleToSelectedRange: boolean): Record<string, AnnotationOptions>
{
  return {
    ...getXAnnotations(messageInfo, mode, data),
    ...getYAnnotations(messageInfo, mode, scaleToSelectedRange),
  };
}

const annotationTextColor = 'rgb(255, 255, 255)';

function getXAnnotations(
  messageInfo: MonitorMessageInfo,
  mode: MonitorMode,
  data: SignalChartData): Record<string, AnnotationOptions>
{
  const xAxisAnnotations: Record<string, AnnotationOptions> = {};
  const drawTime = 'beforeDatasetsDraw';
  const labelPosition = 'end';
  const borderWidth = 0.5;
  const message = messageInfo.message;

  if (message.getPadType() === PadType.Pedal) {
    return {};
  }

  const isLatencyMode = mode === MonitorMode.Latency;

  if (isLatencyMode && message.latencyUs > 0) {
    const latencyColor = 'rgb(175, 69, 160)';
    xAxisAnnotations['latencyAnnotationBox'] = {
      type: 'box',
      xMin: -message.latencyUs / 1000,
      xMax: 0,
      borderWidth: 0,
      backgroundColor: alpha(latencyColor, 0.4),
      drawTime: drawTime,
    };
    xAxisAnnotations['latencyAnnotationLine'] = {
      type: 'line',
      xMin: -message.latencyUs / 1000,
      xMax: -message.latencyUs / 1000,
      borderColor: 'rgb(87, 149, 87)',
      borderWidth: borderWidth,
      drawTime: drawTime,
      label: {
        color: annotationTextColor,
        backgroundColor: latencyColor,
        borderWidth: borderWidth,
        content: 'Latency: ' + Number(message.latencyUs / 1000).toFixed(1) + "ms",
        display: true,
        position: labelPosition,
      }
    };
  } else {
    xAxisAnnotations['scanAnnotationBox'] = {
      type: 'box',
      xMin: 0,
      xMax: indexToTimestampMs(message.triggerEndIndex!, data),
      borderWidth: 0,
      backgroundColor: alpha('rgb(179, 247, 202)', 0.5),
      drawTime: drawTime,
    };
    xAxisAnnotations['scanAnnotationLine'] = {
      type: 'line',
      xMin: 0,
      xMax: 0,
      borderColor: 'rgb(87, 149, 87)',
      borderWidth: borderWidth,
      drawTime: drawTime,
      label: {
        backgroundColor: 'rgb(87, 149, 87)',
        borderColor: 'rgb(119, 119, 119)',
        borderWidth: borderWidth,
        color: annotationTextColor,
        content: 'Scan',
        display: true,
        position: labelPosition,
        xAdjust: (ctx) => {
          const width = ctx.element?.label?.width ?? 0;
          return -width / 2;
        },
      },
    };
  }

  if (!isLatencyMode) {
    const scanEndIndex = message.triggerEndIndex!;
    xAxisAnnotations['maskAnnotationBox'] = {
      type: 'box',
      xMin: indexToTimestampMs(scanEndIndex, data),
      xMax: indexToTimestampMs(-1, data), // mask end time
      borderWidth: 0,
      backgroundColor: alpha('rgb(255, 69, 96)', 0.2),
      drawTime: drawTime,
    };
    xAxisAnnotations['maskAnnotationLine'] = {
      type: 'line',
      xMin: indexToTimestampMs(scanEndIndex, data),
      xMax: indexToTimestampMs(scanEndIndex, data),
      borderColor: 'rgb(179, 16, 16)',
      borderWidth: borderWidth,
      drawTime: drawTime,
      label: {
        backgroundColor: 'rgb(179, 16, 16)',
        borderColor: 'rgb(119, 119, 119)',
        borderWidth: borderWidth,
        color: annotationTextColor,
        content: 'Mask',
        display: true,
        position: labelPosition,
        xAdjust: (ctx) => {
          const width = ctx.element?.label?.width ?? 0;
          return width / 2;
        },
      },
    };  
  }

  return xAxisAnnotations;
}

const yLabelPosition = 'start';
const yLabelBorderColor = 'rgb(51, 51, 51)';
const yLabelBorderWidth = 0.5;
const yAxisDrawTime ='beforeDatasetsDraw';  
const yAxisLineDash: number[] = [];
const yAxisLineWidth = 2;
const yAxisLineOpacity = 0.6;

function getYAnnotations(
  messageInfo: MonitorMessageInfo,
  mode: MonitorMode,
  scaleToSelectedRange: boolean): Record<string, AnnotationOptions>
{
  const message = messageInfo.message;
  const yAxisAnnotations: Record<string, AnnotationOptions<'line'>> = {};

  const isLatencyMode = mode === MonitorMode.Latency;
  const zones = isLatencyMode ? 1 : message.getZonesCount();

  for (let zone = 0; zone < zones; ++zone) {
    const zoneName = getZoneName(message.padType, zone);
    const yScaleID = scaleToSelectedRange ? YAXIS_MULTI_IDS[zone] : YAXIS_SINGLE_ID;
    yAxisAnnotations['thresholdMinZoneAnnotation' + zone] = createYAnnotation(
      zone,
      'Min. Threshold (' + zoneName + ')',
      yScaleID,
      message.getThresholdMinPercent(zone),
      message.getThresholdMinPercent(zone));

    if (!isLatencyMode && (zone === 0 || message.padType === PadType.Drum)) {
      yAxisAnnotations['thresholdMaxZoneAnnotation' + zone] = createYAnnotation(
        zone,
        'Max. Threshold (' + zoneName + ')',
        yScaleID,
        message.getThresholdMaxPercent(zone),
        message.getThresholdMaxPercent(zone));
    }
  }

  if (message.getPadType() === PadType.Pedal) {
    getPedalYAnnotations(message, yAxisAnnotations);
  }

  return yAxisAnnotations;
}

function createYAnnotation(zone: number, name: string, yScaleID: string, min: number, max: number): AnnotationOptions<'line'> {
  return {
    type: 'line',
    yScaleID: yScaleID,
    yMin: min,
    yMax: max,
    borderColor: alpha(chartColors[zone], yAxisLineOpacity),
    borderDash: yAxisLineDash,
    borderWidth: yAxisLineWidth,
    drawTime: yAxisDrawTime,
    label: {
      backgroundColor: chartColors[zone],
      borderColor: yLabelBorderColor,
      borderWidth: yLabelBorderWidth,
      color: annotationTextColor,
      content: name,
      display: true,
      position: yLabelPosition
    }
  };
}

function getPedalYAnnotations(message: MonitorMessage, yAxisAnnotations: Record<string, AnnotationOptions>) {
  const minThreshold = message.getThresholdMinPercent(0);
  const maxThreshold = message.getThresholdMaxPercent(0);
  const almostClosedThreshold = message.getAlmostClosedThresholdPercent();
  const closedThreshold = message.getClosedThresholdPercent();
  const thresholdRange = maxThreshold - minThreshold;
  const almostClosedThresholdRelative = minThreshold + thresholdRange * almostClosedThreshold / 100;
  const closedThresholdRelative = minThreshold + thresholdRange * closedThreshold / 100;

  yAxisAnnotations['almostClosedAnnotation'] = {
    type: 'line',
    yScaleID: YAXIS_SINGLE_ID,
    yMin: almostClosedThresholdRelative,
    yMax: almostClosedThresholdRelative,
    borderColor: alpha(pedalThresholdColors.almostClosed, yAxisLineOpacity),
    borderDash: yAxisLineDash,
    borderWidth: yAxisLineWidth,
    drawTime: yAxisDrawTime,
    label: {
      backgroundColor: pedalThresholdColors.almostClosed,
      borderColor: yLabelBorderColor,
      borderWidth: yLabelBorderWidth,
      color: annotationTextColor,
      content: 'Almost Closed',
      display: true,
      position: 'end'
    }
  };

  yAxisAnnotations['closedAnnotation'] = {
    type: 'line',
    yScaleID: YAXIS_SINGLE_ID,
    yMin: closedThresholdRelative,
    yMax: closedThresholdRelative,
    borderColor: alpha(pedalThresholdColors.closed, yAxisLineOpacity),
    borderDash: yAxisLineDash,
    borderWidth: yAxisLineWidth,
    drawTime: yAxisDrawTime,
    label: {
      backgroundColor: pedalThresholdColors.closed,
      borderColor: yLabelBorderColor,
      borderWidth: yLabelBorderWidth,
      color: annotationTextColor,
      content: 'Closed',
      display: true,
      position: 'end'
    }
  };
}

function indexToTimestampMs(index: number, data: SignalChartData): number | undefined {
  return data.labels?.[index];
}
