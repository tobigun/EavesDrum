// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { useContext } from 'react';

import Box from '@mui/material/Box';
import Button from '@mui/material/Button';
import Grid from '@mui/material/Grid';
import Switch from '@mui/material/Switch';

import PlayIcon from '@mui/icons-material/PlayArrow';

import { useShallow } from 'zustand/shallow';

import { Config, DrumMappingId, DrumPadMappings, DrumPadMappingValues, getPadByIndex, getPadIndexByName, getPadZonesCount, getZonesCount, mappingValuesTyoes, PadRole, PadType, updateConfig, useConfig } from '@config';
import { connection, DrumCommand } from "@/connection/connection";
import { Card, EntryContainer } from '@/components/card';
import { Masonry } from '@/components/masonry';
import { getHeaderBackground, getZoneName } from '@/common';
import { GroupChip } from '@/components/group-chip';
import { MidiNoteSelector } from './midi-note-selector';
import { ConfigFilter } from '@/components/component-enums';
import { ConnectionStateContext } from '@/connection/connection-state';
import { getSupportedMappingIds } from './mappings-filter';
import { Typography, TypographyProps } from '@mui/material';
import { PadTypeSelector } from '../settings/pad-type-selector';
import { RoleInfo } from '@/components/role-info';

type MappingDisplayNames = {
  [Property in keyof Required<DrumPadMappingValues>]: (props: MappingEntryProps) => string;
};
const mappingDisplayNames: MappingDisplayNames = {
  noteMain: ({padType}) => 'Note ' + (padType ? ((padType === PadType.Pedal) ? "Pedal Chick" : getZoneName(padType, 0)) : "Main"),
  noteRim: ({padType}) => 'Note ' + (padType ? getZoneName(padType, 1) : "Rim"),
  noteCup: ({padType}) => 'Note ' + (padType ? getZoneName(padType, 2) : "Cup"),
  closedNotesEnabled: () => 'Enable Closed Notes',
  noteCloseMain: ({padType}) => 'Note Close ' + (padType ? getZoneName(padType, 0) : "Main"),
  noteCloseRim: ({padType}) => 'Note Close ' + (padType ? getZoneName(padType, 1) : "Rim"),
  noteCloseCup: ({padType}) => 'Note Close ' + (padType ? getZoneName(padType, 2) : "Cup"),
  noteCross: ({padIndex}) => 'Note ' + ((padIndex !== undefined && getPadZonesCount(padIndex) === 3) ? 'Side-Rim (Cross-Stick)' : 'Cross-Stick'),
};

function getDisplayName(props: MappingEntryProps) {
  return mappingDisplayNames[props.mappingId](props);
}

function isCrossNoteEnabled(config: Config, padIndex: number): boolean {
  return getZonesCount(config.pads[padIndex].settings.zonesType) === 3;
  // TODO: config.pads[padIndex].settings.crossNoteEnabled;
}

type MappingDependency = Partial<Record<keyof DrumPadMappings, (config: Config, padRole: PadRole, padIndex?: number) => boolean>>;
type MappingDependencies = Record<PadType, MappingDependency>;
const mappingDependencies: MappingDependencies = {
  Drum: {
    noteCross: (config, _, padIndex) => padIndex === undefined || isCrossNoteEnabled(config, padIndex!)
  },
  Cymbal: {
    noteCloseMain: (config, padRole) => config.mappings[padRole]?.closedNotesEnabled ?? false,
    noteCloseRim: (config, padRole) => config.mappings[padRole]?.closedNotesEnabled ?? false,
    noteCloseCup: (config, padRole) => config.mappings[padRole]?.closedNotesEnabled ?? false,
  },
  Pedal: {}
};

export function MappingsPage() {
  const padCount = useConfig(useShallow(config => config.pads.length));

  const roles = useConfig(useShallow(config => Object.keys(config.mappings)));
  const usedRoles = useConfig(useShallow(config => new Set(config.pads.map(pad => pad.role))));
  const unusedRoles = roles.filter(role => !usedRoles.has(role));

  const validPadIndexList = useConfig(useShallow(config => config.pads
    .map((pad, index) => roles.includes(pad.role) ? index : null)
    .filter(index => index != null)
  ));
  const invalidPadIndexList = [...Array(padCount).keys()].filter(index => !validPadIndexList.includes(index));

  const columns = { xs: 1, sm: 2, md: 3, lg: 4, xl: 5 };

  return (
    <>
      <Masonry columns={columns}>
        { validPadIndexList.map(padIndex => <MappingsCardGroupForPad key={padIndex} padIndex={padIndex} />) }
      </Masonry>
      
      { invalidPadIndexList.length > 0 ? <MappingSectionHeader name="Invalid Mappings" color="white" bgcolor="rgb(157, 67, 67)" /> : null }
      
      <Masonry columns={columns}>
        { invalidPadIndexList.map(padIndex => <MappingsCardGroupForPad key={padIndex} padIndex={padIndex} />) }
      </Masonry>      
      
      { unusedRoles.length > 0 ? <MappingSectionHeader name="Unused Roles" color="black" /> : null }
      
      <Masonry columns={columns}>
        { unusedRoles.map(role => <MappingsCardForRole key={role} padRole={role} />) }
      </Masonry>
    </>
  );
}

export function MappingSectionHeader(props: TypographyProps & { name: string }) {
  return (
    <Typography variant="body1" align='center' paddingX={1} border={1} borderColor='rgb(53, 53, 53)' noWrap {...props}>
      {props.name}
    </Typography>
  );
}

function MappingsCardGroupForPad({ padIndex }: {
  padIndex: number
}) {
  const pedalName = useConfig(config => config.pads[padIndex].pedal);
  const pedalIndex = pedalName ? getPadIndexByName(pedalName) : undefined;
  const padType = useConfig(config => config.pads[padIndex].settings.padType);

  if (padType == PadType.Pedal) {
    return null;
  }

  return (
    <Box>
      <MappingsCardForPad padIndex={padIndex} padType={padType} />
      {
        pedalIndex !== undefined && <MappingsCardForPad padIndex={pedalIndex} padType={PadType.Pedal} />
      }
    </Box>
  );
}

function MappingsCardForPad({ padIndex, padType }: {
  padIndex: number,
  padType: PadType
}) {
  const padName = useConfig(config => config.pads[padIndex].name);
  const group = useConfig(config => config.pads[padIndex].group);
  const padRole = useConfig(config => config.pads[padIndex].role);
  const padRoleName = useConfig(config => config.mappings[padRole]?.name);
  const headerBackground = getHeaderBackground(padType);
  
  function handlePadRename(name: string) {
    connection.sendSetPadConfigCommand(padIndex, {name: name});
  }

  return (
    <Box>
      <Card name={padName}
        secondaryTitle={
          <table>
            <tbody>
              <tr><td align='right'>Role:</td><td align='left'>{padRoleName ? padRoleName : padRole}</td></tr>
              <tr><td align='right'>ID:</td><td align='left'>{padRole}</td></tr>
            </tbody>
          </table>
        }
        headerBackground={headerBackground}
        dropProps={{filter: ConfigFilter.Mappings, padRole: padRole}}        
        titleDecorators={<GroupChip group={group} />}
        onRename={handlePadRename}
      >
        <PadTypeSelector padIndex={padIndex}/>
        <MappingsInfo padIndex={padIndex} padRole={padRole} padType={padType} />
      </Card>
    </Box>
  );
}

function MappingsCardForRole({ padRole }: {
  padRole: string
}) {
  const roleName = useConfig(config => config.mappings[padRole].name);

  function handleRoleRename(name: string) {
    updateConfig(config => config.mappings[padRole] = {
      ...config.mappings[padRole],
      name: name
    });
    connection.sendSetRoleMappingsCommand(padRole, {name: name});
  }

  return (
    <Box>
      <Card name={(roleName ? roleName : padRole)}
        secondaryTitle={`ID: ${padRole}`}
        onRename={handleRoleRename}
        color='rgba(42, 64, 86, 1)'
        dropProps={{filter: ConfigFilter.Mappings, padRole: padRole}}
      >
        <MappingsInfo padRole={padRole} />
      </Card>
    </Box>
  );
}

function MappingsInfo({ padIndex, padRole, padType }: {
  padIndex?: number,
  padRole: string,
  padType?: PadType
}) {
  useConfig(useShallow(config => padIndex !== undefined && config.pads[padIndex].settings)); // re-render when pad type changes (type, zones, ...)
  const supportedMappingIds = padIndex !== undefined
    ? getSupportedMappingIds(getPadByIndex(padIndex))
    : Object.keys(useConfig.getState().mappings[padRole]) as DrumMappingId[];

  return (
    <>
      <RoleInfo padRole={padRole} padIndex={padIndex} />
      {
        supportedMappingIds
          .map(mappingId => <MappingEntry key={mappingId} padIndex={padIndex} padRole={padRole} mappingId={mappingId} padType={padType} />)
      }
    </>
  );
}

interface MappingEntryProps {
  padIndex?: number,
  padRole: string,
  mappingId: DrumMappingId,
  padType?: PadType
}

function MappingEntry(props: MappingEntryProps) {
  const { padIndex, padRole, mappingId, padType } = props;

  const mapping = useConfig(config => config.mappings[padRole]?.[mappingId]);
  const isNoteEntry = (mappingValuesTyoes[mappingId] === 'number');
  const isBoolEntry = (mappingValuesTyoes[mappingId] === 'boolean');
  
  const dependsOn = padType ? mappingDependencies[padType][mappingId] : undefined;
  const enabled = useConfig(config => dependsOn ? dependsOn(config, padRole, padIndex) : true);

  return (
    <>
      {
        (isNoteEntry && enabled) ? <MappingEntryNote {...props} noteIndex={mapping as number | undefined} />
          : (isBoolEntry ? <MappingEntryEnable {...props} enabled={mapping as boolean | undefined} /> : null)
      }
    </>
  );
}

function MappingEntryNote(props: MappingEntryProps & {
  noteIndex: number | undefined
}) {
  const { padRole, mappingId, noteIndex } = props;

  const displayName = getDisplayName(props);
  const connected = useContext(ConnectionStateContext);

  function onNoteChange(newNoteIndex?: number) {
    updateConfig(config => config.mappings[padRole] = {
      ...config.mappings[padRole],
      [mappingId]: newNoteIndex
    });
    connection.sendSetRoleMappingsCommand(padRole, { [mappingId]: newNoteIndex ?? null });
  }

  return (
    <EntryContainer name={displayName} labelWidth='9em'>
      <Grid container alignItems='center' flexGrow={1} spacing={1}>
        <Grid size={9}>
          <MidiNoteSelector configNote={noteIndex} onNoteChange={onNoteChange} allowUndefined={true} />
        </Grid>
        <Grid size={3} textAlign='center'>
          <Button variant="outlined" title='Send midi note' disabled={!connected} sx={{ width: '80%' }}
            onClick={() => onPlayMidiNote(noteIndex ?? 0)}
          >
            <PlayIcon color="primary" />
          </Button>
        </Grid>
      </Grid>
    </EntryContainer>
  );
}

function MappingEntryEnable(props: MappingEntryProps & {
  enabled: boolean | undefined
}) {
  const defaultValue = false;
  const { padRole, mappingId, enabled } = props;

  const displayName = getDisplayName(props);
  const connected = useContext(ConnectionStateContext);

  function onChange(_event: unknown, newEnabled: boolean) {
    updateConfig(config => (config.mappings[padRole][mappingId] as boolean) = newEnabled);
    connection.sendSetRoleMappingsCommand(padRole, { [mappingId]: newEnabled });
  }

  return (
    <EntryContainer name={displayName} labelWidth='11em'>
      <Switch checked={enabled ?? defaultValue} onChange={onChange} disabled={!connected} />
    </EntryContainer>
  );
}

function onPlayMidiNote(note: number) {
  connection.sendCommand(DrumCommand.playNote, { note: note });
}
