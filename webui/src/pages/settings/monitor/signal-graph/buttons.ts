// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { Chart, ChartEvent, Plugin } from "chart.js";
import { isEventInRect } from "@/common";
import { chartTextColor } from "@theme";

class ChartButton {
  text: string;
  x = 0;
  y = 0;
  width = 60;
  height = 20;
  onClick: (chart: Chart) => boolean;
  hovered = false;
  enabled = false;
  
  constructor(text: string, onClick: ((chart: Chart) => boolean) = () => false) {
    this.text = text;
    this.onClick = onClick;
  }
};
  
const resetZoomButton = new ChartButton('Reset Zoom', (chart: Chart) => {
  chart.resetZoom();
  return true;
});
  
export const yZoomButton = new ChartButton('Zoom Selected Y-Range');
  
const chartButtons: ChartButton[] = [
  resetZoomButton,
  yZoomButton
];

function isChartEmpty(chart: Chart) {
  return (chart.data.labels?.length ?? 0) === 0;
} 

export const ButtonsPlugin: Plugin = {
  id: 'buttonsPlugin',  
  beforeDraw: (chart: Chart) => {
    if (isChartEmpty(chart)) {
      return;
    }
  
    const chartArea = chart.chartArea;
    let xOffset = chartArea.left;
    chartButtons.forEach(button => {
      button.x = xOffset;
      button.y = (chartArea.top - button.height) / 2;
      drawButton(button, chart.ctx);
      xOffset += button.width + 10;
    });
  },
  afterEvent: (chart: Chart, args: { event: ChartEvent, replay: boolean, changed?: boolean }) => {
    if (isChartEmpty(chart)) {
      return;
    }

    chartButtons.forEach(button => {
      if (args.event.type === 'click') {
        if (isEventInButton(args.event, button)) {
          const changed = button.onClick(chart);
          if (changed) {
            args.changed = true;
          }
        }
      } else if (args.event.type === 'mousemove') {
        const oldHovered = button.hovered;
        button.hovered = isEventInButton(args.event, button);
        if (oldHovered !== button.hovered) {
          args.changed = true;
        }
      }
    });
  }
};
  
function drawButton(button: ChartButton, ctx: CanvasRenderingContext2D) {
  ctx.save();
  
  ctx.font = '14px sans-serif';
  ctx.fillStyle = button.enabled ? 'white' : chartTextColor;
  ctx.textAlign = 'center';
  ctx.textBaseline = 'middle';
  const textWidth = ctx.measureText(button.text).width + 10;
  if (textWidth > button.width) {
    button.width = textWidth;
  }
  ctx.fillText(button.text, button.x + button.width / 2, button.y + button.height / 2);
  
  ctx.lineWidth = button.enabled ? 2 : 1;
  ctx.strokeStyle = button.hovered ? 'white' : chartTextColor;
  ctx.beginPath();
  ctx.roundRect(button.x, button.y, button.width, button.height, 2);
  ctx.stroke();
  
  ctx.restore();
}
  
function isEventInButton(event: ChartEvent, button: ChartButton) {
  return button
      && isEventInRect(event, button.x, button.y, button.width, button.height);
}
  