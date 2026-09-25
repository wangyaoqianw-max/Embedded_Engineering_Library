#include <stdio.h>
#include <string.h>

#include "crc.h"
#include "firmware_image.h"
#include "firmware_version.h"

#define TEST_IMAGE_SIZE      (0x1234UL)
#define TEST_PAYLOAD_CRC32   (0x89ABCDEFUL)

static void test_write_u32_le(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8U);
    data[2] = (uint8_t)(value >> 16U);
    data[3] = (uint8_t)(value >> 24U);
}

static int test_version(void)
{
    firmware_version_t v123 = {1U, 2U, 3U, 0U};
    firmware_version_t v122 = {1U, 2U, 2U, 0U};
    firmware_version_t v130 = {1U, 3U, 0U, 0U};
    firmware_version_t invalid = {1U, 2U, 3U, 1U};

    return ((firmware_version_compare(&v123, &v122) > 0) &&
            (firmware_version_compare(&v123, &v130) < 0) &&
            (firmware_version_compare(&v123, &v123) == 0) &&
            (firmware_version_is_valid(&invalid) == 0U)) ? 0 : 1;
}

static int test_header(void)
{
    firmware_image_header_t header = {0};
    firmware_image_header_t decoded = {0};
    firmware_image_validation_t validation;
    uint8_t raw[FIRMWARE_IMAGE_HEADER_SIZE];

    header.version.major = 1U;
    header.version.minor = 2U;
    header.version.patch = 3U;
    header.imageSize = TEST_IMAGE_SIZE;
    header.payloadCrc32 = TEST_PAYLOAD_CRC32;

    if (firmware_image_encode_header(&header, raw) != PLATFORM_ERR_OK) {
        return 1;
    }

    validation = firmware_image_validate_header(raw, &decoded);
    if ((validation != FIRMWARE_IMAGE_VALIDATION_VALID) ||
        (decoded.imageSize != TEST_IMAGE_SIZE) ||
        (decoded.payloadCrc32 != TEST_PAYLOAD_CRC32)) {
        return 1;
    }

    raw[0x18U] = 1U;
    if (firmware_image_validate_header(raw, &decoded) !=
        FIRMWARE_IMAGE_VALIDATION_INVALID_RESERVED) {
        return 1;
    }

    raw[0x18U] = 0U;
    raw[0x10U] ^= 1U;
    if (firmware_image_validate_header(raw, &decoded) !=
        FIRMWARE_IMAGE_VALIDATION_INVALID_HEADER_CRC) {
        return 1;
    }

    (void)memset(raw, 0xFF, sizeof(raw));
    if (firmware_image_validate_header(raw, &decoded) !=
        FIRMWARE_IMAGE_VALIDATION_EMPTY) {
        return 1;
    }

    (void)memset(raw, 0, sizeof(raw));
    raw[0x00U] = 0x46U;
    raw[0x01U] = 0x57U;
    raw[0x02U] = 0x49U;
    raw[0x03U] = 0x4DU;
    raw[0x04U] = 1U;
    raw[0x06U] = FIRMWARE_IMAGE_HEADER_SIZE;
    test_write_u32_le(&raw[0x3CU], crc32_iso_hdlc_calculate(raw, 0x3CU));
    return (firmware_image_validate_header(raw, &decoded) ==
            FIRMWARE_IMAGE_VALIDATION_INVALID_SIZE) ? 0 : 1;
}

int main(void)
{
    if ((test_version() != 0) || (test_header() != 0)) {
        (void)printf("Firmware image host test failed.\n");
        return 1;
    }

    (void)printf("Firmware image host test passed.\n");
    return 0;
}
