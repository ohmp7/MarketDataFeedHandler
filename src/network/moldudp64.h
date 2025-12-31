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
    char session[SESSION_LENGTH];
    SequenceNumber sequence_number = 0;
    MessageCount message_count = 0;
    bool end_of_session = false;
};

struct MessageView {
    const std::uint8_t* data{nullptr};
    Bytes len{0};
};

struct Session {
    char session[SESSION_LENGTH]{};
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
    MoldUDP64(SequenceNumber request_sequence_num_, int sockfd, const std::string& ip, std::uint16_t port);

    /*
    Returns true if the packet is in-order and contains a complete message; false otherwise.
    */
    bool handle_packet(const std::uint8_t* buf, Bytes len);

    void set_session(const char (&src_session)[SESSION_LENGTH]);

    MessageView message_view() const;

private:
    void request(SequenceNumber sequence_number);

    void read(const std::uint8_t* buf, Bytes len);

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

    Session session;
};