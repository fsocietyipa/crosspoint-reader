#pragma once
#include <string>
namespace obfuscation {
// Host test models credential round-tripping, not device-key cryptography.
inline std::string obfuscateToBase64(const std::string& value) { return value; }
}  // namespace obfuscation
