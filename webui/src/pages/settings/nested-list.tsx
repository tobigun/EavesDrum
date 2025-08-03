// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { ChevronRight, ExpandMore } from "@mui/icons-material";
import { Box, List, ListItemButton, ListItemText, ListSubheader } from "@mui/material";
import { useCallback, useMemo, useState } from "react";

export interface NestedListItemInfo {
    value: any,
    name: string,
    icon?: any,
    subItems?: {
        header: string,
        items: NestedListItemInfo[]
    }
}

const NONE_SELECTED = -1;

export function NestedList({header, itemInfos, onValueChange, parentValueChain = [], indent = 0 }: {
    header: string,
    itemInfos: NestedListItemInfo[],
    onValueChange: (valueChain: any[]) => void,
    parentValueChain?: any[],
    indent?: number
}) {
  const [selectedIndex, setSelectedIndex] = useState(NONE_SELECTED);

  return <List dense={true}
    sx={{ bgcolor: 'background.paper' }}
    component="nav"
    subheader={
      selectedIndex !== NONE_SELECTED ? null :
        <ListSubheader sx={{ pl: indent }} component="div" id="nested-list-subheader">
          {header}
        </ListSubheader>
    }
  >
    { 
      itemInfos.map((_, index) =>
        <NestedListItem key={index} index={index}
          itemInfos={itemInfos}
          selectedIndex={selectedIndex}
          setSelectedIndex={setSelectedIndex}
          parentValueChain={parentValueChain}
          onValueChange={onValueChange}
          indent={indent}
        /> )
    }
  </List>;
}

export function NestedListItem({index, selectedIndex, setSelectedIndex, itemInfos, parentValueChain, indent, onValueChange} : {
    index: number,
    selectedIndex: number,
    setSelectedIndex: (oldValue: number) => void,
    itemInfos: NestedListItemInfo[],
    parentValueChain: any[],
    onValueChange: (valueChain: any[]) => void,
    indent: number
}) {
  const itemInfo = itemInfos[index];
  const hasSubItems = itemInfo.subItems && itemInfo.subItems.items.length > 0;
  const isSelected = selectedIndex === index;
  const isOnlyChild = itemInfos.length === 1;
  const isOpen = isOnlyChild || isSelected; // if menu contains one child only, open per default
  const isHidden = selectedIndex !== NONE_SELECTED && !isOpen; // hide if another item is selected

  // add own value to value chain
  const valueChain = useMemo(() => [...parentValueChain, itemInfo.value], [parentValueChain, itemInfo.value]);

  const handleClick = useCallback(() => {
    if (hasSubItems) {
      setSelectedIndex(isSelected ? NONE_SELECTED : index);
    } else {
      onValueChange(valueChain);
    }
  }, [index, valueChain, isSelected, hasSubItems, setSelectedIndex, onValueChange]);

  return (
    <Box display={isHidden ? 'none' : undefined} key={index}>
      <ListItemButton sx={{ pl: indent }} onClick={handleClick}>
        {!hasSubItems ? null : (isOpen ? <ExpandMore /> : <ChevronRight />)}
        {itemInfo.icon ? <Box display='flex' justifyItems='center' paddingX={1}>{itemInfo.icon}</Box> : null}
        <ListItemText primary={itemInfo.name} />
      </ListItemButton>
      {
        (!isOpen || !hasSubItems) ? null :
          <NestedList
            header={itemInfo.subItems!.header}
            itemInfos={itemInfo.subItems!.items}
            onValueChange={onValueChange}
            parentValueChain={valueChain}
            indent={indent + 4}
          />
      }
    </Box>
  );
}
