#include <stdio.h>
#include <string.h>

#include "crc.h"
#include "firmware_boot_decision.h"
#include "firmware_lifecycle.h"

#define TEST_PAYLOAD_SIZE   (32U)
#define TEST_CAPACITY       (128U)

typedef struct
{
    uint8_t copy[2][FIRMWARE_METADATA_COPY_SIZE];
} metadata_memory_t;

typedef struct
{
    uint8_t header[2][FIRMWARE_IMAGE_HEADER_SIZE];
    uint8_t payload[2][TEST_CAPACITY];
} image_memory_t;

static int metadata_copy_index(firmware_metadata_copy_id_t copy)
{
    return (copy == FIRMWARE_METADATA_COPY_A) ? 0 :
           (copy == FIRMWARE_METADATA_COPY_B) ? 1 : -1;
}

static int slot_index(firmware_slot_t slot)
{
    return (slot == FIRMWARE_SLOT_A) ? 0 :
           (slot == FIRMWARE_SLOT_B) ? 1 : -1;
}

static platform_error_t metadata_read(
    void *context,
    firmware_metadata_copy_id_t copy,
    uint32_t offset,
    uint8_t *data,
    uint32_t length)
{
    metadata_memory_t *memory = (metadata_memory_t *)context;
    int index = metadata_copy_index(copy);

    if ((memory == NULL) || (data == NULL) || (index < 0) ||
        (offset > FIRMWARE_METADATA_COPY_SIZE) ||
        (length > (FIRMWARE_METADATA_COPY_SIZE - offset))) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    (void)memcpy(data, &memory->copy[index][offset], length);
    return PLATFORM_ERR_OK;
}

static platform_error_t metadata_write(
    void *context,
    firmware_metadata_copy_id_t copy,
    uint32_t offset,
    const uint8_t *data,
    uint32_t length)
{
    metadata_memory_t *memory = (metadata_memory_t *)context;
    int index = metadata_copy_index(copy);

    if ((memory == NULL) || (data == NULL) || (index < 0) ||
        (offset > FIRMWARE_METADATA_COPY_SIZE) ||
        (length > (FIRMWARE_METADATA_COPY_SIZE - offset))) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    (void)memcpy(&memory->copy[index][offset], data, length);
    return PLATFORM_ERR_OK;
}

static platform_error_t image_erase(
    void *context,
    firmware_slot_t slot,
    uint32_t payloadSize)
{
    image_memory_t *memory = (image_memory_t *)context;
    int index = slot_index(slot);

    if ((memory == NULL) || (index < 0) ||
        (payloadSize == 0U) || (payloadSize > TEST_CAPACITY)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    (void)memset(memory->header[index], 0xFF, FIRMWARE_IMAGE_HEADER_SIZE);
    (void)memset(memory->payload[index], 0xFF, TEST_CAPACITY);
    return PLATFORM_ERR_OK;
}

static platform_error_t image_read_header(
    void *context,
    firmware_slot_t slot,
    uint8_t rawHeader[FIRMWARE_IMAGE_HEADER_SIZE])
{
    image_memory_t *memory = (image_memory_t *)context;
    int index = slot_index(slot);

    if ((memory == NULL) || (rawHeader == NULL) || (index < 0)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    (void)memcpy(rawHeader, memory->header[index], FIRMWARE_IMAGE_HEADER_SIZE);
    return PLATFORM_ERR_OK;
}

static platform_error_t image_read_payload(
    void *context,
    firmware_slot_t slot,
    uint32_t offset,
    uint8_t *data,
    uint32_t length)
{
    image_memory_t *memory = (image_memory_t *)context;
    int index = slot_index(slot);

    if ((memory == NULL) || (data == NULL) || (index < 0) ||
        (offset > TEST_CAPACITY) ||
        (length > (TEST_CAPACITY - offset))) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    (void)memcpy(data, &memory->payload[index][offset], length);
    return PLATFORM_ERR_OK;
}

static platform_error_t image_write_payload(
    void *context,
    firmware_slot_t slot,
    uint32_t offset,
    const uint8_t *data,
    uint32_t length)
{
    image_memory_t *memory = (image_memory_t *)context;
    int index = slot_index(slot);

    if ((memory == NULL) || (data == NULL) || (index < 0) ||
        (offset > TEST_CAPACITY) ||
        (length > (TEST_CAPACITY - offset))) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    (void)memcpy(&memory->payload[index][offset], data, length);
    return PLATFORM_ERR_OK;
}

static platform_error_t image_write_header(
    void *context,
    firmware_slot_t slot,
    const uint8_t rawHeader[FIRMWARE_IMAGE_HEADER_SIZE])
{
    image_memory_t *memory = (image_memory_t *)context;
    int index = slot_index(slot);

    if ((memory == NULL) || (rawHeader == NULL) || (index < 0)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    (void)memcpy(memory->header[index], rawHeader, FIRMWARE_IMAGE_HEADER_SIZE);
    return PLATFORM_ERR_OK;
}

static firmware_metadata_t baseline_metadata(void)
{
    firmware_metadata_t metadata = {0};

    metadata.sequence = 1U;
    metadata.confirmedSlot = FIRMWARE_SLOT_A;
    metadata.pendingSlot = FIRMWARE_SLOT_NONE;
    metadata.slotAState = FIRMWARE_SLOT_STATE_VALID;
    metadata.slotBState = FIRMWARE_SLOT_STATE_VALID;
    metadata.upgradeState = FIRMWARE_UPGRADE_STATE_NONE;
    metadata.confirmedVersion.major = 1U;
    return metadata;
}

static int commit_baseline(
    firmware_metadata_store_t *store)
{
    firmware_metadata_t metadata = baseline_metadata();
    firmware_metadata_t committed;
    firmware_metadata_copy_id_t copy;

    return (firmware_metadata_store_commit(
                store, &metadata, &committed, &copy) == PLATFORM_ERR_OK) ? 0 : 1;
}

static int load_metadata(
    firmware_metadata_store_t *store,
    firmware_metadata_t *metadata)
{
    firmware_metadata_copy_id_t copy;

    return (firmware_metadata_store_load(
                store, metadata, &copy) == PLATFORM_ERR_OK) ? 0 : 1;
}

static int test_rollback_path(
    firmware_metadata_store_t *store)
{
    firmware_metadata_t metadata;
    firmware_boot_action_t action;

    if ((firmware_lifecycle_request_upgrade(store, FIRMWARE_SLOT_B) != PLATFORM_ERR_OK) ||
        (load_metadata(store, &metadata) != 0) ||
        (metadata.upgradeState != FIRMWARE_UPGRADE_STATE_PENDING) ||
        (firmware_boot_decision_get_action(&metadata, &action) != PLATFORM_ERR_OK) ||
        (action != FIRMWARE_BOOT_ACTION_INSTALL_PENDING)) {
        return 1;
    }

    if ((firmware_lifecycle_mark_trial(store, FIRMWARE_SLOT_B) != PLATFORM_ERR_OK) ||
        (load_metadata(store, &metadata) != 0) ||
        (metadata.upgradeState != FIRMWARE_UPGRADE_STATE_TRIAL) ||
        (firmware_boot_decision_get_action(&metadata, &action) != PLATFORM_ERR_OK) ||
        (action != FIRMWARE_BOOT_ACTION_BEGIN_ROLLBACK)) {
        return 1;
    }

    if ((firmware_lifecycle_begin_rollback(store) != PLATFORM_ERR_OK) ||
        (load_metadata(store, &metadata) != 0) ||
        (metadata.upgradeState != FIRMWARE_UPGRADE_STATE_ROLLBACK) ||
        (firmware_boot_decision_get_action(&metadata, &action) != PLATFORM_ERR_OK) ||
        (action != FIRMWARE_BOOT_ACTION_CONTINUE_ROLLBACK)) {
        return 1;
    }

    if ((firmware_lifecycle_complete_rollback(store) != PLATFORM_ERR_OK) ||
        (load_metadata(store, &metadata) != 0) ||
        (metadata.upgradeState != FIRMWARE_UPGRADE_STATE_NONE) ||
        (metadata.pendingSlot != FIRMWARE_SLOT_NONE) ||
        (metadata.confirmedSlot != FIRMWARE_SLOT_A) ||
        (firmware_boot_decision_get_action(&metadata, &action) != PLATFORM_ERR_OK) ||
        (action != FIRMWARE_BOOT_ACTION_BOOT_CURRENT)) {
        return 1;
    }

    return 0;
}

static int prepare_candidate_image(
    image_memory_t *memory)
{
    firmware_image_header_t header = {0};
    uint8_t rawHeader[FIRMWARE_IMAGE_HEADER_SIZE];
    uint32_t index;

    for (index = 0U; index < TEST_PAYLOAD_SIZE; index++) {
        memory->payload[1][index] = (uint8_t)(0x40U + index);
    }

    header.version.major = 2U;
    header.imageSize = TEST_PAYLOAD_SIZE;
    header.payloadCrc32 = crc32_iso_hdlc_calculate(
        memory->payload[1],
        TEST_PAYLOAD_SIZE);

    if (firmware_image_encode_header(&header, rawHeader) != PLATFORM_ERR_OK) {
        return 1;
    }

    (void)memcpy(memory->header[1], rawHeader, sizeof(rawHeader));
    return 0;
}

static int test_confirm_path(
    firmware_metadata_store_t *metadataStore,
    firmware_storage_t *firmwareStorage,
    image_memory_t *imageMemory)
{
    firmware_metadata_t metadata;

    if ((firmware_lifecycle_request_upgrade(
             metadataStore, FIRMWARE_SLOT_B) != PLATFORM_ERR_OK) ||
        (firmware_lifecycle_mark_trial(
             metadataStore, FIRMWARE_SLOT_B) != PLATFORM_ERR_OK)) {
        return 1;
    }

    imageMemory->payload[1][3] ^= 1U;
    if (firmware_lifecycle_confirm(
            metadataStore,
            firmwareStorage) != PLATFORM_ERR_CHECKSUM) {
        return 1;
    }

    if ((load_metadata(metadataStore, &metadata) != 0) ||
        (metadata.upgradeState != FIRMWARE_UPGRADE_STATE_TRIAL) ||
        (metadata.confirmedSlot != FIRMWARE_SLOT_A)) {
        return 1;
    }

    imageMemory->payload[1][3] ^= 1U;
    if (firmware_lifecycle_confirm(
            metadataStore,
            firmwareStorage) != PLATFORM_ERR_OK) {
        return 1;
    }

    if ((load_metadata(metadataStore, &metadata) != 0) ||
        (metadata.upgradeState != FIRMWARE_UPGRADE_STATE_NONE) ||
        (metadata.pendingSlot != FIRMWARE_SLOT_NONE) ||
        (metadata.confirmedSlot != FIRMWARE_SLOT_B) ||
        (metadata.confirmedVersion.major != 2U)) {
        return 1;
    }

    return 0;
}

int main(void)
{
    metadata_memory_t metadataMemory;
    image_memory_t imageMemory;
    firmware_metadata_backend_t metadataBackend = {0};
    firmware_metadata_store_t metadataStore = FIRMWARE_METADATA_STORE_INITIALIZER;
    firmware_storage_backend_t storageBackend = {0};
    firmware_storage_t firmwareStorage = FIRMWARE_STORAGE_INITIALIZER;

    (void)memset(&metadataMemory, 0xFF, sizeof(metadataMemory));
    (void)memset(&imageMemory, 0xFF, sizeof(imageMemory));

    metadataBackend.read = metadata_read;
    metadataBackend.write = metadata_write;
    metadataBackend.context = &metadataMemory;

    storageBackend.erase_image = image_erase;
    storageBackend.read_header = image_read_header;
    storageBackend.read_payload = image_read_payload;
    storageBackend.write_payload = image_write_payload;
    storageBackend.write_header = image_write_header;
    storageBackend.context = &imageMemory;
    storageBackend.payloadCapacity = TEST_CAPACITY;

    if ((firmware_metadata_store_init(&metadataStore, &metadataBackend) != PLATFORM_ERR_OK) ||
        (firmware_storage_init(&firmwareStorage, &storageBackend) != PLATFORM_ERR_OK) ||
        (commit_baseline(&metadataStore) != 0) ||
        (test_rollback_path(&metadataStore) != 0)) {
        return 1;
    }

    (void)memset(&metadataMemory, 0xFF, sizeof(metadataMemory));
    if ((commit_baseline(&metadataStore) != 0) ||
        (prepare_candidate_image(&imageMemory) != 0) ||
        (test_confirm_path(
             &metadataStore,
             &firmwareStorage,
             &imageMemory) != 0)) {
        return 1;
    }

    (void)printf("Firmware lifecycle host test passed.\n");
    return 0;
}
