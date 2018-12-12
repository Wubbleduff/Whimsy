//------------------------------------------------------------------------------
//
// File Name:  Logging.h
// Author(s):  Michael Fritz
//   Project:  GAM200
//
// Copyright © 2018 DigiPen (USA) Corporation.
//
//------------------------------------------------------------------------------

#pragma once

#include <stdarg.h>
#include <string>

#define DEV_MODE

//------------------------------------------------------------------------------
// Public Structures:
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Public Functions:
//------------------------------------------------------------------------------

#ifdef DEV_MODE
char *FileName(char *path);

// Logs a message to the message file and the command prompt WITH a newline character.
// For example: Use this to record the time since last frame.
#define LogMessage(formatString, ...) (LogMessageFn("%s:%d - " formatString, FileName(__FILE__), __LINE__, __VA_ARGS__))

// Logs a message to the warning file and the command prompt WITH a newline character.
// For example: Use this to record a user doing something they
//              probably shouldn't be, but not an error.
#define LogWarning(formatString, ...) (LogWarningFn("%s:%d - " formatString, FileName(__FILE__), __LINE__, __VA_ARGS__))

// Logs a message to the error file and the command prompt WITH a newline character.
// For example: Use this to record something that would cause the program to crash.
#define LogError(formatString, ...) (LogErrorFn("%s:%d - " formatString, FileName(__FILE__), __LINE__, __VA_ARGS__))

void LogMessageFn(std::string, ...);
void LogWarningFn(std::string formatString, ...);
void LogErrorFn(std::string formatString, ...);

// Initializes the log files
void InitLog();

// Closes the log files
void ExitLog();
#else

#define LogMessage 
#define LogWarning 
#define LogError 

// Initializes the log files
void InitLog();

// Closes the log files
void ExitLog();

#endif