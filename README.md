# Maxlabs Communication Protocol (`MaxlabsProtocol`)

A highly efficient, deterministic, non-blocking RS485 communication protocol for Master-Slave microcontroller networks. 

Designed specifically for custom robotics and industrial PCBs, this protocol replaces standard Modbus RTU. It eliminates Modbus's rigid 16-bit data limitations and timing-based framing by introducing **dynamic payload lengths** and **hard ASCII boundaries**, making it virtually immune to bus noise and buffer desynchronization.

## 🚀 Key Features
*   **Dynamic Data Types:** Send a single bit, a 16-bit integer, or a 64-byte C-Struct using the exact same function codes.
*   **Bulletproof Framing:** Uses `<` and `>` markers. If noise corrupts the line, the receiver instantly resynchronizes on the next start marker.
*   **Mathematical Verification:** Every packet is validated using a bitwise CRC16 checksum before execution.
*   **Zero-Blocking Architecture:** Purely polling-based. Uses no `delay()` or hardware timer interrupts, leaving your MCU free to run safety-critical PID loops.
*   **Ultra-Low Footprint:** Consumes less than 150 bytes of RAM on an 8-bit AVR microcontroller.
*   **Cross-Platform:** Native support for ESP32, STM32, and Arduino (AVR) architectures.

---
MaxlabsProtocol_Dev/
├── platformio.ini                 <-- Multi-board build configuration
├── src/
│   └── main.cpp                   <-- Your bench test code (Aggressor / Defender)
│
└── lib/
    └── MaxlabsProtocol/           <-- THIS FOLDER IS YOUR GITHUB REPO
        ├── library.json           <-- PlatformIO package manifest
        ├── library.properties     <-- Arduino IDE package manifest
        ├── README.md              <-- GitHub documentation
        ├── src/
        │   ├── MaxlabsProtocol.h
        │   └── MaxlabsProtocol.cpp
        └── examples/
            └── Basic_Slave/
                └── Basic_Slave.ino

                
## 📦 Packet Architecture (The Envelope)

Every transmission on the bus follows this strict, dynamically sized byte sequence:

| Byte Offset | Field | Size | Description |
| :--- | :--- | :--- | :--- |
| `0` | **Start Marker** | 1 Byte | Always `<` (`0x3C`) |
| `1` | **Node ID** | 1 Byte | `0x01` to `0x40` (Node), `0x00` (Master), `0xFF` (Broadcast) |
| `2` | **Function Code** | 1 Byte | Standardized operation (See dictionary below) |
| `3` | **Register**| 1 Byte | Target memory location or hardware component (`0x00` - `0xFF`) |
| `4` | **Length (N)** | 1 Byte | Payload size in bytes (`0` to `64`) |
| `5` to `5+N` | **Payload** | N Bytes | Raw data (Bits, Floats, or Structs) |
| `End-1` | **CRC16 Low** | 1 Byte | Checksum of Node, Function, Register, Length, and Payload |
| `End` | **CRC16 High** | 1 Byte | Checksum of Node, Function, Register, Length, and Payload |
| `End+1` | **End Marker** | 1 Byte | Always `>` (`0x3E`) |

---

## 📖 Function Code Dictionary

Instead of defining physical hardware (like "Coils" vs "Registers"), Function Codes in the Maxlabs Protocol define **memory operations**. 

*   `0x01` **READ_REQ**: Master requests data from a Node's Register.
*   `0x02` **READ_RES**: Node replies to Master with the requested data payload.
*   `0x03` **WRITE_REQ**: Master pushes new data/settings to a Node's Register.
*   `0x04` **WRITE_ACK**: Node confirms the write was successfully executed.
*   `0x05` **EXECUTE**: Master commands the Node to trigger a specific routine (e.g., Reboot, Auto-Tune).
*   `0x06` **ERROR**: Node rejects command (Invalid Register, Bad CRC).

---

## 🛠️ Installation

### PlatformIO (Recommended)
Add the following line to your `platformio.ini` file. PIO will automatically clone and compile the library.
```ini
lib_deps =
    [https://github.com/hussam2006/MaxlabsProtocol.git](https://github.com/hussam2006/MaxlabsProtocol.git)


Arduino IDE
Download this repository as a .zip file.

Open the Arduino IDE.

Go to Sketch -> Include Library -> Add .ZIP Library...

Select the downloaded file.

💻 Quick Usage Example (Slave Node)
C++
#include <Arduino.h>
#include <MaxlabsProtocol.h>

// Use HardwareSerial attached to RS485 transceiver
MaxlabsProtocol maxlabs(Serial1); 

const uint8_t MY_NODE_ID = 0x01;

// Buffers
uint8_t rxNode, rxFunc, rxReg, rxLen;
uint8_t rxPayload[64];

void setup() {
    Serial1.begin(38400); 
}

void loop() {
    // 1. Poll the bus continuously (Non-blocking)
    if (maxlabs.poll(rxNode, rxFunc, rxReg, rxPayload, rxLen)) {
        
        // 2. Ignore commands for other nodes
        if (rxNode != MY_NODE_ID && rxNode != 0xFF) return;

        // 3. Process Validated Commands
        if (rxFunc == MAXLABS_FUNC_READ_REQ && rxReg == 0x10) {
            
            // Example: Master requested temperature data (16-bit integer)
            uint16_t currentTemp = 425; // 42.5 C
            
            // Send Response back to Master
            maxlabs.send(MY_NODE_ID, MAXLABS_FUNC_READ_RES, rxReg, (uint8_t*)&currentTemp, sizeof(currentTemp));
        }
    }
}


⚙️ Hardware Recommendations
This library is designed for half-duplex RS485 physical layers.

It is highly recommended to use Auto-Direction RS485 modules (utilizing a MAX13487E or a custom logic-inverter circuit on the DE/RE pins) to eliminate the need for software-driven pin toggling.

If using standard MAX485 chips, ensure pull-up/pull-down bias resistors are correctly applied to the A/B lines to prevent floating bus noise when transmitters are disabled.

Developed by Maxlabs for advanced industrial and robotics integration.