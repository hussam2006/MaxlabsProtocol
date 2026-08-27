#include "esphome.h"
#include "MaxlabsProtocol.h"

class MaxlabsGateway : public PollingComponent {
  public:
    // Create an ESPHome sensor entity
    Sensor *stm32_sensor = new Sensor();
    
    HardwareSerial* rs485_bus;
    MaxlabsProtocol* maxlabs;

    // Set polling interval to 2000ms (2 seconds)
    MaxlabsGateway() : PollingComponent(2000) {}

    void setup() override {
      // Initialize HardwareSerial for ESP32-S3 (RX = 18, TX = 17)
      rs485_bus = new HardwareSerial(2);
      rs485_bus->begin(38400, SERIAL_8N1, 18, 17);
      
      // Pass the serial stream to our protocol library
      maxlabs = new MaxlabsProtocol(*rs485_bus);
    }

    // update() is called automatically by ESPHome every 2000ms
    void update() override {
      // Master Request: Ask Node 0x01 (STM32) for Register 0x10 data
      // Payload is empty (nullptr) with length 0 since it's just a read request
      maxlabs->send(0x01, MAXLABS_FUNC_READ_REQ, 0x10, nullptr, 0);
    }

    // loop() runs continuously as fast as the ESP32 can process it
    void loop() override {
      uint8_t rxNode, rxFunc, rxReg, rxLen;
      uint8_t rxPayload[MAXLABS_MAX_PAYLOAD];
      
      // Non-blocking listener perfectly handles the background traffic
      if (maxlabs->poll(rxNode, rxFunc, rxReg, rxPayload, rxLen)) {
         
         // If we get a response from Node 0x01, it's a READ_RES, and it's for Reg 0x10
         if (rxNode == 0x01 && rxFunc == MAXLABS_FUNC_READ_RES && rxReg == 0x10) {
             
             // Example: Assuming the STM32 sent back a 32-bit integer (4 bytes)
             if (rxLen == 4) { 
                 uint32_t sensorValue = ((uint32_t)rxPayload[0] << 24) |
                                        ((uint32_t)rxPayload[1] << 16) |
                                        ((uint32_t)rxPayload[2] << 8)  |
                                        ((uint32_t)rxPayload[3]);
                 
                 // Push the verified data to Home Assistant
                 stm32_sensor->publish_state(sensorValue);
             }
         }
      }
    }
};