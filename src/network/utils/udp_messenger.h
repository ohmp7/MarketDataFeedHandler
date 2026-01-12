#pragma once

#include <cstddef>
#include <cstdint>
#include <netinet/in.h>
#include <string>

/*
UDP Wrapper Class
*/
class UdpMessenger {
public:
    using Bytes = std::size_t;

    UdpMessenger(int sockfd, const std::string& ip, std::uint16_t port);

    // Preventing Object Copy (sockfd corruption safeguard)
    UdpMessenger(const UdpMessenger& other) = delete;
    UdpMessenger& operator=(const UdpMessenger& other) = delete;

    void SendDatagram(const void* data, Bytes length) const;

    private:
    int sockfd_{-1};
    sockaddr_in destaddr_{};
};
