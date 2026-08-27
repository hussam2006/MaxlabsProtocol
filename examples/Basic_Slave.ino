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