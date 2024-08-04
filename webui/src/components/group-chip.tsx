// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { Chip } from "@mui/material";
import { groupColors } from "../theme";

export const GroupChip = ({ group }: { group: string }) => (!group || group === '')
  ? null
  : <Chip label={group} size="small" sx={{ backgroundColor: getGroupColor(group) }} />;

  
let groupColorIndex = 0;
const groupColorAssignment = new Map<string, string>();
  
function getGroupColor(group: string): string {
  let color = groupColorAssignment.get(group);
  if (color) {
    return color;
  }
  color = groupColors[groupColorIndex++];
  groupColorAssignment.set(group, color);
  return color;
}
  