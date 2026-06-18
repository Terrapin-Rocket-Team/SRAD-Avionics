#ifndef ARC_NODE_H
#define ARC_NODE_H

#include <Arduino.h>

// arc-protocol is plain C; the headers carry their own extern "C" guards.
#include "arc_protocol.h"
#include "arc_messages_fc_coord.h"
#include "arc_messages_radio.h"

// Thin ARC-network adapter for the nosecone flight computer.
//
// The FC is an ARC node (ARC_ADDR_FC_N). Frames are built with arc-protocol,
// COBS-encoded, and written to the serial link toward the Teensy hub. The hub
// learns our route from the broadcast heartbeats and forwards downlink traffic.
class ArcNode
{
public:
    ArcNode(uint8_t address, Stream &link);

    // Broadcast a NETMGMT heartbeat. Heartbeats are always broadcast so the hub
    // and ground can learn our route (see project heartbeat convention).
    bool sendHeartbeat();

    // Send an FC_COORD FLIGHT_TELEMETRY frame addressed to ground for
    // display/downlink (see project addressing convention).
    bool sendFlightTelemetry(const arc_fc_coord_flight_telemetry_t &telem);

    // Hand an opaque vendor frame (e.g. RadioMessage/APRS) to the proprietary
    // data radio (ARC_ADDR_RADIO_DATA) as a RADIO/DATA_DOWNLINK message. The
    // hub forwards it and the data radio transmits the bytes as-is -- it does
    // not speak arc-protocol.
    bool sendDataRadioDownlink(const uint8_t *frame, size_t len);

    // Generic frame send. dst/family/type are the constants in arc_protocol.h.
    bool send(uint8_t dst, uint8_t family, uint8_t type,
              const uint8_t *payload, size_t payload_len, uint8_t flags = 0);

    // Also write the raw bytes of every frame we send to an extra stream (e.g.
    // the USB console), in addition to the hub link.
    void addMirror(Stream &stream);

private:
    static constexpr size_t kMaxMirrors = 2;

    uint8_t address_;
    Stream &link_;
    uint8_t session_;
    uint16_t seq_;
    Stream *mirror_[kMaxMirrors] = {};
    size_t mirror_count_ = 0;
};

#endif // ARC_NODE_H
