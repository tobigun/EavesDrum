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
}));

export function initConfig() {
  connection.registerOnJsonDataListener('config', config => {
    useConfig.setState(config, true);
  });  
}

type ConfigUpdateFunc = (config: Config) => void;
export function updateConfig(updater: ConfigUpdateFunc) {
  useConfig.setState(produce((config) => { updater(config); }));
}

export function getPadIndexByName(padName: string): number | undefined {
  const index = useConfig.getState().pads.findIndex(pad => pad.name === padName);
  return index == -1 ? undefined : index;
}

export function getPadByIndex(padIndex: number): DrumPadConfig {
  return useConfig.getState().pads[padIndex];
}

export function isPadPinConnectedToMux(pin: number, padIndex?: number): boolean {
  const connectorId = padIndex !== undefined ? getPadByIndex(padIndex)?.connector : undefined;
  const connector = connectorId !== undefined ? useConfig.getState().connectors[connectorId] : undefined;
  return (typeof connector?.pins[pin]) === 'object';
}

export interface Config {
  general: GeneralConfig;
  pads: DrumPadConfig[];
  connectors: Record<ConnectorId, ConnectorConfig>;
  mappings: Record<PadRole, DrumPadMappings>;
  monitor?: MonitorConfig;
  isDirty?: boolean; // false if not present
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
  pedal?: string; // name of the pedal pad
  connector?: ConnectorId;
  settings: DrumPadSettings;
}

export interface DrumPadMappings {
  name?: string;

  noteMain?: number;
  noteRim?: number;
  noteCup?: number;

  closedNotesEnabled?: boolean;
  noteCloseMain?: number;
  noteCloseRim?: number;
  noteCloseCup?: number;

  noteCross?: number;
}

export type DrumPadMappingValues = Omit<DrumPadMappings, "name">;
export type DrumMappingId = keyof DrumPadMappingValues;

type MappingTypeName<T> =
  T extends number ? "number" :
  T extends boolean ? "boolean" :
  never;

type MappingTypes = Required<{
  [K in keyof DrumPadMappingValues]: MappingTypeName<DrumPadMappingValues[K]>
}>;

export const mappingValuesTyoes: MappingTypes = {
  noteMain: "number",
  noteRim: "number",
  noteCup: "number",
  closedNotesEnabled: "boolean",
  noteCloseMain: "number",
  noteCloseRim: "number",
  noteCloseCup: "number",
  noteCross: "number",
};

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

export interface MonitorConfig {
  padIndex?: number,
  triggeredByAllPads?: boolean, // false if not present
  latencyTest?: boolean; // false if not present
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

export function getPadZonesCount(padIndex: number): number {
  return getZonesCount(useConfig.getState().pads[padIndex]?.settings.zonesType);
}