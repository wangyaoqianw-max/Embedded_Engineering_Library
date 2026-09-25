#include <stdio.h>
#include <string.h>

#include "crc.h"
#include "ymodem_parser.h"

static void build_packet(
    uint8_t control,
    uint8_t blockNumber,
    const uint8_t *data,
    uint16_t dataSize,
    uint8_t *packet,
    uint16_t *packetSize)
{
    uint16_t crc;

    packet[0] = control;
    packet[1] = blockNumber;
    packet[2] = (uint8_t)~blockNumber;
    (void)memcpy(&packet[3], data, dataSize);
    crc = crc16_xmodem_calculate(data, dataSize);
    packet[3U + dataSize] = (uint8_t)(crc >> 8U);
    packet[4U + dataSize] = (uint8_t)crc;
    *packetSize = (uint16_t)(dataSize + 5U);
}

static int feed_packet(
    ymodem_parser_t *parser,
    const uint8_t *packet,
    uint16_t packetSize,
    ymodem_packet_t *decodedPacket,
    uint16_t chunkSize)
{
    uint16_t index;
    ymodem_parser_event_t event;

    for (index = 0U; index < packetSize; index++) {
        if (ymodem_parser_feed_byte(parser, packet[index], &event, decodedPacket) != PLATFORM_ERR_OK) {
            return 1;
        }

        if (index + 1U < packetSize) {
            if (event != YMODEM_PARSER_EVENT_NONE) {
                return 1;
            }
        } else if (event != YMODEM_PARSER_EVENT_PACKET) {
            return 1;
        }

        if ((chunkSize != 0U) && ((index + 1U) % chunkSize == 0U)) {
            /* Deliberately keep feed calls one byte at a time. */
        }
    }

    return 0;
}

static int test_soh_fragmented_packet(void)
{
    uint8_t data[128];
    uint8_t packet[133];
    uint16_t packetSize;
    ymodem_parser_t parser;
    ymodem_packet_t decodedPacket = {0};
    uint16_t index;

    for (index = 0U; index < sizeof(data); index++) {
        data[index] = (uint8_t)index;
    }

    build_packet(YMODEM_SOH, 1U, data, sizeof(data), packet, &packetSize);
    ymodem_parser_init(&parser);
    if ((packetSize != sizeof(packet)) || feed_packet(&parser, packet, packetSize, &decodedPacket, 7U) != 0) {
        return 1;
    }

    return (decodedPacket.blockNumber == 1U) &&
                   (decodedPacket.dataSize == sizeof(data)) &&
                   (memcmp(decodedPacket.data, data, sizeof(data)) == 0) ? 0 : 1;
}

static int test_stx_packet_and_control_values_in_payload(void)
{
    uint8_t data[1024];
    uint8_t packet[1029];
    uint16_t packetSize;
    ymodem_parser_t parser;
    ymodem_packet_t decodedPacket = {0};
    uint16_t index;

    (void)memset(data, 0x55, sizeof(data));
    data[0] = YMODEM_SOH;
    data[1] = YMODEM_STX;
    data[2] = YMODEM_EOT;
    data[3] = YMODEM_CAN;
    build_packet(YMODEM_STX, 2U, data, sizeof(data), packet, &packetSize);
    ymodem_parser_init(&parser);
    if (feed_packet(&parser, packet, packetSize, &decodedPacket, 31U) != 0) {
        return 1;
    }

    for (index = 0U; index < 4U; index++) {
        if (decodedPacket.data[index] != data[index]) {
            return 1;
        }
    }

    return (decodedPacket.blockNumber == 2U) && (decodedPacket.dataSize == sizeof(data)) ? 0 : 1;
}

static int test_complement_and_crc_errors_reset_candidate(void)
{
    uint8_t data[128];
    uint8_t packet[133];
    uint16_t packetSize;
    ymodem_parser_t parser;
    ymodem_packet_t decodedPacket = {0};
    ymodem_parser_event_t event = YMODEM_PARSER_EVENT_NONE;
    uint16_t index;

    (void)memset(data, 0xA5, sizeof(data));
    data[0] = YMODEM_STX;
    build_packet(YMODEM_SOH, 3U, data, sizeof(data), packet, &packetSize);
    packet[2] ^= 1U;
    ymodem_parser_init(&parser);
    for (index = 0U; index < packetSize; index++) {
        if (ymodem_parser_feed_byte(&parser, packet[index], &event, &decodedPacket) != PLATFORM_ERR_OK) {
            return 1;
        }
    }

    if (event != YMODEM_PARSER_EVENT_PACKET_ERROR) {
        return 1;
    }

    build_packet(YMODEM_SOH, 4U, data, sizeof(data), packet, &packetSize);
    packet[packetSize - 1U] ^= 1U;
    for (index = 0U; index < packetSize; index++) {
        if (ymodem_parser_feed_byte(&parser, packet[index], &event, &decodedPacket) != PLATFORM_ERR_OK) {
            return 1;
        }
    }

    if (event != YMODEM_PARSER_EVENT_PACKET_ERROR) {
        return 1;
    }

    if (ymodem_parser_feed_byte(&parser, YMODEM_EOT, &event, &decodedPacket) != PLATFORM_ERR_OK) {
        return 1;
    }

    return event == YMODEM_PARSER_EVENT_EOT ? 0 : 1;
}

static int test_noise_eot_and_can(void)
{
    ymodem_parser_t parser;
    ymodem_packet_t decodedPacket = {0};
    ymodem_parser_event_t event = YMODEM_PARSER_EVENT_NONE;

    ymodem_parser_init(&parser);
    if ((ymodem_parser_feed_byte(&parser, 0x00U, &event, &decodedPacket) != PLATFORM_ERR_OK) ||
        (event != YMODEM_PARSER_EVENT_NONE) ||
        (ymodem_parser_feed_byte(&parser, YMODEM_EOT, &event, &decodedPacket) != PLATFORM_ERR_OK) ||
        (event != YMODEM_PARSER_EVENT_EOT) ||
        (ymodem_parser_feed_byte(&parser, YMODEM_CAN, &event, &decodedPacket) != PLATFORM_ERR_OK) ||
        (event != YMODEM_PARSER_EVENT_CAN)) {
        return 1;
    }

    return 0;
}

int main(void)
{
    if ((test_soh_fragmented_packet() != 0) ||
        (test_stx_packet_and_control_values_in_payload() != 0) ||
        (test_complement_and_crc_errors_reset_candidate() != 0) ||
        (test_noise_eot_and_can() != 0)) {
        (void)printf("S05 YMODEM parser host test failed.\n");
        return 1;
    }

    (void)printf("S05 YMODEM parser host test passed.\n");
    return 0;
}
