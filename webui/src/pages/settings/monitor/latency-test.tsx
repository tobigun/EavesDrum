// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import Box from "@mui/material/Box";
import Button from "@mui/material/Button";
import Stack from "@mui/material/Stack";
import Typography from "@mui/material/Typography";
import LatencySvg from "@/image/latency.svg?react";

import { getPadSettingsByIndex, isPadPinConnectedToMux, useConfig } from "@config";
import { connection, DrumCommand } from "@connection";
import { Alert, Step, StepLabel, Stepper } from "@mui/material";
import { PropsWithChildren, useEffect, useState } from "react";
import { Monitor } from "./monitor-card";
import { createThresholdSliderProps } from "../settings-pad";
import { SettingSlider } from "../setting-slider";
import { MidiNoteSelector } from "@/pages/mappings/midi-note-selector";
import { MonitorMode } from "./monitor-mode";
import { MonitorMessage } from "./monitor-message";
import { InfoBox } from "@components/info-box";
import { MuxMonitorInputCheck } from "./signal-graph/mux-monitor-input-check";

export function LatencyTestWizard({closeLatencyTest} : {
  closeLatencyTest: () => void
}) {
  const [activeStep, setActiveStep] = useState(0);
  const [nextEnabled, setNextEnabled] = useState(true);
  const [threshold, setThreshold] = useState(0);
  const [midiNote, setMidiNote] = useState(38);
    
  function handleNext() {
    setActiveStep(currentActiveStep => currentActiveStep + 1);
  };

  function handleBack() {
    setNextEnabled(true);
    setActiveStep(currentActiveStep => currentActiveStep - 1);
  };

  const labels = [
    "Introduction",
    "Select trigger input",
    "Connect soundcard",
    "Adjust trigger settings",
    "Perform test"
  ];

  function getNextStep() {
    switch (activeStep) {
    case 0: return <StepIntroduction />;
    case 1: return <StepSelectChannel setNextEnabled={setNextEnabled} />;
    case 2: return <StepConnectSoundcard />;
    case 3: return <StepAdjustTriggerSettings
      threshold={threshold} setThreshold={setThreshold}
      midiNote={midiNote} setMidiNote={setMidiNote} />;
    case 4: return <StepFinalTest
      threshold={threshold}
      midiNote={midiNote} setMidiNote={setMidiNote} />;
    default: return null;
    }
  }

  const isLastStep = (activeStep === labels.length - 1);

  return (
    <>
      <Stepper activeStep={activeStep}>
        {
          labels.map(label => <Step key={label}><StepLabel>{label}</StepLabel></Step>)
        }
      </Stepper>

      {getNextStep()}
      
      <Box display='flex' flexDirection='row' marginTop={2} marginLeft={5} marginRight={20}>
        <Button color="inherit"  variant={isLastStep ? "outlined" : undefined} onClick={closeLatencyTest}>
          {isLastStep ? 'Exit' : 'Abort'}
        </Button>
        <Box sx={{ flex: '1 1 auto' }} />
        <Button onClick={handleBack} disabled={activeStep === 0}>Back</Button>
        <Button onClick={handleNext} disabled={!nextEnabled || isLastStep} variant="outlined">
          {activeStep === 0 ? 'Start' : 'Next'}
        </Button>
      </Box>
    </>
  );
}

function StepIntroduction() {
  return (
    <LatencyTestStep>
      <Typography>
        This test measures the round trip time (RTT) of your system which defines the latency from the drum hit to the sound you hear from the speakers.
      </Typography>
      <p>The latency path consists of following parts:</p>
      <Box width='80%' minWidth='400px' display='flex' color={'white'} justifyContent='center' alignItems={'center'}>
        <LatencySvg className="latencySvg" width={'90%'} />
      </Box>
      <ol>
        <li>Hit Detection Time of the Trigger Module: this is equal to the choosen Scan-Time (e.g. 3ms)</li>
        <li>Midi to Computer (via USB or BT): for USB insignificant (way below 1ms)</li>
        <li>Processing by the Drum Software</li>
        <li>Computer to Soundcard: most part of the latency is introduced here. Highly depends on the soundcard hardware and drivers
          <ul>
            <li>If available use ASIO drivers (on Windows) to reduce the latency</li>
            <li>Reduce the buffer sizes as much as possible</li>
          </ul>
        </li>
      </ol>
      <Typography>
        The test will trigger a drum sound via Midi and measure the time until the sound from the soundcard is received. See latency path parts (2) - (4) in the illustration.
      </Typography>
      <Alert severity="error">
        During the procedure you will have to connect your soundcard to the trigger module.
        This may <b>cause damage</b> to soundcard or trigger module, as the inputs have an 1.5V DC offset voltage.<br />
        <b>Continue only at your own risk!</b>
      </Alert>
    </LatencyTestStep>
  );
}

function StepSelectChannel({ setNextEnabled } : {
  setNextEnabled: (enabled: boolean) => void
}) {
  const monitoredPadIndex = useConfig(config => config.monitor.padIndex);
  const isMux = isPadPinConnectedToMux(0, monitoredPadIndex);

  useEffect(() => {
    if (isMux) {
      // try to activate latency test if pad changed as latency test cannot be enabled if no monitored pad is selected
      connection.sendLatencyTestOnCommand('preview', 0);
    }
    setNextEnabled(isMux);
  }, [setNextEnabled, isMux, monitoredPadIndex]);

  return (
    <LatencyTestStep>
      <Typography sx={{ mt: 2, mb: 1 }}>
        Now select the input of the trigger module for monitoring that you want to connect your soundcard to.
      </Typography>
      <Typography>Current monitor selection:</Typography>
      <MuxMonitorInputCheck />
      <Alert sx={{width: 'fit-content'}} severity="info">
        Only the first pin of the input will trigger the playback detection (usually the left channel of a TRS connector).<br />
        This pin also has to be a multiplexer input, as other inputs might be damaged by the soundcard&apos;s output.
      </Alert>
    </LatencyTestStep>
  );
}

function StepConnectSoundcard() {
  return (
    <>
      <LatencyTestStep>
        <Typography>
          Now connect your Soundcard to the selected monitor input.
        </Typography>
        <ul>
          <li>You should see the signal of the soundcard in the signal graph below</li>
          <li>Make sure that no sound is played at the moment</li>
          <li>An automatic noise-floor offset calibration will take place</li>
          <li>Wait until the signal settles at 0%, then press &quot;Next&quot;</li>
        </ul>
      </LatencyTestStep>
      <LatencyMonitor />
    </>
  );
}

function StepAdjustTriggerSettings({threshold, setThreshold, midiNote, setMidiNote} : {
  threshold: number,
  setThreshold: (value: number) => void,
  midiNote: number,
  setMidiNote: (value: number) => void,
}) {
  const monitoredPadIndex = useConfig(config => config.monitor.padIndex);
  if (monitoredPadIndex === undefined) {
    return "No pad selected";
  }

  function onMidiTestClicked() {
    connection.sendCommand(DrumCommand.playNote, { note: midiNote });
  };

  function changeValues(newValues: number[]) {
    setThreshold(newValues[0]);
  }
  
  function commitValues(newValues: number[]) {
    const newThreshold = newValues[0];
    setThreshold(newThreshold);
    connection.sendLatencyTestOnCommand('preview', newThreshold);
  };
  
  return (
    <>
      <LatencyTestStep>
        <Typography>
          1. Set the trigger threshold slightly above the signal level so that the monitor is not triggered anymore.
        </Typography>
        <Box width='40%' paddingLeft={5}>
          <SettingSlider label="Trigger Threshold" {...createThresholdSliderProps()}
            count={1}
            settingValues={[threshold]}
            changeValueFunc={changeValues}  
            commitValueFunc={commitValues} 
          />
        </Box>
        <Typography>
            2. Now open your Drum Software and press the &quot;Play MIDI note&quot; button below to trigger the playback of the MIDI note.<br/>
        </Typography>
        <Stack direction='row' width='fit-content' paddingLeft={5} spacing={2}>
          <MidiNoteSelector configNote={midiNote} onNoteChange={setMidiNote} />
          <Button variant="outlined" onClick={onMidiTestClicked}>Play MIDI note</Button>
        </Stack>
        <Box>
          This will play the sound via the soundcard and trigger the selected input. As a result a part of the soundwave will be shown in the graph.
          <p>
          Repeat steps (1) and (2) until the trigger monitor only refreshes when the MIDI button is pressed.
          </p>
        </Box>
      </LatencyTestStep>
      <LatencyMonitor />
    </>
  );
}


function StepFinalTest({threshold, midiNote, setMidiNote} : {
  threshold: number,
  midiNote: number,
  setMidiNote: (value: number) => void,
}) {
  const [latencyMs, setLatencyMs] = useState<number>();

  useEffect(() => {
    const handle = connection.registerOnBinaryDataListener(monitorData => {
      const message = new MonitorMessage(monitorData);
      if (message.latencyUs > 0) {
        setLatencyMs(message.latencyUs / 1000);
      } else {
        setLatencyMs(undefined);
      }
    });

    return () => {
      connection.unregisterListener(handle);
    };
  }, []);

  const monitoredPadIndex = useConfig(config => config.monitor.padIndex);
  if (monitoredPadIndex === undefined) {
    return "No pad selected";
  }

  const settings = getPadSettingsByIndex(monitoredPadIndex);
  const scanTimeMs = settings.scanTimeUs! / 1000;

  function onMeasureLatencyClicked() {
    connection.sendLatencyTestOnCommand('test', threshold, midiNote);
  };
  
  return (
    <>
      <LatencyTestStep>
        <Typography>
          Press the &quot;Measure Latency&quot; button to start the measurement.
        </Typography>
        <Stack direction='row' width='fit-content' paddingLeft={5}>
          <MidiNoteSelector configNote={midiNote} onNoteChange={setMidiNote} />
          <Button variant="outlined" onClick={onMeasureLatencyClicked}>Measure Latency</Button>
        </Stack>
        {
          !latencyMs ? null :
            <Stack>
              Latency results: 
              <InfoBox width='fit-content'>
                <Box>Measured (w/o Hit Detection):</Box>
                <Box>{Number(latencyMs).toFixed(2)} ms</Box>
                <Box>Total (with Hit Detection):</Box>
                <Box>{Number(latencyMs + scanTimeMs).toFixed(2)} ms (= Measured [{Number(latencyMs).toFixed(2)} ms] + Scan-Time [{Number(scanTimeMs).toFixed(2)} ms])</Box>
              </InfoBox>
            </Stack>
        }
        <Typography>
          You can improve the results by using ASIO drivers and reducing the buffer sizes. Note that reducing the buffer size might result in distorted
          playback or cracking sounds.
        </Typography>
      </LatencyTestStep>
      <LatencyMonitor />
    </>
  );
}

function LatencyMonitor() {
  return (
    <Box paddingLeft={5}>
      <Monitor mode={MonitorMode.Latency} showHitGraph={false} />
    </Box>
  );
}

function LatencyTestStep(props: PropsWithChildren) {
  return <Stack marginTop={5} marginBottom={2} marginX={5} spacing={2} width='fit-content'>
    {props.children}
  </Stack>;
}
