// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import MuiMasonry, { MasonryProps } from '@mui/lab/Masonry';

export function Masonry(props: MasonryProps) {
  return (
    <MuiMasonry sequential={false}
      spacing={1}
      sx={{ "&.MuiMasonry-root": { margin: 0 } }}
      {...props}>
      {props.children}
    </MuiMasonry>
  );
}
