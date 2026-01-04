#pragma once

#include "event.h"
#include "udp_messenger.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>
#include <string>

using Clock = std::chrono::steady_clock;

struct PacketHeader {
    char session[kSessionLength];
    SequenceNumber sequence_number = 0;
    MessageCount message_count = 0;
    bool end_of_session = false;
};

struct MessageView {
    const std::uint8_t* data{nullptr};
    Bytes len{0};
};

struct Session {
    char session[kSessionLength]{};
    bool set = false;
};

class PacketTruncatedError : public std::exception {
public:
    PacketTruncatedError(Bytes received, Bytes expected);
    virtual const char* what() const noexcept override;
private:
    std::string message_;
};

PacketHeader ParsePacketHeader(const std::uint8_t* buf, Bytes len);

/*
Client Handler for MoldUDP64 Network Protocol, a lightweight protocol layer built on top of UDP.
*/
class MoldUDP64 {
public:
    MoldUDP64(SequenceNumber request_sequence_num, int sockfd, const std::string& ip, std::uint16_t port);

    /*
    Returns true if the packet is in-order and contains a complete message; false otherwise.
    */
    bool HandlePacket(const std::uint8_t* buf, Bytes len);

    void SetSession(const char (&src_session)[kSessionLength]);

    MessageView message_view() const;

private:
    void Request(SequenceNumber sequence_number);

    void Read(const std::uint8_t* buf, Bytes len);

    static constexpr auto kTimeout = std::chrono::milliseconds(1000);

    // Recovery window upper bound (exclusive)
    // State Helpers: std::nullopt = uninitialized ; 0 = synchronized, > 0 = recovery mode
    static constexpr SequenceNumber kSynchronized = 0;
    std::optional<SequenceNumber> request_until_sequence_num_ =  std::nullopt;

    // next sequence number in order
    SequenceNumber next_expected_sequence_num_;

    Clock::time_point last_request_sent_{};
    UdpMessenger messenger_;
    MessageView message_view_{};

    Session session_;
};