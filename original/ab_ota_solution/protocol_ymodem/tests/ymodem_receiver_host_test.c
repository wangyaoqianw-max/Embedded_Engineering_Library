#include <stdio.h>
#include <string.h>

#include "crc.h"
#include "ymodem_config.h"
#include "ymodem_receiver.h"

typedef struct
{
    uint8_t txData[64];
    platform_size_t txLength;
    platform_bool_t failWrite;
} test_transport_context_t;

typedef struct
{
    platform_error_t beginResult;
    platform_error_t writeResult;
    platform_error_t endResult;
    uint32_t beginCount;
    uint32_t writeCount;
    uint32_t endCount;
    uint32_t abortCount;
    uint32_t fileSize;
    uint32_t dataLength;
    char filename[YMODEM_CFG_FILENAME_MAX_LEN + 1U];
    uint8_t data[256];
} test_sink_context_t;

static platform_error_t test_transport_write(
    void *context,
    const uint8_t *data,
    uint32_t dataLength,
    uint32_t timeoutMs)
{
    test_transport_context_t *service = (test_transport_context_t *)context;

    (void)timeoutMs;

    if ((service == NULL) || (data == NULL) || (dataLength > sizeof(service->txData))) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (service->failWrite != 0U) {
        return PLATFORM_ERR_IO;
    }

    (void)memcpy(&service->txData[service->txLength], data, dataLength);
    service->txLength += dataLength;
    return PLATFORM_ERR_OK;
}

static platform_error_t test_sink_begin(void *context, const char *filename, uint32_t fileSize)
{
    test_sink_context_t *sink = (test_sink_context_t *)context;

    sink->beginCount++;
    sink->fileSize = fileSize;
    (void)strncpy(sink->filename, filename, sizeof(sink->filename) - 1U);
    sink->filename[sizeof(sink->filename) - 1U] = '\0';
    return sink->beginResult;
}

static platform_error_t test_sink_write(void *context, const uint8_t *data, uint32_t length)
{
    test_sink_context_t *sink = (test_sink_context_t *)context;

    sink->writeCount++;
    if ((data == NULL) || (length > (sizeof(sink->data) - sink->dataLength))) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (sink->writeResult != PLATFORM_ERR_OK) {
        return sink->writeResult;
    }

    (void)memcpy(&sink->data[sink->dataLength], data, length);
    sink->dataLength += length;
    return PLATFORM_ERR_OK;
}

static platform_error_t test_sink_end(void *context)
{
    test_sink_context_t *sink = (test_sink_context_t *)context;

    sink->endCount++;
    return sink->endResult;
}

static void test_sink_abort(void *context)
{
    test_sink_context_t *sink = (test_sink_context_t *)context;

    sink->abortCount++;
}

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

static void build_block0(uint8_t *data, const char *filename, uint32_t fileSize)
{
    size_t filenameLength = strlen(filename);

    (void)memset(data, 0, YMODEM_PACKET_DATA_SIZE_128);
    (void)memcpy(data, filename, filenameLength);
    (void)snprintf((char *)&data[filenameLength + 1U],
                   YMODEM_PACKET_DATA_SIZE_128 - filenameLength - 1U,
                   " %lu 15251747316 100644",
                   (unsigned long)fileSize);
}

static void build_block0_with_fields(
    uint8_t *data,
    const char *filename,
    const char *fields)
{
    size_t filenameLength = strlen(filename);

    (void)memset(data, 0, YMODEM_PACKET_DATA_SIZE_128);
    (void)memcpy(data, filename, filenameLength);
    (void)snprintf((char *)&data[filenameLength + 1U],
                   YMODEM_PACKET_DATA_SIZE_128 - filenameLength - 1U,
                   "%s",
                   fields);
}

static int feed_bytes(ymodem_receiver_t *receiver, const uint8_t *data, uint16_t length, uint32_t *nowMs)
{
    uint16_t index;

    for (index = 0U; index < length; index++) {
        if (ymodem_receiver_feed_byte(receiver, data[index], *nowMs) != PLATFORM_ERR_OK) {
            return 1;
        }
        (*nowMs)++;
    }

    return 0;
}

static int send_packet(
    ymodem_receiver_t *receiver,
    uint8_t blockNumber,
    const uint8_t *data,
    uint16_t dataSize,
    uint32_t *nowMs)
{
    uint8_t packet[YMODEM_PACKET_MAX_SIZE];
    uint16_t packetSize;

    build_packet(
        (dataSize == YMODEM_PACKET_DATA_SIZE_128) ? YMODEM_SOH : YMODEM_STX,
        blockNumber,
        data,
        dataSize,
        packet,
        &packetSize);
    return feed_bytes(receiver, packet, packetSize, nowMs);
}

static int init_receiver(
    ymodem_receiver_t *receiver,
    test_transport_context_t *uart,
    test_sink_context_t *sinkContext)
{
    ymodem_receiver_config_t config;
    ymodem_sink_t sink;

    sink.begin = test_sink_begin;
    sink.write = test_sink_write;
    sink.end = test_sink_end;
    sink.abort = test_sink_abort;
    sink.context = sinkContext;
    config.transport.write = test_transport_write;
    config.transport.context = uart;
    config.sink = sink;

    if ((ymodem_receiver_init(receiver, &config) != PLATFORM_ERR_OK) ||
        (ymodem_receiver_start(receiver) != PLATFORM_ERR_OK)) {
        return 1;
    }

    return (uart->txLength == 1U) && (uart->txData[0] == YMODEM_C) ? 0 : 1;
}

static int test_normal_single_file_flow(void)
{
    uint8_t block0[YMODEM_PACKET_DATA_SIZE_128];
    uint8_t data1[YMODEM_PACKET_DATA_SIZE_128];
    uint8_t data2[YMODEM_PACKET_DATA_SIZE_128];
    uint8_t emptyBlock[YMODEM_PACKET_DATA_SIZE_128] = {0U};
    test_transport_context_t uart = {0};
    test_sink_context_t sink = {0};
    ymodem_receiver_t receiver;
    ymodem_receiver_status_t status;
    uint32_t nowMs = 0U;
    uint16_t index;

    build_block0(block0, "firmware.img", 130U);
    for (index = 0U; index < sizeof(data1); index++) {
        data1[index] = (uint8_t)index;
        data2[index] = 0x1AU;
    }
    data2[0] = 0xA5U;
    data2[1] = 0x5AU;

    if (init_receiver(&receiver, &uart, &sink) != 0) {
        return 1;
    }

    if ((send_packet(&receiver, 0U, block0, sizeof(block0), &nowMs) != 0) ||
        (receiver.context.state != YMODEM_RECEIVER_STATE_RECEIVE_DATA) ||
        (sink.beginCount != 1U) ||
        (sink.fileSize != 130U) ||
        (strcmp(sink.filename, "firmware.img") != 0) ||
        (receiver.context.block0Metadata.fileSize != 130U) ||
        (strcmp(receiver.context.block0Metadata.filename, "firmware.img") != 0) ||
        (receiver.context.block0Metadata.modificationTime != 1789382350U) ||
        (receiver.context.block0Metadata.fileMode != 33188U) ||
        (receiver.context.block0Metadata.serialNumber != 0U)) {
        return 1;
    }

    if ((ymodem_receiver_get_status(&receiver, &status) != PLATFORM_ERR_OK) ||
        (status.block0Metadata.fileSize != 130U) ||
        (strcmp(status.block0Metadata.filename, "firmware.img") != 0) ||
        (status.block0Metadata.modificationTime != 1789382350U) ||
        (status.block0Metadata.fileMode != 33188U) ||
        (status.block0Metadata.serialNumber != 0U)) {
        return 1;
    }

    if ((send_packet(&receiver, 1U, data1, sizeof(data1), &nowMs) != 0) ||
        (send_packet(&receiver, 2U, data2, sizeof(data2), &nowMs) != 0) ||
        (sink.writeCount != 2U) ||
        (sink.dataLength != 130U) ||
        (memcmp(sink.data, data1, sizeof(data1)) != 0) ||
        (sink.data[128] != 0xA5U) ||
        (sink.data[129] != 0x5AU)) {
        return 1;
    }

    if ((ymodem_receiver_feed_byte(&receiver, YMODEM_EOT, nowMs++) != PLATFORM_ERR_OK) ||
        (receiver.context.state != YMODEM_RECEIVER_STATE_WAIT_EOT_CONFIRM) ||
        (uart.txData[uart.txLength - 1U] != YMODEM_NAK) ||
        (ymodem_receiver_feed_byte(&receiver, YMODEM_EOT, nowMs++) != PLATFORM_ERR_OK) ||
        (receiver.context.state != YMODEM_RECEIVER_STATE_WAIT_END_HEADER) ||
        (uart.txData[uart.txLength - 2U] != YMODEM_ACK) ||
        (uart.txData[uart.txLength - 1U] != YMODEM_C) ||
        (send_packet(&receiver, 0U, emptyBlock, sizeof(emptyBlock), &nowMs) != 0) ||
        (receiver.context.state != YMODEM_RECEIVER_STATE_FINISHED) ||
        (sink.endCount != 1U) ||
        (uart.txData[uart.txLength - 1U] != YMODEM_ACK)) {
        return 1;
    }

    return 0;
}

static int test_duplicate_and_sequence_handling(void)
{
    uint8_t block0[YMODEM_PACKET_DATA_SIZE_128];
    uint8_t data[YMODEM_PACKET_DATA_SIZE_128] = {0U};
    test_transport_context_t uart = {0};
    test_sink_context_t sink = {0};
    ymodem_receiver_t receiver;
    uint32_t nowMs = 0U;

    build_block0(block0, "fw.img", sizeof(data) * 2U);
    if ((init_receiver(&receiver, &uart, &sink) != 0) ||
        (send_packet(&receiver, 0U, block0, sizeof(block0), &nowMs) != 0) ||
        (send_packet(&receiver, 1U, data, sizeof(data), &nowMs) != 0)) {
        return 1;
    }

    if ((send_packet(&receiver, 1U, data, sizeof(data), &nowMs) != 0) ||
        (sink.writeCount != 1U) ||
        (uart.txData[uart.txLength - 1U] != YMODEM_ACK) ||
        (send_packet(&receiver, 3U, data, sizeof(data), &nowMs) != 0) ||
        (uart.txData[uart.txLength - 1U] != YMODEM_NAK) ||
        (receiver.statistics.duplicatePacketCount != 1U) ||
        (receiver.statistics.sequenceErrorCount != 1U) ||
        (receiver.context.retryCount != 1U) ||
        (send_packet(&receiver, 2U, data, sizeof(data), &nowMs) != 0) ||
        (sink.writeCount != 2U) ||
        (receiver.context.retryCount != 0U)) {
        return 1;
    }

    return 0;
}

static int test_block0_validation_and_sink_failure(void)
{
    uint8_t block0[YMODEM_PACKET_DATA_SIZE_128];
    uint8_t data[YMODEM_PACKET_DATA_SIZE_128] = {0U};
    uint8_t packet[YMODEM_PACKET_MAX_SIZE];
    uint16_t packetSize;
    test_transport_context_t uart;
    test_sink_context_t sink;
    ymodem_receiver_t receiver;
    uint32_t nowMs;
    uint16_t index;
    const char *invalidValues[] = {"", "abc", "4294967296", "0"};
    const char *invalidMetadata[] = {
        " 55884",
        " 55884 15251747316",
        " 55884 15251747318 100644",
        " 55884 15251747316 100648",
        " 55884 15251747316 100644 128 7",
        "  55884 15251747316 100644"
    };

    for (index = 0U; index < sizeof(invalidValues) / sizeof(invalidValues[0]); index++) {
        (void)memset(&uart, 0, sizeof(uart));
        (void)memset(&sink, 0, sizeof(sink));
        nowMs = 0U;
        (void)memset(block0, 0, sizeof(block0));
        (void)strcpy((char *)block0, "fw.img");
        (void)strcpy((char *)&block0[7], invalidValues[index]);
        build_packet(YMODEM_SOH, 0U, block0, sizeof(block0), packet, &packetSize);
        if (init_receiver(&receiver, &uart, &sink) != 0) {
            return 1;
        }
        (void)feed_bytes(&receiver, packet, packetSize, &nowMs);
        if ((receiver.context.state != YMODEM_RECEIVER_STATE_ERROR) ||
            (sink.beginCount != 0U)) {
            return 1;
        }
    }

    (void)memset(&uart, 0, sizeof(uart));
    (void)memset(&sink, 0, sizeof(sink));
    nowMs = 0U;
    build_block0(block0, "OTA_APP_s04_v1.1.0.img", 55884U);
    build_packet(YMODEM_SOH, 0U, block0, sizeof(block0), packet, &packetSize);
    if ((init_receiver(&receiver, &uart, &sink) != 0) ||
        (feed_bytes(&receiver, packet, packetSize, &nowMs) != 0) ||
        (receiver.context.state != YMODEM_RECEIVER_STATE_RECEIVE_DATA) ||
        (sink.beginCount != 1U) ||
        (sink.fileSize != 55884U) ||
        (strcmp(sink.filename, "OTA_APP_s04_v1.1.0.img") != 0) ||
        (receiver.context.block0Metadata.fileSize != 55884U) ||
        (strcmp(receiver.context.block0Metadata.filename, "OTA_APP_s04_v1.1.0.img") != 0) ||
        (receiver.context.block0Metadata.modificationTime != 1789382350U) ||
        (receiver.context.block0Metadata.fileMode != 33188U) ||
        (receiver.context.block0Metadata.serialNumber != 0U)) {
        return 1;
    }

    (void)memset(&uart, 0, sizeof(uart));
    (void)memset(&sink, 0, sizeof(sink));
    nowMs = 0U;
    build_block0_with_fields(block0, "fw.img", " 55884 15251747316 100644 123");
    build_packet(YMODEM_SOH, 0U, block0, sizeof(block0), packet, &packetSize);
    if ((init_receiver(&receiver, &uart, &sink) != 0) ||
        (feed_bytes(&receiver, packet, packetSize, &nowMs) != 0) ||
        (receiver.context.state != YMODEM_RECEIVER_STATE_RECEIVE_DATA) ||
        (receiver.context.block0Metadata.modificationTime != 1789382350U) ||
        (receiver.context.block0Metadata.fileMode != 33188U) ||
        (receiver.context.block0Metadata.serialNumber != 83U)) {
        return 1;
    }

    for (index = 0U; index < sizeof(invalidMetadata) / sizeof(invalidMetadata[0]); index++) {
        (void)memset(&uart, 0, sizeof(uart));
        (void)memset(&sink, 0, sizeof(sink));
        nowMs = 0U;
        build_block0_with_fields(block0, "fw.img", invalidMetadata[index]);
        build_packet(YMODEM_SOH, 0U, block0, sizeof(block0), packet, &packetSize);
        if (init_receiver(&receiver, &uart, &sink) != 0) {
            return 1;
        }
        (void)feed_bytes(&receiver, packet, packetSize, &nowMs);
        if ((receiver.context.state != YMODEM_RECEIVER_STATE_ERROR) ||
            (sink.beginCount != 0U)) {
            return 1;
        }
    }

    (void)memset(&uart, 0, sizeof(uart));
    (void)memset(&sink, 0, sizeof(sink));
    nowMs = 0U;
    (void)memset(block0, 'A', sizeof(block0));
    block0[65] = '\0';
    block0[66] = '1';
    build_packet(YMODEM_SOH, 0U, block0, sizeof(block0), packet, &packetSize);
    if (init_receiver(&receiver, &uart, &sink) != 0) {
        return 1;
    }
    (void)feed_bytes(&receiver, packet, packetSize, &nowMs);
    if (receiver.context.state != YMODEM_RECEIVER_STATE_ERROR) {
        return 1;
    }

    (void)memset(&uart, 0, sizeof(uart));
    (void)memset(&sink, 0, sizeof(sink));
    sink.beginResult = PLATFORM_ERR_IO;
    nowMs = 0U;
    build_block0(block0, "fw.img", 1U);
    build_packet(YMODEM_SOH, 0U, block0, sizeof(block0), packet, &packetSize);
    if (init_receiver(&receiver, &uart, &sink) != 0) {
        return 1;
    }
    (void)feed_bytes(&receiver, packet, packetSize, &nowMs);
    if ((receiver.context.state != YMODEM_RECEIVER_STATE_ERROR) ||
        (sink.abortCount != 1U)) {
        return 1;
    }

    (void)memset(&uart, 0, sizeof(uart));
    (void)memset(&sink, 0, sizeof(sink));
    sink.writeResult = PLATFORM_ERR_IO;
    nowMs = 0U;
    build_block0(block0, "fw.img", 1U);
    if ((init_receiver(&receiver, &uart, &sink) != 0) ||
        (send_packet(&receiver, 0U, block0, sizeof(block0), &nowMs) != 0) ||
        (send_packet(&receiver, 1U, data, sizeof(data), &nowMs) == 0) ||
        (receiver.context.state != YMODEM_RECEIVER_STATE_ERROR) ||
        (sink.writeCount != 1U) ||
        (sink.abortCount != 1U)) {
        return 1;
    }

    return 0;
}

static int test_crc_eot_short_and_second_header_failures(void)
{
    uint8_t block0[YMODEM_PACKET_DATA_SIZE_128];
    uint8_t data[YMODEM_PACKET_DATA_SIZE_128] = {0U};
    uint8_t packet[YMODEM_PACKET_MAX_SIZE];
    uint16_t packetSize;
    test_transport_context_t uart = {0};
    test_sink_context_t sink = {0};
    ymodem_receiver_t receiver;
    uint32_t nowMs = 0U;

    build_block0(block0, "fw.img", sizeof(data));
    if ((init_receiver(&receiver, &uart, &sink) != 0) ||
        (send_packet(&receiver, 0U, block0, sizeof(block0), &nowMs) != 0)) {
        return 1;
    }

    build_packet(YMODEM_SOH, 1U, data, sizeof(data), packet, &packetSize);
    packet[packetSize - 1U] ^= 1U;
    (void)feed_bytes(&receiver, packet, packetSize, &nowMs);
    if ((uart.txData[uart.txLength - 1U] != YMODEM_NAK) ||
        (receiver.statistics.crcErrorCount != 1U)) {
        return 1;
    }

    (void)ymodem_receiver_feed_byte(&receiver, YMODEM_EOT, nowMs++);
    if ((receiver.context.state != YMODEM_RECEIVER_STATE_ERROR) ||
        (sink.abortCount != 1U)) {
        return 1;
    }

    (void)memset(&uart, 0, sizeof(uart));
    (void)memset(&sink, 0, sizeof(sink));
    nowMs = 0U;
    if ((init_receiver(&receiver, &uart, &sink) != 0) ||
        (send_packet(&receiver, 0U, block0, sizeof(block0), &nowMs) != 0) ||
        (send_packet(&receiver, 1U, data, sizeof(data), &nowMs) != 0) ||
        (ymodem_receiver_feed_byte(&receiver, YMODEM_EOT, nowMs++) != PLATFORM_ERR_OK) ||
        (ymodem_receiver_feed_byte(&receiver, YMODEM_EOT, nowMs++) != PLATFORM_ERR_OK)) {
        return 1;
    }
    (void)send_packet(&receiver, 0U, block0, sizeof(block0), &nowMs);
    if (receiver.context.state != YMODEM_RECEIVER_STATE_ERROR) {
        return 1;
    }

    return 0;
}

static int test_cancel_and_retry_timeout(void)
{
    uint8_t block0[YMODEM_PACKET_DATA_SIZE_128];
    test_transport_context_t uart = {0};
    test_sink_context_t sink = {0};
    ymodem_receiver_t receiver;
    uint32_t nowMs = 0U;
    uint32_t index;

    build_block0(block0, "fw.img", 1U);
    if ((init_receiver(&receiver, &uart, &sink) != 0) ||
        (send_packet(&receiver, 0U, block0, sizeof(block0), &nowMs) != 0) ||
        (ymodem_receiver_feed_byte(&receiver, YMODEM_CAN, nowMs++) != PLATFORM_ERR_OK) ||
        (receiver.context.state != YMODEM_RECEIVER_STATE_ABORTED) ||
        (sink.abortCount != 1U)) {
        return 1;
    }

    (void)memset(&uart, 0, sizeof(uart));
    (void)memset(&sink, 0, sizeof(sink));
    nowMs = 0U;
    if ((init_receiver(&receiver, &uart, &sink) != 0) ||
        (send_packet(&receiver, 0U, block0, sizeof(block0), &nowMs) != 0) ||
        (ymodem_receiver_cancel(&receiver) != PLATFORM_ERR_OK) ||
        (receiver.context.state != YMODEM_RECEIVER_STATE_ABORTED) ||
        (sink.abortCount != 1U) ||
        (uart.txLength != 5U) ||
        (uart.txData[3] != YMODEM_CAN) ||
        (uart.txData[4] != YMODEM_CAN)) {
        return 1;
    }

    (void)memset(&uart, 0, sizeof(uart));
    (void)memset(&sink, 0, sizeof(sink));
    if (init_receiver(&receiver, &uart, &sink) != 0) {
        return 1;
    }

    for (index = 0U; index < (YMODEM_CFG_MAX_RETRY + 1U); index++) {
        nowMs += YMODEM_CFG_PACKET_TIMEOUT_MS;
        (void)ymodem_receiver_tick(&receiver, nowMs);
    }

    if ((receiver.context.state != YMODEM_RECEIVER_STATE_ERROR) ||
        (receiver.statistics.timeoutCount != YMODEM_CFG_MAX_RETRY + 1U)) {
        return 1;
    }

    return 0;
}

int main(void)
{
    if ((test_normal_single_file_flow() != 0) ||
        (test_duplicate_and_sequence_handling() != 0) ||
        (test_block0_validation_and_sink_failure() != 0) ||
        (test_crc_eot_short_and_second_header_failures() != 0) ||
        (test_cancel_and_retry_timeout() != 0)) {
        (void)printf("S05 YMODEM receiver host test failed.\n");
        return 1;
    }

    (void)printf("S05 YMODEM receiver host test passed.\n");
    return 0;
}
