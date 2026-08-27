#include "MaxlabsProtocol.h"

MaxlabsProtocol::MaxlabsProtocol(Stream& serialPort) : _serial(serialPort) {}

bool MaxlabsProtocol::poll(uint8_t &outNode, uint8_t &outFunc, uint8_t &outReg, uint8_t* outPayload, uint8_t &outLen) {
  while (_serial.available() > 0) {
    uint8_t b = _serial.read();

    switch (_rxState) {
      case WAIT_START:
        if (b == '<') _rxState = WAIT_NODE;
        break;

      case WAIT_NODE:
        _rxNode = b;
        _rxState = WAIT_FUNC;
        break;

      case WAIT_FUNC:
        _rxFunc = b;
        _rxState = WAIT_REG;
        break;

      case WAIT_REG:
        _rxReg = b;
        _rxState = WAIT_LEN;
        break;

      case WAIT_LEN:
        _rxLen = b;
        if (_rxLen > MAXLABS_MAX_PAYLOAD) {
          _rxState = WAIT_START; // Drop packet to prevent buffer overflow
        } else if (_rxLen == 0) {
          _rxState = WAIT_CRC1;
        } else {
          _rxIndex = 0;
          _rxState = WAIT_PAYLOAD;
        }
        break;

      case WAIT_PAYLOAD:
        _rxPayload[_rxIndex++] = b;
        if (_rxIndex >= _rxLen) _rxState = WAIT_CRC1;
        break;

      case WAIT_CRC1:
        _rxCRC1 = b;
        _rxState = WAIT_CRC2;
        break;

      case WAIT_CRC2:
        _rxCRC2 = b;
        _rxState = WAIT_END;
        break;

      case WAIT_END:
        if (b == '>') {
          // Reconstruct array to verify CRC
          uint8_t crcBuffer[MAXLABS_MAX_PAYLOAD + 4];
          crcBuffer[0] = _rxNode;
          crcBuffer[1] = _rxFunc;
          crcBuffer[2] = _rxReg;
          crcBuffer[3] = _rxLen;
          if (_rxLen > 0) memcpy(&crcBuffer[4], _rxPayload, _rxLen);

          uint16_t calculatedCRC = calculateCRC(crcBuffer, 4 + _rxLen);
          uint16_t receivedCRC = (_rxCRC2 << 8) | _rxCRC1;
          
          _rxState = WAIT_START; 
          
          if (calculatedCRC == receivedCRC) {
            outNode = _rxNode;
            outFunc = _rxFunc;
            outReg  = _rxReg;
            outLen  = _rxLen;
            if (_rxLen > 0) memcpy(outPayload, _rxPayload, _rxLen);
            return true; 
          }
        }
        _rxState = WAIT_START;
        break;
    }
  }
  return false;
}

void MaxlabsProtocol::send(uint8_t node, uint8_t func, uint8_t reg, const uint8_t* payload, uint8_t payloadLen) {
  uint8_t crcLen = 4 + payloadLen;
  uint8_t crcBuffer[crcLen];
  
  crcBuffer[0] = node;
  crcBuffer[1] = func;
  crcBuffer[2] = reg;
  crcBuffer[3] = payloadLen;
  if (payloadLen > 0) memcpy(&crcBuffer[4], payload, payloadLen);

  uint16_t crc = calculateCRC(crcBuffer, crcLen);

  _serial.write('<');
  _serial.write(crcBuffer, crcLen);
  _serial.write(crc & 0xFF);         
  _serial.write((crc >> 8) & 0xFF);  
  _serial.write('>');
}

uint16_t MaxlabsProtocol::calculateCRC(const uint8_t *buffer, uint8_t len) {
  uint16_t crc = 0xFFFF;
  for (uint8_t pos = 0; pos < len; pos++) {
    crc ^= (uint16_t)buffer[pos];
    for (int i = 8; i != 0; i--) {
      if ((crc & 0x0001) != 0) { crc >>= 1; crc ^= 0xA001; } 
      else { crc >>= 1; }
    }
  }
  return crc;
}