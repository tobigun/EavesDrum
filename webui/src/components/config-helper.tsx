// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { Config, useConfig } from "@/config";
import { stringify } from "yaml";

export function downloadCurrentConfig() {
  const configState = useConfig.getState();

  // ignore sections that are UI specific (like isDirty or version)
  const config : Config = {
    general: configState.general,
    mux: configState.mux,
    connectors: configState.connectors,
    pads: configState.pads,
    mappings: configState.mappings
  };

  const configContent = stringify(config);
  const schemaUrl = "https://raw.githubusercontent.com/tobigun/EavesDrum/refs/tags/config-schema-v1.0/config/config.jsonc";
  const schema = "# yaml-language-server: $schema=" + schemaUrl + "\n";

  // simulate a click on an anchor element
  const element = document.createElement("a");
  const downloadFile = new Blob([schema, configContent], {type: 'application/json'});
  element.href = URL.createObjectURL(downloadFile);
  element.download = "config.yaml";
  document.body.appendChild(element);
  element.click(); // required for firefox
};


export async function loadJsonSchema(): Promise<any> {
  const res = await fetch("config.jsonc");
  const schema = await res.text();
  return JSON.parse(schema);
}
