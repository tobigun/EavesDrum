// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { Breadcrumbs, Chip, emphasize } from "@mui/material";
import { alpha, styled } from "@mui/system";

export const CardBreadcrumbs = styled(Breadcrumbs)({
  marginTop: '5px',
  '& .MuiBreadcrumbs-separator': {
    margin: 2,
  }
});

export const CardBreadcrumb = styled(Chip)(({ theme }) => ({
  backgroundColor: theme.palette.grey[600],
  height: theme.spacing(3),
  color: theme.palette.text.secondary,
  '& .MuiChip-icon': {
    color: alpha(theme.palette.text.secondary, 0.5),
    height: '80%'
  },
  '& .MuiChip-label': {
    padding: '7px'
  },  
  '&:hover': {
    backgroundColor: emphasize(theme.palette.grey[600], 0.3)
  }
}));
