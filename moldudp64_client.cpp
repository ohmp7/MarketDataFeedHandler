#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <exception>
#include <iostream>
#include <optional>
#include <string>

using Bytes = std::size_t;
using MessageCount = std::uint16_t;
using SequenceNumber = std::uint64_t;
using Clock = std::chrono::steady_clock;

class PacketTruncatedError : public std::exception {
public:
    PacketTruncatedError(Bytes received, Bytes expected)
        : message_("Packet Truncated Error: received " + std::to_string(received) +
                   " bytes, but was expecting >= " + std::to_string(expected)) {}
    
    virtual const char* what() const noexcept override {
        return message_.c_str();
    }
private:
    std::string message_;
};

template <typename T>
static T read_big_endian(const std::uint8_t* buf,  Bytes offset) {
    T converted = 0;

    for (Bytes i = 0; i < sizeof(T); ++i) {
        // shift left 8 bits
        converted = converted << 8;

        // get next byte from buf
        std::uint8_t next_byte = buf[offset + i];

        // OR operation with lower 8 bits
        converted = converted | static_cast<T>(next_byte);
    }

    return converted;
}

class MoldUDP64 {
public:
    MoldUDP64(SequenceNumber request_sequence_num_)
        : next_expected_sequence_num(request_sequence_num_) {}

    void handle_packet(const std::uint8_t* buf, Bytes len) {
        // parse packet header
        if (len < HEADER_LENGTH) throw PacketTruncatedError(len, HEADER_LENGTH);

        Bytes curr_offset = 0;

        std::memcpy(session, buf + curr_offset, SESSION_LENGTH);
        curr_offset += SESSION_LENGTH;

        SequenceNumber sequence_number = read_big_endian<SequenceNumber>(buf, curr_offset);
        curr_offset += sizeof(SequenceNumber);

        MessageCount message_count = read_big_endian<MessageCount>(buf, curr_offset);  // INVARIANT: 1 message per packet
        bool session_has_ended = (message_count == END_SESSION);
        if (session_has_ended) message_count = 0;
        curr_offset += sizeof(MessageCount);

        SequenceNumber next_sequence_number = sequence_number + message_count;
        
        if (next_expected_sequence_num == 0) {
            next_expected_sequence_num = sequence_number;
        }

        if (sequence_number > next_expected_sequence_num) { 
            // A packet has been dropped/delayed
            if (!request_until_sequence_num) {                     // Back Fill
                
                request_until_sequence_num = next_sequence_number;
                request(next_expected_sequence_num);

            } else if (*request_until_sequence_num == SYNCHRONIZED) {         // Gap Fill

                request_until_sequence_num = next_sequence_number;
                request(next_expected_sequence_num);

            } else {                                                         // Already in recovery mode
                request_until_sequence_num = std::max(*request_until_sequence_num, next_sequence_number);
                
                if (Clock::now() - last_request_sent > TIMEOUT) {
                    request(next_expected_sequence_num);
                }
            }
        } else {

            // old/duplicate packet check
            if (sequence_number < next_expected_sequence_num) return;

            // recovery check
            if (!request_until_sequence_num || *request_until_sequence_num != SYNCHRONIZED) {
                if (!request_until_sequence_num) {

                    request_until_sequence_num = SYNCHRONIZED;

                } else if (*request_until_sequence_num == next_sequence_number) {
                    
                    request_until_sequence_num = SYNCHRONIZED;
                
                } else {

                    request(next_sequence_number);

                }
            }

            if (session_has_ended) {
                return;
            } else if (sequence_number == next_expected_sequence_num) {
                read();  // design choice: can be redudant because packets must be read in order
            }
        }
    }

    void request(SequenceNumber sequence_number) {
        // handle request ...

        last_request_sent = Clock::now();
    }

    void read() {
        next_expected_sequence_num++;
    }

private:
    static constexpr Bytes SESSION_LENGTH = 10;
    static constexpr Bytes HEADER_LENGTH = 20;
    static constexpr std::uint16_t END_SESSION = 0xFFFF;
    static constexpr auto TIMEOUT = std::chrono::milliseconds(1000);

    // status
    static constexpr std::nullopt_t SEQUENCE_LIMIT_UNKNOWN = std::nullopt;
    static constexpr SequenceNumber SYNCHRONIZED = 0;

    Clock::time_point last_request_sent{};

    SequenceNumber next_expected_sequence_num;
    std::optional<SequenceNumber> request_until_sequence_num = SEQUENCE_LIMIT_UNKNOWN;
    
    char session[SESSION_LENGTH]{};
};