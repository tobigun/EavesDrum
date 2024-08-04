// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { Box, BoxProps } from "@mui/material";

export function InfoBox(props: BoxProps) {    
  return (
    <Box {...props} margin={1} display='inline-grid' gridTemplateColumns='1fr 1fr' color='black' bgcolor='#d9e5e7'
      gap={1} border={1} padding={1} borderRadius='5px'>
    </Box>
  );
}
