// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { useContext } from "react";

import { MenuItem, Select, SelectChangeEvent, Stack } from "@mui/material";

import { NumberInput } from "@/components/number-input";
import { ConnectionStateContext } from "@/connection/connection-state";

const NONE = -1;
const NOTE_MAX = 127;

export function MidiNoteSelector({ configNote, onNoteChange, allowUndefined = false }: {
  configNote: number | undefined,
  onNoteChange: (note?: number) => void,
  allowUndefined?: boolean
}) {
  const connected = useContext(ConnectionStateContext);
  const minValue = allowUndefined ? NONE : 0;

  const handleSelectChange = (event: SelectChangeEvent) => {
    const value = Number(event.target.value);
    onNoteChange(value !== NONE ? value : undefined);
  };

  const handleValueChange = (value: number) => {
    if (Number.isInteger(value)) {
      const newValue = Math.max(minValue, Math.min(value, NOTE_MAX));
      onNoteChange(newValue !== NONE ? newValue : undefined);
    }
  };

  return (
    <Stack direction='row' alignItems='center'>
      <NumberInput disabled={!connected} value={configNote ?? NONE}
        onValueChange={(value) => value !== null && handleValueChange(value)}
        min={minValue} max={NOTE_MAX} />
      <Select disabled={!connected} size='small' value={configNote?.toString() ?? `${NONE}`}
        onChange={handleSelectChange}
        sx={{ minWidth: '5.5em', width: '50%', textAlign: 'center' }}
      >
        {allowUndefined ? <MenuItem key={NONE} value={NONE}>&mdash;</MenuItem> : null}
        {notes.map((note, index) => <MenuItem key={index} value={index}>{note}</MenuItem>)}
      </Select>
    </Stack>
  );
}

const baseNotes = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
const notes = Array.from({ length: NOTE_MAX + 1 }, (_, key) => {
  const baseNote = baseNotes[key % baseNotes.length];
  const octave = Math.trunc(key / baseNotes.length) - 2;
  return `${baseNote}${octave}`;
});