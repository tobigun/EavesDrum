// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { Chart, ChartArea, Plugin } from "chart.js";
import { yZoomButton } from "./buttons";

export const ChartBackgroundPlugin: Plugin = {
  id: 'chartBackground',
  beforeDraw: (chart: Chart) => {
    const ctx = chart.ctx;
    const chartArea = chart.chartArea;
    const labels = chart.data.labels;
    const isChartEmpty = (labels?.length ?? 0) === 0;
  
    ctx.save();
    drawGridBackground(ctx, chartArea);
    if (isChartEmpty) {
      drawNoDataMessage(ctx, chartArea, "Signal chart not available");
    }
    ctx.restore();
  }
};
  
function drawNoDataMessage(ctx: CanvasRenderingContext2D, chartArea: ChartArea, message: string) {
  const x = chartArea.left + chartArea.width/2;
  const y = chartArea.top + chartArea.height/2;
  
  ctx.font = `bold 40px sans-serif`;
  ctx.fillStyle = 'rgba(0, 0, 0, 0.3)';
  ctx.textAlign = 'center';
  ctx.textBaseline = 'middle';
  
  ctx.fillText(message, x, y);
}
  
function drawGridBackground(ctx: CanvasRenderingContext2D, chartArea: ChartArea) {
  const numRows = 10;
  const colorEven = 'rgba(243, 243, 243, 0.89)';
  const colorUneven = 'rgba(255, 255, 255, 0.89)';
  
  if (yZoomButton.enabled) {
    ctx.fillStyle = colorUneven;
    ctx.fillRect(chartArea.left, chartArea.top, chartArea.width, chartArea.height);
  } else {
    const segmentHeight = chartArea.height / numRows;
    let y = chartArea.top;
    for (let row = 0; row < numRows; row++) {
      ctx.fillStyle = (row % 2 == 0) ? colorEven : colorUneven;
      ctx.fillRect(chartArea.left, y, chartArea.width, segmentHeight);
      y += segmentHeight;
    }  
  }
}
  