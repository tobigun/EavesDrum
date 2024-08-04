// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { lighten, styled, TableRow } from "@mui/material";

export const StyledTableRow = styled(TableRow)(({ theme }) => ({
  '& td': {
    color: "#000000",
    padding: "7px"
  },
  '& th': {
    padding: "7px"
  },
  '&:nth-of-type(odd)': {
    backgroundColor: lighten(theme.palette.background.paper, 0.9),
  },
  '&:nth-of-type(even)': {
    backgroundColor: lighten(theme.palette.background.paper, 1),
  },
}));
