//------------------------------------------------------------------------------
//
// File Name:  Logging.cpp
// Author(s):  Michael Fritz
//   Project:  GAM200
//
// Copyright © 2018 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------
#include "Logging.h"

#include <stdio.h>
#include <assert.h>

//------------------------------------------------------------------------------
// Private Variables:
//------------------------------------------------------------------------------

#define LOGGING

static FILE *messageLogFile;
static FILE *warningLogFile;
static FILE *errorLogFile;

//------------------------------------------------------------------------------
// Public Functions:
//------------------------------------------------------------------------------

char *FileName(char *path)
{
  char *name = strrchr(path, '\\') + 1;
  if(name == 0)
  {
    return 0;
  }

  return name;
}

// Logs a message to the message file WITH a newline character.
// For example: Use this to record the time since last frame.
void LogMessageFn(std::string formatString, ...)
{
  va_list argPtr;
  va_start(argPtr, formatString);

  if(messageLogFile == 0)
  {
#ifdef LOGGING
    printf("Message log file is not open!!!\n");
#endif
    return;
  }

  vfprintf_s(messageLogFile, formatString.c_str(), argPtr);
  fprintf_s(messageLogFile, "\n");

  vprintf_s(formatString.c_str(), argPtr);
  printf_s("\n");

  va_end(argPtr);
}

// Logs a message to the warning file WITH a newline character.
// For example: Use this to record a user doing something they
//              probably shouldn't be, but not an error.
void LogWarningFn(std::string formatString, ...)
{
  va_list argPtr;

  va_start(argPtr, formatString);

  if(messageLogFile == 0)
  {
#ifdef LOGGING
    printf("Warning log file is not open!!!\n");
#endif
    return;
  }

  vfprintf(warningLogFile, formatString.c_str(), argPtr);
  fprintf(warningLogFile, "\n");

  // TODO: Add a visual or sound effect to grab attention?
  printf("---WARNING---\n");
  vprintf(formatString.c_str(), argPtr);
  printf("\n");

  va_end(argPtr);
}

// Logs a message to the error file WITH a newline character.
// For example: Use this to record something that would cause the program to crash.
void LogErrorFn(std::string formatString, ...)
{
  va_list argPtr;

  va_start(argPtr, formatString);

  if(messageLogFile == 0)
  {
#ifdef LOGGING
    printf("Error log file is not open!!!\n");
#endif
    return;
  }

  vfprintf(errorLogFile, formatString.c_str(), argPtr);
  fprintf(errorLogFile, "\n");

  // TODO: Add a visual or sound effect to grab attention?
  printf("---ERROR---\n");
  vprintf(formatString.c_str(), argPtr);
  printf("\n");

  va_end(argPtr);

  fclose(errorLogFile);

  assert(0);
}

#ifdef LOGGING
// Initializes the log files
void InitLog()
{
  fopen_s(&messageLogFile, "MessageLog.txt", "wt");
  if(messageLogFile == 0)
  {
    printf("Could not open message log file\n");
    return;
  }

  fopen_s(&warningLogFile, "WarningLog.txt", "wt");
  if(messageLogFile == 0)
  {
    printf("Could not open warning log file\n");
    return;
  }

  fopen_s(&errorLogFile, "ErrorLog.txt", "wt");
  if(messageLogFile == 0)
  {
    printf("Could not open error log file\n");
    return;
  }
}

// Closes the log files
void ExitLog()
{
  fclose(messageLogFile);
}

#else

void InitLog()
{
}

// Closes the log files
void ExitLog()
{
}

#endif