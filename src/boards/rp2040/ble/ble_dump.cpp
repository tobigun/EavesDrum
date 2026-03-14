#if 0
extern "C" int _write(int handle, char* buffer, int size) {
    Serial1.write(buffer, size);
    return size;
}

void reset() {}

void log_packet(uint8_t packet_type, uint8_t in, uint8_t *packet, uint16_t len) {
  char buffer[256];
  int pos = 0;
  pos += snprintf(buffer + pos, sizeof(buffer) - pos, "HCI Packet: type=%u, in=%u, len=%u, data=", packet_type, in, len);
  for (int i = 0; i < len && pos < sizeof(buffer) - 3; i++) {
    pos += snprintf(buffer + pos, sizeof(buffer) - pos, "%02x ", packet[i]);
  }
  Serial1.println(buffer);
}

void log_message(int log_level, const char * format, va_list argptr) {
  char buffer[256];
  vsnprintf(buffer, sizeof(buffer), format, argptr);
  Serial1.print(buffer);
}

hci_dump_t hci_dump_implementation = {
    .reset = reset,
    .log_packet = log_packet,
    .log_message = log_message
};

  /*
  setvbuf(stdout, NULL, _IONBF, 0); 
  hci_dump_init(&hci_dump_implementation);
  hci_dump_enable_log_level(HCI_DUMP_LOG_LEVEL_DEBUG, 1);
  hci_dump_enable_log_level(HCI_DUMP_LOG_LEVEL_INFO, 1);
  hci_dump_enable_log_level(HCI_DUMP_LOG_LEVEL_ERROR, 1);
  hci_dump_enable_packet_log(true);
  */
#endif
