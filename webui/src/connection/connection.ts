// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { DrumPadConfig, DrumPadMappings, DrumPadSettings, updateConfig } from '@config';

export enum DrumCommand {
  getConfig = "getConfig",
  setConfig = "setConfig",
  setPadConfig = "setPadConfig",
  setSettings = "setSettings",
  setMappings = "setMappings",
  setGeneral = "setGeneral",
  setMonitor = "setMonitor",
  playNote = "playNote",
  saveConfig = "saveConfig",
  restoreConfig = "restoreConfig",
  latencyTest = "latencyTest",
  triggerMonitor = "triggerMonitor",
  getEvents = "getEvents",
  getStats = "getStats"
}

enum ConnectionEventType {
    onConnectionChangeEvent = 'onConnectionChange',
    onConnectionBinaryData = 'onConnectionBinaryData',
    onConnectionJsonData = 'onConnectionJsonData'
}

interface EventListenerHandle {
    type: string;
    listener: EventListener;
}

export class Connection {
  websock?: WebSocket;
  timeoutID?: number;

  connect() {
    if (this.websock) {
      console.log("Connection already activated");
    } else {
      console.log("Connection activated");
      this.connectInternal();
    }
  }

  private connectInternal() {
    const sock = new WebSocket("ws://" + location.host + "/ws");
    this.websock = sock;
    sock.binaryType = "arraybuffer";

    sock.onopen = () => {
      if (this.websock !== sock) {
        return;
      }

      console.log("Websock open");
      this.publishEvent(ConnectionEventType.onConnectionChangeEvent, true);
    };
    sock.onclose = (event: CloseEvent) => {
      if (this.websock !== sock) {
        console.log(`Websock closed (code: ${event.code})`);
        return;
      }

      this.publishEvent(ConnectionEventType.onConnectionChangeEvent, false);

      // if websocket instance is still in use (i.e. disconnect() was not called), try to reconnect
      console.log(`Websock closed (code: ${event.code}) -> reconnect`);
      this.timeoutID = setTimeout(() => {
        this.connectInternal();
      }, 1000);
    };
    sock.onerror = () => {
      if (this.websock === sock) {
        //console.log("Unknown websock error");
      }
    };
    sock.onmessage = (event: MessageEvent) => {
      if (this.websock !== sock) {
        return;
      }

      if (event.data instanceof ArrayBuffer) {
        this.publishEvent(ConnectionEventType.onConnectionBinaryData, event.data);
      } else {
        const json = JSON.parse(event.data);
        this.publishEvent(ConnectionEventType.onConnectionJsonData, json);
      }
    };
  }

  disconnect() {
    console.log("Connection deactivated");
    clearTimeout(this.timeoutID);

    const sock = this.websock;
    this.websock = undefined;
    sock?.close();
  }

  send(data: string | ArrayBufferLike | ArrayBufferView): void {
    if (this.websock?.readyState === WebSocket.OPEN) {
      this.websock?.send(data);
    }
  }

  sendCommand(command: DrumCommand, args?: any) {
    this.send(JSON.stringify({ [command]: { ...args }}));
  }

  sendCommandWithDirtyFlag(command: DrumCommand, args: any, dirty: boolean) {
    if (dirty) {
      updateConfig(config => config.isDirty = true);
    }    
    this.sendCommand(command, args);
  }

  sendSetConfigCommand(values: Partial<DrumPadConfig>) {
    this.sendCommandWithDirtyFlag(DrumCommand.setConfig, { ...values }, true);
  }

  sendSetPadConfigCommand(padIndex: number, values: Partial<DrumPadConfig>) {
    this.sendCommandWithDirtyFlag(DrumCommand.setPadConfig, {[padIndex]: { ...values }}, true);
  }

  sendSetSettingsCommand(values: any) {
    this.sendCommandWithDirtyFlag(DrumCommand.setSettings, { ...values }, true);
  }

  sendSetPadSettingsCommand(padIndex: number, values: Partial<DrumPadSettings>) {
    this.sendSetSettingsCommand({[padIndex]: { ...values }});
  }

  sendSetMappingsCommand(values: any, replace = false) {
    this.sendCommandWithDirtyFlag(DrumCommand.setMappings, { ...values, _replace: replace }, true);
  }

  sendSetRoleMappingsCommand(padRole: string, values: Partial<DrumPadMappings>, replace = false) {
    this.sendSetMappingsCommand({[padRole]: { ...values }}, replace);
  }

  sendLatencyTestOnCommand(mode: 'preview' | 'test', threshold: number, midiNote?: number) {
    connection.sendCommand(DrumCommand.latencyTest, {
      enabled: true,
      preview: mode === 'preview',
      threshold: threshold,
      midiNote: midiNote
    });
  }

  sendLatencyTestOffCommand() {
    connection.sendCommand(DrumCommand.latencyTest, { enabled: false });
  }

  registerOnChangeListener(listener: (connected: boolean) => void) {
    return this.registerListener(ConnectionEventType.onConnectionChangeEvent,
      (event: CustomEventInit) => listener(event.detail));
  }

  registerOnBinaryDataListener(listener: (data: ArrayBuffer) => void): EventListenerHandle {
    return this.registerListener(ConnectionEventType.onConnectionBinaryData,
      (event: CustomEventInit) => listener(event.detail));
  }

  registerOnJsonDataListener(type: string, listener: (data: any) => void) {
    return this.registerListener(ConnectionEventType.onConnectionJsonData,
      (event: CustomEventInit) => {
        const data = event.detail;
        if (data[type]) {
          listener(data[type]);
        }
      });
  }

  unregisterListener(handle: EventListenerHandle) {
    document.removeEventListener(handle.type, handle.listener);
  }

  private registerListener(type: string, customEventListener: EventListener): EventListenerHandle {
    document.addEventListener(type, customEventListener);
    return { type: type, listener: customEventListener };
  }

  private publishEvent(type: ConnectionEventType, data: any) {
    const event = new CustomEvent(type, { detail: data });
    document.dispatchEvent(event);
  }
}

export let connection: Connection;

export function initConnection() {
  connection = new Connection();
}
