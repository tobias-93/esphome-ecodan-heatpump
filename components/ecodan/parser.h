#ifndef ECODAN_HEATPUMP_INCLUDE_PARSER_H
#define ECODAN_HEATPUMP_INCLUDE_PARSER_H

#include <string>
#include <cstdint>
#include "fields.h"
#include "commands.h"

namespace ecodan {
namespace parser {

float parsePacketNumberItem(uint8_t *packet, varTypeEnum varType, uint8_t index);

std::string parsePacketTextItem(uint8_t *packet, varTypeEnum varType, uint8_t index);

bool parsePacketBoolItem(uint8_t *packet, varTypeEnum varType, uint8_t index);

uint8_t parseModeStringToInt(commandVarTypeEnum commandVarType, const std::string &value);

} // namespace parser
} // namespace ecodan

#endif // ECODAN_HEATPUMP_INCLUDE_PARSER_H