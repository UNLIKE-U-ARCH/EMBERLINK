#pragma once

// Author attribution: Sourish Raj Gubbala.
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <string>
#include <cmath>
#include <cstdarg>

#ifndef RTC_DATA_ATTR
#define RTC_DATA_ATTR
#endif

#ifndef DEVICE_DISCONNECTED_C
#define DEVICE_DISCONNECTED_C (-127.0f)
#endif

#ifndef INPUT
#define INPUT 0
#endif

#ifndef OUTPUT
#define OUTPUT 1
#endif

#ifndef SERIAL_8N1
#define SERIAL_8N1 0x00
#endif

#ifndef A0
#define A0 36
#endif

class String {
public:
    String() = default;
    String(const char* value) : value_(value ? value : "") {}
    String(const std::string& value) : value_(value) {}
    String(int value) : value_(std::to_string(value)) {}
    String(unsigned int value) : value_(std::to_string(value)) {}
    String(long value) : value_(std::to_string(value)) {}
    String(unsigned long value) : value_(std::to_string(value)) {}
    String(float value, int precision = 2) {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%.*f", precision, value);
        value_ = buffer;
    }

    size_t length() const { return value_.length(); }
    bool empty() const { return value_.empty(); }
    int indexOf(char ch) const {
        const auto pos = value_.find(ch);
        return pos == std::string::npos ? -1 : static_cast<int>(pos);
    }
    int indexOf(const char* text) const {
        const auto pos = value_.find(text);
        return pos == std::string::npos ? -1 : static_cast<int>(pos);
    }
    int indexOf(const String& text) const {
        const auto pos = value_.find(text.value_);
        return pos == std::string::npos ? -1 : static_cast<int>(pos);
    }
    void trim() {
        const auto start = value_.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) {
            value_.clear();
            return;
        }
        const auto end = value_.find_last_not_of(" \t\r\n");
        value_ = value_.substr(start, end - start + 1);
    }
    void toUpperCase() {
        for (char& c : value_) {
            if (c >= 'a' && c <= 'z') {
                c = static_cast<char>(c - ('a' - 'A'));
            }
        }
    }
    void replace(const char* from, const char* to) {
        const std::string target(from);
        const std::string replacement(to);
        size_t pos = 0;
        while ((pos = value_.find(target, pos)) != std::string::npos) {
            value_.replace(pos, target.length(), replacement);
            pos += replacement.length();
        }
    }
    String substring(size_t from, size_t to = std::string::npos) const {
        if (from >= value_.size()) {
            return String();
        }
        const size_t end = (to == std::string::npos) ? value_.size() : std::min(to, value_.size());
        return String(value_.substr(from, end - from));
    }
    const char* c_str() const { return value_.c_str(); }
    char& operator[](size_t index) { return value_[index]; }
    const char& operator[](size_t index) const { return value_[index]; }
    String operator+(const String& other) const { return String(value_ + other.value_); }
    String operator+(const char* other) const { return String(value_ + other); }
    String operator+(int value) const { return String(value_ + std::to_string(value)); }
    String operator+(unsigned int value) const { return String(value_ + std::to_string(value)); }
    String operator+(float value) const {
        char buffer[64];
        snprintf(buffer, sizeof(buffer), "%f", value);
        return String(value_ + buffer);
    }
    bool operator==(const String& other) const { return value_ == other.value_; }
    bool operator!=(const String& other) const { return value_ != other.value_; }

    std::string str() const { return value_; }

private:
    std::string value_;
};

inline String operator+(const char* lhs, const String& rhs) {
    return String(lhs) + rhs;
}

class SerialClass {
public:
    void begin(unsigned long, uint32_t = 0, int = 0, int = 0) {}
    void println(const char* text = "") { (void)text; }
    void print(const char* text = "") { (void)text; }
    void print(const String& text) { (void)text; }
    void write(uint8_t value) { (void)value; }
    int available() { return 0; }
    int read() { return 0; }
    void flush() {}
    int printf(const char*, ...) { return 0; }
};

inline SerialClass Serial;
inline SerialClass Serial2;

inline unsigned long millis() { return 0; }
inline void delay(unsigned long) {}
inline void pinMode(int, int) {}
inline int analogRead(int) { return 1024; }
inline int analogReadMilliVolts(int) { return 3300; }
