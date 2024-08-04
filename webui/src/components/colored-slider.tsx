// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { styled } from '@mui/material/styles';
import Slider, { SliderProps } from '@mui/material/Slider';

export type ColoredSliderProps = SliderProps & {
  trackColor?: string,
  trackBackground?: string
};
export const ColoredSlider = styled(Slider, {
  shouldForwardProp: (prop) => prop !== "trackColor" && prop !== "trackBackground"
})<ColoredSliderProps>(({ trackColor, trackBackground }) => ({
  '& .MuiSlider-track': {
    background: trackBackground,
    color: trackColor
  }
}));
