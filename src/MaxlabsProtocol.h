#ifndef MAXLABS_PROTOCOL_H
#define MAXLABS_PROTOCOL_H

#include <Arduino.h>

// Standardized Function Codes
#define MAXLABS_FUNC_READ_REQ   0x01
#define MAXLABS_FUNC_READ_RES   0x02
#define MAXLABS_FUNC_WRITE_REQ  0x03
#define MAXLABS_FUNC_WRITE_ACK  0x04
#define MAXLABS_FUNC_EXECUTE    0x05
#define MAXLABS_FUNC_ERROR      0x06

// Configurable Memory Buffer (Adjust based on project needs)
#define MAXLABS_MAX_PAYLOAD 64 

class MaxlabsProtocol {
  public:
    // Pass any Serial interface (HardwareSerial, SoftwareSerial)
    MaxlabsProtocol(Stream& serialPort);
    
    // Non-blocking poll. Returns true if a valid, verified packet arrived.
    bool poll(uint8_t &outNode, uint8_t &outFunc, uint8_t &outReg, uint8_t* outPayload, uint8_t &outLen);
    
    // Sends a framed packet.
    void send(uint8_t node, uint8_t func, uint8_t reg, const uint8_t* payload, uint8_t payloadLen);

  private:
    Stream& _serial;
    uint16_t calculateCRC(const uint8_t *buffer, uint8_t len);
    
    // Non-blocking state machine definitions
    enum RxState { 
      WAIT_START, WAIT_NODE, WAIT_FUNC, WAIT_REG, WAIT_LEN, 
      WAIT_PAYLOAD, WAIT_CRC1, WAIT_CRC2, WAIT_END 
    };
    
    RxState _rxState = WAIT_START;
    
    // Internal parsing variables
    uint8_t _rxNode;
    uint8_t _rxFunc;
    uint8_t _rxReg;
    uint8_t _rxLen;
    uint8_t _rxPayload[MAXLABS_MAX_PAYLOAD];
    uint8_t _rxIndex = 0;
    uint8_t _rxCRC1;
    uint8_t _rxCRC2;
};

#endif