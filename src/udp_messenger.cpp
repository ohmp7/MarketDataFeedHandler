#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstddef>
#include <string> 

using Bytes = std::size_t;

class UdpMessenger {
public:
    UdpMessenger(const std::string& ip, std::uint16_t port) {
        if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0 ) {
            // throw error
            return;
        }

        memset(&destaddr, 0, sizeof(destaddr));

        destaddr.sin_family = AF_INET;
        destaddr.sin_port = htons(port);

        if (inet_pton(AF_INET, ip.c_str(), &destaddr.sin_addr) < 1) {
            close(sockfd);
            sockfd = -1;

            //throw error
            return;
        }
    }

    // Preventing object copy (sockfd corruption safeguard)
    UdpMessenger(const UdpMessenger& temp) = delete;
    UdpMessenger& operator=(const UdpMessenger& temp) = delete;

    ~UdpMessenger() { if (sockfd >= 0) close(sockfd); }
    
    void send(const void* data, Bytes len) {
        ssize_t sent = sendto(sockfd, data, len, 0, (const struct sockaddr *) &destaddr, sizeof(destaddr));
        if (sent < 0) {
            // throw error
            return;
        }
    }
    
private:
    int sockfd{-1};
    sockaddr_in destaddr{};
};