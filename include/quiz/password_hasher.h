#pragma once
#include <string>
#include <string_view>
namespace quiz {
class PasswordHasher {
public:
    PasswordHasher();
    std::string hash(std::string_view password) const;
    bool verify(std::string_view password, const std::string& encoded) const;
    static bool validEncoding(const std::string& encoded);
};
}
