#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <netinet/in.h>

/*
UDP Wrapper Class
*/
class UdpMessenger {
public:
    using Bytes = std::size_t;

    UdpMessenger(int sockfd, const std::string& ip, std::uint16_t port);

    // Preventing Object Copy (sockfd corruption safeguard)
    UdpMessenger(const UdpMessenger& temp) = delete;
    UdpMessenger& operator=(const UdpMessenger& temp) = delete;

    void send_datagram(const void* data, Bytes len) const;
private:
    int sockfd{-1};
    sockaddr_in destaddr{};
};