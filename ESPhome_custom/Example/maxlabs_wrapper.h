// ==============================================================================
// PROJECT: Maxlabs Universal RS485 Protocol Wrapper (ESPHome)
// FILE: MaxlabsEsphomeWrapper.h
// VERSION: 1.2.1 
// AUTHOR: Hussam (Maxlabs)
//
// DESCRIPTION:
// A universal, application-agnostic ESPHome C++ wrapper for the Maxlabs Protocol.
// Maintains a generic 2D memory map of incoming payloads and provides an automated,
// staggered polling engine to prevent RS485 bus collisions. 
//
// VERSION HISTORY (CHANGELOG):
// v1.0.0 - Initial deployment. Hardcoded application logic for Washing Machine.
// v1.0.1 - Added specific read/write functions for wash recipes and fault codes.
// v1.0.5 - Integrated with LCD HMI UI functions.
// v1.1.0 - MAJOR REWRITE. Completely stripped application logic. Introduced universal 
//          memory_map, automated staggered polling engine via register_node(), and 
//          generic get_byte()/write_byte() methods for raw YAML data mapping.
// v1.2.0 - Fix library mismatch protocol.
// v1.2.1 - Useing read_byte and write_byte, and at the very bottom, we natively 
//          declare the global_maxlabs variable.
// ==============================================================================

#pragma once
#include "esphome.h"
#include <MaxlabsProtocol.h>
#include <vector>

#define MAXLABS_MAX_NODES 10 // Supports up to 10 nodes on the RS485 bus

// ========================================================================
// UART STREAM BRIDGE (PATCHED FOR ESPHOME API)
// Translates ESPHome UART component into standard Arduino Stream.
// ========================================================================
class ESPHomeUARTStream : public Stream {
private:
    esphome::uart::UARTComponent *uart;
public:
    ESPHomeUARTStream(esphome::uart::UARTComponent *u) : uart(u) {}
    int available() override { return uart->available(); }
    int read() override { 
        uint8_t c; 
        if (uart->read_byte(&c)) return c; 
        return -1; 
    }
    int peek() override { 
        uint8_t c; 
        if (uart->peek_byte(&c)) return c; 
        return -1; 
    }
    void flush() override { uart->flush(); }
    size_t write(uint8_t c) override { uart->write_byte(c); return 1; }
    size_t write(const uint8_t *buffer, size_t size) override { uart->write_array(buffer, size); return size; }
};

struct MaxlabsRegisterMemory {
    uint8_t payload[MAXLABS_MAX_PAYLOAD];
    uint8_t length;
    unsigned long last_update;
};

struct MaxlabsPollTarget {
    uint8_t node;
    uint8_t reg;
};

class MaxlabsEsphomeWrapper : public esphome::Component {
private:
    ESPHomeUARTStream* stream_bridge;
    MaxlabsProtocol* protocol;
    MaxlabsRegisterMemory memory_map[MAXLABS_MAX_NODES][256];
    
    // Auto-Polling Engine Variables
    std::vector<MaxlabsPollTarget> poll_targets;
    unsigned long last_poll_time = 0;
    uint8_t poll_index = 0;
    unsigned long poll_interval_ms = 200; 

public:
    MaxlabsEsphomeWrapper(esphome::uart::UARTComponent *uart_comp) {
        stream_bridge = new ESPHomeUARTStream(uart_comp);
        protocol = new MaxlabsProtocol(*stream_bridge);
        memset(memory_map, 0, sizeof(memory_map));
    }

    void register_node(uint8_t node, uint8_t reg = 0x10) {
        poll_targets.push_back({node, reg});
    }

    void set_poll_interval(unsigned long ms) {
        poll_interval_ms = ms;
    }

    void setup() override {}

    void loop() override {
        uint8_t rxNode, rxFunc, rxReg, rxLen;
        uint8_t rxPayload[MAXLABS_MAX_PAYLOAD];

        if (protocol->poll(rxNode, rxFunc, rxReg, rxPayload, rxLen)) {
            if (rxFunc == MAXLABS_FUNC_READ_RES || rxFunc == MAXLABS_FUNC_WRITE_REQ) {
                if (rxNode < MAXLABS_MAX_NODES) {
                    memcpy(memory_map[rxNode][rxReg].payload, rxPayload, rxLen);
                    memory_map[rxNode][rxReg].length = rxLen;
                    memory_map[rxNode][rxReg].last_update = millis();
                }
            }
        }

        if (!poll_targets.empty()) {
            if (millis() - last_poll_time >= poll_interval_ms) {
                last_poll_time = millis();
                
                uint8_t target_node = poll_targets[poll_index].node;
                uint8_t target_reg = poll_targets[poll_index].reg;
                
                request_read(target_node, target_reg);
                
                poll_index++;
                if (poll_index >= poll_targets.size()) {
                    poll_index = 0;
                }
            }
        }
    }

    void write_register(uint8_t node, uint8_t reg, uint8_t* payload, uint8_t len) {
        protocol->send(node, MAXLABS_FUNC_WRITE_REQ, reg, payload, len);
    }

    void write_byte(uint8_t node, uint8_t reg, uint8_t value) {
        uint8_t payload[1] = { value };
        protocol->send(node, MAXLABS_FUNC_WRITE_REQ, reg, payload, 1);
    }

    void request_read(uint8_t node, uint8_t reg) {
        protocol->send(node, MAXLABS_FUNC_READ_REQ, reg, nullptr, 0);
    }

    uint8_t get_byte(uint8_t node, uint8_t reg, uint8_t byte_index) {
        if (node < MAXLABS_MAX_NODES && byte_index < memory_map[node][reg].length) {
            return memory_map[node][reg].payload[byte_index];
        }
        return 0; 
    }

    int16_t get_int16(uint8_t node, uint8_t reg, uint8_t start_index) {
        if (node < MAXLABS_MAX_NODES && (start_index + 1) < memory_map[node][reg].length) {
            int16_t val;
            memcpy(&val, &memory_map[node][reg].payload[start_index], 2);
            return val;
        }
        return 0;
    }

    bool is_node_online(uint8_t node, uint8_t heartbeat_reg, unsigned long timeout_ms = 3000) {
        if (node >= MAXLABS_MAX_NODES) return false;
        return (millis() - memory_map[node][heartbeat_reg].last_update) < timeout_ms;
    }
};

// ========================================================================
// GLOBAL VARIABLE DEFINITION (Bypasses ESPHome YAML Compiler Bugs)
// ========================================================================
extern MaxlabsEsphomeWrapper* global_maxlabs;
__attribute__((weak)) MaxlabsEsphomeWrapper* global_maxlabs = nullptr;
