#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>

using Bytes = std::size_t;
using MessageCount = std::uint16_t;
using MessageDataSize = std::uint16_t;
using SequenceNumber = std::uint64_t;

using Depth = std::size_t;
using Price = std::uint32_t;
using Quantity = std::uint32_t;
using InstrumentId = std::uint32_t;
using SubscriberId = std::uint32_t;
using Timestamp = std::uint64_t;

enum class Side : std::uint8_t {
  kBid = 0,
  kAsk = 1,
};

enum class LevelEvent : std::uint8_t {
  kAddLevel = 0,
  kModifyLevel = 1,
};

/*

Message Payload (Network-Byte-Order / Big Endian)

offset 20 - 21:     msg_len         (2 bytes, u16)        ex. 'msg_len' = 22 (bytes after 'msg_len')
offset 22 - 25:     instrument_id   (4 bytes, u32)        ex. 'AAPL' = 1
offset 26:          side            (1 byte, u8)          ex. 'BID' = 0, 'ASK' = 1
offset 27:          event           (1 byte, u8)          ex. 'ADD' = 0, 'REDUCE' = 1
offset 28 - 31:     price           (4 bytes, u32)        ex. 'price' = 32 (USD)
offset 32 - 35:     quantity        (4 bytes, u32)        ex. 'quantity' = 5917
offset 36 - 43:     exchange_ts     (8 bytes, u64)        ex. 1234567891234567890 (ns)

*/

inline constexpr Bytes kSessionLength = 10;
inline constexpr Bytes kHeaderLength = 20;
inline constexpr Bytes kMessageCount = 1;
inline constexpr Bytes kPacketSize = 44;
inline constexpr Bytes kMessageHeaderLength = 2;
inline constexpr Timestamp kCancellationPollInterval = 500;

inline constexpr MessageCount kEndSession = 0xFFFF;
inline constexpr MessageCount kMaxMessageCount = kEndSession - 1;

inline constexpr std::uint32_t kMaxExchangeEvents = 1000000;

struct MarketEvent {
    InstrumentId instrument_id;
    Side side;
    LevelEvent event;
    Price price;
    Quantity quantity;
    Timestamp exchange_ts;
};