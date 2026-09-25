#include <stdio.h>
#include <string.h>

#include "crc.h"
#include "firmware_image_sink.h"

#define TEST_CAPACITY    (256U)

typedef struct
{
    uint8_t header[FIRMWARE_IMAGE_HEADER_SIZE];
    uint8_t payload[TEST_CAPACITY];
    uint32_t eraseCount;
    uint32_t headerWriteCount;
} test_backend_t;

static platform_error_t backend_erase(
    void *context,
    firmware_slot_t slot,
    uint32_t payloadSize)
{
    test_backend_t *backend = (test_backend_t *)context;
    (void)slot;

    if ((backend == NULL) || (payloadSize == 0U) ||
        (payloadSize > TEST_CAPACITY)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    backend->eraseCount++;
    (void)memset(backend->header, 0xFF, sizeof(backend->header));
    (void)memset(backend->payload, 0xFF, sizeof(backend->payload));
    return PLATFORM_ERR_OK;
}

static platform_error_t backend_read_header(
    void *context,
    firmware_slot_t slot,
    uint8_t rawHeader[FIRMWARE_IMAGE_HEADER_SIZE])
{
    test_backend_t *backend = (test_backend_t *)context;
    (void)slot;
    (void)memcpy(rawHeader, backend->header, FIRMWARE_IMAGE_HEADER_SIZE);
    return PLATFORM_ERR_OK;
}

static platform_error_t backend_read_payload(
    void *context,
    firmware_slot_t slot,
    uint32_t offset,
    uint8_t *data,
    uint32_t length)
{
    test_backend_t *backend = (test_backend_t *)context;
    (void)slot;
    (void)memcpy(data, &backend->payload[offset], length);
    return PLATFORM_ERR_OK;
}

static platform_error_t backend_write_payload(
    void *context,
    firmware_slot_t slot,
    uint32_t offset,
    const uint8_t *data,
    uint32_t length)
{
    test_backend_t *backend = (test_backend_t *)context;
    (void)slot;
    (void)memcpy(&backend->payload[offset], data, length);
    return PLATFORM_ERR_OK;
}

static platform_error_t backend_write_header(
    void *context,
    firmware_slot_t slot,
    const uint8_t rawHeader[FIRMWARE_IMAGE_HEADER_SIZE])
{
    test_backend_t *backend = (test_backend_t *)context;
    (void)slot;
    backend->headerWriteCount++;
    (void)memcpy(backend->header, rawHeader, FIRMWARE_IMAGE_HEADER_SIZE);
    return PLATFORM_ERR_OK;
}

static int test_sink_flow(void)
{
    test_backend_t backendContext;
    firmware_storage_backend_t backend = {0};
    firmware_storage_t storage = FIRMWARE_STORAGE_INITIALIZER;
    firmware_image_sink_t sink = FIRMWARE_IMAGE_SINK_INITIALIZER;
    ymodem_sink_t contract;
    firmware_image_header_t header = {0};
    uint8_t rawHeader[FIRMWARE_IMAGE_HEADER_SIZE];
    uint8_t payload[50];
    uint8_t image[FIRMWARE_IMAGE_HEADER_SIZE + sizeof(payload)];
    uint32_t index;

    (void)memset(&backendContext, 0, sizeof(backendContext));
    for (index = 0U; index < sizeof(payload); index++) {
        payload[index] = (uint8_t)(0x80U + index);
    }

    backend.erase_image = backend_erase;
    backend.read_header = backend_read_header;
    backend.read_payload = backend_read_payload;
    backend.write_payload = backend_write_payload;
    backend.write_header = backend_write_header;
    backend.context = &backendContext;
    backend.payloadCapacity = TEST_CAPACITY;

    header.version.major = 1U;
    header.imageSize = sizeof(payload);
    header.payloadCrc32 = crc32_iso_hdlc_calculate(payload, sizeof(payload));

    if ((firmware_image_encode_header(&header, rawHeader) != PLATFORM_ERR_OK) ||
        (firmware_storage_init(&storage, &backend) != PLATFORM_ERR_OK) ||
        (firmware_image_sink_init(&sink, &storage) != PLATFORM_ERR_OK) ||
        (firmware_image_sink_set_target_slot(&sink, FIRMWARE_SLOT_B) != PLATFORM_ERR_OK) ||
        (firmware_image_sink_get_contract(&sink, &contract) != PLATFORM_ERR_OK)) {
        return 1;
    }

    (void)memcpy(image, rawHeader, FIRMWARE_IMAGE_HEADER_SIZE);
    (void)memcpy(&image[FIRMWARE_IMAGE_HEADER_SIZE], payload, sizeof(payload));

    if ((contract.begin(contract.context, "firmware.img", sizeof(image)) != PLATFORM_ERR_OK) ||
        (contract.write(contract.context, image, 17U) != PLATFORM_ERR_OK) ||
        (contract.write(contract.context, &image[17], sizeof(image) - 17U) != PLATFORM_ERR_OK) ||
        (backendContext.eraseCount != 1U) ||
        (backendContext.headerWriteCount != 0U) ||
        (contract.end(contract.context) != PLATFORM_ERR_OK) ||
        (backendContext.headerWriteCount != 1U) ||
        (memcmp(backendContext.payload, payload, sizeof(payload)) != 0)) {
        return 1;
    }

    contract.abort(contract.context);
    if ((contract.begin(contract.context, "firmware.img",
                        FIRMWARE_IMAGE_HEADER_SIZE + TEST_CAPACITY + 1U) == PLATFORM_ERR_OK) ||
        (backendContext.headerWriteCount != 1U)) {
        return 1;
    }

    return 0;
}

int main(void)
{
    if (test_sink_flow() != 0) {
        (void)printf("Firmware image sink host test failed.\n");
        return 1;
    }

    (void)printf("Firmware image sink host test passed.\n");
    return 0;
}
