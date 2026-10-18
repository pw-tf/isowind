#ifndef OPENMW_LOG_HPP
#define OPENMW_LOG_HPP


#ifdef __GNUC__
#pragma GCC system_header
#endif

#if defined(NOLOGS)
#define LOG_INIT(logLevel)
#define LOG_QUIT()
#define LOG_MESSAGE(level, msg, ...)
#define LOG_MESSAGE_SIMPLE(level, msg, ...)
#else
#define LOG_INIT(logLevel) TimedLog::Create(logLevel)
#define LOG_QUIT() TimedLog::Delete()
#if defined(_MSC_VER)
#define LOG_MESSAGE(level, msg, ...) TimedLog::Get().print((level), (1), (__FILE__), (__LINE__), (msg), __VA_ARGS__)
#define LOG_MESSAGE_SIMPLE(level, msg, ...) TimedLog::Get().print((level), (1), (0), (0), (msg), __VA_ARGS__)
#define LOG_APPEND(level, msg, ...) TimedLog::Get().print((level), (0), (0), (0), (msg), __VA_ARGS__)
#else
#define LOG_MESSAGE(level, msg, args...) TimedLog::Get().print((level), (1), (__FILE__), (__LINE__), (msg), ##args)
#define LOG_MESSAGE_SIMPLE(level, msg, args...) TimedLog::Get().print((level), (1), (0), (0), (msg), ##args)
#define LOG_APPEND(level, msg, args...) TimedLog::Get().print((level), (0), (0), (0), (msg), ##args)
#endif
#endif

class TimedLog
{
public:
    enum
    {
        LOG_VERBOSE = 0,
        LOG_INFO,
        LOG_WARN,
        LOG_ERROR,
        LOG_FATAL
    };
    static void Create(int logLevel);
    static void Delete();
    static const TimedLog &Get();
    static int GetLevel();
    static void SetLevel(int level);
    void print(int level, bool hasPrefix, const char *file, int line, const char *message, ...) const;

    static std::string getFilenameTimestamp();
private:
    TimedLog(int logLevel);
    /// Not implemented
    TimedLog(const TimedLog &) = delete;
    /// Not implemented
    TimedLog &operator=(TimedLog &) = delete;
    static TimedLog *sTimedLog;
    int logLevel;
};


#endif //OPENMW_LOG_HPP
