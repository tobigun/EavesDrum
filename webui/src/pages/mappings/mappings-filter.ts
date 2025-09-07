// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { DrumPadConfig, DrumMappingId, PadType, getZonesCount, DrumPadMappingValues } from "@/config";

type MappingConstraints = {
  [K in keyof DrumPadMappingValues]: { requiredZones: number };
};

const mappingItemsDrum: MappingConstraints = {
  noteMain: { requiredZones: 1 },
  noteRim: { requiredZones: 2 },
  noteCross: { requiredZones: 2 },
};

const mappingItemsCymbal: MappingConstraints = {
  noteMain: { requiredZones: 1 },
  noteRim: { requiredZones: 2 },
  noteCup: { requiredZones: 3 },
};

const mappingItemsHiHat: MappingConstraints = {
  closedNotesEnabled: { requiredZones: 1 },
  noteMain: { requiredZones: 1 },
  noteCloseMain: { requiredZones: 1 },
  noteRim: { requiredZones: 2 },
  noteCloseRim: { requiredZones: 2 },
  noteCup: { requiredZones: 3 },
  noteCloseCup: { requiredZones: 3 },
};

const mappingItemsPedalControl: MappingConstraints = {
  pedalChickEnabled: { requiredZones: 1 },
  noteMain: { requiredZones: 1 },
};


export function getSupportedMappingIds(pad: DrumPadConfig): DrumMappingId[] {
  const zoneCount = getZonesCount(pad.settings.zonesType);
  const mappingInfos = getPadMappingInfo(pad);
  const mappingIds = Object.keys(mappingInfos) as DrumMappingId[];

  return mappingIds.filter(mappingId => {
    return mappingInfos[mappingId]!.requiredZones <= zoneCount;
  });
}

function getPadMappingInfo(pad: DrumPadConfig): MappingConstraints {
  switch (pad.settings.padType) {
  case PadType.Drum:
    return mappingItemsDrum;
  case PadType.Cymbal:
    if (pad.pedal) { // HiHat
      return mappingItemsHiHat;
    } else {
      return mappingItemsCymbal;
    }
  case PadType.Pedal:
    return mappingItemsPedalControl;
  }
}