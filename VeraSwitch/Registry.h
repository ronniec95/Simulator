#pragma once
#include <Windows.h>
#include <stb.h>
#include <string>

namespace AARC {
    namespace Registry {

        template <typename CHARTYPE>
        inline auto create_folder_with_key(const CHARTYPE *const loc, const CHARTYPE *const key,
                                           const std::string &value) noexcept {
            static_assert(std::is_same<decltype(loc), const char *const>::value, "Location must be a const char*");
            auto *reg = stb_reg_open("wHKCU", const_cast<char *>(loc));
            if (!(nullptr == reg && key == nullptr && value.empty())) {
                stb_reg_write_string(reg, const_cast<char *>(key), const_cast<char *>(value.c_str()));
            }
            stb_reg_close(reg);
        }
        template <typename CHARTYPE, int N>
        auto create_folder_with_key(const CHARTYPE *const loc, const CHARTYPE *const key,
                                    std::array<uint8_t, N> &value) noexcept {
            auto *reg = stb_reg_open("wHKCU", const_cast<char *>(loc));
            if (!(nullptr == reg && key == nullptr)) {
                stb_reg_write(reg, const_cast<char *>(key), static_cast<void *>(value.data()), N);
            }
            stb_reg_close(reg);
        }

        template <typename CHARTYPE> auto read_string(const CHARTYPE *const loc, const CHARTYPE *const key) noexcept {
            static_assert(std::is_same<decltype(loc), const char *const>::value, "Location must be a const char*");
            auto *reg = stb_reg_open("rHKCU", const_cast<char *>(loc));
            char  buffer[MAX_PATH];
            stb_reg_read_string(reg, const_cast<char *>(key), buffer, sizeof(buffer));
            stb_reg_close(reg);
            return std::string(buffer);
        }
    } // namespace Registry
} // namespace AARC