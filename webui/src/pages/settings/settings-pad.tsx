// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { Stack } from "@mui/material";
import { DrumPadSettings, PadRole, PadType, useConfig } from "@config";
import { SettingSliderEntry, SettingSliderEntryProps } from "./setting-slider";
import { useShallow } from "zustand/shallow";
import { chartColors, pedalThresholdColors } from "@theme";
import { SettingCurveTypeEntry } from "./setting-curve";
import { getZoneName } from "@/common";
import { SettingEnableEntry } from "./setting-enable";
import { createFractionConverter, SettingsConverter } from "./converter";

const settingIdsOrdered: (keyof DrumPadSettings)[] = [
  'zoneThresholds',
  'curveType',
  'scanTimeUs',
  'maskTimeMs',
  'moveDetectTolerance',
  'almostClosedThreshold',
  'closedThreshold',
  'chickDetectTimeoutMs',
  'headRimBias',
  'crossNoteEnabled',
];

export interface SettingsValueAccessor {
  count: number,
  getValue: (padSettings: Partial<DrumPadSettings>, index?: number) => any,
  setValue: (padSettings: Partial<DrumPadSettings>, value: any, index?: number) => Partial<DrumPadSettings>
}

type SettingDisplayNames = Record<string, (padType: PadType, zone: number) => string>;
const settingDisplayNames: SettingDisplayNames = {
  padType: () => 'Type',
  zoneThresholdsMinMax: (padType, zone) => getZoneName(padType, zone) + ' Range [%]',
  zoneThreshold: (padType, zone) => getZoneName(padType, zone) + ' Threshold (min) [%]',
  scanTimeUs: () => 'Scan-Time [ms]',
  maskTimeMs: () => 'Mask-Time [ms]',
  curveType: () => 'Curve Type',
  almostClosedThreshold: () => 'Almost / Closed Threshold [%]',
  moveDetectTolerance: () => 'Move Detection Tolerance [%]',
  chickDetectTimeoutMs: () => 'Chick Detect Timeout [ms]',
  headRimBias: () => 'Head/Rim Bias [%]',
  crossNoteEnabled: () => 'Enable Cross-Stick Detection',
};

function getDisplayName(settingId: string, padType: PadType, zone = 0) {
  return settingDisplayNames[settingId]?.(padType, zone);
}
    
export function SettingsElements({ padRole, padType }: {
  padRole: string,
  padType: PadType
}) {
  const settingIds = useConfig(useShallow(config => Object.keys(config.settings[padRole])));
  return <Stack gap={1}>
    {
      settingIdsOrdered
        .filter(settingId => settingIds.includes(settingId))
        .map(settingId => <SettingsElement key={settingId} padRole={padRole} settingId={settingId} padType={padType} />)
    }
  </Stack>;
}

const thresholdsConverter: SettingsConverter = {
  fromConfig: value => Number(Number(value * 100 / 1023).toFixed(1)),
  toConfig: value => Math.round(value * 1023 / 100)
};

export function createThresholdSliderProps(): Omit<Partial<SettingSliderEntryProps>, 'valueAccessor'> {
  return {
    convert: thresholdsConverter,
    step: 0.1,
    sliderProps: {
      valueLabelDisplay: 'auto',
      valueLabelFormat: (value, index) => (index === 0 ? 'Min' : 'Max') + ": "
        + thresholdsConverter.fromConfig(value).toFixed(1) + "% "
        + "(" + value + ")"
    }
  };
};

function createValueAccessorBySettingIds(settingIds: keyof DrumPadSettings | (keyof DrumPadSettings)[]): SettingsValueAccessor {
  settingIds = Array.isArray(settingIds) ? settingIds : Array.of(settingIds);
  return {
    count: settingIds.length,
    getValue: (padSettings, index = 0) => padSettings[settingIds[index]],
    setValue: (padSettings, value, index = 0) => { padSettings[settingIds[index]] = value; return padSettings; },
  };
}

function createZoneThresholdAccessor(zone: number, hasMax: boolean): SettingsValueAccessor {
  return {
    count: hasMax ? 2 : 1,
    getValue: (padSettings, index = 0) =>
      padSettings.zoneThresholds?.[zone][index === 0 ? 'min' : 'max'],
    setValue: (padSettings, value, index = 0) => {
      if (!padSettings.zoneThresholds) {
        padSettings.zoneThresholds = [];
      }
      padSettings.zoneThresholds[zone] = {
        ...padSettings.zoneThresholds[zone],
        [index === 0 ? 'min' : 'max']: value
      };
      return padSettings;
    }
  };
}

function SettingsElement({padRole, settingId, padType} : {
  padRole: string,
  settingId: keyof DrumPadSettings,
  padType: PadType
}) {
  const defaultProps = {
    padRole: padRole,
    label: getDisplayName(settingId, padType),
    valueAccessor: createValueAccessorBySettingIds(settingId)
  };
  
  switch (settingId) {
  case "zoneThresholds":
    return <ZoneThresholdsSliderElements key={settingId} padRole={padRole} padType={padType} />;

  case "scanTimeUs":
    return <SettingSliderEntry {...defaultProps} key={settingId}
      max={20} step={0.1}
      convert={createFractionConverter(1000)} />;
  
  case "maskTimeMs":
    return <SettingSliderEntry {...defaultProps} key={settingId} />;
  
  case "curveType":
    return <SettingCurveTypeEntry {...defaultProps} key={settingId} />;
  
  case "moveDetectTolerance":
    return <SettingSliderEntry {...defaultProps} key={settingId}
      {...createThresholdSliderProps()} max={20} />;
    
  case "almostClosedThreshold": // also: closedThreshold
    return <AlmostClosedThresholdSliderEntry padRole={padRole} key={settingId}
      {...createThresholdSliderProps()} />;
    
  case "chickDetectTimeoutMs":
    return <SettingSliderEntry {...defaultProps} key={settingId} />;

  case "headRimBias":
    return <SettingSliderEntry {...defaultProps} key={settingId} min={-100} max={100} />;

  case "crossNoteEnabled":
    return <SettingEnableEntry {...defaultProps} key={settingId} />;
  }
}

function ZoneThresholdsSliderElements({padRole, padType} : {
  padRole: PadRole,
  padType: PadType
}) {
  const zoneThresholds = useConfig(config => config.settings[padRole].zoneThresholds);
  return zoneThresholds.map((zoneThreshold, zone) => {
    const hasMax = zoneThreshold.max !== undefined;
    const displayName = getDisplayName(hasMax ? 'zoneThresholdsMinMax' : 'zoneThreshold', padType, zone);
    return <SettingSliderEntry {...createThresholdSliderProps()} key={zone}
      padRole={padRole}
      label={displayName}
      valueAccessor={createZoneThresholdAccessor(zone, hasMax)}
      trackColor={chartColors[zone]} />;
  });
}

function AlmostClosedThresholdSliderEntry({padRole} : {padRole: PadRole}) {
  const valueAccessor = createValueAccessorBySettingIds(['almostClosedThreshold', 'closedThreshold']);
  return <SettingSliderEntry
    padRole={padRole}
    key='pedalStateThresholds'
    label='Pedal State Thresholds [%]'
    sliderProps={{
      track: 'normal', sx: {
        '& [data-index="0"]': { backgroundColor: pedalThresholdColors.almostClosed },
        '& [data-index="1"]': { backgroundColor: pedalThresholdColors.closed }
      },
      valueLabelDisplay: 'auto',
      valueLabelFormat: (_value, index) => index === 0 ? 'Almost Closed (Used for Chick Detection)' : 'Closed'
    }}
    valueAccessor={valueAccessor}
    trackBackground={`linear-gradient(to right, ${pedalThresholdColors.almostClosed}, ${pedalThresholdColors.closed})`}
  />;
}
