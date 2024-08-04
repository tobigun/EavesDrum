// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { createTheme } from '@mui/material/styles';
import { red } from '@mui/material/colors';

export const chartTextColor = '#CCCCCC';

export const chartColors = ['rgb(0, 143, 251)', 'rgb(0, 227, 150)', 'rgb(225, 138, 37)'];

export const pedalThresholdColors = {
  closed: 'rgb(180, 48, 0, 0.8)',
  almostClosed: 'rgb(230, 169, 0, 0.8)',
};

export const groupColors = ["rgba(23, 188, 45, 0.6)", "rgb(135, 135, 135)", "rgb(142, 143, 43)"];

export const theme = createTheme({
  cssVariables: true,
  typography: {
    fontFamily: "'Open Sans', sans-serif",
  },
  palette: {
    text: {
      primary: 'rgb(255,255,255)',
      secondary: 'rgb(255,255,255)',
      disabled: 'rgb(255,255,255)',
    },
    background: {
      default:'rgb(236, 240, 241)',
      paper: 'rgb(68, 72, 87)'
    },        
    primary: {
      main: 'rgba(255,255,255,0.8)',
    },
    secondary: {
      main: 'rgba(255, 255, 255, 0.5)',
      light: 'rgb(255, 255, 255)',
      dark: 'rgba(255, 255, 255, 0.5)',
    },
    divider: 'rgba(255, 255, 255, 0.5)',
    error: {
      main: red.A400,
    },
  },
  components: {
    MuiButton: {
      styleOverrides: {
        outlined: {
          borderColor: 'rgba(255, 255, 255, 0.5)',
          "&:hover": {
            borderColor: 'rgb(255, 255, 255)',
          },
        }
      }
    }
  }
});