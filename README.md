# **Market Data Feed Handler**

This repository showcases a low-latency C++ market data feed handler simulator. The feed handler _(referred to here as the **Market Plant**)_ ingests UDP unicast packets from a simulated exchange using Nasdaq’s **[MoldUDP64](https://www.nasdaqtrader.com/content/technicalsupport/specifications/dataproducts/moldudp64.pdf) Protocol**, maintains an in-memory **L2 price-level order book**, and streams real-time updates to subscribers via **server-side gRPC streaming** (**[gRPC](https://grpc.io/)**).

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
example

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
