// Copyright (c) 2025 Tobias Gunkel
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#define YAML_DISABLE_CJSON // disable all cJSON functions
#include <ArduinoYaml.h>
#include "event_log.h"

struct yaml_stream_handler_data_t_copy {
  Stream* streamPtr;
  size_t* size;
};

class YAMLParser {
public:
  static void parseConfig(fs::File& localeFile, JsonObject& jsonObject) {
    YAMLNode yamlnode = loadStream(localeFile);
    if (yamlnode.isNull()) {
      eventLog.log(Level::ERROR, String("Could not load config"));
      return;
    }

    DeserializationError error = deserializeYml_JsonObject(yamlnode.getDocument(), yamlnode.getNode(), jsonObject);
    if (error) {
      eventLog.log(Level::ERROR, String("Could not deserialize config: ") + error.c_str());
      return;
    }
  }

private:
  static void handle_parser_error(yaml_parser_t* p) {
    switch (p->error) {
    case YAML_MEMORY_ERROR:
      eventLog.log(Level::ERROR, "Memory error: Not enough memory for parsing");
      break;
    case YAML_READER_ERROR:
      if (p->problem_value != -1)
        eventLog.log(Level::ERROR, String(p->problem) + ": #" + p->problem_value + " at " + (long)p->problem_offset);
      else
        eventLog.log(Level::ERROR, String(p->problem) + " at " + (long)p->problem_offset);
      break;
    case YAML_PARSER_ERROR:
    case YAML_SCANNER_ERROR:
      if (p->context)
        eventLog.log(Level::ERROR, String(p->context) + " at line " + (long)(p->context_mark.line + 1) + ", column " + (long)(p->context_mark.column + 1) + "\n"
            + p->problem + " at line " + (long)(p->problem_mark.line + 1) + ", column " + (long)(p->problem_mark.column + 1));
      else
        eventLog.log(Level::ERROR, String(p->problem) + " at line " + (long)(p->problem_mark.line + 1) + ", column " + (long)(p->problem_mark.column + 1));
      break;
    case YAML_COMPOSER_ERROR:
    case YAML_WRITER_ERROR:
    case YAML_EMITTER_ERROR:
    case YAML_NO_ERROR:
      break;
    }
  }

  static YAMLNode loadStream(fs::File& localeFile) {
    yaml_parser_t parser;

    if (yaml_parser_initialize(&parser) != 1) {
      handle_parser_error(&parser);
    }

    yaml_parser_set_encoding(&parser, YAML_UTF8_ENCODING);

    size_t bytes_read = 0;
    yaml_stream_handler_data_t_copy shd = {&localeFile, &bytes_read};
    yaml_parser_set_input(&parser, &_yaml_stream_reader, &shd);

    std::shared_ptr<yaml_document_t> document = CreateDocument();
    if (yaml_parser_load(&parser, document.get()) != 1) {
      handle_parser_error(&parser);
    }

    yaml_parser_delete(&parser);

    yaml_node_t* root = yaml_document_get_root_node(document.get());
    return YAMLNode(document, root);
  }
};
