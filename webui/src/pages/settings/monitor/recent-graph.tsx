// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { alpha, Box, BoxProps, darken, Stack, styled, Typography, TypographyProps } from '@mui/material';
import { MonitorMessage, MonitorMessageInfo } from "./monitor-message";
import { useCallback } from 'react';
import { useConfig } from '@config';
import { chartColors } from '@theme';

export const numRecentMessageInfos = 20;

const HitInfoList = styled(Stack)({
  overflowY: 'scroll',
  height: '100%',
  width: 'fit-content',
  // use more unobtrusive scrollbar for Edge/Chrome
  '&::-webkit-scrollbar': {
    width: '0.4em',
  },
  '&::-webkit-scrollbar-track': {
    background: 'rgba(0, 0, 0, 0.2)',
  },
  '&::-webkit-scrollbar-thumb': {
    background: '#888',
  },
  '&::-webkit-scrollbar-thumb:hover': {
    background: '#555'
  }
});

export function RecentHitsGraph({selectedMessageInfo, setSelectedMessageInfo, recentMessageInfos} : {
  selectedMessageInfo?: MonitorMessageInfo,
  setSelectedMessageInfo: (msg: MonitorMessageInfo) => void,
  recentMessageInfos: MonitorMessageInfo[]
}) {
  const monitoredPadIndex = useConfig(config => config.monitor?.padIndex);

  const onMessageSelected = useCallback((messageInfo: MonitorMessageInfo) => {
    setSelectedMessageInfo(messageInfo);
  }, [setSelectedMessageInfo]);

  return <HitInfoList direction='column' gap={1} paddingRight={1}>
    {
      recentMessageInfos.map((messageInfo, messageInfoIndex) => {
        if (messageInfo !== undefined) {
          const message = messageInfo.message;
          const padName = useConfig.getState().pads[message.padIndex].name;
          const hitZoneIndex = findHitZone(message);
          const hitValue = hitZoneIndex !== -1 ? message.getHitValuePercent(hitZoneIndex)! : 0;
          const bgcolor = hitZoneIndex !== -1 ? alpha(chartColors[hitZoneIndex], 0.2 + 0.8 * (hitValue / 100)) : '#000000';
          const isSelected = (messageInfo.id === selectedMessageInfo?.id);
          const textColor = darkenColorByTime(messageInfoIndex);
          const relativeTimestampMs = recentMessageInfos[0].timestampMs - messageInfo.timestampMs;

          return <Stack alignItems='center' key={messageInfoIndex}>
            <HitInfoBox bgcolor={bgcolor} color={textColor} border={isSelected ? 1 : 0} borderColor='white' onClick={() => onMessageSelected(messageInfo)}>
              <Stack>
                {
                  monitoredPadIndex !== messageInfo.message.padIndex
                    ? <Typography fontSize={10}>{padName}</Typography>
                    : null
                }
                <Typography>{hitValue !== undefined ? hitValue.toFixed(0) + '%' : ''}</Typography>
              </Stack>
            </HitInfoBox>
            <HitTimeLabel messageInfoIndex={messageInfoIndex} relativeTimestampMs={relativeTimestampMs} />
          </Stack>;
        } else {
          return <Stack alignItems='center' key={messageInfoIndex}>
            <HitInfoBox />
            <HitTimeLabel messageInfoIndex={messageInfoIndex} />
          </Stack>;
        }
      })
    }
  </HitInfoList>;
}

function HitInfoBox(props: BoxProps) {
  const boxSize = 50;
  const bgcolor = (props.bgcolor as string) ?? 'rgb(170, 170, 170, 0.1)';

  return <Box {...props} textAlign='center' alignContent='center' className='disableCaret'
    width={boxSize} height={boxSize} borderRadius={1} bgcolor={bgcolor} flexShrink={0}
    sx={{ '&:hover': { bgcolor: darken(bgcolor, 0.4) } }}
  >
    {props.children}
  </Box>;
}
  
function HitTimeLabel({messageInfoIndex, relativeTimestampMs} : {
  messageInfoIndex: number,
  relativeTimestampMs?: number,
}) {
  const noTimePlaceholder = '-';
  let formattedTime = noTimePlaceholder;
  if (relativeTimestampMs !== undefined) {
    const timeDiffSec = relativeTimestampMs / 1000;
    formattedTime = (timeDiffSec > 9 ? timeDiffSec.toFixed(1) : timeDiffSec.toFixed(3)) + "s";      
  }
    
  return <HitAnnotationLabel messageInfoIndex={messageInfoIndex} visibility={formattedTime === noTimePlaceholder ? 'hidden' : 'visible'}>
    {formattedTime}
  </HitAnnotationLabel>;
}

function HitAnnotationLabel(props : TypographyProps & {messageInfoIndex: number}) {
  const textColor = darkenColorByTime(props.messageInfoIndex);
  return <Typography visibility={props.visibility} textAlign='center' color={textColor} fontSize='0.7em'>
    {props.children}
  </Typography>;
}

function findHitZone(message: MonitorMessage) {
  return message.hits.findIndex(value => value !== 0);
}
  
function darkenColorByTime(messageInfoIndex: number) : string {
  const offset = 0.3;
  const opaqueness = 1 - messageInfoIndex / (numRecentMessageInfos - 1);
  return alpha('#FFFFFF',  offset + (1 - offset) * opaqueness);
}
