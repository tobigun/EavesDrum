// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { Doughnut } from 'react-chartjs-2';
import { PadType } from '@config';
import { Chart as ChartJS, ChartData, ChartOptions, ArcElement, ChartDataset, Plugin } from 'chart.js';
import { chartColors } from '@theme';
import { MonitorMessageInfo } from './monitor-message';
import { getZoneName } from '@/common';

const maxCircumferenceDegrees = 270;
const nonValueBarColor = 'rgb(204, 204, 204, 0.2)';

ChartJS.register([ArcElement]);

const DatasetLabelsPlugin : Plugin = {
  id: 'datasetLabelsPlugin',
  beforeDatasetsDraw(chart: ChartJS<'doughnut'>) {
    const ctx = chart.ctx;
    const data = chart.config.data;
    const datasetCount = data.datasets.length;

    for (let i = 1; i < datasetCount; i += 2) { // ignore spacers at uneven indexes
      const meta = chart.getDatasetMeta(i).data[0];
      if (!meta) {
        continue;
      }
      
      const { innerRadius, outerRadius, circumference: circumferenceRadians } = meta.getProps(['innerRadius', 'outerRadius', 'circumference']);
  
      const label = data.labels?.[i];
      const value = chart.config.data.datasets[i].data[0] ?? '-';
      const radius = (outerRadius + innerRadius) / 2;
      const height = outerRadius - innerRadius;
      const fontSize = Math.min(height * 0.6, 20);
      
      const endAngleRadians = 2 * Math.PI * maxCircumferenceDegrees / 360;
      const gapAngleRadians = value === 0 ? 0 : 0.04;
      const startOffsetRadians = Math.PI / 2;
      const arcStartAngleRadians = circumferenceRadians - startOffsetRadians + gapAngleRadians;
      const arcEndAngleRadians = endAngleRadians - startOffsetRadians;

      ctx.save();
      ctx.translate(meta.x, meta.y);

      // draw dataset label
      ctx.font = `bold ${fontSize}px sans-serif`;
      ctx.fillStyle = 'white';
      ctx.textAlign = 'end';
      ctx.textBaseline = 'middle';
      ctx.fillText(`${label}: ${value.toFixed(1)}%`, -5, -radius);

      // draw background track
      if (arcStartAngleRadians < arcEndAngleRadians) {
        ctx.beginPath();
        ctx.strokeStyle = nonValueBarColor;
        ctx.lineWidth = height;
        ctx.arc(0, 0, radius, arcStartAngleRadians, arcEndAngleRadians);
        ctx.stroke();
      }

      ctx.restore();
    }
  }
};

export function HitGraph({messageInfo} : {
  messageInfo: MonitorMessageInfo,
}) {
  const hitGraphData = updateHitGraph(messageInfo);

  return <Doughnut options={hitGraphOptions} data={hitGraphData} plugins={[DatasetLabelsPlugin]} />;
}

function createSpacer(datasetIndex: number) : ChartDataset<'doughnut'> {
  const spaceBetweenBars = datasetIndex === 0 ? 0 : 0.2; // outer spacer does not have a width. It just prevents the chart from zooming in if < ~66%
  return {
    label: 'spacer' + datasetIndex, // required by react-chartjs to find old dataset on update (otherwise animation starts at 0)
    borderWidth: 0,
    backgroundColor: 'transparent',
    circumference: 360,
    data: [1], // dummy data
    weight: spaceBetweenBars
  };
}

function createDataset(datasetIndex: number, value: number, color: string) : ChartDataset<'doughnut'> {
  return {
    label: 'data' + datasetIndex, // required by react-chartjs to find old dataset on update (otherwise animation starts at 0)
    borderWidth: 0,
    backgroundColor: [color],
    circumference: maxCircumferenceDegrees * value / 100,
    data: [value],
  };
}

function updateHitGraph(messageInfo: MonitorMessageInfo): ChartData<'doughnut'> {
  const message = messageInfo.message;
  const labels = (message.getPadType() === PadType.Pedal)
    ? ['Pedal', 'Chick']
    : [...Array(message.getZonesCount())].map((_, index) => getZoneName(message.getPadType(), index));
  const datasetCount = labels.length;
  
  // Workaround: the border is drawn _over_ and not between the bars so we cannot make them transparent -> use a spacer instead
  [...Array(datasetCount)].map((_, index) => labels.splice(datasetCount - index - 1, 0, 'spacer' + index));
  
  const datasets = [];
  for (let datasetIndex = 0; datasetIndex < datasetCount; ++datasetIndex) {
    datasets.push(createSpacer(datasetIndex)); // Workaround: see above
    const value = message.getHitValuePercent(datasetIndex) ?? 0;
    datasets.push(createDataset(datasetIndex, value, chartColors[datasetIndex]));
  }

  return {
    labels: labels,
    datasets: datasets
  };
}

const hitGraphOptions: ChartOptions<'doughnut'> = {
  cutout: '50%',
  plugins: {
    legend: {
      display: false
    },
    tooltip: {
      enabled: false
    }
  },
};
