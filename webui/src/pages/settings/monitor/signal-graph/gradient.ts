// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { alpha } from "@mui/material";
import { ScriptableContext } from "chart.js";
import { chartColors } from "@theme";

let lastWidth: number;
let lastHeight: number;
let gradientCache: CanvasGradient[] = [];

export function getGradient(ctx: ScriptableContext<'line'>, datasetIndex: number) {
  const chartArea = ctx.chart.chartArea;
  if (datasetIndex === undefined || !chartArea) { // This case happens on initial chart load
    return;
  }

  validateCache(chartArea.width, chartArea.height);
  if (gradientCache[datasetIndex]) {
    return gradientCache[datasetIndex];
  }

  const drawContext = ctx.chart.ctx;

  const color = chartColors[datasetIndex];
  const gradient = drawContext.createLinearGradient(0, chartArea.bottom, 0, chartArea.top);
  gradient.addColorStop(1, alpha(color, 0.5));
  gradient.addColorStop(0.2, alpha(color, 0.5));
  gradient.addColorStop(0, alpha(color, 0.0));
  gradientCache[datasetIndex] = gradient;

  return gradient;
}

function validateCache(chartWidth: number, chartHeight: number) {
  if (lastWidth !== chartWidth || lastHeight !== chartHeight) {
    gradientCache = [];
    lastWidth = chartWidth;
    lastHeight = chartHeight;
  }
}