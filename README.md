# **Market Data Feed Handler**

This repository showcases a low-latency C++ market data feed handler simulator. The feed handler _(referred to as the **Market Plant**)_ ingests UDP unicast datagrams from a simulated exchange using Nasdaq’s **[MoldUDP64](https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/moldudp64.pdf) Protocol**, maintains an in-memory **L2 price-level order book**, and streams real-time updates to subscribers via **server-side [gRPC](https://grpc.io/) streaming**.

The motivation behind this project was to build a high-performance middleware service that ingests exchange-style UDP feeds and efficiently scales to one-to-many subscribers.

## **Architecture**

<img width="829" height="495" alt="MarketPlantDiagram" src="https://github.com/user-attachments/assets/73399350-64db-483a-91b3-3da1115c8652" />
example

### **Market Feed Data Handler**
high-level overview

### **Networking**
high-level overview

#### **gRPC**
example

#### **UDP Unicast**

Below is an example of the message payload utilized (big-endian / Network Byte Order). _Offsets are byte offsets from the start of the UDP datagram buffer._

| Offset (bytes) | Field           | Size | Type | Example |
|---:|---|---:|---|---|
| 20–21 | `msg_len`        | 2  | `u16` | `22` (bytes after `msg_len`) |
| 22–25 | `instrument_id`  | 4  | `u32` | `AAPL = 1` |
| 26    | `side`           | 1  | `u8`  | `BID = 0`, `ASK = 1` |
| 27    | `event`          | 1  | `u8`  | `ADD = 0`, `REDUCE = 1` |
| 28–31 | `price`          | 4  | `u32` | `32` (USD) |
| 32–35 | `quantity`       | 4  | `u32` | `5917` |
| 36–43 | `exchange_ts`    | 8  | `u64` | `1234567891234567890` (ns) |


### **Exchange**
high-level overview

### **Subscriber**
high-level overview

## **Project Structure**
high-level overview

## **Usage**
high-level overview

## **Styling**

- **[Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)**:
- **[Protobuf Styling](https://protobuf.dev/programming-guides/style/)**:
