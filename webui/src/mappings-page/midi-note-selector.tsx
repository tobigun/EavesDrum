// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { useContext } from "react";

import { MenuItem, Select, SelectChangeEvent, Stack } from "@mui/material";

import { ConnectionStateContext } from "../connection/connection";
import { NumberInput } from "../components/number-input";

export function MidiNoteSelector({ configNote, onNoteChange }: { configNote: number, onNoteChange: (note: number) => void }) {
  const connected = useContext(ConnectionStateContext);

  const handleSelectChange = (event: SelectChangeEvent) => {
    onNoteChange(Number(event.target.value));
  };

  const handleValueChange = (value: number) => {
    if (Number.isInteger(value)) {
      onNoteChange(Math.max(0, Math.min(value, 127)));
    }
  };

  return (
    <Stack direction='row' alignItems='center'>
      <NumberInput disabled={!connected} value={configNote}
        onValueChange={(value) => value !== null && handleValueChange(value)}
        min={0} max={127} />
      <Select disabled={!connected} size='small' value={configNote.toString()}
        onChange={handleSelectChange}
        sx={{ minWidth: '5.5em', width: '50%', textAlign: 'center' }}
      >
        {notes.map((note, index) => <MenuItem key={index} value={index}>{note}</MenuItem>)}
      </Select>
    </Stack>
  );
}

const baseNotes = ['C', 'C#', 'D', 'D#', 'E', 'F', 'F#', 'G', 'G#', 'A', 'A#', 'B'];
const notes = Array.from({ length: 128 }, (_, key) => {
  const baseNote = baseNotes[key % baseNotes.length];
  const octave = Math.trunc(key / baseNotes.length) - 2;
  return `${baseNote}${octave}`;
});