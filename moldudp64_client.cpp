#include <cstdint>
#include <iostream>

struct SequenceGap {
    bool active = false;
    std::uint64_t request_until_sequence_num = 0;
};

class MoldUDP64 {
public:
    MoldUDP64(std::uint64_t request_sequence_num_)
        : expected_request_sequence_num(request_sequence_num_) {}

    void handle_packet() {}
    void end_session() {}

private:
    static constexpr std::uint8_t SESSION_LENGTH = 10;
    static constexpr std::uint16_t TIMEOUT = 1000;  // ms

    char session[SESSION_LENGTH]{};
    std::uint64_t expected_request_sequence_num;
    SequenceGap gap;

    void truncated_packet() {}
};