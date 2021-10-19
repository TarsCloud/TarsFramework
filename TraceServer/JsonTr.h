#pragma once

#include <ostream>
#include <string>
#include <iostream>

struct JsonStrWrapper {
public:
    explicit JsonStrWrapper(const char *_p) : p(_p) {}

    const char *const p;
};

template<class T>
struct JsonWrapper {
public:
    explicit JsonWrapper(const T &_t) : t(_t) {};
    const T &t;
};

template<std::size_t N>
static JsonStrWrapper jsonTr(const char *_p) {
    JsonStrWrapper _w(_p);
    return _w;
};

static JsonStrWrapper jsonTr(const char *_p) {
    JsonStrWrapper _w(_p);
    return _w;
};

static JsonStrWrapper jsonTr(const std::string &p) {
    JsonStrWrapper _w(p.c_str());
    return _w;
}

template<typename T>
typename std::enable_if<std::is_arithmetic<T>::value || std::is_enum<T>::value, JsonWrapper<T>>::type
static jsonTr(const T &t) {
    JsonWrapper<T> _w(t);
    return _w;
}

static std::ostream &operator<<(std::ostream &os, const JsonStrWrapper &t) {
    os << "\"" << t.p << "\"";
    return os;
}

template<class T>
typename std::enable_if<!std::is_same<T, bool>::value, std::ostream &>::type
static operator<<(std::ostream &os, const JsonWrapper<T> &t) {
    os << t.t;
    return os;
}

template<class T>
typename std::enable_if<std::is_same<T, bool>::value, std::ostream &>::type
static operator<<(std::ostream &os, const JsonWrapper<T> &t) {
    os << (t.t ? "true" : "false");
    return os;
}