#include "ArcNode.h"

ArcNode::ArcNode(uint8_t address, Stream &link)
    : address_(address), link_(link), session_(0), seq_(0) {}

void ArcNode::addMirror(Stream &stream)
{
    if (mirror_count_ < kMaxMirrors)
        mirror_[mirror_count_++] = &stream;
}

bool ArcNode::send(uint8_t dst, uint8_t family, uint8_t type,
                   const uint8_t *payload, size_t payload_len, uint8_t flags)
{
    uint8_t frame[ARC_MAX_FRAME_SIZE];
    const int frame_len = arc_frame_build(frame, sizeof(frame),
                                          address_, dst, flags, session_, seq_++,
                                          family, type, payload, payload_len);
    if (frame_len < 0)
        return false;

    // COBS-frame for the serial link (trailing 0x00 delimits frames on the bus).
    uint8_t encoded[ARC_MAX_ENCODED_SIZE];
    const int enc_len = arc_cobs_encode(frame, static_cast<size_t>(frame_len),
                                        encoded, sizeof(encoded));
    if (enc_len < 0)
        return false;

    // Same raw bytes out the hub link and every mirror (e.g. USB console).
    const bool ok = link_.write(encoded, static_cast<size_t>(enc_len)) ==
                    static_cast<size_t>(enc_len);
    for (size_t i = 0; i < mirror_count_; ++i)
        mirror_[i]->write(encoded, static_cast<size_t>(enc_len));

    return ok;
}

bool ArcNode::sendHeartbeat()
{
    return send(ARC_ADDR_BROADCAST, ARC_FAMILY_NETMGMT, ARC_NETMGMT_HEARTBEAT,
                nullptr, 0);
}

bool ArcNode::sendFlightTelemetry(const arc_fc_coord_flight_telemetry_t &telem)
{
    uint8_t payload[ARC_FC_COORD_FLIGHT_TELEMETRY_PAYLOAD_SIZE];
    const int n = arc_fc_coord_flight_telemetry_encode(&telem, payload, sizeof(payload));
    if (n < 0)
        return false;

    return send(ARC_ADDR_GROUND, ARC_FAMILY_FC_COORD, ARC_FC_COORD_FLIGHT_TELEMETRY,
                payload, static_cast<size_t>(n));
}

bool ArcNode::sendDataRadioDownlink(const uint8_t *frame, size_t len)
{
    return send(ARC_ADDR_RADIO_DATA, ARC_FAMILY_RADIO, ARC_RADIO_DATA_DOWNLINK,
                frame, len);
}
