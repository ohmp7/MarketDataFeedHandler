#include "udp_messenger.h"

#include <arpa/inet.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

UdpMessenger::UdpMessenger(int sockfd_, const std::string& ip, std::uint16_t port) 
    : sockfd(sockfd_) {

    memset(&destaddr, 0, sizeof(destaddr));
    destaddr.sin_family = AF_INET;
    destaddr.sin_port = htons(port);
    
    if (inet_pton(AF_INET, ip.c_str(), &destaddr.sin_addr) < 1) {
        throw std::runtime_error("Error: failed to convert IPv4 address from text to binary form.");
    }
}

void UdpMessenger::send_datagram(const void* data, Bytes len) const {
    ssize_t sent = sendto(sockfd, data, len, 0, reinterpret_cast<const sockaddr*>(&destaddr), sizeof(destaddr));
    if (sent < 0 || static_cast<Bytes>(sent) != len) {
        throw std::runtime_error("Error: failed to transmit the message to the socket.");
    }
}
