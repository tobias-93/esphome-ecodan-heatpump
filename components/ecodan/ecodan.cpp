#include "ecodan.h"

constexpr uint8_t ecodan::commands::command_power_state::packetMask[PACKET_BUFFER_SIZE];
constexpr uint8_t ecodan::commands::command_force_dhw::packetMask[PACKET_BUFFER_SIZE];
constexpr uint8_t ecodan::commands::command_holiday_mode::packetMask[PACKET_BUFFER_SIZE];
constexpr uint8_t ecodan::commands::command_mode_select_zone1::packetMask[PACKET_BUFFER_SIZE];
constexpr uint8_t ecodan::commands::command_mode_select_zone2::packetMask[PACKET_BUFFER_SIZE];
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
  // Validate temperature range for room temperature setpoints (not legionella or hot water)
  if (this->key_.find("temp_setpoint") != std::string::npos && 
      this->key_.find("legionella") == std::string::npos &&
      this->key_.find("hot_water") == std::string::npos) {
    if (value < MIN_TEMPERATURE || value > MAX_TEMPERATURE) {
      ESP_LOGW(TAG, "Temperature %.1f°C out of range [%.1f-%.1f°C], ignoring", 
               value, MIN_TEMPERATURE, MAX_TEMPERATURE);
      return;
    }
  }
  
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
    
    ESP_LOGV(TAG, "Zone 2 setpoint: %.1f°C (preserving Zone 1: %.1f°C)", value, zone1_temp);
  } else {
    #define ECODAN_WRITE_NUMBER(nb) \
      if (this->key_ == #nb) { \
        send = true; \
        memcpy(sendBuffer, command_##nb::packetMask, PACKET_BUFFER_SIZE); \
        sendBuffer[command_##nb::varIndex] = temp1; \
        sendBuffer[command_##nb::varIndex + 1] = temp2; \
        ESP_LOGV(TAG, "Setting %s to %.1f°C", #nb, value); \
      }
      ECODAN_NUMBER_LIST(ECODAN_WRITE_NUMBER, )
  }
  
  if (send == false) {
    return;
  }
  this->heatpump_->sendSerialPacket(sendBuffer);
  this->publish_state(value);
}

void EcodanClimate::dump_config() {
  LOG_CLIMATE("", "Ecodan Climate", this);
  ESP_LOGCONFIG(TAG, "  Climate controls Zone %d", this->zone_);
}

void EcodanClimate::setup() {
  this->target_temperature = NAN;
  this->mode = climate::CLIMATE_MODE_HEAT;
  this->action = climate::CLIMATE_ACTION_HEATING;
}

climate::ClimateTraits EcodanClimate::traits() {
  auto traits = climate::ClimateTraits();
  traits.set_supports_current_temperature(true);
  traits.set_supports_two_point_target_temperature(false);
  traits.set_visual_min_temperature(this->min_temperature_);
  traits.set_visual_max_temperature(this->max_temperature_);
  traits.set_visual_target_temperature_step(this->temperature_step_);
  traits.set_visual_current_temperature_step(0.1f);
  
  traits.set_supported_modes({
    climate::CLIMATE_MODE_HEAT
  });
  
  traits.set_supports_action(true);
  
  return traits;
}

void EcodanClimate::control(const climate::ClimateCall &call) {
  if (this->heatpump_ == nullptr) {
    ESP_LOGE(TAG, "Climate Zone %d: Heatpump not set", this->zone_);
    return;
  }

  bool state_changed = false;

  if (call.get_target_temperature().has_value()) {
    float target_temp = *call.get_target_temperature();
    
    if (target_temp < this->min_temperature_ || target_temp > this->max_temperature_) {
      ESP_LOGW(TAG, "Climate Zone %d: Temperature %.1f°C out of range [%.1f-%.1f°C]", 
               this->zone_, target_temp, this->min_temperature_, this->max_temperature_);
      return;
    }
    
    target_temp = round(target_temp / this->temperature_step_) * this->temperature_step_;
    
    if (std::abs(this->target_temperature - target_temp) > 0.01f) {
      uint8_t sendBuffer[PACKET_BUFFER_SIZE];
      uint16_t temperature = target_temp * 100;
      uint8_t temp1 = (uint8_t) (temperature >> 8);
      uint8_t temp2 = (uint8_t) (temperature & 0x00ff);
      
      if (this->zone_ == 1) {
        memcpy(sendBuffer, command_zone1_room_temp_setpoint::packetMask, PACKET_BUFFER_SIZE);
        sendBuffer[command_zone1_room_temp_setpoint::varIndex] = temp1;
        sendBuffer[command_zone1_room_temp_setpoint::varIndex + 1] = temp2;
      } else if (this->zone_ == 2) {
        memcpy(sendBuffer, command_zone2_room_temp_setpoint::packetMask, PACKET_BUFFER_SIZE);
        
        float zone1_temp = this->heatpump_->get_zone1_room_temp_setpoint();
        uint16_t zone1_temperature = zone1_temp * 100;
        uint8_t zone1_temp1 = (uint8_t) (zone1_temperature >> 8);
        uint8_t zone1_temp2 = (uint8_t) (zone1_temperature & 0x00ff);
        
        sendBuffer[15] = zone1_temp1;
        sendBuffer[16] = zone1_temp2;
        sendBuffer[command_zone2_room_temp_setpoint::varIndex] = temp1;
        sendBuffer[command_zone2_room_temp_setpoint::varIndex + 1] = temp2;
      } else {
        ESP_LOGE(TAG, "Climate: Invalid zone %d", this->zone_);
        return;
      }
      
      this->heatpump_->sendSerialPacket(sendBuffer);
    }
    
    this->target_temperature = target_temp;
    state_changed = true;
  }

  if (state_changed) {
    // Ensure mode is always set correctly for heat pump
    // (Action will be set by zone activity status)
    this->mode = climate::CLIMATE_MODE_HEAT;
    
    this->publish_state();
  }
}

void EcodanClimate::update_current_temperature(float temperature) {
  if (this->current_temperature != temperature) {
    this->current_temperature = temperature;
    
    // Ensure mode is always set correctly for heat pump
    // (Action will be set by zone activity status)
    this->mode = climate::CLIMATE_MODE_HEAT;
    
    this->publish_state();
  }
}

void EcodanClimate::update_target_temperature(float temperature) {
  bool first_update = std::isnan(this->target_temperature);
  
  if (!std::isnan(temperature) && (first_update || std::abs(this->target_temperature - temperature) > 0.01f)) {
    this->target_temperature = temperature;
    
    // Ensure mode is always set correctly for heat pump
    // (Action will be set by zone activity status)
    this->mode = climate::CLIMATE_MODE_HEAT;
    
    this->publish_state();
  }
}

EcodanHeatpump::EcodanHeatpump (uart::UARTComponent *parent) : uart::UARTDevice(parent) {}

void EcodanHeatpump::setup() {
  // This will be called by App.setup()
  buildEntityList();
  state_ = ComponentState::INITIALIZING;
  last_read_time_ = millis(); // Initialize timing
  
  // Initialize operation queue
  pending_operation_.is_pending = false;
  operation_in_progress_ = false;
  
  // Initialize climate entities with correct mode and action
  if (climate_zone1_ != nullptr) {
    climate_zone1_->mode = climate::CLIMATE_MODE_HEAT;
    climate_zone1_->action = climate::CLIMATE_ACTION_HEATING;
  }
  if (climate_zone2_ != nullptr) {
    climate_zone2_->mode = climate::CLIMATE_MODE_HEAT;
    climate_zone2_->action = climate::CLIMATE_ACTION_HEATING;
  }
}

void EcodanHeatpump::update() {
  // Called at configured update_interval (default: 30s)
  // Start a new reading cycle if we're idle
  if (state_ == ComponentState::IDLE && isInitialized) {
    current_entity_index_ = 0;
    state_ = ComponentState::READING_ENTITIES;
    ESP_LOGI(TAG, "Starting new sensor reading cycle");
  }
}

void EcodanHeatpump::loop() {
  // Called frequently by ESPHome main loop
  // Handle state machine and serial communication
  processStateMachine();
}

void EcodanHeatpump::initialize() {
  int i;
  for (i = 0; i < CONNECT_LEN; i++) {
    write(CONNECT[i]);
  }
  flush();
  ESP_LOGV(TAG, "Sent connect command");
}

void EcodanHeatpump::receiveSerialPacket() {
  uint8_t receiveBuffer[PACKET_BUFFER_SIZE];

  if (readPacket(receiveBuffer) == RCVD_PKT_CONNECT_SUCCESS) {
    parsePacket(receiveBuffer);
  }
}

int EcodanHeatpump::readPacket(uint8_t *data) {
  // Non-blocking read - only process available bytes
  while (available() > 0) {
    switch (read_state_) {
      case 0: // Looking for start byte
        data[0] = read();
        if (data[0] == CONNECT[0]) {
          read_state_ = 1;
          bytes_read_ = 1;
          ESP_LOGV(TAG, "Found start byte");
        }
        break;
        
      case 1: // Reading header
        if (bytes_read_ < 5) {
          data[bytes_read_] = read();
          bytes_read_++;
          if (bytes_read_ == 5) {
            expected_length_ = data[4] + 5;
            read_state_ = 2;
            ESP_LOGV(TAG, "Read header, expecting %d total bytes", expected_length_ + 1);
          }
        }
        break;
        
      case 2: // Reading payload and checksum
        if (bytes_read_ <= expected_length_) {
          data[bytes_read_] = read();
          bytes_read_++;
          
          if (bytes_read_ > expected_length_) {
            // We have the complete packet including checksum
            uint8_t checksum = calculateCheckSum(data);
            if (data[expected_length_] == checksum) {
              // Reset state for next packet
              read_state_ = 0;
              bytes_read_ = 0;
              expected_length_ = 0;
              
              ESP_LOGV(TAG, "Received packet: type=0x%02x, address=0x%02x", data[1], data[5]);
              
              if (data[1] == 0x7a) {
                isInitialized = true;
                ESP_LOGI(TAG, "Connected successfully");
              }
              return RCVD_PKT_CONNECT_SUCCESS;
            } else {
              ESP_LOGE(TAG, "CRC ERROR: expected 0x%02x, got 0x%02x", checksum, data[expected_length_]);
              // Reset state on error
              read_state_ = 0;
              bytes_read_ = 0;
              expected_length_ = 0;
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
  ESP_LOGV(TAG, "Queuing user command");
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
  ESP_LOGV(TAG, "Parsing packet: type=0x%02x, payload[0]=0x%02x", packet[1], packet[5]);
  
  // Handle operation responses
  if (operation_in_progress_) {
    if (pending_operation_.is_user_command && packet[1] == 0x61) {
      ESP_LOGV(TAG, "User command completed");
      operation_in_progress_ = false;
      // Clear waiting_for_response if it was set during user command handling
      if (waiting_for_response_) {
        waiting_for_response_ = false;
      }
    } else if (!pending_operation_.is_user_command && packet[1] == 0x62 && 
               packet[5] == pending_operation_.expected_response_address) {
      ESP_LOGV(TAG, "Sensor read completed for address 0x%02x", packet[5]);
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

  // Update climate actions based on zone activity status
  if (field_zone_activity_status::address == packet[5] && 0x62 == packet[1]) {
    uint8_t zone_activity = parsePacketNumberItem(packet, field_zone_activity_status::varType, field_zone_activity_status::varIndex);
    ESP_LOGD(TAG, "Zone activity status: %d", zone_activity);
    
    // Zone activity status mapping (protocol):
    // 0 = No zones active
    // 2 = Zone 1 active
    // 3 = Zone 2 active
    // 1 = Both zones active
    if (this->climate_zone1_ != nullptr) {
      if (zone_activity == 2 || zone_activity == 1) {
        // Zone 1 is active
        this->climate_zone1_->mode = climate::CLIMATE_MODE_HEAT;
        this->climate_zone1_->action = climate::CLIMATE_ACTION_HEATING;
      } else {
        // Zone 1 is idle
        this->climate_zone1_->mode = climate::CLIMATE_MODE_HEAT;
        this->climate_zone1_->action = climate::CLIMATE_ACTION_IDLE;
      }
      this->climate_zone1_->publish_state();
    }
    
    if (this->climate_zone2_ != nullptr) {
      if (zone_activity == 3 || zone_activity == 1) {
        // Zone 2 is active
        this->climate_zone2_->mode = climate::CLIMATE_MODE_HEAT;
        this->climate_zone2_->action = climate::CLIMATE_ACTION_HEATING;
      } else {
        // Zone 2 is idle
        this->climate_zone2_->mode = climate::CLIMATE_MODE_HEAT;
        this->climate_zone2_->action = climate::CLIMATE_ACTION_IDLE;
      }
      this->climate_zone2_->publish_state();
    }
  }

  // Update climate entities with zone temperature readings
  if (field_zone1_room_temperature::address == packet[5] && 0x62 == packet[1] && this->climate_zone1_ != nullptr) {
    auto temperature = parsePacketNumberItem(packet, field_zone1_room_temperature::varType, field_zone1_room_temperature::varIndex);
    ESP_LOGV(TAG, "Updating Climate Zone 1 current temperature to %.1f°C", temperature);
    this->climate_zone1_->update_current_temperature(temperature);
  }
  
  if (field_zone2_room_temperature::address == packet[5] && 0x62 == packet[1] && this->climate_zone2_ != nullptr) {
    auto temperature = parsePacketNumberItem(packet, field_zone2_room_temperature::varType, field_zone2_room_temperature::varIndex);
    ESP_LOGV(TAG, "Updating Climate Zone 2 current temperature to %.1f°C", temperature);
    this->climate_zone2_->update_current_temperature(temperature);
  }

  // Update climate entities with zone setpoint readings from heat pump
  if (field_zone1_room_temp_setpoint::address == packet[5] && 0x62 == packet[1] && this->climate_zone1_ != nullptr) {
    auto setpoint = parsePacketNumberItem(packet, field_zone1_room_temp_setpoint::varType, field_zone1_room_temp_setpoint::varIndex);
    ESP_LOGD(TAG, "Updating Climate Zone 1 target temperature from heat pump: %.1f°C", setpoint);
    this->climate_zone1_->update_target_temperature(setpoint);
  }
  
  if (field_zone2_room_temp_setpoint::address == packet[5] && 0x62 == packet[1] && this->climate_zone2_ != nullptr) {
    auto setpoint = parsePacketNumberItem(packet, field_zone2_room_temp_setpoint::varType, field_zone2_room_temp_setpoint::varIndex);
    ESP_LOGD(TAG, "Updating Climate Zone 2 target temperature from heat pump: %.1f°C", setpoint);
    this->climate_zone2_->update_target_temperature(setpoint);
  }
}

void EcodanHeatpump::setRemoteTemperature(float value, uint8_t zone) {
  if (zone != 1 && zone != 2) {
    ESP_LOGW(TAG, "Invalid zone %d, must be 1 or 2", zone);
    return;
  }
  
  uint8_t sendBuffer[PACKET_BUFFER_SIZE];
  
  // Select the appropriate command based on zone
  if (zone == 1) {
    memcpy(sendBuffer, command_zone1_room_temp::packetMask, PACKET_BUFFER_SIZE);
  } else {
    memcpy(sendBuffer, command_zone2_room_temp::packetMask, PACKET_BUFFER_SIZE);
  }
  
  if (value > 0) {
    // Validate temperature range
    if (value < MIN_TEMPERATURE || value > MAX_TEMPERATURE) {
      ESP_LOGW(TAG, "Remote temperature %.1f°C out of range [%.1f-%.1f°C], ignoring", 
               value, MIN_TEMPERATURE, MAX_TEMPERATURE);
      return;
    }
    
    ESP_LOGI(TAG, "Setting Zone %d remote temperature to %.1f°C", zone, value);
    
    if (zone == 1) {
      encodeRemoteTemperature(sendBuffer, value);
    } else {
      encodeRemoteTemperatureZone2(sendBuffer, value);
    }
  } else {
    ESP_LOGI(TAG, "Switching Zone %d back to builtin temperature sensor", zone);
    
    // Special encoding to disable remote temperature control
    if (zone == 1) {
      sendBuffer[command_zone1_room_temp::varIndex - 1] = 0x00;
      sendBuffer[command_zone1_room_temp::varIndex + 1] = 0x80;
    } else {
      // Zone 2 disable encoding (may need to be adjusted based on protocol testing)
      sendBuffer[command_zone2_room_temp::varIndex - 1] = 0x00;
      sendBuffer[command_zone2_room_temp::varIndex + 1] = 0x80;
    }
  }
  
  this->queueCommand(sendBuffer);
}

void EcodanHeatpump::set_climate_zone(EcodanClimate*& zone_member, EcodanClimate* ecodanClimate) {
  zone_member = ecodanClimate;
  if (ecodanClimate != nullptr) {
    ecodanClimate->set_heatpump(this);
  }
}

void EcodanHeatpump::set_climate_zone1(EcodanClimate* ecodanClimate) {
  set_climate_zone(climate_zone1_, ecodanClimate);
}

void EcodanHeatpump::set_climate_zone2(EcodanClimate* ecodanClimate) {
  set_climate_zone(climate_zone2_, ecodanClimate);
}

float EcodanHeatpump::get_zone_room_temp_setpoint(EcodanClimate* climate_zone) {
  // Use climate entity target temperature if available and valid
  if (climate_zone != nullptr && !std::isnan(climate_zone->target_temperature)) {
    return climate_zone->target_temperature;
  }
  // Default fallback temperature if no climate or number entity
  return 20.0f; // Reasonable default room temperature
}

float EcodanHeatpump::get_zone1_room_temp_setpoint() {
  return get_zone_room_temp_setpoint(climate_zone1_);
}

float EcodanHeatpump::get_zone2_room_temp_setpoint() {
  return get_zone_room_temp_setpoint(climate_zone2_);
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
  
  ESP_LOGV(TAG, "Building entity list...");
  
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
  
  // Add setpoint and temperature readings for climate entities
  if (this->climate_zone1_ != nullptr) {
    addEntityIfNotPresent(field_zone1_room_temp_setpoint::address, "climate", "Zone 1 setpoint reading for climate");
  }
  
  if (this->climate_zone2_ != nullptr) {
    addEntityIfNotPresent(field_zone2_room_temp_setpoint::address, "climate", "Zone 2 setpoint reading for climate");
    addEntityIfNotPresent(field_zone2_room_temperature::address, "climate", "Zone 2 temperature reading for climate");
  }
  
  // Always add zone activity status for climate actions (if any climate entities are configured)
  if (this->climate_zone1_ != nullptr || this->climate_zone2_ != nullptr) {
    addEntityIfNotPresent(field_zone_activity_status::address, "climate", "zone activity status reading for climate actions");
  }
  
  ESP_LOGI(TAG, "Built entity list with %d unique addresses", entity_list_.size());
}

void EcodanHeatpump::processStateMachine() {
  uint32_t now = millis();
  
  // Always check for incoming packets
  receiveSerialPacket();
  
  // Allow other tasks to run (watchdog-friendly)
  delayMicroseconds(1);
  
  static uint32_t last_debug_log = 0;
  if (now - last_debug_log > STATE_LOG_INTERVAL_MS) {
    ESP_LOGV(TAG, "State: %d, entities: %d, op_pending: %s", 
             (int)state_, entity_list_.size(),
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
        ESP_LOGI(TAG, "Starting to read %d entities", entity_list_.size());
      }
      break;
      
    case ComponentState::READING_ENTITIES:
      // Check if we have a pending user command that needs priority FIRST
      if (pending_operation_.is_pending && pending_operation_.is_user_command && !operation_in_progress_) {
        ESP_LOGV(TAG, "User command pending, interrupting reading cycle");
        state_ = ComponentState::SENDING_OPERATION;
      } else {
        handleReading();
      }
      break;
      
    case ComponentState::WAITING_FOR_RESPONSE:
      // This state is no longer used
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
    if (now - last_command_time_ > INIT_RETRY_INTERVAL_MS) { // Try every 2 seconds
      if (init_retry_count_ < MAX_INIT_RETRIES) {
        ESP_LOGD(TAG, "Initialization attempt %d/%d", init_retry_count_ + 1, MAX_INIT_RETRIES);
        initialize();
        init_retry_count_++;
        last_command_time_ = now;
      } else {
        ESP_LOGE(TAG, "Failed to initialize after %d attempts, resetting retry counter", MAX_INIT_RETRIES);
        // Reset and try again after a longer delay
        init_retry_count_ = 0;
        last_command_time_ = now - INIT_RETRY_DELAY_MS;
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
    ESP_LOGI(TAG, "Finished reading all entities, component ready");
    last_cycle_complete_time_ = millis();
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
    uint8_t sendBuffer[PACKET_BUFFER_SIZE];
    buildSensorReadPacket(sendBuffer, entity_list_[current_entity_index_].address);
    pending_address_ = entity_list_[current_entity_index_].address;
    
    ESP_LOGV(TAG, "Reading entity %d/%d (address 0x%02x)", 
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
    state_ = ComponentState::READING_ENTITIES;
    return;
  }
  
  ESP_LOGV(TAG, "Sending %s", pending_operation_.is_user_command ? "user command" : "sensor read");
  sendQueuedOperation();
  state_ = ComponentState::WAITING_FOR_OPERATION_RESPONSE;
}

void EcodanHeatpump::handleWaitingForOperationResponse() {
  uint32_t now = millis();
  
  // Timeout after configured duration for all responses
  if (now - last_command_time_ > OPERATION_TIMEOUT_MS) {
    ESP_LOGV(TAG, "Operation timeout, resuming");
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
    ESP_LOGV(TAG, "Operation completed, resuming");
    
    // For sensor reads, the response handling is done in parsePacket
    // Just make sure we transition back to reading
    state_ = ComponentState::READING_ENTITIES;
  }
}

bool EcodanHeatpump::isTimeToReadNext() {
  uint32_t now = millis();
  bool ready = (now - last_read_time_) > READOUT_DELAY;

  return ready;
}

void EcodanHeatpump::queueCommand(uint8_t *commandBuffer) {
  if (!isInitialized) {
    return;
  }
  
  if (pending_operation_.is_pending) {
    ESP_LOGV(TAG, "Replacing pending operation with new user command");
  }
  
  memcpy(pending_operation_.buffer, commandBuffer, PACKET_BUFFER_SIZE);
  pending_operation_.is_pending = true;
  pending_operation_.is_user_command = true;
  pending_operation_.queued_time = millis();
  
  ESP_LOGV(TAG, "User command queued");
}

void EcodanHeatpump::queueSensorRead(uint8_t *readBuffer, uint8_t address) {
  if (pending_operation_.is_pending && pending_operation_.is_user_command) {
    ESP_LOGV(TAG, "User command already queued, sensor read will wait");
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
    return;
  }

  checkSum = calculateCheckSum(pending_operation_.buffer);
  packetLength = pending_operation_.buffer[4] + 5;
  pending_operation_.buffer[packetLength] = checkSum;
  
  if (pending_operation_.is_user_command) {
    ESP_LOGV(TAG, "Sending user command");
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

void EcodanHeatpump::encodeRemoteTemperature(uint8_t *buffer, float temperature) {
  // Round temperature to nearest 0.5°C increment
  temperature = round(temperature * 2.0f) / 2.0f;
  
  // Ecodan remote temperature encoding (discovered through protocol analysis)
  // This encoding allows setting room temperature from external sensor
  uint8_t temp1 = static_cast<uint8_t>(3 + ((temperature - 10.0f) * 2.0f));
  uint8_t temp2 = static_cast<uint8_t>((temperature * 2.0f) + 128.0f);
  
  buffer[command_zone1_room_temp::varIndex] = temp1;
  buffer[command_zone1_room_temp::varIndex + 1] = temp2;
}

void EcodanHeatpump::encodeRemoteTemperatureZone2(uint8_t *buffer, float temperature) {
  // Round temperature to nearest 0.5°C increment
  temperature = round(temperature * 2.0f) / 2.0f;
  
  // Zone 2 remote temperature encoding (based on Zone 1 analysis)
  // This encoding allows setting Zone 2 room temperature from external sensor
  uint8_t temp1 = static_cast<uint8_t>(3 + ((temperature - 10.0f) * 2.0f));
  uint8_t temp2 = static_cast<uint8_t>((temperature * 2.0f) + 128.0f);
  
  buffer[command_zone2_room_temp::varIndex] = temp1;
  buffer[command_zone2_room_temp::varIndex + 1] = temp2;
}

void EcodanHeatpump::addEntityIfNotPresent(uint8_t address, const char* type, const std::string& description) {
  bool already_added = false;
  for (const auto& entity : entity_list_) {
    if (entity.address == address) {
      already_added = true;
      break;
    }
  }
  if (!already_added) {
    EntityInfo info = {address, true, type};
    entity_list_.push_back(info);
    ESP_LOGD(TAG, "Added %s at address 0x%02x", description.c_str(), address);
  }
}

void EcodanHeatpump::buildSensorReadPacket(uint8_t *buffer, uint8_t address) {
  // Standard sensor read packet template
  static const uint8_t READ_PACKET_TEMPLATE[PACKET_BUFFER_SIZE] = {
    0xfc, 0x42, 0x02, 0x7a, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
  };
  
  memcpy(buffer, READ_PACKET_TEMPLATE, PACKET_BUFFER_SIZE);
  buffer[5] = address; // Set the target address
}

} // namespace ecodan_
} // namespace esphome