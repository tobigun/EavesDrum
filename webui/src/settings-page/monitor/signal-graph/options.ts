// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { ChartOptions, LegendItem, TooltipItem } from 'chart.js';
import { getAnnotations } from './annotations';
import { createScaleOptions } from './scales';
import { SignalChartData } from './signal-graph';
import { chartTextColor } from '../../../theme';
import { MonitorMessageInfo } from '../monitor-message';
import { MonitorMode } from '../monitor-mode';


export function createSignalGraphOptions(
  scaleToSelectedRange: boolean,
  data: SignalChartData,
  mode: MonitorMode,
  messageInfo?: MonitorMessageInfo
): ChartOptions<'line'> {
  const result: ChartOptions<'line'> = {
    ...signalGraphOptions
  };
  
  if (!messageInfo || messageInfo.message.history.length === 0) {
    return result;
  }

  result.scales = createScaleOptions(messageInfo, scaleToSelectedRange, data.labels!, mode);
  
  result.plugins = {
    ...signalGraphOptions.plugins,
    annotation: {
      annotations: getAnnotations(messageInfo, data, mode, scaleToSelectedRange)
    }
  };
      
  return result;
}

const hiddenLegendPrefix = 'hidden';

export const signalGraphOptions: ChartOptions<'line'> = {
  animation: false,
  normalized: true,
  responsive: true,
  maintainAspectRatio: false,
  color: chartTextColor,
  backgroundColor: '#f3f3f3',
  borderColor: "#535A6C",
  datasets: {
    line: {
      pointRadius: 0, // disable for all 'line' datasets
      tension: 0.1
    }
  },
  events: ['mousemove', 'mouseout', 'click', 'touchstart', 'touchmove', 'dblclick'],
  plugins: {
    filler: {
      propagate: false,
    },
    legend: {
      position: 'top',
      align: 'end',
      labels: {
        filter: (item: LegendItem) => !item.text.startsWith(hiddenLegendPrefix)
      }
    },
    tooltip: {
      mode: 'index',
      intersect: false,
      filter: (tooltipItem: TooltipItem<'line'>) => !tooltipItem.dataset.label?.startsWith(hiddenLegendPrefix),
      callbacks: {
        title(tooltipItems: TooltipItem<'line'>[]) {
          return [ Number(tooltipItems[0].parsed.x).toFixed(1) + " ms" ];
        },
        label: function (tooltipItem: TooltipItem<'line'>) {
          return tooltipItem.dataset.label + ': ' + tooltipItem.parsed.y.toFixed(1) + "%";
        },
      },
    },
    zoom: {
      limits: {
        x: {min: 'original', max: 'original'},
        y: {min: 'original', max: 'original'},
      },
      zoom: {
        wheel: {
          enabled: false,
        },
        drag: {
          enabled: true,
          backgroundColor: 'rgba(150, 150, 150, 0.3)'
        },
        pinch: {
          enabled: true
        },
        mode: 'x',
      }
    }
  },
  scales: {
    x: { // fallback config
      type: 'linear',
      display: false
    },
    y: { // fallback config
      type: 'linear',
      display: false
    }
  },
  transitions: {
    zoom: {
      animation: {
        duration: 0
      }
    }
  }
};
