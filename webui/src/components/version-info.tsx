// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { Box, Divider, Paper, Stack, Table, TableBody, TableCell, TableContainer, TableHead, Typography } from "@mui/material";
import { useConfig } from "@config";
import { StyledTableRow } from "@/components/table-row";
import { licenses } from "./licenses";

export function VersionInfo() {
  const backendVersionInfo = useConfig(config => config.version);

  return <Stack paddingX={4} paddingY={1}>
    <Typography>Backend:</Typography>
    <Divider />
    <Box>
      <Typography>Version: {backendVersionInfo?.packageVersion ?? '<not connected>'}</Typography>
      <Typography>Commit: {backendVersionInfo?.gitCommitHash ?? '<not connected>'}</Typography>
      <Typography noWrap>Build Date: {backendVersionInfo?.buildTime ?? '<not connected>'}</Typography>
    </Box>

    <p/>

    <Typography>UI:</Typography>
    <Divider />
    <Box>
      <Typography>Version: {APP_VERSION.packageVersion}</Typography>
      <Typography>Commit: {APP_VERSION.gitCommitHash}</Typography>
      <Typography noWrap>Build Date: {APP_VERSION.buildTime}</Typography>
    </Box>
    
    <p/>
    
    <Typography>Logo Fonts:</Typography>
    <Divider />
    <Box>
      <ul>
        <li>
          Audiowide
          <ul>
            <li>https://fonts.google.com/specimen/Audiowide</li>
            <li>Copyright (c) 2012, Brian J. Bonislawsky DBA Astigmatic (AOETI) (astigma@astigmatic.com)</li>
            <li>SIL Open Font License, Version 1.1</li>
          </ul>
        </li>
        <li>
          Philosopher
          <ul>
            <li>https://fonts.google.com/specimen/Philosopher</li>
            <li>Designed by Jovanny Lemonad</li>
            <li>Copyright 2011 The Philosopher Project Authors (https://github.com/alexeiva/philosopher)</li>
            <li>SIL Open Font License, Version 1.1</li>
          </ul>
        </li>
      </ul>
    </Box>

    <Typography>Licenses Backend:</Typography>
    <Divider />
    <LicenseInfoBackend />

    <Typography>Licenses UI:</Typography>
    <Divider />
    <LicenseInfoUI />
  </Stack>;
}

function LicenseInfoBackend() {
  return <Box sx={{ margin: 1 }}>
    <TableContainer component={Paper}>
      <Table>
        <TableHead>
          <StyledTableRow sx={{ "& th": { backgroundColor: 'rgba(113, 113, 113, 0.88)'} }}>
            <TableCell>Library</TableCell>
            <TableCell>License</TableCell>
          </StyledTableRow>
        </TableHead>
        <TableBody>
          {
            licenses.backend.map(entry => <StyledTableRow key={entry.name}>
              <TableCell>{entry.name}</TableCell>
              <TableCell>{entry.license}</TableCell>
            </StyledTableRow>)
          }
        </TableBody>
      </Table>
    </TableContainer>
  </Box>;
}

function LicenseInfoUI() {
  return <Box sx={{ margin: 1 }}>
    <TableContainer component={Paper}>
      <Table>
        <TableHead>
          <StyledTableRow sx={{ "& th": { backgroundColor: 'rgba(113, 113, 113, 0.88)'} }}>
            <TableCell>Library</TableCell>
            <TableCell>License</TableCell>
          </StyledTableRow>
        </TableHead>
        <TableBody>
          {
            licenses.ui.map(entry => <StyledTableRow key={entry.name}>
              <TableCell>{entry.name}</TableCell>
              <TableCell>{entry.license}</TableCell>
            </StyledTableRow>)
          }
        </TableBody>
      </Table>
    </TableContainer>
  </Box>;
}
