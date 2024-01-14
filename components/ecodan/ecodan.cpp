#include "ecodan.h"

namespace esphome {
namespace ecodan_ {

static const char *TAG = "ecodan";

void EcodanSwitch::dump_config() {
  LOG_SWITCH("", "Ecodan Switch", this);
  ESP_LOGCONFIG(TAG, "  Switch has key %s", this->key_.c_str());
}

void EcodanSwitch::write_state(bool state) {
  uint8_t sendBuffer[PACKET_BUFFER_SIZE];
  bool send = false;
  #define ECODAN_WRITE_SWITCH(sw) \
    if (this->key_ == #sw) { \
      send = true; \
      memcpy(sendBuffer, command_##sw::packetMask, PACKET_BUFFER_SIZE); \
      sendBuffer[command_##sw::varIndex] = state ? 1 : 0; \
    }
    ECODAN_SWITCH_LIST(ECODAN_WRITE_SWITCH, )
  if (send == false) {
    return;
  }
  this->heatpump_->sendSerialPacket(sendBuffer);
  this->publish_state(state);
}

void EcodanSelect::dump_config() {
  LOG_SELECT("", "Ecodan Select", this);
  ESP_LOGCONFIG(TAG, "  Select has key %s", this->key_.c_str());
}

void EcodanSelect::control(const std::string &value) {
  uint8_t sendBuffer[PACKET_BUFFER_SIZE];
  bool send = false;
  #define ECODAN_WRITE_SELECT(sl) \
    if (this->key_ == #sl) { \
      send = true; \
      memcpy(sendBuffer, command_##sl::packetMask, PACKET_BUFFER_SIZE); \
      uint8_t mode = parseModeStringToInt(command_##sl::varType, value); \
      sendBuffer[command_##sl::varIndex] = mode; \
    }
    ECODAN_SELECT_LIST(ECODAN_WRITE_SELECT, )
  if (send == false) {
    return;
  }
  this->heatpump_->sendSerialPacket(sendBuffer);
  this->publish_state(value);
}

void EcodanNumber::dump_config() {
  LOG_NUMBER("", "Ecodan Number", this);
  ESP_LOGCONFIG(TAG, "  Number has key %s", this->key_.c_str());
}

void EcodanNumber::control(float value) {
  uint8_t sendBuffer[PACKET_BUFFER_SIZE], temp1, temp2;
  uint16_t temperature = value * 100;
  temp1 = (uint8_t) (temperature >> 8);
  temp2 = (uint8_t) (temperature & 0x00ff);
  bool send = false;
  #define ECODAN_WRITE_NUMBER(nb) \
    if (this->key_ == #nb) { \
      send = true; \
      memcpy(sendBuffer, command_##nb::packetMask, PACKET_BUFFER_SIZE); \
      sendBuffer[command_##nb::varIndex] = temp1; \
      sendBuffer[command_##nb::varIndex + 1] = temp2; \
    }
    ECODAN_NUMBER_LIST(ECODAN_WRITE_NUMBER, )
  if (send == false) {
    return;
  }
  this->heatpump_->sendSerialPacket(sendBuffer);
  this->publish_state(value);
}

EcodanHeatpump::EcodanHeatpump (uart::UARTComponent *parent) : uart::UARTDevice(parent) {}

void EcodanHeatpump::setup() {
  // This will be called by App.setup()
  set_update_interval(POLL_INTERVAL);
}

void EcodanHeatpump::update() {
  // This will be called by App.loop()
  if (!isInitialized) {
    initialize();
    receiveSerialPacket();
  } else {
    uint8_t sendBuffer[PACKET_BUFFER_SIZE] = {0xfc, 0x42, 0x02, 0x7a, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t usedAddresses[20] = {};
    bool addressUsed = false;
    uint8_t nextAddress = 0;
#define ECODAN_READ_ENTITY(e, type) \
    if (field_##e::address != 0xff) { \
      addressUsed = false; \
      for (int i = 0; i < 20; i++) { \
        if (usedAddresses[i] == field_##e::address) { \
          addressUsed = true; \
          break; \
        } \
      } \
      if (!addressUsed && this->type##_##e##_ != nullptr) { \
        usedAddresses[nextAddress] = field_##e::address; \
        nextAddress++; \
        sendBuffer[5] = field_##e::address; \
        sendSerialPacket(sendBuffer); \
        delay(READOUT_DELAY); \
        receiveSerialPacket(); \
      } \
    }
#define ECODAN_READ_SENSOR(p_s) ECODAN_READ_ENTITY(p_s, s)
    ECODAN_SENSOR_LIST(ECODAN_READ_SENSOR, )
#define ECODAN_READ_TEXT_SENSOR(p_ts) ECODAN_READ_ENTITY(p_ts, ts)
    ECODAN_TEXT_SENSOR_LIST(ECODAN_READ_TEXT_SENSOR, )
#define ECODAN_READ_SWITCH(p_sw) ECODAN_READ_ENTITY(p_sw, sw)
    ECODAN_SWITCH_LIST(ECODAN_READ_SWITCH, )
#define ECODAN_READ_SELECT(p_sl) ECODAN_READ_ENTITY(p_sl, sl)
    ECODAN_SELECT_LIST(ECODAN_READ_SELECT, )
#define ECODAN_READ_NUMBER(p_nb) ECODAN_READ_ENTITY(p_nb, nb)
    ECODAN_NUMBER_LIST(ECODAN_READ_NUMBER, )
  }
}

void EcodanHeatpump::initialize() {
  int i;
  for (i = 0; i < CONNECT_LEN; i++) {
    write(CONNECT[i]);
  }
  flush();
  ESP_LOGD(TAG, "Sent connect command");
}

void EcodanHeatpump::receiveSerialPacket() {
  uint8_t receiveBuffer[PACKET_BUFFER_SIZE];

  if (readPacket(receiveBuffer) == RCVD_PKT_CONNECT_SUCCESS) {
    parsePacket(receiveBuffer);
  }
}

int EcodanHeatpump::readPacket(uint8_t *data) {
  bool foundStart = false;
  int dataSum = 0;
  uint8_t checksum = 0;
  uint8_t dataLength = 0;
  if (available() > 0) { // read until we get start byte 0xfc
    while (available() > 0 && !foundStart) {
      data[0] = read();
      if (data[0] == CONNECT[0]) {
        foundStart = true;
        delay(READOUT_DELAY); // found that this delay increases accuracy when reading, might not be needed though
      }
    }
    if (!foundStart) {
      return RCVD_PKT_FAIL;
    }
    for (int i = 1; i < 5; i++) { // read header
      data[i] = read();
    }
    if (data[0] == CONNECT[0]) { // check header
      dataLength = data[4] + 5;
      for (int i = 5; i < dataLength; i++) {
        data[i] = read(); // read the payload data
      }
      uint8_t checksum = calculateCheckSum(data);
      data[dataLength] = read();   // read checksum byte
      if (data[dataLength] == checksum) { // correct packet found
        if (data[1] == 0x7a) {
          isInitialized = true;
          ESP_LOGI(TAG, "Connected successfully");
        }
        return RCVD_PKT_CONNECT_SUCCESS;
      } else {
        ESP_LOGE(TAG, "CRC ERROR");
      }
    }
  }
  return RCVD_PKT_FAIL;
}

void EcodanHeatpump::sendSerialPacket(uint8_t *sendBuffer) {
  int i;
  uint8_t packetLength;
  uint8_t checkSum;

  checkSum = calculateCheckSum(sendBuffer);
  packetLength = sendBuffer[4] + 5;
  sendBuffer[packetLength] = checkSum;
  for (i = 0; i <= packetLength; i++) {
    write(sendBuffer[i]);
  }
  flush();
}

uint8_t EcodanHeatpump::calculateCheckSum(uint8_t *data) {
  uint8_t dataLength = 0;
  int dataSum = 0;
  uint8_t checksum = 0;

  dataLength = data[4] + 5;
  for (int i = 0; i < dataLength; i++) { // sum up the header bytes...
    dataSum += data[i];
  }
  checksum = (0xfc - dataSum) & 0xff; // calculate checksum
  return checksum;
}

void EcodanHeatpump::parsePacket(uint8_t *packet) {
#define ECODAN_PUBLISH_ENTITY(e, type, parser) \
  if (this->type##_##e##_ != nullptr && \
    field_##e::address == packet[5] && \
    0x62 == packet[1]) \
    type##_##e##_->publish_state(parser(packet, field_##e::varType, field_##e::varIndex));
#define ECODAN_PUBLISH_SENSOR(p_s) ECODAN_PUBLISH_ENTITY(p_s, s, parsePacketNumberItem)
  ECODAN_SENSOR_LIST(ECODAN_PUBLISH_SENSOR, )
#define ECODAN_PUBLISH_TEXT_SENSOR(p_ts) ECODAN_PUBLISH_ENTITY(p_ts, ts, parsePacketTextItem)
  ECODAN_TEXT_SENSOR_LIST(ECODAN_PUBLISH_TEXT_SENSOR, )
#define ECODAN_PUBLISH_SWITCH(p_sw) ECODAN_PUBLISH_ENTITY(p_sw, sw, parsePacketBoolItem)
  ECODAN_SWITCH_LIST(ECODAN_PUBLISH_SWITCH, )
#define ECODAN_PUBLISH_SELECT(p_sl) ECODAN_PUBLISH_ENTITY(p_sl, sl, parsePacketTextItem)
  ECODAN_SELECT_LIST(ECODAN_PUBLISH_SELECT, )
#define ECODAN_PUBLISH_NUMBER(p_nb) ECODAN_PUBLISH_ENTITY(p_nb, nb, parsePacketNumberItem)
  ECODAN_NUMBER_LIST(ECODAN_PUBLISH_NUMBER, )
}

void EcodanHeatpump::setRemoteTemperature(float value) {
  uint8_t sendBuffer[PACKET_BUFFER_SIZE], temp1, temp2;
  memcpy(sendBuffer, command_zone1_room_temp::packetMask, PACKET_BUFFER_SIZE);
  if (value > 0) {
    ESP_LOGI(TAG, "Room temperature set to: %f", value);
    // uint16_t temperature = value * 100;
    // temp1 = (uint8_t) (temperature >> 8);
    // temp2 = (uint8_t) (temperature & 0x00ff);
    // sendBuffer[command_zone1_room_temp::varIndex] = temp1;
    // sendBuffer[command_zone1_room_temp::varIndex + 1] = temp2;
    value = value * 2;
    value = round(value);
    value = value / 2;
    float temp1 = 3 + ((value - 10) * 2);
    sendBuffer[command_zone1_room_temp::varIndex] = (int)temp1;
    float temp2 = (value * 2) + 128;
    sendBuffer[command_zone1_room_temp::varIndex + 1] = (int)temp2;
  } else {
    ESP_LOGI(TAG, "Room temperature control back to builtin sensor");
    sendBuffer[command_zone1_room_temp::varIndex - 1] = 0x00;
    sendBuffer[command_zone1_room_temp::varIndex + 1] = 0x80;
  }
  this->sendSerialPacket(sendBuffer);
}

void EcodanHeatpump::dump_config() {
  ESP_LOGCONFIG(TAG, "ecodan:");

#define ECODAN_LOG_SENSOR(s) LOG_SENSOR("  ", #s, this->s_##s##_);
  ECODAN_SENSOR_LIST(ECODAN_LOG_SENSOR, )

#define ECODAN_LOG_TEXT_SENSOR(ts) LOG_TEXT_SENSOR("  ", #ts, this->ts_##ts##_);
  ECODAN_TEXT_SENSOR_LIST(ECODAN_LOG_TEXT_SENSOR, )

#define ECODAN_LOG_SWITCH(sw) this->sw_##sw##_->dump_config();
  ECODAN_SWITCH_LIST(ECODAN_LOG_SWITCH, )

#define ECODAN_LOG_SELECT(sl) this->sl_##sl##_->dump_config();
  ECODAN_SELECT_LIST(ECODAN_LOG_SELECT, )

#define ECODAN_LOG_NUMBER(nb) this->nb_##nb##_->dump_config();
  ECODAN_NUMBER_LIST(ECODAN_LOG_NUMBER, )
}

} // namespace ecodan_
} // namespace esphome