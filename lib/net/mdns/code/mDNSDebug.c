#include "mDNSDebug.h"
#include "mdnsembeddedapi.h"


mDNSexport int mDNS_LoggingEnabled = 0;
mDNSexport int mDNS_PacketLoggingEnabled = 0;

#if MDNS_DEBUGMSGS
mDNSexport int mDNS_DebugMode = mDNStrue;
#else
mDNSexport int mDNS_DebugMode = mDNSfalse;
#endif

// Note, this uses mDNS_vsnprintf instead of standard "vsnprintf", because mDNS_vsnprintf knows
// how to print special data types like IP addresses and length-prefixed domain names
#if MDNS_DEBUGMSGS > 1
mDNSexport void verbosedebugf_(const char *format, ...)
{
    char buffer[512];
    va_list ptr;
    va_start(ptr, format);
    buffer[mDNS_vsnprintf(buffer, sizeof(buffer), format, ptr)] = 0;
    va_end(ptr);
    mDNSPlatformWriteDebugMsg(buffer);
}
#endif

// Log message with default "mDNSResponder" ident string at the start
mDNSlocal void LogMsgWithLevelv(mDNSLogLevel_t logLevel, const char *format, va_list ptr)
{
    char buffer[512];
    buffer[mDNS_vsnprintf((char *)buffer, sizeof(buffer), format, ptr)] = 0;
    mDNSPlatformWriteLogMsg(ProgramName, buffer, logLevel);
}

#define LOG_HELPER_BODY(L) \
	{ \
	va_list ptr; \
	va_start(ptr,format); \
	LogMsgWithLevelv(L, format, ptr); \
	va_end(ptr); \
	}

// see mDNSDebug.h
#if !MDNS_HAS_VA_ARG_MACROS
void LogMsg_(const char *format, ...)       LOG_HELPER_BODY(MDNS_LOG_MSG)
void LogOperation_(const char *format, ...) LOG_HELPER_BODY(MDNS_LOG_OPERATION)
void LogSPS_(const char *format, ...)       LOG_HELPER_BODY(MDNS_LOG_SPS)
void LogInfo_(const char *format, ...)      LOG_HELPER_BODY(MDNS_LOG_INFO)
#endif

#if MDNS_DEBUGMSGS
void debugf_(const char *format, ...)       LOG_HELPER_BODY(MDNS_LOG_DEBUG)
#endif

// Log message with default "mDNSResponder" ident string at the start
mDNSexport void LogMsgWithLevel(mDNSLogLevel_t logLevel, const char *format, ...)
LOG_HELPER_BODY(logLevel)
