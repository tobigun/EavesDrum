// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { useCallback, useContext } from "react";
import { updateConfig, useConfig } from "@config";
import { connection, ConnectionStateContext } from "@connection";
import { SettingEntryContainer } from "./setting-entry";
import { Stack } from "@mui/material";
import { ColoredSlider, ColoredSliderProps } from "@components/colored-slider";
import { NumberInput } from "@components/number-input";
import { SettingsValueAccessor } from "./settings-pad";
import { useShallow } from "zustand/shallow";
import { identityConverter, SettingsConverter } from "./converter";

export interface SettingSliderProps {
  label: string,
  min?: number,
  max?: number,
  step?: number,
  convert?: SettingsConverter,
  sliderProps?: ColoredSliderProps,
  trackColor?: string,
  trackBackground?: string,
}

export interface SettingSliderEntryProps extends SettingSliderProps {
  padRole: string,
  valueAccessor: SettingsValueAccessor
}

// A specialized form that directly updates the config and sends config commands
export function SettingSliderEntry(props: SettingSliderEntryProps) {
  const valueAccessor = props.valueAccessor;
  const padRole = props.padRole;

  const settingIndices = [...Array(props.valueAccessor.count)]
    .map((_, index) => index);

  const settingValues = useConfig(useShallow(config =>
    settingIndices.map(index =>
      valueAccessor.getValue(config.settings[padRole], index))));

  const changeValues = useCallback((newValues: number[]) => {
    updateConfig(config => settingIndices.forEach(index => 
      valueAccessor.setValue(config.settings[padRole], newValues[index], index)));
  }, [padRole, valueAccessor, settingIndices]);
  
  const sendValues = useCallback((newValues: number[]) => {
    const settings: any = {};
    settingIndices.forEach(index => valueAccessor.setValue(settings, newValues[index], index));
    connection.sendSetPadSettingsCommand(padRole, settings);
  }, [padRole, valueAccessor, settingIndices]);
  
  return <SettingSlider {...props}
    count={valueAccessor.count}
    settingValues={settingValues}  
    changeValueFunc={changeValues}  
    commitValueFunc={sendValues}  
  />;
}

// Combined slider and Number-Field component. If settingValues is an array, multiple sliders are used.
export function SettingSlider({
  label,
  count,
  convert = identityConverter,
  min = 0,
  max = 100,
  step = 1,
  sliderProps = {},
  trackColor = undefined,
  trackBackground = undefined,
  settingValues,
  changeValueFunc,
  commitValueFunc
}: SettingSliderProps & {
  count: number,
  settingValues: any[],
  changeValueFunc: (newValues: number[]) => void,
  commitValueFunc: (newValues: number[]) => void,
})
{
  const settingIndices = [...Array(count)]
    .map((_, index) => index);

  const connected = useContext(ConnectionStateContext);
  
  const handleSliderChange = useCallback((_event: any, newValues: number | number[], activeThumb: number) => {
    newValues = convertToArray(newValues);
    if (isDuplicateValue(newValues, activeThumb)) {
      return; // reject value if another array entry has this value too
    }
    changeValueFunc(newValues); // change config but only send when committed
  }, [changeValueFunc]);
  
  const handleSliderChangeCommit = useCallback(() => {
    // send the last valid value
    commitValueFunc(settingValues);
  }, [settingValues, commitValueFunc]);
  
  const handleValueChange = useCallback((inputIndex: number, newValue: number) => {
    const isValid = (step === 1 && Number.isInteger(newValue)) || (step !== 1 && Number.isFinite(newValue));
    if (!isValid) {
      return;
    }

    newValue = Math.max(min, Math.min(newValue, max));
    const newSettingValues = [...settingValues];
    newSettingValues[inputIndex] = convert.toConfig(newValue);
    if (isDuplicateValue(newSettingValues, inputIndex)) {
      return; // reject value if another array entry has this value too
    }

    // make sure array is ordered by value
    if (newSettingValues.length == 2 && newSettingValues[0] > newSettingValues[1]) {
      newSettingValues.reverse();
    }

    // NumberField creates an event for focus in/out although the value did not change -> filter them out
    const hasChanged = newSettingValues.some((newValue, index) => newValue !== settingValues[index]);
    if (hasChanged) {
      changeValueFunc(newSettingValues);
      commitValueFunc(newSettingValues);  
    }
  }, [settingValues, changeValueFunc, commitValueFunc, min, max, step, convert]);

  return (
    <SettingEntryContainer name={label}>
      <ColoredSlider {...sliderProps} 
        disabled={!connected}
        // do not pass single slider value as array, otherwise it will be handled as a 0-length range
        value={(settingIndices.length == 1) ? settingValues[0] : settingValues}
        min={convert.toConfig(min)}
        max={convert.toConfig(max)}
        step={1}
        trackColor={trackColor}
        trackBackground={trackBackground}
        sx={{
          ...sliderProps?.sx,
          alignSelf: 'center'
        }}
        onChange={handleSliderChange}
        onChangeCommitted={handleSliderChangeCommit} />
      <Stack>
        {
          settingIndices.map(index => (
            <NumberInput key={label + index}
              disabled={!connected}
              value={convert.fromConfig(settingValues[index])}
              min={min}
              max={max}
              step={step}
              smallStep={1} // no matter if step is 0.1 or 1, use 1 if ALT/META key is pressed
              size='small'
              onValueChange={value => value !== null && handleValueChange(index, value)}
            />
          ))
        }
      </Stack>
    </SettingEntryContainer>
  );
}

function isDuplicateValue(values: number[], changedIndex: number): boolean {
  const newValue = values[changedIndex];
  return values
    .filter((_, index) =>  index != changedIndex)
    .some(value => value === newValue);
}

function convertToArray(value: number | number[]): number[] {
  return Array.isArray(value) ? value : Array.of(value);
}
