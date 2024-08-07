#pragma once

#include <iostream>
#include <ctime>
#include <windows.h>
#include <iomanip>
#include <fstream>

namespace logger
{
    enum LogLevel
    {
        INFO,
        WARNING,
        ERR
    };

    inline std::ofstream file;

    inline void initialize_file()
    {
        file = std::ofstream(("memprocfs_logs.txt"), std::ofstream::out | std::ofstream::trunc);
    }

    inline void output(LogLevel level, const char* format, va_list args)
    {
        std::time_t t = std::time(nullptr);
        std::tm tm;

        localtime_s(&tm, &t);

        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

        switch (level) {
        case LogLevel::INFO:
            SetConsoleTextAttribute(hConsole, 10);
            std::cout << ("[Kneegrow] ");
            break;
        case LogLevel::WARNING:
            SetConsoleTextAttribute(hConsole, 6);
            std::cout << ("[WARNING] ");
            break;
        case LogLevel::ERR:
            SetConsoleTextAttribute(hConsole, 4);
            std::cout << ("[ERROR] ");
            break;
        }

        SetConsoleTextAttribute(hConsole, 15);
        vprintf(format, args);
    }

    inline void info(const char* format, ...)
    {
        va_list args;
        va_start(args, format);

        output(LogLevel::INFO, format, args);

        char buffer[4096] = { 0 };
        vsprintf(buffer, format, args);
        file << buffer << std::endl;

        va_end(args);
    }

    inline void warning(const char* format, ...)
    {
        va_list args;
        va_start(args, format);

        output(LogLevel::WARNING, format, args);

        char buffer[4096] = { 0 };
        vsprintf(buffer, format, args);
        file << buffer << std::endl;

        va_end(args);
    }

    inline void error(const char* format, ...)
    {
        va_list args;
        va_start(args, format);

        output(LogLevel::ERR, format, args);

        char buffer[4096] = { 0 };
        vsprintf(buffer, format, args);
        file << buffer << std::endl;

        va_end(args);
    }


}