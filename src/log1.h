#pragma once

#include "Arduino.h"

#define LOG1_LEVEL_TRACE (5)
#define LOG1_LEVEL_DEBUG (4)
#define LOG1_LEVEL_INFO  (3)
#define LOG1_LEVEL_WARN  (2)
#define LOG1_LEVEL_ERROR (1)
#define LOG1_LEVEL_NONE  (0)

#ifndef LOG1_LEVEL
#define LOG1_LEVEL LOG1_LEVEL_NONE
#endif

#define LOG1_TRACE "TRACE: "
#define LOG1_DEBUG "DEBUG: "
#define LOG1_INFO  "INFO:  "
#define LOG1_WARN  "WARN:  "
#define LOG1_ERROR "ERROR: "

#define LOG1_LOG(level, format, ...) \
  Serial.print(level); \
  Serial.printf(__VA_ARGS__); \
  Serial.println();

#if LOG1_LEVEL >= LOG1_LEVEL_ERROR
#define log_e(format, ...) LOG1_LOG(LOG1_ERROR, format, ##__VA_ARGS__);
#else
#define log_e(...) ;
#endif

#if LOG1_LEVEL >= LOG1_LEVEL_WARN
#define log_w(...) LOG1_LOG(LOG1_WARN, format, ##__VA_ARGS__);
#else
#define log_w(...) ;
#endif

#if LOG1_LEVEL >= LOG1_LEVEL_INFO
#define log_i(...) LOG1_LOG(LOG1_INFO, format, ##__VA_ARGS__);
#else
#define log_i(...) ;
#endif

#if LOG1_LEVEL >= LOG1_LEVEL_DEBUG
#define log_d(...) LOG1_LOG(LOG1_DEBUG, format, ##__VA_ARGS__);
#else
#define log_d(...) ;
#endif

#if LOG1_LEVEL >= LOG1_LEVEL_TRACE
#define log_v(...) LOG1_LOG(LOG1_TRACE, format, ##__VA_ARGS__);
#else
#define log_v(...) ;
#endif