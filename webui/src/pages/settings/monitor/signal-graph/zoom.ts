// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { Chart, ChartEvent, Plugin } from "chart.js";
import { isEventInRect } from "@/common";

export const ZoomOutOnDoubleClickPlugin: Plugin = {
  id: 'zoomOutOnDoubleClick',
  afterEvent: (chart: Chart, args: { event: ChartEvent, replay: boolean, changed?: boolean }) => {
    if (args.event.type === 'dblclick') {
      const chartArea = chart.chartArea;
      if (!args.replay // prevent 'too much recursion' exception
            && isEventInRect(args.event, chartArea.left, chartArea.top, chartArea.width, chartArea.height)) {
        chart.resetZoom();
        args.changed = true;
      }
    }
  }
};
  