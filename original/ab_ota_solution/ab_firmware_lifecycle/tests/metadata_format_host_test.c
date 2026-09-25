#include <stdio.h>
#include <string.h>

#include "firmware_metadata.h"

#define TEST_COMMIT_OFFSET (0x7CU)

static void write_u32_le(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8U);
    data[2] = (uint8_t)(value >> 16U);
    data[3] = (uint8_t)(value >> 24U);
}

static firmware_metadata_t valid_metadata(void)
{
    firmware_metadata_t metadata = {0};

    metadata.sequence = 10U;
    metadata.confirmedSlot = FIRMWARE_SLOT_A;
    metadata.pendingSlot = FIRMWARE_SLOT_B;
    metadata.slotAState = FIRMWARE_SLOT_STATE_VALID;
    metadata.slotBState = FIRMWARE_SLOT_STATE_VALID;
    metadata.upgradeState = FIRMWARE_UPGRADE_STATE_TRIAL;
    metadata.confirmedVersion.major = 1U;
    return metadata;
}

int main(void)
{
    firmware_metadata_t metadata = valid_metadata();
    firmware_metadata_t decoded;
    uint8_t raw[FIRMWARE_METADATA_COPY_SIZE];

    if (firmware_metadata_encode_uncommitted(&metadata, raw) != PLATFORM_ERR_OK) {
        return 1;
    }

    write_u32_le(&raw[TEST_COMMIT_OFFSET], FIRMWARE_METADATA_COMMIT_MARKER);
    if (firmware_metadata_decode_committed(raw, &decoded) != PLATFORM_ERR_OK) {
        return 1;
    }

    if ((decoded.sequence != metadata.sequence) ||
        (decoded.confirmedSlot != FIRMWARE_SLOT_A) ||
        (decoded.pendingSlot != FIRMWARE_SLOT_B) ||
        (decoded.upgradeState != FIRMWARE_UPGRADE_STATE_TRIAL)) {
        return 1;
    }

    metadata.pendingSlot = FIRMWARE_SLOT_A;
    if (firmware_metadata_encode_uncommitted(&metadata, raw) == PLATFORM_ERR_OK) {
        return 1;
    }

    if ((firmware_metadata_sequence_is_newer(1U, 0xFFFFFFFFUL) == PLATFORM_FALSE) ||
        (firmware_metadata_sequence_is_newer(0xFFFFFFFFUL, 1U) != PLATFORM_FALSE)) {
        return 1;
    }

    (void)printf("Metadata format host test passed.\n");
    return 0;
}
