#ifndef CANCEL_AFTER_H
#define CANCEL_AFTER_H

#include <atomic>
#include <cstddef>
#include <ostream>
#include <streambuf>
#include <string>
#include <utility>

namespace mpqcli::testing {

/// An error stream that raises a cancellation flag once a chosen line has been written
///
/// Every looping command writes one line per item to its error stream and polls the flag
/// before the next item, so raising the flag as the first such line ends interrupts the
/// run part-way through. A flag set before the call stops the command before its loop
/// starts, which cannot tell a poll inside the loop from one ahead of it.
class CancelAfter : private std::streambuf {
public:
    /// @param prefix The start of the line that raises the flag.
    explicit CancelAfter(std::string prefix) : prefix_(std::move(prefix)), stream_(this) {}

    CancelAfter(const CancelAfter &) = delete;
    CancelAfter &operator=(const CancelAfter &) = delete;

    /// The stream to pass to the command as its error stream
    std::ostream &Err() { return stream_; }

    /// The flag to pass to the command, raised once a line starting with the prefix ends
    const std::atomic<bool> &Cancelled() const { return cancelled_; }

    /// Everything written to the stream so far
    const std::string &Text() const { return text_; }

private:
    int_type overflow(int_type ch) override {
        if (traits_type::eq_int_type(ch, traits_type::eof())) {
            return traits_type::not_eof(ch);
        }
        const char c = traits_type::to_char_type(ch);
        text_.push_back(c);
        if (c == '\n') {
            if (text_.compare(line_start_, prefix_.size(), prefix_) == 0) {
                cancelled_.store(true);
            }
            line_start_ = text_.size();
        }
        return ch;
    }

    std::string prefix_;
    std::string text_;
    std::size_t line_start_ = 0;
    std::atomic<bool> cancelled_{false};
    std::ostream stream_;
};

} // namespace mpqcli::testing

#endif // CANCEL_AFTER_H
