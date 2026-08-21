#include "log.h"

#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

static bool logColour;

#ifndef PLATFORM_LOG_DEFINED
#define PLATFORM_LOG_DEFINED
void platformLog(const logType type, const char *format, va_list va) {
    FILE *out = stderr;
    const char* colourPrefix = ANSI_COLOUR_CODE_RESET;
    const char* textPrefix = "";
    switch (type) {
        case LOG_TYPE_NORMAL:
            out = stdout;
            break;
        case LOG_TYPE_WARNING:
            colourPrefix = ANSI_COLOUR_CODE_BOLD_YELLOW;
            textPrefix = "Warning: ";
            break;
        case LOG_TYPE_ERROR:
            colourPrefix = ANSI_COLOUR_CODE_BOLD_RED;
            textPrefix = "Error: ";
            break;
        case LOG_TYPE_DEBUG:
            colourPrefix = ANSI_COLOUR_CODE_BOLD_PURPLE;
            textPrefix = "Debug: ";
            break;
    }

    if (logColour) fputs(colourPrefix, out);
    fputs(textPrefix, out);
    if (logColour) fputs(ANSI_COLOUR_CODE_RESET, out);
    vfprintf(out, format, va);
}
#else
void platformLog(const logType type, const char *format, va_list va);
#endif

void logInfo(const char* fmt, ...) {
	va_list va;

	va_start(va, fmt);
	platformLog(LOG_TYPE_NORMAL, fmt, va);
	va_end(va);
}

void vLogInfo(const char* fmt, va_list va) {
	platformLog(LOG_TYPE_NORMAL, fmt, va);
}

void logWarn(const char* fmt, ...) {
	va_list va;

	va_start(va, fmt);
	platformLog(LOG_TYPE_WARNING, fmt, va);
	va_end(va);
}

void vLogWarn(const char* fmt, va_list va) {
	platformLog(LOG_TYPE_WARNING, fmt, va);
}


void logError(const char* fmt, ...) {
	va_list va;

	va_start(va, fmt);
	platformLog(LOG_TYPE_ERROR, fmt, va);
	va_end(va);
}

void vLogError(const char* fmt, va_list va) {
	platformLog(LOG_TYPE_ERROR, fmt, va);
}

void logDebug(const char* fmt, ...) {
#ifdef DEBUG
	va_list va;

	va_start(va, fmt);
	platformLog(LOG_TYPE_DEBUG, fmt, va);
	va_end(va);
#else
    (void)fmt;
#endif
}

void vLogDebug(const char* fmt, va_list va) {
#ifdef DEBUG
	platformLog(LOG_TYPE_DEBUG, fmt, va);
#else
    (void)fmt;
    (void)va;
#endif
}
