#include "esphome.h"
#include "MaxlabsProtocol.h"

// Forward declaration so YAML can easily access it globally
class MaxlabsGateway;
MaxlabsGateway* maxlabs_hub = nullptr;

class MaxlabsGateway : public PollingComponent {
  public:
    // --- INPUTS FROM STM32 ---
    Sensor *analog_input = new Sensor();
    BinarySensor *digital_input = new BinarySensor();
    
    HardwareSerial* rs485_bus;
    MaxlabsProtocol* maxlabs;

    // Poll every 2 seconds
    MaxlabsGateway() : PollingComponent(2000) {}

    void setup() override {
      // Setup for ESP32 Phase 2 (RS485 using Pins 33 and 34)
      rs485_bus = new HardwareSerial(1);
      rs485_bus->begin(38400, SERIAL_8N1, 33, 34);
      
      maxlabs = new MaxlabsProtocol(*rs485_bus);
    }

    void update() override {
      // Master Routine: Ask STM32 (Node 0x01) for Analog Data (Reg 0x10)
      maxlabs->send(0x01, MAXLABS_FUNC_READ_REQ, 0x10, nullptr, 0);
    }

    void loop() override {
      uint8_t rxNode, rxFunc, rxReg, rxLen;
      uint8_t rxPayload[MAXLABS_MAX_PAYLOAD];
      
      // Listen continuously for incoming packets
      if (maxlabs->poll(rxNode, rxFunc, rxReg, rxPayload, rxLen)) {
         
         if (rxNode == 0x01 && rxFunc == MAXLABS_FUNC_READ_RES) {
             
             // 1. Analog / 16-bit Sensor (Reg 0x10)
             if (rxReg == 0x10 && rxLen >= 2) { 
                 uint16_t val = (rxPayload[0] << 8) | rxPayload[1];
                 analog_input->publish_state(val);
             }
             
             // 2. Binary Input / Door Sensor (Reg 0x11)
             else if (rxReg == 0x11 && rxLen >= 1) {
                 digital_input->publish_state(rxPayload[0] > 0);
             }
         }
      }
    }

    // =======================================================
    // --- OUTPUT FUNCTIONS (Called by YAML Dashboard) ---
    // =======================================================

    // 1. Digital Output (Send Relay Command)
    void set_relay(bool state) {
        uint8_t payload[1] = { state ? (uint8_t)1 : (uint8_t)0 };
        // Write to Register 0x20
        maxlabs->send(0x01, MAXLABS_FUNC_WRITE_REQ, 0x20, payload, 1);
    }

    // 2. PWM Output (Send 0-100% Value)
    void set_pwm(float value) {
        uint8_t payload[1] = { (uint8_t)value };
        // Write to Register 0x30
        maxlabs->send(0x01, MAXLABS_FUNC_WRITE_REQ, 0x30, payload, 1);
    }

    // 3. Instruction Send (Trigger Specific Routine)
    void set_mode(std::string mode) {
        uint8_t instruction = 0;
        if (mode == "Wash") instruction = 1;
        else if (mode == "Rinse") instruction = 2;
        else if (mode == "Spin") instruction = 3;

        uint8_t payload[1] = { instruction };
        // Send EXECUTE command (0x05) to Register 0x40
        maxlabs->send(0x01, MAXLABS_FUNC_EXECUTE, 0x40, payload, 1);
    }
};