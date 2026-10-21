#ifndef COMPRESSIBLE_TEXT_H
#define COMPRESSIBLE_TEXT_H

#include <cstddef>
#include <cstdint>
#include <string>

namespace mpqcli::testing {

/// Text that compresses well but not trivially, the same bytes on every run
///
/// A fixed word sequence from a small linear congruential generator, so different
/// compression algorithms produce visibly different stored sizes and a test can tell
/// them apart. 12000 words is about 68 KB, well over one archive sector.
inline std::string CompressibleText(std::size_t word_count) {
    static const char *const words[] = {"alpha",   "beta", "gamma", "delta",
                                        "epsilon", "zeta", "eta",   "theta"};
    std::uint32_t state = 12345;
    std::string text;
    for (std::size_t i = 0; i < word_count; ++i) {
        state = state * 1664525U + 1013904223U;
        text += words[(state >> 24) % 8];
        text += ' ';
    }
    return text;
}

} // namespace mpqcli::testing

#endif // COMPRESSIBLE_TEXT_H
