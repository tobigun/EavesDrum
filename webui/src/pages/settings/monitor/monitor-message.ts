// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { ChokeType, getZonesCount, PadType, ZonesType } from "@config";

const uint8_size = 1;
const uint16_size = 2;
const uint32_size = 4;

export class HistoryEntry {
  timeUntilPreviousUs: number;
  isGap: boolean;
  values: Uint8Array;
  constructor(view: DataView) {
    let offset = 0;
    this.timeUntilPreviousUs = view.getUint16(offset, true);
    offset += uint16_size;
    this.isGap = view.getUint8(offset) != 0;
    offset += uint8_size;
    this.values = new Uint8Array(view.buffer, view.byteOffset + offset, 3);
  }
}

export class MonitorMessage {
  padIndex: number;
  velocities: Int8Array;
  hits: Uint8Array;
  thresholdsMin: Uint16Array;
  thresholdsMax: Uint16Array;
  triggerStartIndex?: number;
  triggerEndIndex?: number;
  latencyUs: number;
  isChoked: boolean;
  padType: PadType;
  zonesType: ZonesType;
  chokeType: ChokeType;
  history: HistoryEntry[];

  constructor(data: ArrayBuffer) {
    const view = new DataView(data);

    let offset = 0;
    this.padIndex = view.getUint8(offset);
    offset += uint8_size;

    this.velocities = new Int8Array(data, offset, 3);
    offset += 3 * uint8_size;
    this.hits = new Uint8Array(data, offset, 3);
    offset += 3 * uint8_size;
    this.isChoked = view.getUint8(offset) != 0;
    offset += uint8_size;
    this.thresholdsMin = new Uint16Array(data, offset, 3);
    offset += 3 * uint16_size;
    this.thresholdsMax = new Uint16Array(data, offset, 3);
    offset += 3 * uint16_size;

    const hitIndex = view.getInt16(offset, true);
    this.triggerStartIndex = hitIndex !== -1 ? hitIndex : undefined;
    offset += uint16_size;
    const scanEndIndex = view.getInt16(offset, true);
    this.triggerEndIndex = scanEndIndex !== -1 ? scanEndIndex : undefined;
    offset += uint16_size;
    this.latencyUs = view.getUint32(offset, true);
    offset += uint32_size;

    this.padType = Object.values(PadType)[view.getUint8(offset)];
    offset += uint8_size;
    this.zonesType = Object.values(ZonesType)[view.getUint8(offset)];
    offset += uint8_size;
    this.chokeType = Object.values(ChokeType)[view.getUint8(offset)];
    offset += uint8_size;

    this.history = [];
    while (offset < data.byteLength) {
      this.history.push(new HistoryEntry(new DataView(data, offset)));
      offset += uint16_size + uint8_size + uint8_size * 3;
    }
  }

  getHitValuePercent(index: number): number | undefined {
    return this.hits[index] ? (this.velocities[index] * 100 / 127) : undefined;
  }

  convertThresholdToPercent(threshold: number): number {
    return Math.round(threshold * 100 / 1023);
  }

  getThresholdMinPercent(index: number): number {
    return this.convertThresholdToPercent(this.thresholdsMin[index]);
  }

  getThresholdMaxPercent(index: number): number {
    return this.convertThresholdToPercent(this.thresholdsMax[index]);
  }

  getAlmostClosedThresholdPercent(): number {
    return this.thresholdsMin[1];
  }

  getClosedThresholdPercent(): number {
    return this.thresholdsMax[1];
  }

  getPadType(): PadType {
    return this.padType;
  }

  getZonesCount(): number {
    return getZonesCount(this.zonesType);
  }
}

export class MonitorMessageInfo {
  static drumMessagesNextId = 0;
  
  id: number;
  timestampMs: number;
  message: MonitorMessage;
  
  constructor(message: MonitorMessage) {
    this.message = message;
    this.id = MonitorMessageInfo.drumMessagesNextId++;
    this.timestampMs = new Date().getTime();
  }
}
