// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { create } from 'zustand';
import { produce } from 'immer';
import { connection } from './connection/connection';
import { VersionInfo } from './version';

export type PadRole = string;
export type ConnectorId = string;

export const MAX_SENSOR_VALUE = 1023;
export const MAX_GATE_TIME_MS = 30_000;

export const useConfig = create<Config>(() => ({
  general: { gateTimeMs: 0 },
  pads: [],
  connectors: {},
  mappings: {},
  settings: {},
  monitor: { triggeredByAllPads: false },
  isDirty: false,
  latencyTest: false
}));

export function initConfig() {
  connection.registerOnJsonDataListener('config', config => {
    useConfig.setState(config);
  });  
}

type ConfigUpdateFunc = (config: Config) => void;
export function updateConfig(updater: ConfigUpdateFunc) {
  useConfig.setState(produce((config) => { updater(config); }));
}

export function getPadIndexByRole(padRole?: PadRole): number | undefined {
  if (!padRole) {
    return undefined;
  }
  const index = useConfig.getState().pads.findIndex(pad => pad.role === padRole);
  return index == -1 ? undefined : index;
}

export function getPadSettingsByIndex(padIndex: number): DrumPadSettings {
  const role = useConfig.getState().pads[padIndex].role;
  return useConfig.getState().settings[role];
}

export function getPadByIndex(padIndex?: number): DrumPadConfig | undefined {
  return padIndex !== undefined ? useConfig.getState().pads[padIndex] : undefined;
}

export function isPadPinConnectedToMux(pin: number, padIndex?: number): boolean {
  const connectorId = getPadByIndex(padIndex)?.connector;
  const connector = connectorId !== undefined ? useConfig.getState().connectors[connectorId] : undefined;
  return (typeof connector?.pins[pin]) === 'object';
}

export interface Config {
  general: GeneralConfig;
  pads: DrumPadConfig[];
  connectors: Record<ConnectorId, ConnectorConfig>;
  mappings: Record<PadRole, DrumPadMappings>;
  settings: Record<PadRole, DrumPadSettings>;
  monitor: {padIndex?: number, triggeredByAllPads: boolean};
  latencyTest: boolean;
  isDirty: boolean;
  version?: VersionInfo;
}

export interface GeneralConfig {
  gateTimeMs: number; // 0 .. MAX_GATE_TIME_MS
}

export interface ConnectorConfig {
  pins: (MuxPinConfig | number)[];
}

export interface MuxPinConfig {
  mux: number;
  channel: number;
}

export interface DrumPadConfig {
  name: string;
  role: PadRole;
  group: string;
  enabled: boolean;
  autoCalibrate: boolean;
  pedal?: PadRole;
  connector?: ConnectorId;
}

export interface DrumPadMappings {
  noteMain?: number;
  noteRim?: number;
  noteCup?: number;

  closedNotesEnabled?: boolean;
  noteCloseMain?: number;
  noteCloseRim?: number;
  noteCloseCup?: number;

  noteCross?: number;

  pedalChickEnabled?: boolean;
}

export interface DrumPadSettings {
  padType: PadType;
  zonesType: ZonesType;
  chokeType: ChokeType;
  zoneThresholds: {
    min: number, // [0 .. MAX_SENSOR_VALUE]
    max: number, // [0 .. MAX_SENSOR_VALUE]
  }[],
  scanTimeUs?: number;
  maskTimeMs?: number;
  curveType: keyof typeof CurveType;
  almostClosedThreshold?: number; // %
  closedThreshold?: number; // %
  moveDetectTolerance?: number; // [0 .. MAX_SENSOR_VALUE]
  chickDetectTimeoutMs?: number;
  headRimBias: number; // -100 .. 100
  crossNoteEnabled?: boolean;
}

export enum PadType {
  Drum = "Drum",
  Cymbal = "Cymbal",
  Pedal = "Pedal"
}

export enum ZonesType {
  // Zones1_Switch, // TODO
  Zones1_Controller = 'Zones1_Controller',
  Zones1_Piezo = 'Zones1_Piezo',
  Zones2_Piezos = 'Zones2_Piezos',
  Zones2_PiezoAndSwitch = 'Zones2_PiezoAndSwitch',
  Zones3_Piezos = 'Zones3_Piezos',
  Zones3_PiezoAndSwitches_2TRS = 'Zones3_PiezoAndSwitches_2TRS', // e.g Roland cymbals
  Zones3_PiezoAndSwitches_1TRS = 'Zones3_PiezoAndSwitches_1TRS', // shared pin for edge and cup switch, with 10k resistor at edge switch (e.g. Yamaha cymbals)
}

export enum ChokeType {
  None = 'None',
  Switch_Edge = 'Switch_Edge',
  Switch_Cup = 'Switch_Cup',
  //TouchSensor // TODO: needs additional pin
}

export enum CurveType {
  Linear = "Linear",
  Exp1 = "Exp1",
  Exp2 = "Exp2",
  Log1 = "Log1",
  Log2 = "Log2"
}

export function getZonesCount(zonesType: ZonesType): number {
  switch(zonesType) {
  case ZonesType.Zones1_Controller:
  case ZonesType.Zones1_Piezo:
    return 1;
  case ZonesType.Zones2_Piezos:
  case ZonesType.Zones2_PiezoAndSwitch:
    return 2;
  case ZonesType.Zones3_Piezos:
  case ZonesType.Zones3_PiezoAndSwitches_1TRS:
  case ZonesType.Zones3_PiezoAndSwitches_2TRS:
    return 3;
  }
}

export function getZonesCountByRole(padRole: PadRole): number {
  return getZonesCount(useConfig.getState().settings[padRole]?.zonesType);
}