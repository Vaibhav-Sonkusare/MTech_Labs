# Reliable File Transfer over UDP (Blast-Based Protocol)

## Project Overview

This project implements a **reliable file transfer protocol over UDP**, designed as part of a term project for a Computer Networks course.  
The protocol builds reliability **at the application layer** on top of UDP by using a **blast-based transmission model**, acknowledgments, retransmissions, and explicit state machines (FSMs) for both sender and receiver.

After the academic submission, this project is being **extended and refactored** to improve robustness, observability, and resume readiness.

---

## Key Features Implemented (Till Date)

### 1. Blast-Based Reliable Transfer
- File is split into **fixed-size records**
- Records are grouped into **packets**
- Packets are grouped into **blasts**
- Each blast is followed by an `IS_BLAST_OVER` control packet

### 2. Sender–Receiver Finite State Machines (FSM)
- Explicit FSMs implemented for both sender and receiver
- States handle:
  - File header exchange
  - Blast transmission
  - Missing packet recovery
  - Retransmission rounds
  - Graceful disconnect

### 3. Loss Detection and Recovery
- Receiver maintains a **packet-received bitmap** per blast
- Receiver sends a `REC_MISS` packet indicating missing packets
- Sender retransmits **only missing packets**
- Retransmission continues until receiver reports an empty missing list

### 4. Configurable Packet Loss Injection
- Artificial packet loss introduced at the **network abstraction layer**
- Loss can occur on:
  - Outgoing packets (`udp_send`)
  - Incoming packets (`udp_recv`)
- Loss rate configurable via **command-line argument**
- Used to test reliability logic under adverse conditions

### 5. Timeout Handling
- Socket receive timeout implemented using `SO_RCVTIMEO`
- Prevents sender and receiver from blocking indefinitely
- Enables retransmission of:
  - `FILE_HDR`
  - `IS_BLAST_OVER`

### 6. Robust Header Negotiation
- Sender sends `FILE_HDR`
- Receiver responds with `FILE_HDR_ACK`
- Receiver can suggest:
  - Record size
  - Records per packet
  - Packets per blast
- Sender adapts dynamically

### 7. Disk-Efficient Receiver Design
- Receiver buffers **one blast at a time**
- Data is flushed to disk after each completed blast
- Avoids storing the entire file in memory

---

## Current Limitations

- Performance degrades for **large files (≥ 1 MB)** under non-zero loss
- No congestion control or adaptive rate limiting
- Fixed timeout values (not RTT-based)
- Statistics collection is basic and not yet standardized
- No checksum or data integrity verification beyond packet presence

These limitations are **known and intentional** at this stage and form the basis for future enhancements.

---

## Planned Enhancements (Post-Submission)

- Improved retransmission strategy for high-loss scenarios
- Per-blast and per-packet statistics:
  - Packet loss count
  - Retransmission count
  - Throughput measurement
- Adaptive timeout and window sizing
- Optional checksum for data integrity
- Cleaner modularization and documentation
- Improved logging and debug controls

---

## Design Highlights

- **Protocol-first approach**: all reliability logic implemented above UDP
- **Clear separation of concerns**:
  - `net.c` → network abstraction + loss injection
  - `protocol.h` → wire format definitions
  - `sender.c` / `receiver.c` → FSM-driven logic
- **FSM-driven design**, suitable for academic evaluation and extension

---

## Academic Context

- **Course**: Computer Networks (M.Tech)
- **Project Type**: Term Project
- **Evaluation**: FSM design, protocol correctness, robustness under loss

FSM diagrams for both sender and receiver were designed and submitted as part of the evaluation.

---

## Resume Summary (Draft)

> Designed and implemented a reliable file transfer protocol over UDP using blast-based transmission, FSM-driven sender/receiver logic, selective retransmission, timeout handling, and configurable packet loss injection to simulate real-world network unreliability.

---

## Status

✔ Academic submission complete  
🔧 Actively refactoring and extending for resume-quality project  

---

*This README reflects the project status before adding new features and serves as a stable baseline for future development.*
