#include <stdio.h>
#include <string.h>

#include "firmware_metadata_store.h"

#define TEST_BODY_SIZE   (0x7CU)
#define TEST_MARKER_OFF  (0x7CU)

typedef struct
{
    uint8_t copy[2][FIRMWARE_METADATA_COPY_SIZE];
    uint8_t failBodyWrite;
    uint8_t failMarkerWrite;
} test_backend_t;

static int copy_index(firmware_metadata_copy_id_t copy)
{
    return (copy == FIRMWARE_METADATA_COPY_A) ? 0 :
           (copy == FIRMWARE_METADATA_COPY_B) ? 1 : -1;
}

static platform_error_t backend_read(
    void *context,
    firmware_metadata_copy_id_t copy,
    uint32_t offset,
    uint8_t *data,
    uint32_t length)
{
    test_backend_t *backend = (test_backend_t *)context;
    int index = copy_index(copy);

    if ((backend == NULL) || (data == NULL) || (index < 0) ||
        (offset > FIRMWARE_METADATA_COPY_SIZE) ||
        (length > (FIRMWARE_METADATA_COPY_SIZE - offset))) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    (void)memcpy(data, &backend->copy[index][offset], length);
    return PLATFORM_ERR_OK;
}

static platform_error_t backend_write(
    void *context,
    firmware_metadata_copy_id_t copy,
    uint32_t offset,
    const uint8_t *data,
    uint32_t length)
{
    test_backend_t *backend = (test_backend_t *)context;
    int index = copy_index(copy);

    if ((backend == NULL) || (data == NULL) || (index < 0) ||
        (offset > FIRMWARE_METADATA_COPY_SIZE) ||
        (length > (FIRMWARE_METADATA_COPY_SIZE - offset))) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if ((backend->failBodyWrite != 0U) &&
        (offset == 0U) && (length == TEST_BODY_SIZE)) {
        return PLATFORM_ERR_IO;
    }

    if ((backend->failMarkerWrite != 0U) &&
        (offset == TEST_MARKER_OFF) && (length == 4U) &&
        (data[0] == 0x43U)) {
        return PLATFORM_ERR_IO;
    }

    (void)memcpy(&backend->copy[index][offset], data, length);
    return PLATFORM_ERR_OK;
}

static firmware_metadata_t baseline(void)
{
    firmware_metadata_t metadata = {0};

    metadata.sequence = 10U;
    metadata.confirmedSlot = FIRMWARE_SLOT_A;
    metadata.pendingSlot = FIRMWARE_SLOT_NONE;
    metadata.slotAState = FIRMWARE_SLOT_STATE_VALID;
    metadata.slotBState = FIRMWARE_SLOT_STATE_VALID;
    metadata.upgradeState = FIRMWARE_UPGRADE_STATE_NONE;
    metadata.confirmedVersion.major = 1U;
    return metadata;
}

int main(void)
{
    test_backend_t backendContext;
    firmware_metadata_backend_t backend = {0};
    firmware_metadata_store_t store = FIRMWARE_METADATA_STORE_INITIALIZER;
    firmware_metadata_t metadata;
    firmware_metadata_t committed;
    firmware_metadata_t loaded;
    firmware_metadata_copy_id_t copy;
    firmware_metadata_copy_id_t loadedCopy;

    (void)memset(&backendContext, 0xFF, sizeof(backendContext));
    backendContext.failBodyWrite = 0U;
    backendContext.failMarkerWrite = 0U;
    backend.read = backend_read;
    backend.write = backend_write;
    backend.context = &backendContext;

    if (firmware_metadata_store_init(&store, &backend) != PLATFORM_ERR_OK) {
        return 1;
    }

    metadata = baseline();
    if ((firmware_metadata_store_commit(&store, &metadata, &committed, &copy) != PLATFORM_ERR_OK) ||
        (copy != FIRMWARE_METADATA_COPY_A) ||
        (committed.sequence != 10U)) {
        return 1;
    }

    metadata = committed;
    metadata.pendingSlot = FIRMWARE_SLOT_B;
    metadata.upgradeState = FIRMWARE_UPGRADE_STATE_PENDING;
    if ((firmware_metadata_store_commit(&store, &metadata, &committed, &copy) != PLATFORM_ERR_OK) ||
        (copy != FIRMWARE_METADATA_COPY_B) ||
        (committed.sequence != 11U)) {
        return 1;
    }

    backendContext.failBodyWrite = 1U;
    metadata = committed;
    metadata.upgradeState = FIRMWARE_UPGRADE_STATE_TRIAL;
    if (firmware_metadata_store_commit(&store, &metadata, &loaded, &loadedCopy) == PLATFORM_ERR_OK) {
        return 1;
    }
    backendContext.failBodyWrite = 0U;

    if ((firmware_metadata_store_load(&store, &loaded, &loadedCopy) != PLATFORM_ERR_OK) ||
        (loadedCopy != FIRMWARE_METADATA_COPY_B) ||
        (loaded.sequence != 11U) ||
        (loaded.upgradeState != FIRMWARE_UPGRADE_STATE_PENDING)) {
        return 1;
    }

    backendContext.failMarkerWrite = 1U;
    if (firmware_metadata_store_commit(&store, &metadata, &loaded, &loadedCopy) == PLATFORM_ERR_OK) {
        return 1;
    }
    backendContext.failMarkerWrite = 0U;

    if ((firmware_metadata_store_load(&store, &loaded, &loadedCopy) != PLATFORM_ERR_OK) ||
        (loadedCopy != FIRMWARE_METADATA_COPY_B) ||
        (loaded.upgradeState != FIRMWARE_UPGRADE_STATE_PENDING)) {
        return 1;
    }

    (void)printf("Metadata store host test passed.\n");
    return 0;
}
