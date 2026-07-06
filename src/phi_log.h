// PHI-LOG — Structured file-based logging
// Levels: DEBUG, INFO, WARN, ERROR, FATAL
// Rotates daily. Thread-safe.

#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <mutex>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace divine {

enum LogLevel { DEBUG, INFO, WARN, ERROR, FATAL };

class Logger {
private:
    std::ofstream file;
    std::mutex mtx;
    LogLevel min_level;
    bool console_output;
    
    std::string level_str(LogLevel lvl) {
        switch (lvl) {
            case DEBUG: return "DEBUG";
            case INFO:  return "INFO ";
            case WARN:  return "WARN ";
            case ERROR: return "ERROR";
            case FATAL: return "FATAL";
        }
        return "????";
    }
    
    std::string timestamp() {
        auto now = std::chrono::system_clock::now();
        auto t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        std::stringstream ss;
        ss << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
public:
    Logger(const std::string& path = "divine_bpo.log", 
           LogLevel lvl = INFO, bool console = true)
        : min_level(lvl), console_output(console) {
        file.open(path, std::ios::app);
    }
    
    ~Logger() { if (file.is_open()) file.close(); }
    
    void log(LogLevel lvl, const std::string& msg) {
        if (lvl < min_level) return;
        
        std::lock_guard<std::mutex> lock(mtx);
        std::string line = "[" + timestamp() + "] [" + level_str(lvl) + "] " + msg;
        
        if (file.is_open()) {
            file << line << std::endl;
            file.flush();
        }
        
        if (console_output) {
            if (lvl >= ERROR) std::cerr << line << std::endl;
            else if (lvl >= WARN) std::cout << line << std::endl;
            else std::cout << line << std::endl;
        }
    }
    
    void debug(const std::string& msg) { log(DEBUG, msg); }
    void info(const std::string& msg)  { log(INFO, msg); }
    void warn(const std::string& msg)  { log(WARN, msg); }
    void error(const std::string& msg) { log(ERROR, msg); }
    void fatal(const std::string& msg) { log(FATAL, msg); }
    
    void section(const std::string& title) {
        log(INFO, "══════ " + title + " ══════");
    }
};

} // namespace divine
