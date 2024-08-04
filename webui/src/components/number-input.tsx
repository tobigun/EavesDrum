// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { NumberField } from '@base-ui-components/react/number-field';
import { styled } from '@mui/material';
import React, { useId } from 'react';

const backgroundColor = 'rgb(252 249 250)';
const borderColor = 'rgb(2 5 19 / 8%)';

const widthSmall = '1.5rem';
const heightSmall = '1.5rem';

const widthNormal = '2.5rem';
const heightNormal = '2.5rem';

const StyledNumberFieldRoot = styled(NumberField.Root)({
  display: 'flex',
  flexDirection: 'column',
  alignItems: 'start',
  gap: '0.25rem'
});

const StyledNumberFieldGroup = styled(NumberField.Group)({
  display: 'flex'
});

const StyledNumberFieldDecrement = styled(NumberField.Decrement)(({ theme }) => ({
  boxSizing: 'border-box',
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  width: widthSmall,
  height: heightSmall,
  margin: 0,
  outline: 0,
  padding: 0,
  border: `1px solid ${borderColor}`,
  borderRadius: '0.375rem',
  backgroundColor: backgroundColor,
  backgroundClip: 'padding-box',
  color: theme.palette.background.paper,
  userSelect: 'none',
          
  '@media (hover: hover)': {
    '&:hover': {
      backgroundColor: theme.palette.primary.main
    }
  },
          
  '&:active': {
    backgroundColor: theme.palette.primary.main
  },

  borderTopRightRadius: 0,
  borderBottomRightRadius: 0
}));

const StyledNumberFieldIncrement = styled(NumberField.Increment)(({ theme }) => ({
  boxSizing: 'border-box',
  display: 'flex',
  alignItems: 'center',
  justifyContent: 'center',
  width: widthSmall,
  height: heightSmall,
  margin: 0,
  outline: 0,
  padding: 0,
  border: `1px solid ${borderColor}`,
  borderRadius: '0.375rem',
  backgroundColor: backgroundColor,
  backgroundClip: 'padding-box',
  color: theme.palette.background.paper,
  userSelect: 'none',
            
  '@media (hover: hover)': {
    '&:hover': {
      backgroundColor: theme.palette.primary.main
    }
  },
            
  '&:active': {
    backgroundColor: theme.palette.primary.main
  },

  borderTopLeftRadius: 0,
  borderBottomLeftRadius: 0
}));

const StyledNumberFieldInput = styled(NumberField.Input)(({ theme }) => ({
  boxSizing: 'border-box',
  margin: 0,
  padding: 0,
  borderTop: `1px solid ${theme.palette.background.paper}`,
  borderBottom: `1px solid rgb(53, 53, 53)`,
  borderLeft: 'none',
  borderRight: 'none',
  width: '3rem',
  height: heightSmall,
  fontFamily: 'inherit',
  fontSize: '1rem',
  fontWeight: 'normal',
  backgroundColor: 'transparent',
  color: theme.palette.text.primary,
  
  textAlign: 'center',
  fontVariantNumeric: 'tabular-nums',
  
  '&:hover': {
    zIndex: 1,
    outline: `1px solid ${theme.palette.text.primary}`,
    outlineOffset: '-1px',
  },

  '&:focus': {
    zIndex: 1,
    outline: `2px solid ${theme.palette.secondary.main}`,
    outlineOffset: '-1px',
  }
}));
  
export function NumberInput(props: NumberField.Root.Props & {size?: string, width?: string}) {
  const id = useId();
  const [buttonWidth, height] = props.size === 'small' ? [widthSmall, heightSmall] : [widthNormal, heightNormal];
  return <StyledNumberFieldRoot id={id} allowWheelScrub={true} {...props}>
    <StyledNumberFieldGroup>
      <StyledNumberFieldDecrement sx={{ width: buttonWidth, height: height }}>
        <MinusIcon />
      </StyledNumberFieldDecrement>
      <StyledNumberFieldInput sx={{ width: props.width, height: height }}/>
      <StyledNumberFieldIncrement sx={{ width: buttonWidth, height: height }}>
        <PlusIcon />
      </StyledNumberFieldIncrement>
    </StyledNumberFieldGroup>
  </StyledNumberFieldRoot>;
}
  
function PlusIcon(props: React.ComponentProps<'svg'>) {
  return (
    <svg
      width="10"
      height="10"
      viewBox="0 0 10 10"
      fill="none"
      stroke="currentcolor"
      strokeWidth="1.6"
      xmlns="http://www.w3.org/2000/svg"
      {...props}
    >
      <path d="M0 5H5M10 5H5M5 5V0M5 5V10" />
    </svg>
  );
}
  
function MinusIcon(props: React.ComponentProps<'svg'>) {
  return (
    <svg
      width="10"
      height="10"
      viewBox="0 0 10 10"
      fill="none"
      stroke="currentcolor"
      strokeWidth="1.6"
      xmlns="http://www.w3.org/2000/svg"
      {...props}
    >
      <path d="M0 5H10" />
    </svg>
  );
}
