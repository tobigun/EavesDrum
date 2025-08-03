// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { MouseEvent, ReactElement, useCallback, useState } from "react";
import { ChokeType, getZonesCount, PadType, useConfig, ZonesType } from "@config";
import { connection, DrumCommand } from "@/connection/connection";
import { Popover } from "@mui/material";
import CymbalIcon from '@mui/icons-material/Album';
import DrumIcon from '@mui/icons-material/CircleOutlined';
import ZoneIcon from '@mui/icons-material/AdsClick';
import PedalIcon from '@mui/icons-material/ClosedCaption';
import ChokeIcon from '@mui/icons-material/MusicOff';
import NoChokeIcon from '@mui/icons-material/MusicNote';
import PiezoIcon from '@mui/icons-material/RadioButtonChecked';
import SwitchIcon from '@mui/icons-material/ToggleOn';
import { NestedList, NestedListItemInfo } from "./nested-list";
import { CardBreadcrumb, CardBreadcrumbs } from "@/components/breadcrumb";

const HEADER_PAD_TYPE = "Select Pad Type:";
const HEADER_ZONES_COUNT = "Select Zones Count:";
const HEADER_ZONES_TYPE = "Select Zones Type:";
const HEADER_CHOKE_TYPE = "Select Choke Type:";

enum MenuTopLevel {
  HideMenu,
  PadType,
  ZonesCount,
  ZonesType,
  ChokeType
}

export function PadTypeSelector({padIndex}: {
  padIndex: number
}) {
  const padType = useConfig(config => config.pads[padIndex].settings.padType);
  const zonesType = useConfig(config => config.pads[padIndex].settings.zonesType);
  const chokeType = useConfig(config => config.pads[padIndex].settings.chokeType) ?? ChokeType.None;

  const [anchorEl, setAnchorEl] = useState<HTMLElement>();
  const [openMenuTopLevel, setOpenMenuTopLevel] = useState<MenuTopLevel>(MenuTopLevel.HideMenu);

  const handleClick = useCallback((event: MouseEvent<HTMLElement>, level: MenuTopLevel) => {
    setAnchorEl(event.currentTarget);
    setOpenMenuTopLevel(level);
  }, []);
  
  const handleClose = useCallback(() => {
    setAnchorEl(undefined);
    setOpenMenuTopLevel(MenuTopLevel.HideMenu);
  }, []);

  function getMenuList() {
    const zonesCount = getZonesCount(zonesType);
    switch (openMenuTopLevel) {
    case MenuTopLevel.PadType:
      return <NestedList header={HEADER_PAD_TYPE} itemInfos={getPadTypeItems(padType)} onValueChange={onZoneCountChanged}
        parentValueChain={[]} />;
    case MenuTopLevel.ZonesCount:
      return <NestedList header={HEADER_ZONES_COUNT} itemInfos={getZonesCountItems(padType)} onValueChange={onZoneCountChanged}
        parentValueChain={[padType]} />;
    case MenuTopLevel.ZonesType:
      return <NestedList header={HEADER_ZONES_TYPE} itemInfos={getZonesTypeItems(padType, zonesCount)} onValueChange={onZoneCountChanged}
        parentValueChain={[padType, zonesCount]} />;
    case MenuTopLevel.ChokeType:
      return <NestedList header={HEADER_CHOKE_TYPE} itemInfos={getChokeTypeItems(padType, zonesCount)} onValueChange={onZoneCountChanged}
        parentValueChain={[padType, zonesCount, zonesType]} />;
    } 
  }

  const onZoneCountChanged = useCallback((valueChain: any) => {
    const [newPadType, , newZonesType, newChokeTypeOrUnknown] = valueChain;
    const newChokeType = newChokeTypeOrUnknown ?? ChokeType.None;
    
    if (newPadType !== padType || newZonesType !== zonesType || newChokeType !== chokeType) {
      connection.sendSetPadSettingsCommand(padIndex, {
        padType: newPadType,
        zonesType: newZonesType,
        chokeType: newChokeType
      });
      connection.sendCommand(DrumCommand.getConfig);  
    }

    handleClose();
  }, [padIndex, padType, zonesType, chokeType, handleClose]);
    
  const showMenu = openMenuTopLevel !== MenuTopLevel.HideMenu;

  return <>
    <CardBreadcrumbs separator=''>
      <CardBreadcrumb label={padType} icon={getPadTypeIcon(padType)}
        onClick={event => handleClick(event, MenuTopLevel.PadType)} />
      <CardBreadcrumb label={formatZoneCount(getZonesCount(zonesType))}
        onClick={event => handleClick(event, MenuTopLevel.ZonesCount)} />
      <CardBreadcrumb label={formatZonesTypeNames(zonesType)}
        onClick={event => handleClick(event, MenuTopLevel.ZonesType)} />
      {
        padType !== PadType.Cymbal ? null :
          <CardBreadcrumb label={formatChokeTypeNames(chokeType)}
            onClick={event => handleClick(event, MenuTopLevel.ChokeType)} />
      }
    </CardBreadcrumbs>
    <Popover open={showMenu} anchorEl={anchorEl} onClose={handleClose}>
      {showMenu ? getMenuList() : null}
    </Popover>
  </>;
}

function getPadTypeIcon(padType: PadType) {
  switch (padType) {
  case PadType.Drum: return <DrumIcon />;
  case PadType.Cymbal: return <CymbalIcon />;
  case PadType.Pedal: return <PedalIcon />;
  }
}

function getPadTypeItems(currentPadType: PadType): NestedListItemInfo[] {
  return Object.values(PadType)
    .filter(padType => (currentPadType === PadType.Pedal && padType === PadType.Pedal)
      || (currentPadType !== PadType.Pedal && padType !== PadType.Pedal))
    .map(padType => ({
      value: padType,
      name: padType,
      icon: getPadTypeIcon(padType),
      subItems: { header: HEADER_ZONES_COUNT, items: getZonesCountItems(padType) }
    }));
}

function getZonesCountItems(padType: PadType): NestedListItemInfo[] {  
  return [1, 2, 3]
    .filter(zoneCount => padType !== PadType.Pedal || zoneCount === 1)
    .map(zoneCount => ({
      value: zoneCount,
      name: formatZoneCount(zoneCount),
      icon: <ZoneIcon />,
      subItems: { header: HEADER_ZONES_TYPE, items: getZonesTypeItems(padType, zoneCount) }
    }));
}

function getZonesTypeItems(padType: PadType, zoneCount: number): NestedListItemInfo[] {
  const isPedal = padType === PadType.Pedal;
  return Object.values(ZonesType)
    .filter(zonesType => (isPedal && zonesType === ZonesType.Zones1_Controller)
      || (!isPedal && getZonesCount(zonesType) === zoneCount))
    .map(zonesType => ({
      value: zonesType,
      name: formatZonesTypeNames(zonesType),
      icon: formatZonesTypeIcons(zonesType),
      subItems: { header: HEADER_CHOKE_TYPE, items: getChokeTypeItems(padType, zoneCount) }
    }));
}

function getChokeTypeItems(padType: PadType, zoneCount: number): NestedListItemInfo[] {
  return Object.values(ChokeType)
    .filter(() => padType == PadType.Cymbal && zoneCount > 1)
    .map(chokeType => ({
      value: chokeType,
      name: formatChokeTypeNames(chokeType),
      icon: chokeType === ChokeType.None ? <NoChokeIcon/> : <ChokeIcon />
    }));
}

function formatZoneCount(zoneCount: number): string {
  return String(zoneCount) + "-" + (zoneCount === 1 ? "Zone" : "Zones");
}

function formatZonesTypeNames(value: ZonesType): string {
  switch (value) {
  case ZonesType.Zones1_Controller: return 'Analog Controller';
  case ZonesType.Zones1_Piezo: return 'Piezo';
  case ZonesType.Zones2_Piezos: return '2 Piezos';
  case ZonesType.Zones2_PiezoAndSwitch: return 'Piezo + Switch';
  case ZonesType.Zones3_Piezos: return '3 Piezos';
  case ZonesType.Zones3_PiezoAndSwitches_2TRS: return 'Piezo + 2 Switches (2x TRS)';
  case ZonesType.Zones3_PiezoAndSwitches_1TRS: return 'Piezo + 2 Switches (1x TRS)';
  }
}

function formatZonesTypeIcons(value: ZonesType): ReactElement {
  switch (value) {
  case ZonesType.Zones1_Controller: return <PedalIcon />;
  case ZonesType.Zones1_Piezo: return <PiezoIcon />;
  case ZonesType.Zones2_Piezos: return <><PiezoIcon /><PiezoIcon /></>;
  case ZonesType.Zones2_PiezoAndSwitch: return <><PiezoIcon /><SwitchIcon /></>;
  case ZonesType.Zones3_Piezos: return <><PiezoIcon /><PiezoIcon /><PiezoIcon /></>;
  case ZonesType.Zones3_PiezoAndSwitches_2TRS: return <><PiezoIcon /><SwitchIcon /><SwitchIcon /></>;
  case ZonesType.Zones3_PiezoAndSwitches_1TRS: return <><PiezoIcon /><SwitchIcon /><SwitchIcon /></>;
  }
}

function formatChokeTypeNames(value: ChokeType): string {
  switch (value) {
  case ChokeType.None: return 'No Choke';
  case ChokeType.Switch_Edge: return 'Choke: Edge';
  case ChokeType.Switch_Cup: return 'Choke: Cup';
  }
}
