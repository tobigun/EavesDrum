// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { createContext, PropsWithChildren, useEffect, useState } from "react";
import { connection, DrumCommand } from "./connection";

export const ConnectionStateContext = createContext(false);

export function ConnectionStateProvider({ children }: PropsWithChildren) {
  const [connected, setConnected] = useState(false);

  useEffect(() => {
    const onChangeListenerHandle = connection.registerOnChangeListener(connected => {
      setConnected(connected);
      if (connected) {
        connection.sendCommand(DrumCommand.getConfig);
      }
    });
    connection.connect();
    return () => {
      connection.disconnect();
      connection.unregisterListener(onChangeListenerHandle);
    };
  }, []);

  return (
    <ConnectionStateContext.Provider value= { connected } >
      { children }
    </ConnectionStateContext.Provider>
  );
}
