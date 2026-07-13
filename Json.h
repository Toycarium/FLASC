#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace flasc::json {

// Minimal, write-only JSON DOM. FLASC never needs to *read* JSON - the ivam-format
// JSON we produce is only ever written, not parsed back in - so this intentionally
// skips parsing entirely. Kept dependency-free (no external JSON library) to keep
// the compiled binary small.
class JNode {
public:
    virtual ~JNode() = default;
    virtual void Write(std::string& out, int indent) const = 0;

    std::string ToJsonString() const {
        std::string out;
        Write(out, 0);
        return out;
    }

protected:
    static void WriteIndent(std::string& out, int indent) {
        out.append(static_cast<size_t>(indent) * 2, ' ');
    }
};

// An ordered set of key/value pairs. Write-only: keys are appended in the order
// they're assigned, duplicates are not checked for (callers are expected not to
// assign the same key twice).
class JObject : public JNode {
public:
    void Set(std::string key, std::unique_ptr<JNode> value) {
        members_.emplace_back(std::move(key), std::move(value));
    }

    void Write(std::string& out, int indent) const override;

private:
    std::vector<std::pair<std::string, std::unique_ptr<JNode>>> members_;
};

class JString : public JNode {
public:
    explicit JString(std::string value) : value_(std::move(value)) {}

    void Write(std::string& out, int indent) const override {
        (void)indent;
        WriteEscaped(out, value_);
    }

    static void WriteEscaped(std::string& out, const std::string& value);

private:
    std::string value_;
};

// Any whole-number field (Int8/Int16/Int32/UInt32 - held as long long internally
// so a UInt32 nametableOffset never overflows).
class JInt : public JNode {
public:
    explicit JInt(long long value) : value_(value) {}

    void Write(std::string& out, int indent) const override {
        (void)indent;
        out += std::to_string(value_);
    }

private:
    long long value_;
};

// A float field. Always writes with a decimal point (e.g. "640.0", never "640"),
// matching ivam's own formatting.
class JFloat : public JNode {
public:
    explicit JFloat(double value) : value_(value) {}

    void Write(std::string& out, int indent) const override;

private:
    double value_;
};

inline std::unique_ptr<JNode> MakeString(std::string value) {
    return std::make_unique<JString>(std::move(value));
}
inline std::unique_ptr<JNode> MakeInt(long long value) {
    return std::make_unique<JInt>(value);
}
inline std::unique_ptr<JNode> MakeFloat(double value) {
    return std::make_unique<JFloat>(value);
}

} // namespace flasc::json
