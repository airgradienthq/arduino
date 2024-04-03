#include "PrintLog.h"

/**
 * @brief Construct a new Print Log:: Print Log object
 *
 * @param log Log stream
 * @param tag Tag name
 */
PrintLog::PrintLog(Stream &log, String tag) : log(log), tag(tag)
{
}

PrintLog::~PrintLog()
{
}

/**
 * @brief Print log info
 *
 * @param info Log message
 */
void PrintLog::logInfo(String &info)
{
  logInfo(info.c_str());
}

/**
 * @brief Print log info
 *
 * @param info Log message
 */
void PrintLog::logInfo(const char *info)
{
  log.printf("[%s] Info: %s\r\n", tag.c_str(), info);
}

/**
 * @brief Print log error
 *
 * @param err Log message
 */
void PrintLog::logError(String &err)
{
  logError(err.c_str());
}

/**
 * @brief Print log error
 *
 * @param err Log message
 */
void PrintLog::logError(const char *err)
{
  log.printf("[%s] Error: %s\r\n", tag.c_str(), err);
}

/**
 * @brief Print log warning
 *
 * @param warning Log message
 */
void PrintLog::logWarning(String &warning)
{
  logWarning(warning.c_str());
}

/**
 * @brief Print log warning
 *
 * @param warning Log message
 */
void PrintLog::logWarning(const char *warning)
{
  log.printf("[%s] Warning: %s\r\n", tag.c_str(), warning);
}
