#include "JoaatHash.h"

#include <algorithm>
#include <cctype>

namespace flasc {
namespace JoaatHash {

uint32_t Compute(const std::string& str) {
    std::string key = str;
    std::transform(key.begin(), key.end(), key.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    uint32_t hash = 0;
    for (unsigned char b : key) {
        hash += b;
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}

std::string ToJsonHashString(const std::string& soundName) {
    if (soundName.empty()) {
        return "0x00000000";
    }

    std::string lower = soundName;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    if (lower == "null") {
        // FLASC's historical convention: "NULL" means "write 4 zero bytes",
        // i.e. hash == 0, NOT the joaat hash of the literal word "NULL".
        return "0x00000000";
    }

    return lower;
}

} // namespace JoaatHash
} // namespace flasc
