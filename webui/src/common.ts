// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { ChartEvent } from "chart.js";
import { PadType } from "./config/config";

export function isEventInRect(event: ChartEvent, x: number, y: number, width: number, height: number) {
  return (event.x !== null && event.x >= x && event.x <= x + width)
      && (event.y !== null && event.y >= y && event.y <= y + height);
}

export function getZoneName(padType: PadType, zone: number) {
  switch (padType) {
  case PadType.Drum: return ['Head', 'Rim', 'Side-Rim'][zone];
  case PadType.Cymbal: return ['Bow', 'Edge', 'Cup'][zone];
  case PadType.Pedal: return 'Pedal';
  }
}

export function getHeaderBackground(type: PadType) {
  switch (type) {
  case PadType.Drum: return 'radial-gradient(farthest-side at 30% top, rgba(255, 255, 255, 0.32) 20%, transparent 50%)';
  case PadType.Cymbal: return 'radial-gradient(farthest-side at 30% top, rgba(141, 143, 43, 0.4) 40%, transparent 70%)';
  case PadType.Pedal: return 'radial-gradient(farthest-side at 30% 30%, rgba(141, 143, 43, 0.4) 40%, transparent 70%)';
  };
}
