
// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

export interface SettingsConverter {
  fromConfig: (value: number) => number,
  toConfig: (value: number) => number
}

export const identityConverter: SettingsConverter = {
  fromConfig: value => value,
  toConfig: value => value
};

export function createFractionConverter(fraction: number): SettingsConverter {
  return {
    fromConfig: value => Number(Number(value / fraction).toFixed(1)),
    toConfig: value => Math.round(value * fraction)
  };
}
