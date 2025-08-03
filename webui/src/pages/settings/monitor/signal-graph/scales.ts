// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { ScaleOptions, ScriptableScaleContext, Tick } from "chart.js";
import { lighten } from "@mui/material";
import { PadType } from "@config";
import { chartColors, chartTextColor } from "@theme";
import { MonitorMessageInfo } from "../monitor-message";
import { MonitorMode } from "../monitor-mode";

export const YAXIS_SINGLE_ID = 'y';
export const YAXIS_MULTI_IDS = ['y', 'y1', 'y2'];

const MAX_SENSOR_VALUE = 1023;

export function createScaleOptions(
  messageInfo: MonitorMessageInfo,
  scaleToSelectedRange: boolean,
  labels: number[],
  mode: MonitorMode
): Record<string, ScaleOptions<'linear'>>
{
  const scales: Record<string, ScaleOptions<'linear'>> = {
    x: {
      ...xAxisScaleOptions,
      suggestedMin: (mode === MonitorMode.Latency) ? undefined : labels[0], // resize x-range if latency-annotation is to far in the past
      suggestedMax: labels[labels.length - 1],
    }
  };

  if (scaleToSelectedRange) {
    const message = messageInfo.message;
    const zones = message.getZonesCount();
    for (let i = 0; i < zones; ++i) {
      scales[YAXIS_MULTI_IDS[i]] = {
        ...yAxisMultiScalesOptions[i],
        min: Math.floor(100 * message.thresholdsMin[i] / MAX_SENSOR_VALUE),
        max: (message.getPadType() === PadType.Drum || i === 0) ? Math.ceil(100 * message.thresholdsMax[i] / MAX_SENSOR_VALUE) : 100,
      };
    }    
  } else {
    scales.y = {
      ...yAxisSingleScaleOptions
    };
  }

  return scales;
};

const xAxisScaleOptions: ScaleOptions<'linear'> = {
  beginAtZero: true,
  type: 'linear',
  title: {
    color: chartTextColor,
    display: true,
    font: { weight: 'bold' },
    text: 'Time (ms)',
    padding: { top: 1, bottom: 0 }
  },
  grid: {
    display: true,
    lineWidth: (ctx: ScriptableScaleContext) => {
      return ctx.tick?.value % 5 == 0 ? 1 : 0;
    },
    tickWidth: 1,
    tickLength: 5,
    tickColor: chartTextColor,
  },
  ticks: {
    precision: 1,
    stepSize: 0.5,
    color: chartTextColor,
    includeBounds: false,
    autoSkip: true,
    major: {enabled: true },
    maxTicksLimit: 40,
    callback: (tickValue: number | string, index: number, ticks: Tick[]) => {
      if (tickValue === 0) {
        ticks[index].major = true;
      }
      return tickValue;
    },
  },
};

const yAxisScaleOptionsBase: ScaleOptions<'linear'> = {
  grid: {
    tickColor: chartTextColor,
    tickLength: 4
  },
  ticks: {
    stepSize: 10,
    callback: function (value) {
      return value + '%';
    },
  },
  title: {
    display: false,
    color: chartTextColor,
    text: 'Signal'
  },
};

const yAxisSingleScaleOptions: ScaleOptions<'linear'> = {
  ...yAxisScaleOptionsBase,
  beginAtZero: true,
  min: 0,
  max: 100,
  ticks: {
    ...yAxisScaleOptionsBase.ticks,
    color: chartTextColor,
  },
  title: {
    ...yAxisScaleOptionsBase.title,
    display: true
  },
};

const yAxisMultiScalesOptions: ScaleOptions<'linear'>[] = [...Array(3)].map((_, zone) =>  ({
  ...yAxisScaleOptionsBase,
  ticks: {
    ...yAxisScaleOptionsBase.ticks,
    color: lighten(chartColors[zone], 0.3),
  },
  title: {
    ...yAxisScaleOptionsBase.title,
    display: zone === 0
  },
  border: { color: chartColors[zone] },
  position: zone === 0 ? 'left' : 'right'
}));
