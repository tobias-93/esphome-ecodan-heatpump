#include "parser.h"

using std::string;

namespace ecodan {
namespace parser {

// Number functions
static float parseTemperature(uint8_t *packet, uint8_t index) {
  return ((float) packet[index] * 256 + (float) packet[index + 1]) / 100;
}

static float parseOneByteTemperature(uint8_t *packet, uint8_t index) {
  return (float) packet[index] / 2 - 39;
}

static float parseHexValue(uint8_t *packet, uint8_t index) {
  return (float) packet[index];
}

static float parseDecValue(uint8_t *packet, uint8_t index) {
  return (float) packet[index];
}

static float parseRuntime(uint8_t *packet, uint8_t index) {
  return (float) (packet[index + 1] * 256 + packet[index + 2]) * 100 + packet[index];
}

static float parseOneByteTemperature20(uint8_t *packet, uint8_t index) {
  return (float) packet[index] / 2 - 20;
}

static float parse3ByteValue(uint8_t *packet, uint8_t index) {
  return (float) packet[index] * 256 + (float) packet[index + 1] + (float) packet[index + 2] / 100;
}

static float parse2ByteHexValue(uint8_t *packet, uint8_t index) {
  return (float) (packet[index] * 256 + packet[index + 1]);
}

float parsePacketNumberItem(uint8_t *packet, varTypeEnum varType, uint8_t index) {
  switch (varType) {
  case VarType_TEMPERATURE:
    return parseTemperature(packet, index);
    break;
  case VarType_ONE_BYTE_TEMPERATURE:
    return parseOneByteTemperature(packet, index);
    break;
  case VarType_HEXVALUE:
    return parseHexValue(packet, index);
    break;
  case VarType_DECVALUE:
    return parseDecValue(packet, index);
    break;
  case VarType_RUNTIME:
    return parseRuntime(packet, index);
    break;
  case VarType_ONE_BYTE_TEMPERATURE_20:
    return parseOneByteTemperature20(packet, index);
    break;
  case VarType_3BYTEVALUE:
    return parse3ByteValue(packet, index);
    break;
  case VarType_2BYTEHEXVALUE:
    return parse2ByteHexValue(packet, index);
    break;
  default:
    return -1;
    break;
  }
}

// Text functions
static string unknownValue(uint8_t value) {
  char textStr[50];
  sprintf(textStr, "Unknown value: %02X", value);
  return textStr;
}

static string parseTimeDate(uint8_t *packet, uint8_t index) {
  char textStr[50];
  sprintf(textStr, "20%d/%02d/%02d %02d:%02d:%02d", packet[index],
        packet[index + 1], packet[index + 2], packet[index + 3], packet[index + 4], packet[index + 5]);
  return textStr;
}

static string parseOperatingMode(uint8_t *packet, uint8_t index) {
  switch (packet[index]) {
  case 0:
    return "Stop";
  case 1:
    return "Hot Water";
  case 2:
    return "Heating";
  case 3:
    return "Cooling";
  case 4:
    return "No voltage contact input (HW)";
  case 5:
    return "Freeze Stat";
  case 6:
    return "Legionella";
  case 7:
    return "Heating Eco";
  case 8:
    return "Mode 1";
  default:
    return unknownValue(packet[index]);
  }
}

static string parseHotWaterMode(uint8_t *packet, uint8_t index) {
  switch (packet[index]) {
  case 0:
    return "Normal";
  case 1:
    return "Economy";
  default:
    return unknownValue(packet[index]);
  }
}

static string parseModeSetting(uint8_t *packet, uint8_t index) {
  switch (packet[index]) {
  case 0:
    return "Heating Room Temp";
  case 1:
    return "Heating Flow Temp";
  case 2:
    return "Heating Heat Curve";
  case 3:
    return "Cooling Room Temp";
  case 4:
    return "Cooling Flow Temp";
  default:
    return unknownValue(packet[index]);
  }
}

static string parseDeFrost(uint8_t *packet, uint8_t index) {
  switch (packet[index]) {
  case 0:
    return "Normal";
  case 1:
    return "Standby";
  case 2:
    return "Defrost";
  case 3:
    return "Waiting Restart";
  default:
    return unknownValue(packet[index]);
  }
}

static string parseHeatCool(uint8_t *packet, uint8_t index) {
  switch (packet[index]) {
  case 1:
  case 2:
    return "Heating Mode";
  case 4:
    return "Cooling Mode";
  default:
    return unknownValue(packet[index]);
  }
}

static string parseDate(uint8_t *packet, uint8_t index) {
  char textStr[50];
  sprintf(textStr, "20%d/%02d/%02d", packet[index],
          packet[index + 1], packet[index + 2]);
  return textStr;
}

static string parseOnOffText(uint8_t *packet, uint8_t index) {
  switch (packet[index]) {
  case 0:
    return "Off";
  case 1:
    return "On";
  default:
    return unknownValue(packet[index]);
  }
}

static string parseHeatStage(uint8_t *packet, uint8_t index) {
  switch (packet[index]) {
  case 0:
    return "Off";
  case 1:
    return "Normal";
  case 2:
    return "Boost";
  default:
    return unknownValue(packet[index]);
  }
}

string parsePacketTextItem(uint8_t *packet, varTypeEnum varType, uint8_t index) {
  switch (varType) {
  case VarType_TIME_DATE:
    return parseTimeDate(packet, index);
  case VarType_OPERATING_MODE:
    return parseOperatingMode(packet, index);
  case VarType_HW_MODE:
    return parseHotWaterMode(packet, index);
  case VarType_MODE_SETTING:
    return parseModeSetting(packet, index);
  case VarType_DEFROST:
    return parseDeFrost(packet, index);
  case VarType_HEAT_COOL:
    return parseHeatCool(packet, index);
  case VarType_DATE:
    return parseDate(packet, index);
  case VarType_ON_OFF:
    return parseOnOffText(packet, index);
  case VarType_HEAT_STAGE:
    return parseHeatStage(packet, index);
  default:
    return "";
  }
}

// Boolean functions
static bool parseOnOff(uint8_t *packet, uint8_t index) {
  return packet[index] == 1;
}

bool parsePacketBoolItem(uint8_t *packet, varTypeEnum varType, uint8_t index) {
  switch (varType) {
  case VarType_ON_OFF:
    return parseOnOff(packet, index);
  default:
    return false;
  }
}

uint8_t parseModeStringToInt(commandVarTypeEnum commandVarType, const std::string &value) {
  switch (commandVarType) {
  case CommandVarType_AC_MODE_SETTING:
    if (value == "Heating Room Temp") {
      return 0;
    }
    if (value == "Heating Flow Temp") {
      return 1;
    }
    if (value == "Heating Heat Curve") {
      return 2;
    }
    if (value == "Cooling Room Temp") {
      return 3;
    }
    if (value == "Cooling Flow Temp") {
      return 4;
    }
    return 0;
  case CommandVarType_HOT_WATER_MODE:
    if (value == "Normal") {
      return 0;
    }
    if (value == "Economy") {
      return 1;
    }
    return 0;
  default:
    return 0;
  }
}

} // namespace parser
} // namespace ecodan