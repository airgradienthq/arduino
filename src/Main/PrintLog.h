#ifndef _PRINT_LOG_H_
#define _PRINT_LOG_H_

#include <Arduino.h>

class PrintLog
{
private:
  Stream &log;
  String tag;

public:
  PrintLog(Stream &log, String tag);
  ~PrintLog();

  void logInfo(String &info);
  void logInfo(const char* info);
  void logError(String &err);
  void logError(const char* err);
  void logWarning(String &warning);
  void logWarning(const char* warning);
};

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

#endif /** _PRINT_LOG_H_ */
