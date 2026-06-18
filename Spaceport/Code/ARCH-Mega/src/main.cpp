#include "Arduino.h"
#include "pin_defs.h"
#include "Timer.h"

// ARC wire-format library (https://github.com/DrewBrandt/arc-protocol).
// Pure C with extern "C" guards, so it includes cleanly into this C++ TU.
#include "arc_protocol.h"
#include "arc_messages_power.h"

#define ADC_MAX 1023.

// ----------------------------------------------------------------------
// ARC node identity + link.
//
// This board is the nosecone ARCH-Mega power node. Flash the lower-bay /
// center boards with MY_ADDR = ARC_ADDR_ARCH_MEGA_L / _C respectively.
// ARC frames (COBS-framed) run over the same UART the old text protocol used.
// ----------------------------------------------------------------------
#define MY_ADDR        ARC_ADDR_ARCH_MEGA_N   // 0x30
#define ARC_LINK       Serial1A
#define ARC_LINK_BAUD  115200

// Small scratch buffers: every frame we build or accept is tiny (the biggest
// is a 6-channel STATUS_REPORT, ~33 bytes framed), so we don't need the full
// ARC_MAX_* sizing on this RAM-limited STM32C0.
#define ARC_BUF_SIZE   96

#define NUM_CHANNELS 6
const uint32_t PWR_CHNS[NUM_CHANNELS] = {PWR_CH1, PWR_CH2, PWR_CH3, PWR_CH4, PWR_CH5, PWR_CH6};
const uint8_t  PWR_DEFAULTS[NUM_CHANNELS] = {
    PWR_CH1_DEFAULT, PWR_CH2_DEFAULT, PWR_CH3_DEFAULT,
    PWR_CH4_DEFAULT, PWR_CH5_DEFAULT, PWR_CH6_DEFAULT};
const bool CH_ACTIVE_HIGH = true;  // outputs assert HIGH for ON

HardwareSerial Serial1A(PA_10_R, PA_9_R);

// Bit N (0..5) set => physical channel N+1 is commanded ON.
uint8_t channelMask = 0x00;

Timer voltReadRate(1000);
Timer heartbeatRate(1000);  // 1 Hz broadcast
Timer telemRate(500);       // 2 Hz BOARD_TELEMETRY to ground

// Latest measured rail voltages, refreshed on voltReadRate.
float bat_V = 0, rail_V = 0, charge_V = 0;

// ARC framing state. Session is fixed per power-up; seq increments per frame.
uint8_t  arcSession = 1;
uint16_t arcSeq = 0;

// COBS reassembly buffer for the inbound link (frames delimited by 0x00).
uint8_t rxbuf[ARC_BUF_SIZE];
size_t  rxlen = 0;

void getVoltage(float &bat, float &rail, float &charge);

// ----------------------------------------------------------------------
// Channel control.
// ----------------------------------------------------------------------
void applyChannel(uint8_t idx) {
  bool on = (channelMask >> idx) & 0x01;
  digitalWrite(PWR_CHNS[idx], (on == CH_ACTIVE_HIGH) ? HIGH : LOW);
}

void setChannel(uint8_t idx, bool on) {
  if (idx >= NUM_CHANNELS) return;
  if (on) channelMask |= (uint8_t)(1u << idx);
  else    channelMask &= (uint8_t)~(1u << idx);
  applyChannel(idx);
}

// SET_OUTPUT_MASK: only channels in enable_mask are touched.
void setChannelMask(uint8_t enable_mask, uint8_t state_mask) {
  for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
    if (enable_mask & (1u << i)) {
      setChannel(i, (state_mask >> i) & 0x01);
    }
  }
}

// ----------------------------------------------------------------------
// Derive a board-level charge state from the measured voltages. This board
// has no charge-current sensing, so "charging" vs "plugged" is a heuristic.
// ----------------------------------------------------------------------
uint8_t chargeStatus() {
  if (charge_V < 1.0f) return ARC_POWER_CHARGE_UNPLUGGED;
  if (charge_V > bat_V + 0.2f) return ARC_POWER_CHARGE_CHARGING;
  return ARC_POWER_CHARGE_PLUGGED;
}

// ----------------------------------------------------------------------
// ARC transmit helpers.
// ----------------------------------------------------------------------
void arcWriteEncoded(const uint8_t* frame, int flen) {
  if (flen < 0) return;
  uint8_t encoded[ARC_BUF_SIZE];
  int elen = arc_cobs_encode(frame, (size_t)flen, encoded, sizeof(encoded));
  if (elen < 0) return;
  ARC_LINK.write(encoded, (size_t)elen);
}

void arcSend(uint8_t dst, uint8_t flags, uint8_t family, uint8_t type,
             const uint8_t* payload, size_t payload_len) {
  uint8_t frame[ARC_BUF_SIZE];
  int flen = arc_frame_build(frame, sizeof(frame), MY_ADDR, dst, flags,
                             arcSession, arcSeq++, family, type,
                             payload, payload_len);
  arcWriteEncoded(frame, flen);
}

void arcSendAck(const arc_frame_t* original) {
  uint8_t frame[ARC_BUF_SIZE];
  int flen = arc_frame_build_ack(frame, sizeof(frame), original,
                                 arcSession, arcSeq++);
  arcWriteEncoded(frame, flen);
}

void sendHeartbeat() {
  // Leaf-node heartbeat: broadcast, empty payload. The hub coalesces these.
  arcSend(ARC_ADDR_BROADCAST, 0, ARC_FAMILY_NETMGMT, ARC_NETMGMT_HEARTBEAT,
          NULL, 0);
}

void sendBoardTelemetry() {
  arc_power_board_telemetry_t t;
  t.output_on_mask     = channelMask;
  t.output_fault_mask  = 0x00;  // no per-channel fault sensing
  t.battery_voltage_mv = (uint16_t)(bat_V * 1000.0f);
  t.charge_status      = chargeStatus();
  t.charge_voltage_mv  = (uint16_t)(charge_V * 1000.0f);

  uint8_t payload[ARC_POWER_BOARD_TELEMETRY_PAYLOAD_SIZE];
  int n = arc_power_board_telemetry_encode(&t, payload, sizeof(payload));
  if (n < 0) return;

  // Telemetry is downlinked to the ground station (0x01).
  arcSend(ARC_ADDR_GROUND, 0, ARC_FAMILY_POWER, ARC_POWER_BOARD_TELEMETRY,
          payload, (size_t)n);
}

void sendStatusReport(uint8_t dst) {
  arc_power_status_report_t r;
  r.channel_count = NUM_CHANNELS;
  for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
    r.channels[i].state = (channelMask >> i) & 0x01 ? ARC_POWER_ON : ARC_POWER_OFF;
    // No per-channel current sensing on this board.
    r.channels[i].current_ma = 0;
  }
  // Report the regulated 5V rail as the bus voltage; no temp sensor here.
  r.bus_voltage_mv = (uint16_t)(rail_V * 1000.0f);
  r.temp_c = 0;

  uint8_t payload[ARC_POWER_STATUS_REPORT_SIZE(NUM_CHANNELS)];
  int n = arc_power_status_report_encode(&r, payload, sizeof(payload));
  if (n < 0) return;

  arcSend(dst, 0, ARC_FAMILY_POWER, ARC_POWER_STATUS_REPORT, payload, (size_t)n);
}

// ----------------------------------------------------------------------
// ARC receive: dispatch a fully parsed frame.
// ----------------------------------------------------------------------
void handleFrame(const arc_frame_t* f) {
  // Only act on frames addressed to us (or broadcast).
  if (f->dst != MY_ADDR && f->dst != ARC_ADDR_BROADCAST) return;

  if (f->family == ARC_FAMILY_POWER) {
    switch (f->type) {
      case ARC_POWER_SET_OUTPUT: {
        arc_power_set_output_t msg;
        if (arc_power_set_output_decode(f->payload, f->payload_len, &msg) == ARC_OK) {
          // Channels are 1-based on the wire; bit/pin index is 0-based.
          if (msg.channel >= 1 && msg.channel <= NUM_CHANNELS) {
            setChannel(msg.channel - 1, msg.state == ARC_POWER_ON);
          }
        }
        break;
      }
      case ARC_POWER_SET_OUTPUT_MASK: {
        arc_power_set_output_mask_t msg;
        if (arc_power_set_output_mask_decode(f->payload, f->payload_len, &msg) == ARC_OK) {
          setChannelMask(msg.enable_mask, msg.state_mask);
        }
        break;
      }
      case ARC_POWER_GET_STATUS:
        sendStatusReport(f->src);  // reply to whoever asked
        break;
      default:
        break;
    }
  }

  // Reliable frames expect an ack regardless of family.
  if (f->flags & ARC_FLAG_RELIABLE) {
    arcSendAck(f);
  }
}

// Feed one received byte into the COBS reassembler. On a 0x00 delimiter,
// decode + parse the accumulated frame and dispatch it.
void arcFeedByte(uint8_t b) {
  if (rxlen < sizeof(rxbuf)) {
    rxbuf[rxlen++] = b;
  } else {
    // Overflow without a delimiter: drop and resync on the next 0x00.
    rxlen = 0;
    if (b != 0x00) return;
    rxbuf[rxlen++] = b;
  }

  if (b != 0x00) return;  // not a frame boundary yet

  uint8_t decoded[ARC_BUF_SIZE];
  int dlen = arc_cobs_decode(rxbuf, rxlen, decoded, sizeof(decoded));
  rxlen = 0;
  if (dlen < 0) return;

  arc_frame_t frame;
  if (arc_frame_parse(decoded, (size_t)dlen, &frame) == ARC_OK) {
    handleFrame(&frame);
  }
}

void setup() {
  ARC_LINK.begin(ARC_LINK_BAUD);

  pinMode(BAT_VOLT, INPUT);
  pinMode(RAIL_VOLT, INPUT);
  pinMode(CHARGE_VOLT, INPUT);

  pinMode(STAT, OUTPUT);

  // Bring the outputs up in their default states, staggered 1s apart to spread
  // the inrush, and seed channelMask to match.
  for (uint8_t i = 0; i < NUM_CHANNELS; i++) {
    pinMode(PWR_CHNS[i], OUTPUT);
    if (i > 0) delay(1000);
    digitalWrite(PWR_CHNS[i], PWR_DEFAULTS[i]);
    if (PWR_DEFAULTS[i] == HIGH) channelMask |= (uint8_t)(1u << i);
  }

  digitalWrite(STAT, HIGH);
}

void loop() {
  // Service the ARC link: ingest received command/heartbeat bytes.
  while (ARC_LINK.available()) {
    arcFeedByte((uint8_t)ARC_LINK.read());
  }

  if (voltReadRate.evaluate()) {
    digitalWrite(STAT, HIGH);
    getVoltage(bat_V, rail_V, charge_V);
    digitalWrite(STAT, LOW);
  }

  if (heartbeatRate.evaluate()) {
    sendHeartbeat();
  }

  if (telemRate.evaluate()) {
    sendBoardTelemetry();
  }
}

void getVoltage(float &bat, float &rail, float &charge)
{
    // get adc reading as a percent of max value
    // then multiply by the max readable value of the voltage on the other end of the voltage divider
    bat = float(analogRead(BAT_VOLT)) / ADC_MAX * BAT_VOLT_MAX;
    rail = float(analogRead(RAIL_VOLT)) / ADC_MAX * RAIL_VOLT_MAX;
    charge = float(analogRead(CHARGE_VOLT)) / ADC_MAX * CHARGE_VOLT_MAX;
}
