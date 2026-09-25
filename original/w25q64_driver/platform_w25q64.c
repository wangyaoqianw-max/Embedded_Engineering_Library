/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file platform_w25q64.c
 * @brief W25Q64JV Platform Raw Driver V1 读路径实现
 * @author YaoQian Wang
 * @date 2026-09-12
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "platform_w25q64.h"

#include "platform_time.h"
//******************************** Includes *********************************//

//******************************** Defines *********************************//
#define PLATFORM_W25Q64_CMD_READ_JEDEC_ID    (0x9FU)
#define PLATFORM_W25Q64_CMD_READ_STATUS1     (0x05U)
#define PLATFORM_W25Q64_CMD_READ_DATA        (0x03U)
#define PLATFORM_W25Q64_CMD_WRITE_ENABLE     (0x06U)
#define PLATFORM_W25Q64_CMD_PAGE_PROGRAM     (0x02U)
#define PLATFORM_W25Q64_CMD_SECTOR_ERASE     (0x20U)

#define PLATFORM_W25Q64_SR1_BUSY_MASK        (0x01U)
#define PLATFORM_W25Q64_SR1_WEL_MASK         (0x02U)
#define PLATFORM_W25Q64_INIT_READY_TIMEOUT_MS (1000U)
#define PLATFORM_W25Q64_PROGRAM_TIMEOUT_MS   (10U)
#define PLATFORM_W25Q64_ERASE_TIMEOUT_MS     (500U)
#define PLATFORM_W25Q64_POLL_INTERVAL_MS     (1U)
//******************************** Defines *********************************//

//******************************** Declaring *********************************//
static platform_error_t platform_w25q64_validate_initialized(
    const platform_w25q64_t *flash);
static platform_error_t platform_w25q64_validate_range(
    uint32_t address,
    platform_size_t dataLength);
static void platform_w25q64_encode_address(
    uint32_t address,
    uint8_t *encodedAddress);
static platform_error_t platform_w25q64_finish_transaction(
    platform_w25q64_t *flash,
    platform_error_t operationResult);
static platform_error_t platform_w25q64_command_read(
    platform_w25q64_t *flash,
    const uint8_t *command,
    platform_size_t commandLength,
    uint8_t *response,
    platform_size_t responseLength);
static platform_error_t platform_w25q64_read_status1_raw(
    platform_w25q64_t *flash,
    uint8_t *status);
static platform_error_t platform_w25q64_wait_ready(
    platform_w25q64_t *flash,
    uint32_t timeoutMs);
static platform_error_t platform_w25q64_read_jedec_id_raw(
    platform_w25q64_t *flash,
    platform_w25q64_jedec_id_t *jedecId);
static platform_error_t platform_w25q64_write_enable(
    platform_w25q64_t *flash);
//******************************** Declaring *********************************//

//******************************** Private Functions *************************//
/* 校验 W25Q64JV Raw Driver 是否已完成初始化。 */
static platform_error_t platform_w25q64_validate_initialized(
    const platform_w25q64_t *flash)
{
    if (flash == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (flash->initialized != PLATFORM_TRUE) {
        return PLATFORM_ERR_NOT_INITIALIZED;
    }

    return PLATFORM_ERR_OK;
}

/* 校验 Flash 线性地址和数据长度，避免地址计算溢出。 */
static platform_error_t platform_w25q64_validate_range(
    uint32_t address,
    platform_size_t dataLength)
{
    if (dataLength == 0U) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (address >= PLATFORM_W25Q64_TOTAL_SIZE_BYTES) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    if (dataLength > (PLATFORM_W25Q64_TOTAL_SIZE_BYTES - address)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    return PLATFORM_ERR_OK;
}

/* 将 24 位 Flash 地址编码为 SPI 命令中的高字节序地址。 */
static void platform_w25q64_encode_address(
    uint32_t address,
    uint8_t *encodedAddress)
{
    encodedAddress[0] = (uint8_t)(address >> 16U);
    encodedAddress[1] = (uint8_t)(address >> 8U);
    encodedAddress[2] = (uint8_t)address;
}

/* 结束当前 SPI 事务，并保留事务内首次出现的错误。 */
static platform_error_t platform_w25q64_finish_transaction(
    platform_w25q64_t *flash,
    platform_error_t operationResult)
{
    platform_error_t endResult = PLATFORM_ERR_OK;

    endResult = platform_spi_transaction_end(&flash->spiDevice);
    if (operationResult != PLATFORM_ERR_OK) {
        return operationResult;
    }

    return endResult;
}

/* 执行一次保持片选有效的命令发送和数据接收事务。 */
static platform_error_t platform_w25q64_command_read(
    platform_w25q64_t *flash,
    const uint8_t *command,
    platform_size_t commandLength,
    uint8_t *response,
    platform_size_t responseLength)
{
    platform_error_t result = PLATFORM_ERR_OK;

    if ((flash == NULL) || (command == NULL) || (commandLength == 0U) ||
        (response == NULL) || (responseLength == 0U)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = platform_spi_transaction_begin(&flash->spiDevice);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_spi_write(&flash->spiDevice,
                                command,
                                commandLength);
    if (result == PLATFORM_ERR_OK) {
        result = platform_spi_read(&flash->spiDevice,
                                   response,
                                   responseLength);
    }

    return platform_w25q64_finish_transaction(flash, result);
}

/* 直接读取 Status Register-1，供初始化和忙状态轮询使用。 */
static platform_error_t platform_w25q64_read_status1_raw(
    platform_w25q64_t *flash,
    uint8_t *status)
{
    const uint8_t command = PLATFORM_W25Q64_CMD_READ_STATUS1;

    if ((flash == NULL) || (status == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    return platform_w25q64_command_read(flash,
                                         &command,
                                         1U,
                                         status,
                                         1U);
}

/* 轮询 BUSY 位，等待 Flash 内部操作结束或超时。 */
static platform_error_t platform_w25q64_wait_ready(
    platform_w25q64_t *flash,
    uint32_t timeoutMs)
{
    platform_error_t result = PLATFORM_ERR_OK;
    uint8_t status = 0U;
    uint32_t elapsedMs = 0U;

    while (elapsedMs < timeoutMs) {
        result = platform_w25q64_read_status1_raw(flash, &status);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }

        if ((status & PLATFORM_W25Q64_SR1_BUSY_MASK) == 0U) {
            return PLATFORM_ERR_OK;
        }

        result = platform_time_delay_ms(PLATFORM_W25Q64_POLL_INTERVAL_MS);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }

        elapsedMs++;
    }

    return PLATFORM_ERR_TIMEOUT;
}

/* 直接读取 JEDEC ID，供初始化和公共查询接口复用。 */
static platform_error_t platform_w25q64_read_jedec_id_raw(
    platform_w25q64_t *flash,
    platform_w25q64_jedec_id_t *jedecId)
{
    platform_error_t result = PLATFORM_ERR_OK;
    uint8_t response[3] = {0U};
    const uint8_t command = PLATFORM_W25Q64_CMD_READ_JEDEC_ID;

    if ((flash == NULL) || (jedecId == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    result = platform_w25q64_command_read(flash,
                                           &command,
                                           1U,
                                           response,
                                           sizeof(response));
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    jedecId->manufacturerId = response[0];
    jedecId->memoryType = response[1];
    jedecId->capacityId = response[2];
    return PLATFORM_ERR_OK;
}

/* 发送 Write Enable，并确认 Status Register-1 的 WEL 位置位。 */
static platform_error_t platform_w25q64_write_enable(
    platform_w25q64_t *flash)
{
    platform_error_t result = PLATFORM_ERR_OK;
    uint8_t status = 0U;
    const uint8_t command = PLATFORM_W25Q64_CMD_WRITE_ENABLE;

    if (flash == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    result = platform_spi_transaction_begin(&flash->spiDevice);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_spi_write(&flash->spiDevice,
                                &command,
                                1U);
    result = platform_w25q64_finish_transaction(flash, result);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_w25q64_read_status1_raw(flash, &status);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return ((status & PLATFORM_W25Q64_SR1_WEL_MASK) != 0U) ?
           PLATFORM_ERR_OK : PLATFORM_ERR_IO;
}
//******************************** Private Functions *************************//

//******************************** Functions *********************************//
platform_error_t platform_w25q64_init(
    platform_w25q64_t *flash,
    platform_spi_bus_t *spiBus)
{
    platform_error_t result = PLATFORM_ERR_OK;
    platform_gpio_config_t csConfig = {
        PLATFORM_GPIO_DIRECTION_INPUT,
        PLATFORM_GPIO_PULL_NONE,
        PLATFORM_GPIO_OUTPUT_PUSH_PULL,
        PLATFORM_GPIO_LEVEL_LOW
    };
    platform_w25q64_jedec_id_t jedecId = {0U};

    if ((flash == NULL) || (spiBus == NULL)) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (flash->initialized == PLATFORM_TRUE) {
        return PLATFORM_ERR_ALREADY_INITIALIZED;
    }

    if (spiBus->device.object.state != PLATFORM_OBJECT_STARTED) {
        return PLATFORM_ERR_INVALID_STATE;
    }

    csConfig.direction = PLATFORM_GPIO_DIRECTION_OUTPUT;
    csConfig.outputType = PLATFORM_GPIO_OUTPUT_PUSH_PULL;
    csConfig.pull = PLATFORM_GPIO_PULL_NONE;
    csConfig.initialLevel = PLATFORM_GPIO_LEVEL_HIGH;

    result = platform_gpio_configure(&flash->cs, &csConfig);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_spi_device_init(&flash->spiDevice,
                                      "w25q64",
                                      spiBus,
                                      &flash->cs,
                                      flash->csActiveLevel,
                                      &flash->spiConfig);
    if (result != PLATFORM_ERR_OK) {
        (void)platform_gpio_deinit(&flash->cs);
        return result;
    }

    result = platform_w25q64_wait_ready(flash,
                                         PLATFORM_W25Q64_INIT_READY_TIMEOUT_MS);
    if (result != PLATFORM_ERR_OK) {
        (void)platform_spi_device_deinit(&flash->spiDevice);
        (void)platform_gpio_deinit(&flash->cs);
        return result;
    }

    result = platform_w25q64_read_jedec_id_raw(flash, &jedecId);
    if (result != PLATFORM_ERR_OK) {
        (void)platform_spi_device_deinit(&flash->spiDevice);
        (void)platform_gpio_deinit(&flash->cs);
        return result;
    }

    if ((jedecId.manufacturerId != 0xEFU) ||
        (jedecId.memoryType != 0x40U) ||
        (jedecId.capacityId != 0x17U)) {
        (void)platform_spi_device_deinit(&flash->spiDevice);
        (void)platform_gpio_deinit(&flash->cs);
        return PLATFORM_ERR_IO;
    }

    flash->manufacturerId = jedecId.manufacturerId;
    flash->memoryType = jedecId.memoryType;
    flash->capacityId = jedecId.capacityId;
    flash->initialized = PLATFORM_TRUE;
    return PLATFORM_ERR_OK;
}

platform_error_t platform_w25q64_deinit(platform_w25q64_t *flash)
{
    platform_error_t result = PLATFORM_ERR_OK;
    platform_error_t gpioResult = PLATFORM_ERR_OK;

    result = platform_w25q64_validate_initialized(flash);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_spi_device_deinit(&flash->spiDevice);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    gpioResult = platform_gpio_deinit(&flash->cs);
    flash->initialized = PLATFORM_FALSE;

    return (result != PLATFORM_ERR_OK) ? result : gpioResult;
}

platform_error_t platform_w25q64_read_jedec_id(
    platform_w25q64_t *flash,
    platform_w25q64_jedec_id_t *jedecId)
{
    platform_error_t result = platform_w25q64_validate_initialized(flash);

    if (jedecId == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return platform_w25q64_read_jedec_id_raw(flash, jedecId);
}

platform_error_t platform_w25q64_read_status1(
    platform_w25q64_t *flash,
    uint8_t *status)
{
    platform_error_t result = platform_w25q64_validate_initialized(flash);

    if (status == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return platform_w25q64_read_status1_raw(flash, status);
}

platform_error_t platform_w25q64_read(
    platform_w25q64_t *flash,
    uint32_t address,
    uint8_t *data,
    platform_size_t dataLength)
{
    platform_error_t result = platform_w25q64_validate_initialized(flash);
    uint8_t command[4] = {PLATFORM_W25Q64_CMD_READ_DATA, 0U, 0U, 0U};

    if (data == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_w25q64_validate_range(address, dataLength);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    platform_w25q64_encode_address(address, &command[1]);
    return platform_w25q64_command_read(flash,
                                         command,
                                         sizeof(command),
                                         data,
                                         dataLength);
}

platform_error_t platform_w25q64_page_program(
    platform_w25q64_t *flash,
    uint32_t address,
    const uint8_t *data,
    platform_size_t dataLength)
{
    platform_error_t result = platform_w25q64_validate_initialized(flash);
    uint32_t pageOffset = address % PLATFORM_W25Q64_PAGE_SIZE_BYTES;
    uint8_t command[4] = {PLATFORM_W25Q64_CMD_PAGE_PROGRAM, 0U, 0U, 0U};

    if (data == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_w25q64_validate_range(address, dataLength);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if ((dataLength > PLATFORM_W25Q64_PAGE_SIZE_BYTES) ||
        (dataLength > (PLATFORM_W25Q64_PAGE_SIZE_BYTES - pageOffset))) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = platform_w25q64_wait_ready(
        flash,
        PLATFORM_W25Q64_PROGRAM_TIMEOUT_MS);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_w25q64_write_enable(flash);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    platform_w25q64_encode_address(address, &command[1]);
    result = platform_spi_transaction_begin(&flash->spiDevice);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_spi_write(&flash->spiDevice,
                                command,
                                sizeof(command));
    if (result == PLATFORM_ERR_OK) {
        result = platform_spi_write(&flash->spiDevice,
                                    data,
                                    dataLength);
    }

    result = platform_w25q64_finish_transaction(flash, result);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return platform_w25q64_wait_ready(
        flash,
        PLATFORM_W25Q64_PROGRAM_TIMEOUT_MS);
}

platform_error_t platform_w25q64_write(
    platform_w25q64_t *flash,
    uint32_t address,
    const uint8_t *data,
    platform_size_t dataLength)
{
    platform_error_t result = platform_w25q64_validate_initialized(flash);
    uint32_t currentAddress = address;
    const uint8_t *currentData = data;
    platform_size_t remaining = dataLength;
    platform_size_t pageOffset = 0U;
    platform_size_t pageRemaining = 0U;
    platform_size_t chunkLength = 0U;

    if (data == NULL) {
        return PLATFORM_ERR_NULL_POINTER;
    }

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_w25q64_validate_range(address, dataLength);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    while (remaining > 0U) {
        pageOffset = currentAddress % PLATFORM_W25Q64_PAGE_SIZE_BYTES;
        pageRemaining = PLATFORM_W25Q64_PAGE_SIZE_BYTES - pageOffset;
        chunkLength = (remaining < pageRemaining) ? remaining : pageRemaining;

        result = platform_w25q64_page_program(
            flash,
            currentAddress,
            currentData,
            chunkLength);
        if (result != PLATFORM_ERR_OK) {
            return result;
        }

        currentAddress += (uint32_t)chunkLength;
        currentData += chunkLength;
        remaining -= chunkLength;
    }

    return PLATFORM_ERR_OK;
}

platform_error_t platform_w25q64_sector_erase(
    platform_w25q64_t *flash,
    uint32_t sectorAddress)
{
    platform_error_t result = platform_w25q64_validate_initialized(flash);
    uint8_t command[4] = {PLATFORM_W25Q64_CMD_SECTOR_ERASE, 0U, 0U, 0U};

    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    if ((sectorAddress >= PLATFORM_W25Q64_TOTAL_SIZE_BYTES) ||
        ((sectorAddress % PLATFORM_W25Q64_SECTOR_SIZE_BYTES) != 0U)) {
        return PLATFORM_ERR_INVALID_PARAM;
    }

    result = platform_w25q64_wait_ready(
        flash,
        PLATFORM_W25Q64_ERASE_TIMEOUT_MS);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_w25q64_write_enable(flash);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    platform_w25q64_encode_address(sectorAddress, &command[1]);
    result = platform_spi_transaction_begin(&flash->spiDevice);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    result = platform_spi_write(&flash->spiDevice,
                                command,
                                sizeof(command));
    result = platform_w25q64_finish_transaction(flash, result);
    if (result != PLATFORM_ERR_OK) {
        return result;
    }

    return platform_w25q64_wait_ready(
        flash,
        PLATFORM_W25Q64_ERASE_TIMEOUT_MS);
}
//******************************** Functions *********************************//
