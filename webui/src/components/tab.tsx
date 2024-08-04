// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { PropsWithChildren } from 'react';

import { styled } from '@mui/material/styles';
import Box from '@mui/material/Box';
import MuiTabs from '@mui/material/Tabs';
import MuiTab from '@mui/material/Tab';

export const Tabs = styled(MuiTabs)({
  //borderBottom: '1px solid rgb(0, 0, 0)',
  //backgroundColor: 'rgba(0, 0, 0, 0.31)',
  '& .MuiTabs-indicator': {
    backgroundColor: '#000000',
  },
});
  
export const Tab = styled(MuiTab)({
  color: 'rgb(0, 0, 0)',
  //backgroundColor: 'rgb(208, 255, 0)',
  '&.Mui-selected': {
    color: 'rgb(255, 255, 255)',
    backgroundColor: 'rgba(0, 0, 0, 0.51)',
  },
  '&.Mui-focusVisible': {
    backgroundColor: 'rgb(228, 95, 128)',
  },
});
  
  interface TabContentPageProps {
      index: number;
      value: number;
  }
export function TabContentPage({ children, value, index, ...other }: PropsWithChildren<TabContentPageProps>) {
  return (
    <div
      role="tabpanel"
      hidden={value !== index}
      {...other}
    >
      {value === index && <Box sx={{ padding: 0 }}>{children}</Box>}
    </div>
  );
}
  