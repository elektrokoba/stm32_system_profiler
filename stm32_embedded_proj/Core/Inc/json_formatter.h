/**
  ******************************************************************************
  * @file    json_formatter.h
  * @brief   JSON Formatter Module - Formats system reports as JSON
  ******************************************************************************
  */

#ifndef __JSON_FORMATTER_H
#define __JSON_FORMATTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "system_profiler.h"

/* Function prototypes */
void FormatSystemReportJSON(const SystemReport_t *report, char *buffer, size_t bufferSize);

#ifdef __cplusplus
}
#endif

#endif /* __JSON_FORMATTER_H */
