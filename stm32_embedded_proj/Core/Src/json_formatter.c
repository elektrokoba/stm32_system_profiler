/**
  ******************************************************************************
  * @file    json_formatter.c
  * @brief   JSON Formatter Implementation
  ******************************************************************************
  */

#include "json_formatter.h"
#include <stdio.h>
#include <string.h>

/**
  * @brief  Format system report as JSON string
  * @param  report: Pointer to SystemReport_t structure
  * @param  buffer: Output buffer for JSON string
  * @param  bufferSize: Size of output buffer
  * @retval None
  */
void FormatSystemReportJSON(const SystemReport_t *report, char *buffer, size_t bufferSize)
{
    char *ptr = buffer;
    size_t remaining = bufferSize;
    int written;
    
    /* Start JSON object */
    written = snprintf(ptr, remaining, "{\r\n");
    ptr += written; remaining -= written;
    
    /* Timestamp */
    written = snprintf(ptr, remaining, "  \"timestamp\": %lu,\r\n", report->timestamp);
    ptr += written; remaining -= written;
    
    /* CPU Load */
    written = snprintf(ptr, remaining, "  \"cpu_load\": %.1f,\r\n", report->cpuLoad);
    ptr += written; remaining -= written;
    
    /* Heap statistics */
    written = snprintf(ptr, remaining, "  \"heap_free\": %lu,\r\n", report->heapFree);
    ptr += written; remaining -= written;
    
    written = snprintf(ptr, remaining, "  \"heap_min\": %lu,\r\n", report->heapMin);
    ptr += written; remaining -= written;
    
    written = snprintf(ptr, remaining, "  \"frag_pct\": %.1f,\r\n", report->fragPercent);
    ptr += written; remaining -= written;
    
    /* Tasks array */
    written = snprintf(ptr, remaining, "  \"tasks\": [\r\n");
    ptr += written; remaining -= written;
    
    for (uint8_t i = 0; i < report->taskCount; i++) {
        written = snprintf(ptr, remaining, "    {\"name\": \"%s\", \"runtime_pct\": %.1f, \"stack_free\": %lu}",
                          report->tasks[i].taskName,
                          report->tasks[i].runtimePercent,
                          report->tasks[i].stackFree);
        ptr += written; remaining -= written;
        
        /* Add comma if not last item */
        if (i < report->taskCount - 1) {
            written = snprintf(ptr, remaining, ",\r\n");
        } else {
            written = snprintf(ptr, remaining, "\r\n");
        }
        ptr += written; remaining -= written;
    }
    
    written = snprintf(ptr, remaining, "  ],\r\n");
    ptr += written; remaining -= written;
    
    /* Temperature */
    written = snprintf(ptr, remaining, "  \"temp\": %.1f\r\n", report->temperature);
    ptr += written; remaining -= written;
    
    /* End JSON object */
    written = snprintf(ptr, remaining, "}");
    ptr += written; remaining -= written;
}

/**
  * @brief  Format system report as compact JSON (single line)
  * @param  report: Pointer to SystemReport_t structure
  * @param  buffer: Output buffer for JSON string
  * @param  bufferSize: Size of output buffer
  * @retval None
  */
void FormatSystemReportJSONCompact(const SystemReport_t *report, char *buffer, size_t bufferSize)
{
    char *ptr = buffer;
    size_t remaining = bufferSize;
    int written;
    
    /* Start JSON object */
    written = snprintf(ptr, remaining, "{\"ts\":%lu,\"cpu\":%.1f,\"heap\":%lu,\"min\":%lu,\"frag\":%.1f,\"tasks\":[",
                      report->timestamp, report->cpuLoad, report->heapFree, 
                      report->heapMin, report->fragPercent);
    ptr += written; remaining -= written;
    
    /* Tasks array */
    for (uint8_t i = 0; i < report->taskCount; i++) {
        written = snprintf(ptr, remaining, "{\"n\":\"%s\",\"r\":%.1f,\"s\":%lu}",
                          report->tasks[i].taskName,
                          report->tasks[i].runtimePercent,
                          report->tasks[i].stackFree);
        ptr += written; remaining -= written;
        
        if (i < report->taskCount - 1) {
            written = snprintf(ptr, remaining, ",");
            ptr += written; remaining -= written;
        }
    }
    
    /* Close and add temperature */
    written = snprintf(ptr, remaining, "],\"temp\":%.1f}", report->temperature);
    ptr += written; remaining -= written;
}
