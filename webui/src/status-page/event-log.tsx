// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { useEffect, useState } from 'react';

import Box from '@mui/material/Box';
import Table from '@mui/material/Table';
import TableBody from '@mui/material/TableBody';
import TableCell from '@mui/material/TableCell';
import TableContainer from '@mui/material/TableContainer';
import TableHead from '@mui/material/TableHead';
import Paper from '@mui/material/Paper';

import { connection, DrumCommand } from '../connection/connection';
import { StyledTableRow } from '../components/table-row';

enum LogLevel {
  INFO,
  WARN,
  ERROR
}

interface LogEvent {
    id: number,
    level: LogLevel,
    message: string
}

export function EventLogInfo() {
  const [events, setEvents] = useState<LogEvent[]>([]);

  useEffect(() => {
    const onEventJsonListenerHandle = connection.registerOnJsonDataListener('events', (events: LogEvent[]) => {
      setEvents(events);
    });

    connection.sendCommand(DrumCommand.getEvents);

    return () => {
      connection.unregisterListener(onEventJsonListenerHandle);
    };
  }, []);

  return (
    <Box margin={1}>
      <TableContainer component={Paper}>
        <Table>
          <TableHead>
            <StyledTableRow sx={{ "& th": { backgroundColor: 'rgba(82, 82, 82, 0.81)'} }}>
              <TableCell sx={{ borderRight: "solid 1px white" }}>ID</TableCell>
              <TableCell>Level</TableCell>
              <TableCell>Message</TableCell>
            </StyledTableRow>
          </TableHead>
          <TableBody>
            {
              events.map(event => (
                <StyledTableRow key={event.id}>
                  <TableCell scope="row" sx={{ borderRight: "solid 1px white" }}>{event.id}</TableCell>
                  <TableCell>{LogLevel[event.level]}</TableCell>
                  <TableCell>{event.message}</TableCell>
                </StyledTableRow>
              ))}
            {
              events.length == 0 &&
                            <StyledTableRow>
                              <TableCell colSpan={3} align='center' sx={{ fontStyle: 'italic' }}>No Events logged</TableCell>
                            </StyledTableRow>
            }
          </TableBody>
        </Table>
      </TableContainer>
    </Box>
  );
}