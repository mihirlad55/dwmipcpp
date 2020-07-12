/**
 * @file packet.cpp
 *
 * This file contains the implementation details for the Packet class.
 */

#include <cstring>

#include "packet.hpp"

namespace dwmipc {

Packet::Packet(const uint32_t payload_size)
    : size(payload_size + HEADER_SIZE) {
    // Use malloc since primitive type, and to allow realloc
    this->data = (uint8_t *)malloc(sizeof(uint8_t) * size);
    this->header = (Header *)data;
    this->payload = (char *)(data + HEADER_SIZE);
    this->header->size = payload_size;
    std::memcpy(this->header->magic, DWM_MAGIC, DWM_MAGIC_LEN);
    this->header->type = 0;
}

Packet::Packet(const MessageType type, const std::string &msg)
    : Packet(msg.size()) {
    this->header->type = type;
    std::strncpy(this->payload, msg.c_str(), msg.size());
}

Packet::~Packet() { free(this->data); }

void Packet::realloc_to_header_size() {
    this->size = this->header->size + HEADER_SIZE;
    this->data = (uint8_t *)realloc(this->data, this->size);
    this->header = (Header *)this->data;
    this->payload = (char *)(this->data + HEADER_SIZE);
}

} // namespace dwmipc
