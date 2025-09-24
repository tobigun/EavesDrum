// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { defineConfig } from 'vite';
import { compression } from 'vite-plugin-compression2';
import svgr from 'vite-plugin-svgr';
import { visualizer } from 'rollup-plugin-visualizer';
import react from '@vitejs/plugin-react';
import packageJson from './package.json';
import { VersionInfo } from './src/version';
import { simpleGit } from 'simple-git';
import tsconfigPaths from "vite-tsconfig-paths";

//const proxyHost = '192.168.7.1:80'; // real device
const proxyHost = '127.0.0.1:80'; // simulation

const commitHash = await simpleGit('..').revparse(['--short', 'HEAD']);
const buildTime = new Date().toISOString().split('.')[0] + "Z";

// https://vite.dev/config/
export default defineConfig(({ mode }) => {
  const isDevMode = (mode === 'development');
  return {
    build: {
      outDir: '../data',
      emptyOutDir: true, // also necessary
      sourcemap: isDevMode
    },
    define: {
      'APP_VERSION': {
        packageVersion: packageJson.version,
        gitCommitHash: commitHash,
        buildTime: buildTime
      } as VersionInfo
    },
    plugins: [
      false! && visualizer({ open: false, filename: 'bundle-visualization.html' }),
      svgr(),
      react(),
      tsconfigPaths(),
      compression({
        exclude: /\.(yaml)$/,
        algorithms: ['gzip'],
        deleteOriginalAssets: true
      })
    ],
    server: {
      port: 8080,
      strictPort: true,
      proxy: {
        '/config.jsonc': `http://${proxyHost}`,
        '/config.yaml': `http://${proxyHost}`,
        '/ws': {
          target: `ws://${proxyHost}/`,
          ws: true,
          secure: false,
          rewriteWsOrigin: true,
          configure: (proxy: any) => {
            proxy.on("proxyReqWs", (proxyReq: any) => {
              console.log(`Proxy: forward WS Request: ${proxyReq.host}:${proxyReq.port ?? 80}${proxyReq.path}`);
            });
            proxy.on("close", () => {
              console.log('Proxy: close');
            });
          }
        }
      }
    }
  };
});
