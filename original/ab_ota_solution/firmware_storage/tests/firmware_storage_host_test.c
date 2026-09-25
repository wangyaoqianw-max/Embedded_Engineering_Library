#include <stdio.h>
#include <string.h>

#include "crc.h"
#include "firmware_storage.h"

#define TEST_CAPACITY    (512U)

typedef struct
{
    uint8_t header[2][FIRMWARE_IMAGE_HEADER_SIZE];
    uint8_t payload[2][TEST_CAPACITY];
} test_backend_t;

static int slot_index(firmware_slot_t slot)
{
    return (slot == FIRMWARE_SLOT_A) ? 0 :
           (slot == FIRMWARE_SLOT_B) ? 1 : -1;
}

static platform_error_t test_erase(void *context, firmware_slot_t slot, uint32_t payloadSize)
{
    test_backend_t *backend = (test_backend_t *)context;
    int index = slot_index(slot);

    if ((backend == NULL) || (index < 0) || (payloadSize == 0U) ||
        (payloadSize > TEST_CAPACITY)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    (void)memset(backend->header[index], 0xFF, FIRMWARE_IMAGE_HEADER_SIZE);
    (void)memset(backend->payload[index], 0xFF, TEST_CAPACITY);
    return PLATFORM_ERR_OK;
}

static platform_error_t test_read_header(
    void *context,
    firmware_slot_t slot,
    uint8_t rawHeader[FIRMWARE_IMAGE_HEADER_SIZE])
{
    test_backend_t *backend = (test_backend_t *)context;
    int index = slot_index(slot);

    if ((backend == NULL) || (rawHeader == NULL) || (index < 0)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    (void)memcpy(rawHeader, backend->header[index], FIRMWARE_IMAGE_HEADER_SIZE);
    return PLATFORM_ERR_OK;
}

static platform_error_t test_read_payload(
    void *context,
    firmware_slot_t slot,
    uint32_t offset,
    uint8_t *data,
    uint32_t length)
{
    test_backend_t *backend = (test_backend_t *)context;
    int index = slot_index(slot);

    if ((backend == NULL) || (data == NULL) || (index < 0) ||
        (offset > TEST_CAPACITY) || (length > (TEST_CAPACITY - offset))) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    (void)memcpy(data, &backend->payload[index][offset], length);
    return PLATFORM_ERR_OK;
}

static platform_error_t test_write_payload(
    void *context,
    firmware_slot_t slot,
    uint32_t offset,
    const uint8_t *data,
    uint32_t length)
{
    test_backend_t *backend = (test_backend_t *)context;
    int index = slot_index(slot);

    if ((backend == NULL) || (data == NULL) || (index < 0) ||
        (offset > TEST_CAPACITY) || (length > (TEST_CAPACITY - offset))) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    (void)memcpy(&backend->payload[index][offset], data, length);
    return PLATFORM_ERR_OK;
}

static platform_error_t test_write_header(
    void *context,
    firmware_slot_t slot,
    const uint8_t rawHeader[FIRMWARE_IMAGE_HEADER_SIZE])
{
    test_backend_t *backend = (test_backend_t *)context;
    int index = slot_index(slot);

    if ((backend == NULL) || (rawHeader == NULL) || (index < 0)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    (void)memcpy(backend->header[index], rawHeader, FIRMWARE_IMAGE_HEADER_SIZE);
    return PLATFORM_ERR_OK;
}

static int test_storage_flow(void)
{
    test_backend_t backendContext;
    firmware_storage_backend_t backend = {0};
    firmware_storage_t storage = FIRMWARE_STORAGE_INITIALIZER;
    firmware_image_header_t header = {0};
    firmware_image_header_t decoded = {0};
    firmware_image_validation_t validation;
    uint8_t rawHeader[FIRMWARE_IMAGE_HEADER_SIZE];
    uint8_t payload[300];
    uint32_t index;

    (void)memset(&backendContext, 0xFF, sizeof(backendContext));
    for (index = 0U; index < sizeof(payload); index++) {
        payload[index] = (uint8_t)index;
    }

    backend.erase_image = test_erase;
    backend.read_header = test_read_header;
    backend.read_payload = test_read_payload;
    backend.write_payload = test_write_payload;
    backend.write_header = test_write_header;
    backend.context = &backendContext;
    backend.payloadCapacity = TEST_CAPACITY;

    if (firmware_storage_init(&storage, &backend) != PLATFORM_ERR_OK) {
        return 1;
    }

    header.version.major = 1U;
    header.imageSize = sizeof(payload);
    header.payloadCrc32 = crc32_iso_hdlc_calculate(payload, sizeof(payload));
    if ((firmware_image_encode_header(&header, rawHeader) != PLATFORM_ERR_OK) ||
        (firmware_storage_erase_slot(&storage, FIRMWARE_SLOT_B, sizeof(payload)) != PLATFORM_ERR_OK) ||
        (firmware_storage_write_payload(&storage, FIRMWARE_SLOT_B, 0U, payload, sizeof(payload)) != PLATFORM_ERR_OK) ||
        (firmware_storage_write_header(&storage, FIRMWARE_SLOT_B, rawHeader) != PLATFORM_ERR_OK) ||
        (firmware_storage_validate_image(&storage, FIRMWARE_SLOT_B, &decoded, &validation) != PLATFORM_ERR_OK) ||
        (validation != FIRMWARE_IMAGE_VALIDATION_VALID)) {
        return 1;
    }

    backendContext.payload[1][10] ^= 1U;
    if ((firmware_storage_validate_image(&storage, FIRMWARE_SLOT_B, &decoded, &validation) != PLATFORM_ERR_OK) ||
        (validation != FIRMWARE_IMAGE_VALIDATION_INVALID_PAYLOAD_CRC)) {
        return 1;
    }

    if ((firmware_storage_erase_slot(&storage, FIRMWARE_SLOT_A, TEST_CAPACITY + 1U) == PLATFORM_ERR_OK) ||
        (firmware_storage_write_payload(&storage, FIRMWARE_SLOT_A, TEST_CAPACITY, payload, 1U) == PLATFORM_ERR_OK)) {
        return 1;
    }

    return 0;
}

int main(void)
{
    if (test_storage_flow() != 0) {
        (void)printf("Firmware storage host test failed.\n");
        return 1;
    }

    (void)printf("Firmware storage host test passed.\n");
    return 0;
}
