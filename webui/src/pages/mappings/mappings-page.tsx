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
import { Card, EntryContainer, RoleInfo } from '@/components/card';
import { Masonry } from '@/components/masonry';
import { getHeaderBackground, getZoneName } from '@/common';
import { GroupChip } from '@/components/group-chip';
import { MidiNoteSelector } from './midi-note-selector';
import { ConfigFilter } from '@/components/component-enums';
import { ConnectionStateContext } from '@/connection/connection-state';
import { getSupportedMappingIds } from './mappings-filter';

type MappingDisplayNames = {
  [Property in keyof Required<DrumPadMappingValues>]: (props: MappingEntryProps) => string;
};
const mappingDisplayNames: MappingDisplayNames = {
  noteMain: ({padType}) => 'Note ' + ((padType === PadType.Pedal) ? "Pedal Chick" : getZoneName(padType, 0)),
  noteRim: ({padType}) => 'Note ' + getZoneName(padType, 1),
  noteCup: ({padType}) => 'Note ' + getZoneName(padType, 2),
  closedNotesEnabled: () => 'Enable Closed Notes',
  noteCloseMain: ({padType}) => 'Note Close ' + getZoneName(padType, 0),
  noteCloseRim: ({padType}) => 'Note Close ' + getZoneName(padType, 1),
  noteCloseCup: ({padType}) => 'Note Close ' + getZoneName(padType, 2),
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
    noteCloseMain: (config, padRole) => config.mappings[padRole].closedNotesEnabled!,
    noteCloseRim: (config, padRole) => config.mappings[padRole].closedNotesEnabled!,
    noteCloseCup: (config, padRole) => config.mappings[padRole].closedNotesEnabled!,
  },
  Pedal: {}
};

export function MappingsPage() {
  const padCount = useConfig(useShallow(config => config.pads.length));

  return (
    <Masonry columns={{ xs: 1, sm: 2, md: 3, lg: 4, xl: 5 }}>
      {
        [...Array(padCount)].map((_, padIndex) =>
          <MappingsCard key={padIndex} padIndex={padIndex} />)
      }
    </Masonry>
  );
}

function MappingsCard({ padIndex }: {
  padIndex: number
}) {
  const padName = useConfig(config => config.pads[padIndex].name);
  const padRole = useConfig(config => config.pads[padIndex].role);
  const group = useConfig(config => config.pads[padIndex].group);
  const pedalName = useConfig(config => config.pads[padIndex].pedal);
  const pedalIndex = pedalName ? getPadIndexByName(pedalName) : undefined;
  const padType = useConfig(config => config.pads[padIndex].settings.padType);
  const headerBackground = getHeaderBackground(padType);

  if (padType == PadType.Pedal) {
    return null;
  }

  return (
    <Box>
      <Card name={padName}
        headerBackground={headerBackground}
        dropProps={{filter: ConfigFilter.Mappings, padRole: padRole}}        
        titleDecorators={<GroupChip group={group} />}
      >
        <MappingsPadInfo padIndex={padIndex} padRole={padRole} padType={padType} />
        {
          pedalIndex !== undefined && <>
            <Box padding={1} width='100%' />
            <MappingsPadInfo padIndex={pedalIndex} padRole={getPadByIndex(pedalIndex).role} padType={PadType.Pedal} />
          </>
        }
      </Card>
    </Box>
  );
}

function MappingsPadInfo({ padIndex, padRole, padType }: {
  padIndex: number,
  padRole: string,
  padType: PadType
}) {
  const supportedMappingIds = getSupportedMappingIds(getPadByIndex(padIndex));

  return (
    <>
      <RoleInfo padRole={padRole} />
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
  padType: PadType
}

function MappingEntry(props: MappingEntryProps) {
  const { padIndex, padRole, mappingId, padType } = props;

  const mapping = useConfig(config => config.mappings[padRole][mappingId]);
  const isNoteEntry = (mappingValuesTyoes[mappingId] === 'number');
  
  const dependsOn = mappingDependencies[padType][mappingId];
  const enabled = useConfig(config => dependsOn ? dependsOn(config, padRole, padIndex) : true);

  return (
    <>
      {
        isNoteEntry
          ? (enabled ? <MappingEntryNote {...props} noteIndex={mapping as number | undefined} /> : null)
          : <MappingEntryEnable {...props} enabled={mapping as boolean | undefined} />
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
    updateConfig(config => (config.mappings[padRole][mappingId] as number | undefined) = newNoteIndex);
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
