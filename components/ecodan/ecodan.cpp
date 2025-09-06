#include "ecodan.h"

constexpr uint8_t ecodan::commands::command_power_state::packetMask[PACKET_BUFFER_SIZE];
constexpr uint8_t ecodan::commands::command_force_dhw::packetMask[PACKET_BUFFER_SIZE];
constexpr uint8_t ecodan::commands::command_holiday_mode::packetMask[PACKET_BUFFER_SIZE];
constexpr uint8_t ecodan::commands::command_mode_select::packetMask[PACKET_BUFFER_SIZE];
constexpr uint8_t ecodan::commands::command_hot_water_mode::packetMask[PACKET_BUFFER_SIZE];
constexpr uint8_t ecodan::commands::command_hot_water_setpoint::packetMask[PACKET_BUFFER_SIZE];
constexpr uint8_t ecodan::commands::command_zone1_room_temp_setpoint::packetMask[PACKET_BUFFER_SIZE];
constexpr uint8_t ecodan::commands::command_zone1_flow_temp_setpoint::packetMask[PACKET_BUFFER_SIZE];
constexpr uint8_t ecodan::commands::command_zone2_room_temp_setpoint::packetMask[PACKET_BUFFER_SIZE];
constexpr uint8_t ecodan::commands::command_zone2_flow_temp_setpoint::packetMask[PACKET_BUFFER_SIZE];
constexpr uint8_t ecodan::commands::command_zone1_room_temp::packetMask[PACKET_BUFFER_SIZE];

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
  
  // Special handling for zone2_room_temp_setpoint - include current Zone 1 value
  if (this->key_ == "zone2_room_temp_setpoint") {
    send = true;
    memcpy(sendBuffer, command_zone2_room_temp_setpoint::packetMask, PACKET_BUFFER_SIZE);
    
    // Get current Zone 1 room temperature setpoint
    float zone1_temp = this->heatpump_->get_zone1_room_temp_setpoint();
    uint16_t zone1_temperature = zone1_temp * 100;
    uint8_t zone1_temp1 = (uint8_t) (zone1_temperature >> 8);
    uint8_t zone1_temp2 = (uint8_t) (zone1_temperature & 0x00ff);
    
    // Fill Zone 1 position (bytes 15-16)
    sendBuffer[15] = zone1_temp1;
    sendBuffer[16] = zone1_temp2;
    
    // Fill Zone 2 position (bytes 17-18)
    sendBuffer[command_zone2_room_temp_setpoint::varIndex] = temp1;
    sendBuffer[command_zone2_room_temp_setpoint::varIndex + 1] = temp2;
    
    ESP_LOGD(TAG, "Number %s: value=%.1f, temp=%d, temp1=0x%02x, temp2=0x%02x, varIndex=%d, zone1_temp=%.1f", 
             "zone2_room_temp_setpoint", value, temperature, temp1, temp2, command_zone2_room_temp_setpoint::varIndex, zone1_temp);
  } else {
    #define ECODAN_WRITE_NUMBER(nb) \
      if (this->key_ == #nb) { \
        send = true; \
        memcpy(sendBuffer, command_##nb::packetMask, PACKET_BUFFER_SIZE); \
        sendBuffer[command_##nb::varIndex] = temp1; \
        sendBuffer[command_##nb::varIndex + 1] = temp2; \
        ESP_LOGD(TAG, "Number %s: value=%.1f, temp=%d, temp1=0x%02x, temp2=0x%02x, varIndex=%d", \
                 #nb, value, temperature, temp1, temp2, command_##nb::varIndex); \
      }
      ECODAN_NUMBER_LIST(ECODAN_WRITE_NUMBER, )
  }
  
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
  buildEntityList();
  state_ = ComponentState::INITIALIZING;
  last_read_time_ = millis(); // Initialize timing
  
  // Initialize operation queue
  pending_operation_.is_pending = false;
  operation_in_progress_ = false;
}

void EcodanHeatpump::update() {
  // This will be called by App.loop() - keep it non-blocking
  // Reset to reading entities if we're idle (start new cycle)
  if (state_ == ComponentState::IDLE && isInitialized) {
    ESP_LOGD(TAG, "Starting new polling cycle");
    current_entity_index_ = 0;
    state_ = ComponentState::READING_ENTITIES;
  }
  
  processStateMachine();
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
  static uint8_t read_state = 0; // 0: looking for start, 1: reading header, 2: reading payload
  static uint8_t bytes_read = 0;
  static uint8_t expected_length = 0;
  
  // Non-blocking read - only process available bytes
  while (available() > 0) {
    switch (read_state) {
      case 0: // Looking for start byte
        data[0] = read();
        if (data[0] == CONNECT[0]) {
          read_state = 1;
          bytes_read = 1;
          ESP_LOGV(TAG, "Found start byte");
        }
        break;
        
      case 1: // Reading header
        if (bytes_read < 5) {
          data[bytes_read] = read();
          bytes_read++;
          if (bytes_read == 5) {
            expected_length = data[4] + 5;
            read_state = 2;
            ESP_LOGV(TAG, "Read header, expecting %d total bytes", expected_length + 1);
          }
        }
        break;
        
      case 2: // Reading payload and checksum
        if (bytes_read <= expected_length) {
          data[bytes_read] = read();
          bytes_read++;
          
          if (bytes_read > expected_length) {
            // We have the complete packet including checksum
            uint8_t checksum = calculateCheckSum(data);
            if (data[expected_length] == checksum) {
              // Reset state for next packet
              read_state = 0;
              bytes_read = 0;
              expected_length = 0;
              
              ESP_LOGD(TAG, "Received valid packet: type=0x%02x, address=0x%02x", data[1], data[5]);
              
              if (data[1] == 0x7a) {
                isInitialized = true;
                ESP_LOGI(TAG, "Connected successfully");
              }
              return RCVD_PKT_CONNECT_SUCCESS;
            } else {
              ESP_LOGE(TAG, "CRC ERROR: expected 0x%02x, got 0x%02x", checksum, data[expected_length]);
              // Reset state on error
              read_state = 0;
              bytes_read = 0;
              expected_length = 0;
            }
          }
        }
        break;
    }
  }
  
  return RCVD_PKT_FAIL; // No complete packet yet
}

void EcodanHeatpump::sendSerialPacket(uint8_t *sendBuffer) {
  // Queue user commands for sending
  ESP_LOGD(TAG, "Queuing user command for sending");
  queueCommand(sendBuffer);
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
  ESP_LOGD(TAG, "Parsing packet: type=0x%02x, payload[0]=0x%02x", packet[1], packet[5]);
  
  // Handle operation responses
  if (operation_in_progress_) {
    if (pending_operation_.is_user_command && packet[1] == 0x61) {
      ESP_LOGD(TAG, "Received Set Response (type 0x61) for user command - completed");
      operation_in_progress_ = false;
      // Clear waiting_for_response if it was set during user command handling
      if (waiting_for_response_) {
        waiting_for_response_ = false;
      }
    } else if (!pending_operation_.is_user_command && packet[1] == 0x62 && 
               packet[5] == pending_operation_.expected_response_address) {
      ESP_LOGD(TAG, "Received Get Response (type 0x62) for sensor read address 0x%02x - completed", packet[5]);
      operation_in_progress_ = false;
      waiting_for_response_ = false;
      current_entity_index_++;
      last_read_time_ = millis();
    }
  }
  
#define ECODAN_PUBLISH_ENTITY(e, type, parser) \
  if (this->type##_##e##_ != nullptr && \
    field_##e::address == packet[5] && \
    0x62 == packet[1]) { \
    auto value = parser(packet, field_##e::varType, field_##e::varIndex); \
    ESP_LOGV(TAG, "Publishing %s %s with value", #type, #e); \
    type##_##e##_->publish_state(value); \
  }
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
  this->queueCommand(sendBuffer);
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

void EcodanHeatpump::buildEntityList() {
  entity_list_.clear();
  
  ESP_LOGD(TAG, "Building entity list...");
  
  // Build list of all configured entities with their addresses
#define ECODAN_ADD_SENSOR(s) \
  if (field_##s::address != 0xff && this->s_##s##_ != nullptr) { \
    bool already_added = false; \
    for (const auto& entity : entity_list_) { \
      if (entity.address == field_##s::address) { \
        already_added = true; \
        break; \
      } \
    } \
    if (!already_added) { \
      entity_list_.push_back({field_##s::address, true, "s"}); \
      ESP_LOGV(TAG, "Added sensor %s at address 0x%02x", #s, field_##s::address); \
    } \
  }
  ECODAN_SENSOR_LIST(ECODAN_ADD_SENSOR, )

#define ECODAN_ADD_TEXT_SENSOR(ts) \
  if (field_##ts::address != 0xff && this->ts_##ts##_ != nullptr) { \
    bool already_added = false; \
    for (const auto& entity : entity_list_) { \
      if (entity.address == field_##ts::address) { \
        already_added = true; \
        break; \
      } \
    } \
    if (!already_added) { \
      entity_list_.push_back({field_##ts::address, true, "ts"}); \
      ESP_LOGV(TAG, "Added text sensor %s at address 0x%02x", #ts, field_##ts::address); \
    } \
  }
  ECODAN_TEXT_SENSOR_LIST(ECODAN_ADD_TEXT_SENSOR, )

#define ECODAN_ADD_SWITCH(sw) \
  if (field_##sw::address != 0xff && this->sw_##sw##_ != nullptr) { \
    bool already_added = false; \
    for (const auto& entity : entity_list_) { \
      if (entity.address == field_##sw::address) { \
        already_added = true; \
        break; \
      } \
    } \
    if (!already_added) { \
      entity_list_.push_back({field_##sw::address, true, "sw"}); \
      ESP_LOGV(TAG, "Added switch %s at address 0x%02x", #sw, field_##sw::address); \
    } \
  }
  ECODAN_SWITCH_LIST(ECODAN_ADD_SWITCH, )

#define ECODAN_ADD_SELECT(sl) \
  if (field_##sl::address != 0xff && this->sl_##sl##_ != nullptr) { \
    bool already_added = false; \
    for (const auto& entity : entity_list_) { \
      if (entity.address == field_##sl::address) { \
        already_added = true; \
        break; \
      } \
    } \
    if (!already_added) { \
      entity_list_.push_back({field_##sl::address, true, "sl"}); \
      ESP_LOGV(TAG, "Added select %s at address 0x%02x", #sl, field_##sl::address); \
    } \
  }
  ECODAN_SELECT_LIST(ECODAN_ADD_SELECT, )

#define ECODAN_ADD_NUMBER(nb) \
  if (field_##nb::address != 0xff && this->nb_##nb##_ != nullptr) { \
    bool already_added = false; \
    for (const auto& entity : entity_list_) { \
      if (entity.address == field_##nb::address) { \
        already_added = true; \
        break; \
      } \
    } \
    if (!already_added) { \
      entity_list_.push_back({field_##nb::address, true, "nb"}); \
      ESP_LOGV(TAG, "Added number %s at address 0x%02x", #nb, field_##nb::address); \
    } \
  }
  ECODAN_NUMBER_LIST(ECODAN_ADD_NUMBER, )
  
  ESP_LOGI(TAG, "Built entity list with %d unique addresses", entity_list_.size());
}

void EcodanHeatpump::processStateMachine() {
  uint32_t now = millis();
  
  // Always check for incoming packets
  receiveSerialPacket();
  
  // Allow other tasks to run (watchdog-friendly)
  delayMicroseconds(1);
  
  static uint32_t last_debug_log = 0;
  if (now - last_debug_log > 5000) { // Log state every 5 seconds
    ESP_LOGD(TAG, "State machine: state=%d, initialized=%s, entities=%d, op_pending=%s", 
             (int)state_, isInitialized ? "true" : "false", entity_list_.size(),
             pending_operation_.is_pending ? "true" : "false");
    last_debug_log = now;
  }
  
  switch (state_) {
    case ComponentState::INITIALIZING:
      handleInitializing();
      break;
      
    case ComponentState::CONNECTED:
      // Check if we have a pending user command to send first
      if (pending_operation_.is_pending && pending_operation_.is_user_command) {
        state_ = ComponentState::SENDING_OPERATION;
      } else {
        // Reset entity reading
        current_entity_index_ = 0;
        state_ = ComponentState::READING_ENTITIES;
        ESP_LOGD(TAG, "Starting to read %d entities", entity_list_.size());
      }
      break;
      
    case ComponentState::READING_ENTITIES:
      // Check if we have a pending user command that needs priority FIRST
      if (pending_operation_.is_pending && pending_operation_.is_user_command && !operation_in_progress_) {
        ESP_LOGD(TAG, "User command pending, interrupting reading cycle");
        state_ = ComponentState::SENDING_OPERATION;
      } else {
        handleReading();
      }
      break;
      
    case ComponentState::WAITING_FOR_RESPONSE:
      // This state is no longer used - operations go through WAITING_FOR_OPERATION_RESPONSE
      ESP_LOGW(TAG, "In deprecated WAITING_FOR_RESPONSE state, transitioning to READING_ENTITIES");
      state_ = ComponentState::READING_ENTITIES;
      break;
      
    case ComponentState::SENDING_OPERATION:
      handleSendingOperation();
      break;
      
    case ComponentState::WAITING_FOR_OPERATION_RESPONSE:
      handleWaitingForOperationResponse();
      break;
      
    case ComponentState::IDLE:
      // Check if we have a pending user command to send
      if (pending_operation_.is_pending && pending_operation_.is_user_command && isInitialized) {
        state_ = ComponentState::SENDING_OPERATION;
      } else {
        // Stay idle until next update cycle
        ESP_LOGV(TAG, "State machine idle");
      }
      break;
  }
}

void EcodanHeatpump::handleInitializing() {
  uint32_t now = millis();
  
  if (!isInitialized) {
    if (now - last_command_time_ > 2000) { // Try every 2 seconds
      if (init_retry_count_ < 10) { // Max 10 retries
        ESP_LOGD(TAG, "Sending initialization command (attempt %d/10)", init_retry_count_ + 1);
        initialize();
        init_retry_count_++;
        last_command_time_ = now;
      } else {
        ESP_LOGE(TAG, "Failed to initialize after 10 attempts, resetting retry counter");
        // Reset and try again after a longer delay
        init_retry_count_ = 0;
        last_command_time_ = now - 1000; // Wait 1 more second before retrying
      }
    }
  } else {
    ESP_LOGI(TAG, "Initialization successful after %d attempts", init_retry_count_);
    init_retry_count_ = 0;
    state_ = ComponentState::CONNECTED;
  }
}

void EcodanHeatpump::handleReading() {
  if (current_entity_index_ >= entity_list_.size()) {
    // Finished reading all entities
    ESP_LOGD(TAG, "Finished reading all %d entities, going to IDLE", entity_list_.size());
    state_ = ComponentState::IDLE;
    return;
  }
  
  uint32_t now = millis();
  if (!waiting_for_response_ && !operation_in_progress_ && isTimeToReadNext()) {
    // Check if there's already a sensor read operation queued
    if (pending_operation_.is_pending && !pending_operation_.is_user_command) {
      // Sensor read already queued, send it
      state_ = ComponentState::SENDING_OPERATION;
      return;
    }
    
    // Queue new sensor read request
    uint8_t sendBuffer[PACKET_BUFFER_SIZE] = {0xfc, 0x42, 0x02, 0x7a, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    
    sendBuffer[5] = entity_list_[current_entity_index_].address;
    pending_address_ = entity_list_[current_entity_index_].address;
    
    ESP_LOGD(TAG, "Queuing sensor read for entity %d/%d, address 0x%02x", 
             current_entity_index_ + 1, entity_list_.size(), pending_address_);
    
    queueSensorRead(sendBuffer, pending_address_);
    waiting_for_response_ = true;
    last_command_time_ = now;
    
    // Immediately transition to sending the operation
    state_ = ComponentState::SENDING_OPERATION;
  } else if (waiting_for_response_) {
    ESP_LOGV(TAG, "Still waiting for response from previous request");
  } else if (operation_in_progress_) {
    ESP_LOGV(TAG, "Operation in progress, waiting...");
  } else {
    ESP_LOGV(TAG, "Waiting for next read time (last_read: %d, now: %d, delay: %d)", 
             last_read_time_, now, READOUT_DELAY);
  }
}

void EcodanHeatpump::handleSendingOperation() {
  if (!pending_operation_.is_pending) {
    ESP_LOGW(TAG, "In SENDING_OPERATION state but no operation pending");
    state_ = ComponentState::READING_ENTITIES;
    return;
  }
  
  ESP_LOGD(TAG, "Sending queued %s", pending_operation_.is_user_command ? "user command" : "sensor read");
  sendQueuedOperation();
  state_ = ComponentState::WAITING_FOR_OPERATION_RESPONSE;
}

void EcodanHeatpump::handleWaitingForOperationResponse() {
  uint32_t now = millis();
  
  // Timeout after 300ms for all responses
  if (now - last_command_time_ > 300) {
    ESP_LOGD(TAG, "Operation timeout reached, resuming normal operation");
    operation_in_progress_ = false;
    
    // For sensor reads, advance to next entity
    if (waiting_for_response_) {
      waiting_for_response_ = false;
      current_entity_index_++;
      last_read_time_ = now;
    }
    
    state_ = ComponentState::READING_ENTITIES;
    return;
  }
  
  // If we received the expected response, resume normal operation
  if (!operation_in_progress_) {
    ESP_LOGD(TAG, "Operation response received, resuming normal operation");
    
    // For sensor reads, the response handling is done in parsePacket
    // Just make sure we transition back to reading
    state_ = ComponentState::READING_ENTITIES;
  }
}

bool EcodanHeatpump::isTimeToReadNext() {
  uint32_t now = millis();
  bool ready = (now - last_read_time_) > READOUT_DELAY;
  ESP_LOGV(TAG, "isTimeToReadNext: now=%d, last=%d, delay=%d, ready=%s", 
           now, last_read_time_, READOUT_DELAY, ready ? "true" : "false");
  return ready;
}

void EcodanHeatpump::queueCommand(uint8_t *commandBuffer) {
  if (!isInitialized) {
    ESP_LOGW(TAG, "Cannot queue command - system not initialized yet");
    return;
  }
  
  if (pending_operation_.is_pending) {
    ESP_LOGW(TAG, "Operation already queued, replacing with new user command");
  }
  
  memcpy(pending_operation_.buffer, commandBuffer, PACKET_BUFFER_SIZE);
  pending_operation_.is_pending = true;
  pending_operation_.is_user_command = true;
  pending_operation_.queued_time = millis();
  
  ESP_LOGD(TAG, "User command queued successfully");
}

void EcodanHeatpump::queueSensorRead(uint8_t *readBuffer, uint8_t address) {
  if (pending_operation_.is_pending && pending_operation_.is_user_command) {
    ESP_LOGD(TAG, "User command already queued, sensor read will wait");
    return; // Give priority to user commands
  }
  
  memcpy(pending_operation_.buffer, readBuffer, PACKET_BUFFER_SIZE);
  pending_operation_.is_pending = true;
  pending_operation_.is_user_command = false;
  pending_operation_.expected_response_address = address;
  pending_operation_.queued_time = millis();
  
  ESP_LOGV(TAG, "Sensor read queued for address 0x%02x", address);
}

void EcodanHeatpump::sendQueuedOperation() {
  int i;
  uint8_t packetLength;
  uint8_t checkSum;
  
  if (!pending_operation_.is_pending) {
    ESP_LOGE(TAG, "Attempted to send operation but none queued");
    return;
  }

  checkSum = calculateCheckSum(pending_operation_.buffer);
  packetLength = pending_operation_.buffer[4] + 5;
  pending_operation_.buffer[packetLength] = checkSum;
  
  if (pending_operation_.is_user_command) {
    ESP_LOGD(TAG, "Sending Set Request (0x41): cmd=0x%02x, length=%d", 
             pending_operation_.buffer[5], packetLength + 1);
    
    // Debug: Print the full packet for user commands
    char packet_str[256] = "";
    for (i = 0; i <= packetLength; i++) {
      char byte_str[8];
      snprintf(byte_str, sizeof(byte_str), "0x%02x ", pending_operation_.buffer[i]);
      strcat(packet_str, byte_str);
    }
    ESP_LOGD(TAG, "Full packet: %s", packet_str);
  } else {
    ESP_LOGV(TAG, "Sending Get Request (0x42): address=0x%02x, length=%d", 
             pending_operation_.buffer[5], packetLength + 1);
  }
  
  for (i = 0; i <= packetLength; i++) {
    write(pending_operation_.buffer[i]);
  }
  flush();
  
  // Mark operation as sent
  pending_operation_.is_pending = false;
  operation_in_progress_ = true;
  last_command_time_ = millis();
}

} // namespace ecodan_
} // namespace esphome