#pragma once
#include "udp_messenger.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>
#include <string>


using Bytes = std::size_t;
using MessageCount = std::uint16_t;
using SequenceNumber = std::uint64_t;
using Clock = std::chrono::steady_clock;

class PacketTruncatedError : public std::exception {
public:
    PacketTruncatedError(Bytes received, Bytes expected);
    virtual const char* what() const noexcept override;
private:
    std::string message_;
};
/*
Handling Network-Byte-Order Integers.
*/
template <typename T>
inline T read_big_endian(const std::uint8_t* buf,  Bytes offset) {
    T converted = 0;
    for (Bytes i = 0; i < sizeof(T); ++i) {
        converted <<= 8;
        std::uint8_t next_byte = buf[offset + i];
        converted = converted | static_cast<T>(next_byte);
    }
    return converted;
}

template <typename T>
inline void write_big_endian(std::uint8_t* buf, Bytes offset, T value) {
    for (Bytes i = 0; i < sizeof(T); ++i) {
        buf[offset + sizeof(T) - i - 1] = static_cast<uint8_t>(value & 0xFF);
        value >>= 8;
    }
}

struct MessageView {
    const std::uint8_t* data{nullptr};
    Bytes len{0};
};

/*
Client Handler for MoldUDP64 Network Protocol, a lightweight protocol layer built on top of UDP.
*/
class MoldUDP64 {
public:
    MoldUDP64(SequenceNumber request_sequence_num_, int sockfd, const std::string& ip, std::uint16_t port);

    /*
    Returns true if the packet is in-order and contains a complete message; false otherwise.
    */
    bool handle_packet(const std::uint8_t* buf, Bytes len);

    MessageView message_view() const;

private:
    void request(SequenceNumber sequence_number);

    void read(const std::uint8_t* buf, Bytes len);

    static constexpr Bytes SESSION_LENGTH = 10;
    static constexpr Bytes HEADER_LENGTH = 20;
    static constexpr Bytes MESSAGE_HEADER_LENGTH = 2;

    static constexpr MessageCount END_SESSION = 0xFFFF;
    static constexpr MessageCount MAX_MESSAGE_COUNT = END_SESSION - 1;
    static constexpr auto TIMEOUT = std::chrono::milliseconds(1000);

    // Recovery window upper bound (exclusive)
    // State Helpers: std::nullopt = uninitialized ; 0 = synchronized, > 0 = recovery mode
    static constexpr SequenceNumber SYNCHRONIZED = 0;
    std::optional<SequenceNumber> request_until_sequence_num =  std::nullopt;

    // next sequence number in order
    SequenceNumber next_expected_sequence_num;

    Clock::time_point last_request_sent{};
    UdpMessenger messenger;
    MessageView msg{};

    char session[SESSION_LENGTH]{};
};