#ifndef ECODAN_HEATPUMP_INCLUDE_FIELDS_H
#define ECODAN_HEATPUMP_INCLUDE_FIELDS_H

#include <cstdint>

namespace ecodan {

using std::uint8_t;

enum varTypeEnum {
  VarType_TIME_DATE,
  VarType_TEMPERATURE,
  VarType_ONE_BYTE_TEMPERATURE,
  VarType_POWER_STATE,
  VarType_OPERATING_MODE,
  VarType_WATERFLOW,
  VarType_HW_MODE,
  VarType_MODE_SETTING,
  VarType_HEAT_COOL,
  VarType_DEFROST,
  VarType_HEXVALUE,
  VarType_DECVALUE,
  VarType_RUNTIME,
  VarType_ONE_BYTE_TEMPERATURE_20,
  VarType_DATE,
  VarType_3BYTEVALUE,
  VarType_2BYTEHEXVALUE,
  VarType_ON_OFF,
  VarType_HEAT_STAGE,
};

namespace fields {

#define DEFINE_FIELD(d_fieldname, d_address, d_varIndex, d_varType) \
  struct field_##d_fieldname { \
    static constexpr uint8_t address = d_address; \
    static constexpr uint8_t varIndex = d_varIndex; \
    static constexpr varTypeEnum varType = d_varType; \
  }

// Numeric fields
DEFINE_FIELD(error_code, 0x03, 7, VarType_2BYTEHEXVALUE);
DEFINE_FIELD(frequency, 0x04, 6, VarType_DECVALUE);
DEFINE_FIELD(output_power, 0x07, 11, VarType_DECVALUE);
DEFINE_FIELD(zone1_room_temp_setpoint, 0x09, 6, VarType_TEMPERATURE);
DEFINE_FIELD(zone2_room_temp_setpoint, 0x09, 8, VarType_TEMPERATURE);
DEFINE_FIELD(zone1_flow_temp_setpoint, 0x09, 10, VarType_TEMPERATURE);
DEFINE_FIELD(zone2_flow_temp_setpoint, 0x09, 12, VarType_TEMPERATURE);
DEFINE_FIELD(legionella_temp_setpoint, 0x09, 14, VarType_TEMPERATURE);
DEFINE_FIELD(hot_water_max_temp_drop, 0x09, 16, VarType_DECVALUE);
DEFINE_FIELD(hot_water_temp_range_max, 0x09, 17, VarType_ONE_BYTE_TEMPERATURE_20);
DEFINE_FIELD(hot_water_temp_range_min, 0x09, 18, VarType_ONE_BYTE_TEMPERATURE_20);
DEFINE_FIELD(zone1_room_temperature, 0x0b, 6, VarType_TEMPERATURE);
DEFINE_FIELD(gas_return_temperature, 0x0b, 13, VarType_TEMPERATURE);
DEFINE_FIELD(gas_return_temp_signed, 0x0b, 15, VarType_ONE_BYTE_TEMPERATURE);
DEFINE_FIELD(outside_temperature, 0x0b, 16, VarType_ONE_BYTE_TEMPERATURE);
DEFINE_FIELD(water_feed_temperature, 0x0c, 6, VarType_TEMPERATURE);
DEFINE_FIELD(water_feed_temp_signed, 0x0c, 8, VarType_ONE_BYTE_TEMPERATURE);
DEFINE_FIELD(water_return_temperature, 0x0c, 9, VarType_TEMPERATURE);
DEFINE_FIELD(water_return_temp_signed, 0x0c, 11, VarType_ONE_BYTE_TEMPERATURE);
DEFINE_FIELD(hot_water_temperature, 0x0c, 12, VarType_TEMPERATURE);
DEFINE_FIELD(runtime, 0x13, 8, VarType_RUNTIME);
DEFINE_FIELD(water_flow, 0x14, 17, VarType_DECVALUE);
DEFINE_FIELD(hot_water_setpoint, 0x26, 13, VarType_TEMPERATURE);
DEFINE_FIELD(zone1_flow_temp_setpoint2, 0x26, 15, VarType_TEMPERATURE);
DEFINE_FIELD(zone2_flow_temp_setpoint2, 0x26, 17, VarType_TEMPERATURE);
DEFINE_FIELD(zone1_room_temp_setpoint2, 0x29, 9, VarType_TEMPERATURE);
DEFINE_FIELD(zone2_room_temp_setpoint2, 0x29, 11, VarType_TEMPERATURE);
DEFINE_FIELD(energy_cons_yesterday, 0xA1, 9, VarType_3BYTEVALUE);
DEFINE_FIELD(energy_prod_yesterday, 0xA2, 9, VarType_3BYTEVALUE);
DEFINE_FIELD(energy_cooling_cons_yesterday, 0xA1, 12, VarType_3BYTEVALUE);
DEFINE_FIELD(energy_cooling_prod_yesterday, 0xA2, 12, VarType_3BYTEVALUE);
DEFINE_FIELD(energy_dhw_cons_yesterday, 0xA1, 15, VarType_3BYTEVALUE);
DEFINE_FIELD(energy_dhw_prod_yesterday, 0xA2, 15, VarType_3BYTEVALUE);

// Text fields
DEFINE_FIELD(date_time, 0x01, 6, VarType_TIME_DATE);
DEFINE_FIELD(defrost, 0x02, 8, VarType_DEFROST);
DEFINE_FIELD(heating_stage, 0x07, 9, VarType_HEAT_STAGE);
DEFINE_FIELD(operating_mode, 0x26, 9, VarType_OPERATING_MODE);
DEFINE_FIELD(hot_water_mode, 0x26, 10, VarType_HW_MODE);
DEFINE_FIELD(mode_select, 0x26, 11, VarType_MODE_SETTING);
DEFINE_FIELD(heat_cool, 0x26, 12, VarType_HEAT_COOL);
DEFINE_FIELD(hot_water_timer, 0x28, 9, VarType_ON_OFF);
DEFINE_FIELD(date_energy_cons, 0xA1, 6, VarType_DATE);
DEFINE_FIELD(date_energy_prod, 0xA2, 6, VarType_DATE);
DEFINE_FIELD(holiday_mode, 0x28, 9, VarType_ON_OFF);

// Boolean fields
DEFINE_FIELD(power_state, 0x26, 8, VarType_ON_OFF);
DEFINE_FIELD(force_dhw, 0x05, 12, VarType_ON_OFF);
} // namespace fields
} // namespace ecodan

#endif // ECODAN_HEATPUMP_INCLUDE_FIELDS_H
