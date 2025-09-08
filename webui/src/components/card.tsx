// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { PropsWithChildren, ReactNode, useRef, useState } from 'react';

import { styled } from '@mui/material/styles';
import AccordionDetails from '@mui/material/AccordionDetails';
import MuiAccordion, { AccordionProps } from '@mui/material/Accordion';
import MuiAccordionSummary, { AccordionSummaryProps, accordionSummaryClasses } from '@mui/material/AccordionSummary';
import Stack from '@mui/material/Stack';
import ToggleButton, { ToggleButtonProps } from '@mui/material/ToggleButton';
import Typography from '@mui/material/Typography';

import ArrowForwardIcon from '@mui/icons-material/ArrowForwardIosSharp';
import RenameIcon from "@mui/icons-material/Edit";
import RenameDoneIcon from "@mui/icons-material/Done";
import RenameCancelIcon from "@mui/icons-material/Close";

import { CardConfigDropOverlay, ConfigDropProps } from './file-upload';
import { Button, ButtonProps, IconButton, Input } from '@mui/material';
import { CardSize } from './component-enums';

const CardAccordion = styled((props: AccordionProps) => (
  <MuiAccordion disableGutters {...props} />
))(({ theme }) => ({
  margin: 0,
  border: `1px solid ${theme.palette.divider}`,
  '&:not(:last-child)': {
    borderBottom: 0,
  },
  '&::before': {
    display: 'none',
  }
}));
  
const UnstyledCardAccordionSummary = (props: AccordionSummaryProps) => (
  <MuiAccordionSummary expandIcon={<ArrowForwardIcon color='primary' sx={{ fontSize: '0.9rem' }} />} {...props} />
);
const CardAccordionSummary = styled(UnstyledCardAccordionSummary)(({ theme }) => ({
  flexDirection: 'row-reverse',
  [`.${accordionSummaryClasses.content}`]: {
    margin: 0,
    justifyContent: "space-between",
    alignItems: "center"
  },
  [`& .${accordionSummaryClasses.expandIconWrapper}.${accordionSummaryClasses.expanded}`]: {
    transform: 'rotate(90deg)',
  },
  [`& .${accordionSummaryClasses.content}`]: {
    marginLeft: theme.spacing(1),
  },
}));

interface CardProps {
    name: ReactNode | string;
    size?: CardSize;
    titleDecorators?: React.ReactNode;
    edgeDecorators?: React.ReactNode;
    dropProps?: ConfigDropProps;
    headerBackground?: string;
    onRename?: (name: string) => void;
}
export function Card(props: AccordionProps & CardProps) {
  const [editMode, setEditMode] = useState(false);
  const onRename = props.onRename;
  const editModeSupported = onRename !== undefined;
  const inputRef = useRef<HTMLInputElement>(null);

  function handleRenameClick(event: any) {
    event.stopPropagation();
    setEditMode(true);
  }
  
  function handleRenameDone(event: any) {
    event.stopPropagation();
    setEditMode(false);

    const newName = inputRef?.current?.value;
    if (newName && newName !== "") {
      onRename?.(newName);
    }
  }

  function handleRenameCancelled(event: any) {
    event.stopPropagation();
    setEditMode(false);
  }

  return (
    <CardAccordion
      defaultExpanded={props.defaultExpanded}
      expanded={props.expanded}
      onChange={props.onChange}
      sx={{ backgroundColor: props.color }}
    >
      <Stack direction='row' alignItems='center' sx={{background: props.headerBackground}}>
        <CardAccordionSummary sx={{ paddingRight: 0 }}>
          <Stack direction="row" spacing={1} alignItems='center'>
            {
              (editModeSupported && editMode)
                ? <Input inputRef={inputRef} defaultValue={props.name} onClick={(event) => event.stopPropagation()} />
                : <Typography variant="h5">{props.name}</Typography>
            }
            {props.titleDecorators}
          </Stack>
        </CardAccordionSummary>
        <Stack spacing={1} direction="row" alignItems='center' sx={{ paddingRight: 2 }}>
          {
            !editModeSupported ? null : ( editMode ?
              <>
                <IconButton size="small" onClick={handleRenameDone} color="primary" >
                  <RenameDoneIcon />
                </IconButton>
                <IconButton size="small" onClick={handleRenameCancelled} color="primary" >
                  <RenameCancelIcon />
                </IconButton>
              </>
              :
              <IconButton size="small" onClick={handleRenameClick} color="primary" sx={{display: 'none', '.Mui-expanded &': {display: 'block'}}} >
                <RenameIcon />
              </IconButton>
            )
          }
          {props.edgeDecorators}
        </Stack>
      </Stack>
      { props.dropProps ?
        <CardConfigDropOverlay dropProps={props.dropProps}>
          <AccordionDetails>
            {props.children}
          </AccordionDetails>
        </CardConfigDropOverlay>
        :
        <AccordionDetails>
          {props.children}
        </AccordionDetails>
      }
    </CardAccordion>
  );
}

const panelElementMinHeight = '30px';
const panelElementPadding = '2px 7px';
const panelElementFontSize = 13;

const UnstyledPanelButton = (props: ButtonProps) => (
  <Button size='small' variant='outlined' {...props}></Button>
);
export const PanelButton = styled(UnstyledPanelButton)(({ theme }) => ({
  padding: panelElementPadding,
  minHeight: panelElementMinHeight,
  fontSize: theme.typography.pxToRem(panelElementFontSize)
}));

const UnstyledPanelToggleButton = (props: ToggleButtonProps) => (
  <ToggleButton size='small' {...props}></ToggleButton>
);
export const PanelToggleButton = styled(UnstyledPanelToggleButton, {
  shouldForwardProp: (prop) => prop !== "selectedColor"
})<ToggleButtonProps<'button', { selectedColor?: string }>>(
  ({ theme, selectedColor }) => ({
    padding: panelElementPadding,
    minHeight: panelElementMinHeight,
    color: theme.palette.secondary.dark,
    border: '1px solid',
    fontSize: theme.typography.pxToRem(panelElementFontSize),
    "& > *": { // icon
      color: theme.palette.secondary.dark,
    },
    "&:hover, &.Mui-selected:hover": {
      borderColor: theme.palette.secondary.light,
    },
    "&.Mui-selected": {
      color: selectedColor ?? theme.palette.secondary.light,
      borderColor: theme.palette.secondary.light,
      "& > *": { // icon
        color: selectedColor ?? theme.palette.secondary.light,
      },
    },
  }));

export const PanelIconToggleButton = styled(PanelToggleButton)({
  padding: ['0', '0', '0', '0'], // make button (e.g. monitor) same width and height
  minHeight: panelElementMinHeight,
  minWidth: '30px'
});
  
export function EntryContainer(props: PropsWithChildren<{ name: string, labelWidth: string }>) {
  return (
    <Stack direction='row' flexWrap='wrap'>
      <DisplayNameLabel name={props.name} labelWidth={props.labelWidth} />
      { props.children }
    </Stack>
  );
}

function DisplayNameLabel({ name, labelWidth }: { name: string, labelWidth: string }) {
  return (
    <Typography minWidth={labelWidth} maxWidth={labelWidth}
      marginTop={1} variant="body1" title={name} noWrap
    >
      {name + ':'}
    </Typography>
  );
}

