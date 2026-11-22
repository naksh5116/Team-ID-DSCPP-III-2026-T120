// Json.hpp - a small JSON writer for the web API (no library).
//
//   JsonWriter j;
//   j.beginObject().field("id", "VR-2026-000004").field("age", 67)
//    .key("visits").beginArray().value("a").value("b").endArray()
//    .endObject();
//   j.str()  ->  {"id":"VR-2026-000004","age":67,"visits":["a","b"]}
//
// Commas are placed automatically; strings are escaped for JSON.
#ifndef VR_WEB_JSON_HPP
#define VR_WEB_JSON_HPP

#include <cstdio>
#include <string>
#include <vector>

class JsonWriter {
public:
    JsonWriter& beginObject() { open('{'); return *this; }
    JsonWriter& endObject() { close('}'); return *this; }
    JsonWriter& beginArray() { open('['); return *this; }
    JsonWriter& endArray() { close(']'); return *this; }

    JsonWriter& key(const std::string& k)
    {
        comma();
        out_ += quote(k);
        out_ += ':';
        afterKey_ = true;
        return *this;
    }

    JsonWriter& value(const std::string& v) { raw(quote(v)); return *this; }
    JsonWriter& value(const char* v) { raw(quote(v ? v : "")); return *this; }
    JsonWriter& value(bool v) { raw(v ? "true" : "false"); return *this; }
    JsonWriter& value(int v) { raw(std::to_string(v)); return *this; }
    JsonWriter& value(long v) { raw(std::to_string(v)); return *this; }
    JsonWriter& value(long long v) { raw(std::to_string(v)); return *this; }
    JsonWriter& value(unsigned long v) { raw(std::to_string(v)); return *this; }
    JsonWriter& value(unsigned long long v) { raw(std::to_string(v)); return *this; }
    JsonWriter& value(double v)
    {
        char buf[32];
        std::snprintf(buf, sizeof buf, "%.2f", v);   // amounts and metres: two decimals
        raw(buf);
        return *this;
    }
    JsonWriter& null() { raw("null"); return *this; }

    template <typename T>
    JsonWriter& field(const std::string& k, const T& v)
    {
        key(k);
        return value(v);
    }

    const std::string& str() const { return out_; }

    static std::string quote(const std::string& s)
    {
        std::string q = "\"";
        for (unsigned char ch : s) {
            switch (ch) {
            case '"': q += "\\\""; break;
            case '\\': q += "\\\\"; break;
            case '\n': q += "\\n"; break;
            case '\r': q += "\\r"; break;
            case '\t': q += "\\t"; break;
            default:
                if (ch < 0x20) {
                    char buf[8];
                    std::snprintf(buf, sizeof buf, "\\u%04x", ch);
                    q += buf;
                } else {
                    q += static_cast<char>(ch);
                }
            }
        }
        return q + "\"";
    }

private:
    void comma()
    {
        if (afterKey_)
            return;
        if (!first_.empty()) {
            if (!first_.back())
                out_ += ',';
            first_.back() = false;
        }
    }
    void raw(const std::string& text)
    {
        comma();
        afterKey_ = false;
        out_ += text;
    }
    void open(char c)
    {
        comma();
        afterKey_ = false;
        out_ += c;
        first_.push_back(true);
    }
    void close(char c)
    {
        out_ += c;
        if (!first_.empty())
            first_.pop_back();
    }

    std::string out_;
    std::vector<bool> first_;   // per open container: nothing written yet
    bool afterKey_ = false;
};

#endif // VR_WEB_JSON_HPP
