#include <array>
#include <catch2/catch_test_macros.hpp>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string>

#include "temp_dir.h"
#include "util/hash.h"
#include "write_file.h"

namespace fs = std::filesystem;

using mpqcli::ComputeFileCrc32;
using mpqcli::ComputeFileMd5;
using mpqcli::testing::TempDir;
using mpqcli::testing::WriteFile;

namespace {

constexpr std::size_t md5_size = 16;

// Every byte value in turn, 70000 of them: more than one 65536-byte read, and holding
// the 0x0A and 0x1A that a text-mode read would mangle on Windows
std::string LargeContent() {
    std::string content(70000, '\0');
    for (std::size_t i = 0; i < content.size(); ++i) {
        content[i] = static_cast<char>(i % 256);
    }
    return content;
}

// The digest as lowercase hex, so a failed comparison prints something readable
std::string Md5Hex(const fs::path &path) {
    std::array<uint8_t, md5_size> digest{};
    REQUIRE(ComputeFileMd5(path, digest.data()));
    std::ostringstream hex;
    hex << std::hex << std::setfill('0');
    for (const uint8_t byte : digest) {
        hex << std::setw(2) << static_cast<unsigned>(byte);
    }
    return hex.str();
}

} // namespace

TEST_CASE("ComputeFileCrc32", "[hash]") {
    TempDir dir;
    const fs::path file = dir.Path() / "input";

    SECTION("matches the published value for a known string") {
        WriteFile(file, "The quick brown fox jumps over the lazy dog");
        const auto crc = ComputeFileCrc32(file);
        REQUIRE(crc.has_value());
        REQUIRE(*crc == 0x414FA339U);
    }

    SECTION("matches the standard check value") {
        WriteFile(file, "123456789");
        const auto crc = ComputeFileCrc32(file);
        REQUIRE(crc.has_value());
        REQUIRE(*crc == 0xCBF43926U);
    }

    SECTION("is zero for an empty file") {
        WriteFile(file, "");
        const auto crc = ComputeFileCrc32(file);
        REQUIRE(crc.has_value());
        REQUIRE(*crc == 0U);
    }

    SECTION("covers the whole of a file larger than one read") {
        WriteFile(file, LargeContent());
        const auto crc = ComputeFileCrc32(file);
        REQUIRE(crc.has_value());
        REQUIRE(*crc == 0x634F3D0DU);
    }

    SECTION("returns nothing for a file that does not exist") {
        REQUIRE_FALSE(ComputeFileCrc32(dir.Path() / "missing").has_value());
    }
}

TEST_CASE("ComputeFileMd5", "[hash]") {
    TempDir dir;
    const fs::path file = dir.Path() / "input";

    SECTION("matches the published value for a known string") {
        WriteFile(file, "The quick brown fox jumps over the lazy dog");
        REQUIRE(Md5Hex(file) == "9e107d9d372bb6826bd81d3542a419d6");
    }

    SECTION("matches the published value for an empty file") {
        WriteFile(file, "");
        REQUIRE(Md5Hex(file) == "d41d8cd98f00b204e9800998ecf8427e");
    }

    SECTION("covers the whole of a file larger than one read") {
        WriteFile(file, LargeContent());
        REQUIRE(Md5Hex(file) == "bd6270ad16e95c85d32f9a518e1f76e7");
    }

    SECTION("returns false for a file that does not exist") {
        std::array<uint8_t, md5_size> digest{};
        REQUIRE_FALSE(ComputeFileMd5(dir.Path() / "missing", digest.data()));
    }
}
