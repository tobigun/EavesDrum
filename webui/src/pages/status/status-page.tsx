// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

import { EventLogInfo } from './event-log';
import { StatisticsTable } from './statistics';

export function StatusPage() {
  return (
    <>
      <StatisticsTable />
      <EventLogInfo />
    </>
  );
}
