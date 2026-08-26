/*
    debug_log.h
    R.A.Lincoln   2026

    Logging macros


    // Logging static text and variables together
    LOG("Loco %d speed updated to %d (Forward: %s)", locoAddress, speedStep, isForward ? "YES" : "NO");

    // Logging simple variable status
    LOG("Raw speed byte received: %d", rawSpeedByte);

    // Logging hex values or pointers
    LOG("Buffer pointer: %p, char: 0x%02X", buffer, buffer[0]);

    Integer / int	        %d or %i	    LOG("Count: %d", count);
    Unsigned / uint16_t	  %u	          LOG("Address: %u", address);
    Hexadecimal	          %X or %02X	  LOG("Hex: 0x%02X", byteVal);
    Boolean / String	    %s	          LOG("Active: %s", state ? "true" : "false");
    Float / Double	      %f	          LOG("Voltage: %.2fV", voltage);
    Character	            %c	          LOG("Char: %c", myChar);

*/



// debug_log.h
#pragma once
#include <Arduino.h>

#define LOG_LEVEL_NONE    0
#define LOG_LEVEL_ERROR   1
#define LOG_LEVEL_WARN    2
#define LOG_LEVEL_INFO    3
#define LOG_LEVEL_DEBUG   4
#define LOG_LEVEL_VERBOSE 5

// Default build level if not explicitly defined prior to include
#ifndef LOG_LEVEL
  #define LOG_LEVEL LOG_LEVEL_DEBUG
#endif

#define _LOG_FORMAT(tag, fmt, ...) \
    Serial.printf("[%08lu][%s][%s:%d] " fmt "\n", millis(), tag, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define LOG(fmt, ...) _LOG_FORMAT("LOG", fmt, ##__VA_ARGS__)
#define STOP while(true) yield(); delay(10);

#if (LOG_LEVEL >= LOG_LEVEL_ERROR)
  #define LOGE(fmt, ...) _LOG_FORMAT("ERR", fmt, ##__VA_ARGS__)
#else
  #define LOGE(fmt, ...) ((void)0)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_WARN)
  #define LOGW(fmt, ...) _LOG_FORMAT("WRN", fmt, ##__VA_ARGS__)
#else
  #define LOGW(fmt, ...) ((void)0)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_INFO)
  #define LOGI(fmt, ...) _LOG_FORMAT("INF", fmt, ##__VA_ARGS__)
#else
  #define LOGI(fmt, ...) ((void)0)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_DEBUG)
  #define LOGD(fmt, ...) _LOG_FORMAT("DBG", fmt, ##__VA_ARGS__)
#else
  #define LOGD(fmt, ...) ((void)0)
#endif

#if (LOG_LEVEL >= LOG_LEVEL_VERBOSE)
  #define LOGV(fmt, ...) _LOG_FORMAT("VRB", fmt, ##__VA_ARGS__)
#else
  #define LOGV(fmt, ...) ((void)0)
#endif
