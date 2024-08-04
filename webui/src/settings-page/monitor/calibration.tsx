// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { PropsWithChildren, useEffect } from "react";
import { connection, DrumCommand } from "../../connection/connection";
import { Box, Button, Stack, Step, StepLabel, Stepper, Typography } from "@mui/material";
import { Monitor } from "./monitor-card";
import { MuxMonitorInputCheck } from "./signal-graph/mux-monitor-input-check";

export function CalibrationWizard({closeCalibration} : {
  closeCalibration: () => void
}) {
  /*
  const [activeStep, setActiveStep] = useState(0);  
  const [nextEnabled, setNextEnabled] = useState(true);

  function handleNext() {
    setActiveStep(currentActiveStep => currentActiveStep + 1);
  };

  function handleBack() {
    setNextEnabled(true);
    setActiveStep(currentActiveStep => currentActiveStep - 1);
  };
  */

  const activeStep = 0;
  
  const labels = [
    "DC Voltage Offset Calibration"
  ];

  function getNextStep() {
    switch (activeStep) {
    case 0: return <StepIntroduction />;
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
        <Button color="inherit"  variant={isLastStep ? "outlined" : undefined} onClick={closeCalibration}>
          {isLastStep ? 'Exit' : 'Abort'}
        </Button>
        <Box sx={{ flex: '1 1 auto' }} />
        {/* for future expansion
        <Button onClick={handleBack} disabled={activeStep === 0}>Back</Button>
        <Button onClick={handleNext} disabled={!nextEnabled || isLastStep} variant="outlined">
          {activeStep === 0 ? 'Start' : 'Next'}
        </Button>
        */}
      </Box>
    </>
  );
}

function StepIntroduction() {
  useAutoTriggerInput();

  return (
    <>
      <CalibrationStep>
        <Typography>This test helps you to calibrate the DC offset of the Multiplexers.</Typography>
        <Typography>1. Unplug all cables from the input connectors</Typography>
        <Typography>2. Select one of the inputs that is connected to a Multiplexer (press the record icon of the pad for this)</Typography>
        <MuxMonitorInputCheck />
        <Typography>3. Turn the potentiometer on the PCB with a screwdriver to increase or decrease the voltage offset</Typography>
        <Typography>4. Stop turning the potentiometer when the signal level is approximately at 0% (i.e. the signal touches the Time axis)</Typography>
        <Typography>5. When you are done, click &quot;Exit&quot; to leave the calibration</Typography>
      </CalibrationStep>
      <Box paddingLeft={5}>
        <Monitor showHitGraph={false} />
      </Box>
    </>
  );
}

function useAutoTriggerInput() {
  useEffect(() => {
    const timer = setInterval(() => {
      connection.sendCommand(DrumCommand.triggerMonitor, {});
    }, 100);
    return () => {
      clearInterval(timer);
    };
  }, []);
}

function CalibrationStep(props: PropsWithChildren) {
  return <Stack marginTop={5} marginBottom={2} marginX={5} spacing={2} width='fit-content'>
    {props.children}
  </Stack>;
}
