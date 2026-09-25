/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file test_impl_platform_spi.c
 * @brief 验证 STM32 SPI1 Impl 生命周期、配置校验和 HAL 映射
 * @author YaoQian Wang
 * @date 2026-09-05
 * @version V1.0
 *
 *****************************************************************************/

#include "impl_platform_spi.h"
#include "spi.h"

#define TEST_ASSERT(condition)       \
    do {                             \
        if (!(condition)) {          \
            return __LINE__;         \
        }                            \
    } while (0)

SPI_HandleTypeDef hspi1;
HAL_StatusTypeDef g_fakeHalSpiTransmitResult;
SPI_HandleTypeDef *g_fakeHalSpiTransmitHandle;
uint8_t *g_fakeHalSpiTransmitData;
uint16_t g_fakeHalSpiTransmitLength;
uint32_t g_fakeHalSpiTransmitTimeoutMs;
uint32_t g_fakeHalSpiTransmitCount;
uint32_t g_fakePclk2Hz;

#include "../../04_Impl/impl_mcu/impl_platform_spi.c"

static void reset_fake_hal(void)
{
    hspi1 = (SPI_HandleTypeDef){0};
    hspi1.Instance = &hspi1;
    hspi1.Init.Mode = SPI_MODE_MASTER;
    hspi1.Init.Direction = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity = SPI_POLARITY_HIGH;
    hspi1.Init.CLKPhase = SPI_PHASE_2EDGE;
    hspi1.Init.NSS = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
    hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi1.State = HAL_SPI_STATE_READY;
    g_fakeHalSpiTransmitResult = HAL_OK;
    g_fakeHalSpiTransmitHandle = NULL;
    g_fakeHalSpiTransmitData = NULL;
    g_fakeHalSpiTransmitLength = 0U;
    g_fakeHalSpiTransmitTimeoutMs = 0U;
    g_fakeHalSpiTransmitCount = 0U;
    g_fakePclk2Hz = 100000000U;
}

static platform_spi_device_config_t make_config(void)
{
    platform_spi_device_config_t config = {
        PLATFORM_SPI_MODE_3,
        PLATFORM_SPI_BIT_ORDER_MSB_FIRST,
        8U,
        20000000U
    };

    return config;
}

static int construct_and_start(platform_spi_bus_t *bus)
{
    platform_error_t result = impl_platform_spi1_construct(
        bus,
        "spi1",
        PLATFORM_DEVICE_CAP_NONE);

    if (result != PLATFORM_ERR_OK) {
        return __LINE__;
    }
    result = bus->device.lifecycle->init(bus);
    if (result != PLATFORM_ERR_OK) {
        return __LINE__;
    }
    result = bus->device.lifecycle->start(bus);
    if (result != PLATFORM_ERR_OK) {
        return __LINE__;
    }

    return 0;
}

static int test_construct_and_lifecycle_follow_platform_state_model(void)
{
    platform_spi_bus_t bus = PLATFORM_SPI_BUS_INITIALIZER;

    reset_fake_hal();
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                impl_platform_spi1_construct(NULL,
                                             "spi1",
                                             PLATFORM_DEVICE_CAP_NONE));
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                impl_platform_spi1_construct(&bus,
                                             NULL,
                                             PLATFORM_DEVICE_CAP_NONE));
    TEST_ASSERT(PLATFORM_ERR_OK ==
                impl_platform_spi1_construct(&bus,
                                             "spi1",
                                             PLATFORM_DEVICE_CAP_NONE));
    TEST_ASSERT(PLATFORM_DEVICE_CLASS_SPI == bus.device.dev_class);
    TEST_ASSERT(PLATFORM_OBJECT_CREATED == bus.device.object.state);

    hspi1.Instance = NULL;
    TEST_ASSERT(PLATFORM_ERR_NOT_INITIALIZED == bus.device.lifecycle->init(&bus));
    hspi1.Instance = &hspi1;
    hspi1.State = HAL_SPI_STATE_RESET;
    TEST_ASSERT(PLATFORM_ERR_NOT_INITIALIZED == bus.device.lifecycle->init(&bus));
    hspi1.State = HAL_SPI_STATE_READY;
    TEST_ASSERT(PLATFORM_ERR_OK == bus.device.lifecycle->init(&bus));
    TEST_ASSERT(PLATFORM_OBJECT_INITIALIZED == bus.device.object.state);
    TEST_ASSERT(PLATFORM_DEVICE_POWER_IDLE == bus.device.power_state);
    TEST_ASSERT(PLATFORM_ERR_OK == bus.device.lifecycle->start(&bus));
    TEST_ASSERT(PLATFORM_OBJECT_STARTED == bus.device.object.state);
    TEST_ASSERT(PLATFORM_DEVICE_POWER_ACTIVE == bus.device.power_state);
    TEST_ASSERT(PLATFORM_ERR_OK == bus.device.lifecycle->process(&bus));
    TEST_ASSERT(PLATFORM_ERR_OK == bus.device.lifecycle->stop(&bus));
    TEST_ASSERT(PLATFORM_OBJECT_STOPPED == bus.device.object.state);
    TEST_ASSERT(PLATFORM_ERR_OK == bus.device.lifecycle->deinit(&bus));
    TEST_ASSERT(PLATFORM_OBJECT_CREATED == bus.device.object.state);
    TEST_ASSERT(PLATFORM_DEVICE_POWER_OFF == bus.device.power_state);

    return 0;
}

static int test_initialized_bus_can_deinit_for_start_failure_rollback(void)
{
    platform_spi_bus_t bus = PLATFORM_SPI_BUS_INITIALIZER;

    reset_fake_hal();
    TEST_ASSERT(PLATFORM_ERR_OK ==
                impl_platform_spi1_construct(&bus,
                                             "spi1",
                                             PLATFORM_DEVICE_CAP_NONE));
    TEST_ASSERT(PLATFORM_ERR_OK == bus.device.lifecycle->init(&bus));
    TEST_ASSERT(PLATFORM_OBJECT_INITIALIZED == bus.device.object.state);
    TEST_ASSERT(PLATFORM_ERR_OK == bus.device.lifecycle->deinit(&bus));
    TEST_ASSERT(PLATFORM_OBJECT_CREATED == bus.device.object.state);
    TEST_ASSERT(PLATFORM_DEVICE_POWER_OFF == bus.device.power_state);

    return 0;
}

static int test_apply_config_accepts_actual_mode_order_bits_and_clock(void)
{
    platform_spi_bus_t bus = PLATFORM_SPI_BUS_INITIALIZER;
    platform_spi_device_t device = PLATFORM_SPI_DEVICE_INITIALIZER;
    platform_spi_device_config_t config = make_config();

    reset_fake_hal();
    TEST_ASSERT(0 == construct_and_start(&bus));
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_spi_device_init(&device, "device", &bus, NULL,
                                         PLATFORM_GPIO_LEVEL_LOW, &config));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_spi_transaction_begin(&device));
    TEST_ASSERT(&device == bus.activeDevice);
    TEST_ASSERT(PLATFORM_ERR_OK == platform_spi_transaction_end(&device));

    config.maxClockHz = 12500000U;
    device.config = config;
    TEST_ASSERT(PLATFORM_ERR_OK == platform_spi_transaction_begin(&device));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_spi_transaction_end(&device));

    return 0;
}

static int test_apply_config_rejects_each_fixed_hardware_mismatch(void)
{
    platform_spi_bus_t bus = PLATFORM_SPI_BUS_INITIALIZER;
    platform_spi_device_t device = PLATFORM_SPI_DEVICE_INITIALIZER;
    platform_spi_device_config_t config = make_config();

    reset_fake_hal();
    TEST_ASSERT(0 == construct_and_start(&bus));
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_spi_device_init(&device, "device", &bus, NULL,
                                         PLATFORM_GPIO_LEVEL_LOW, &config));

    device.config.mode = PLATFORM_SPI_MODE_0;
    TEST_ASSERT(PLATFORM_ERR_NOT_SUPPORTED ==
                platform_spi_transaction_begin(&device));
    device.config = config;
    device.config.bitOrder = PLATFORM_SPI_BIT_ORDER_LSB_FIRST;
    TEST_ASSERT(PLATFORM_ERR_NOT_SUPPORTED ==
                platform_spi_transaction_begin(&device));
    device.config = config;
    hspi1.Init.DataSize = SPI_DATASIZE_16BIT;
    TEST_ASSERT(PLATFORM_ERR_NOT_SUPPORTED ==
                platform_spi_transaction_begin(&device));
    hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
    device.config.maxClockHz = 12499999U;
    TEST_ASSERT(PLATFORM_ERR_NOT_SUPPORTED ==
                platform_spi_transaction_begin(&device));
    TEST_ASSERT(NULL == bus.activeDevice);

    return 0;
}

static int test_clock_derivation_covers_all_hal_prescalers(void)
{
    platform_spi_bus_t bus = PLATFORM_SPI_BUS_INITIALIZER;
    platform_spi_device_t device = PLATFORM_SPI_DEVICE_INITIALIZER;
    platform_spi_device_config_t config = make_config();
    uint32_t prescalers[] = {
        SPI_BAUDRATEPRESCALER_2,
        SPI_BAUDRATEPRESCALER_4,
        SPI_BAUDRATEPRESCALER_8,
        SPI_BAUDRATEPRESCALER_16,
        SPI_BAUDRATEPRESCALER_32,
        SPI_BAUDRATEPRESCALER_64,
        SPI_BAUDRATEPRESCALER_128,
        SPI_BAUDRATEPRESCALER_256
    };
    uint32_t divisors[] = {2U, 4U, 8U, 16U, 32U, 64U, 128U, 256U};
    platform_size_t index = 0U;

    reset_fake_hal();
    TEST_ASSERT(0 == construct_and_start(&bus));
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_spi_device_init(&device, "device", &bus, NULL,
                                         PLATFORM_GPIO_LEVEL_LOW, &config));

    for (index = 0U; index < ARRAY_SIZE(prescalers); index++) {
        hspi1.Init.BaudRatePrescaler = prescalers[index];
        device.config.maxClockHz = g_fakePclk2Hz / divisors[index];
        TEST_ASSERT(PLATFORM_ERR_OK == platform_spi_transaction_begin(&device));
        TEST_ASSERT(PLATFORM_ERR_OK == platform_spi_transaction_end(&device));
    }

    hspi1.Init.BaudRatePrescaler = 0xFFFFFFFFU;
    TEST_ASSERT(PLATFORM_ERR_NOT_SUPPORTED ==
                platform_spi_transaction_begin(&device));

    return 0;
}

static int test_write_uses_finite_timeout_and_maps_hal_status(void)
{
    platform_spi_bus_t bus = PLATFORM_SPI_BUS_INITIALIZER;
    platform_spi_device_t device = PLATFORM_SPI_DEVICE_INITIALIZER;
    platform_spi_device_config_t config = make_config();
    uint8_t data[3] = {0x11U, 0x22U, 0x33U};
    HAL_StatusTypeDef halStatuses[] = {HAL_OK, HAL_BUSY, HAL_TIMEOUT, HAL_ERROR};
    platform_error_t expected[] = {
        PLATFORM_ERR_OK,
        PLATFORM_ERR_BUSY,
        PLATFORM_ERR_TIMEOUT,
        PLATFORM_ERR_IO
    };
    platform_size_t index = 0U;

    reset_fake_hal();
    TEST_ASSERT(0 == construct_and_start(&bus));
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_spi_device_init(&device, "device", &bus, NULL,
                                         PLATFORM_GPIO_LEVEL_LOW, &config));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_spi_transaction_begin(&device));

    for (index = 0U; index < ARRAY_SIZE(halStatuses); index++) {
        g_fakeHalSpiTransmitResult = halStatuses[index];
        TEST_ASSERT(expected[index] ==
                    platform_spi_write(&device, data, sizeof(data)));
        TEST_ASSERT(&hspi1 == g_fakeHalSpiTransmitHandle);
        TEST_ASSERT(data == g_fakeHalSpiTransmitData);
        TEST_ASSERT(sizeof(data) == g_fakeHalSpiTransmitLength);
        TEST_ASSERT(0U < g_fakeHalSpiTransmitTimeoutMs);
        TEST_ASSERT(0xFFFFFFFFU != g_fakeHalSpiTransmitTimeoutMs);
        TEST_ASSERT(&device == bus.activeDevice);
    }

    TEST_ASSERT(PLATFORM_ERR_OK == platform_spi_transaction_end(&device));

    return 0;
}

static int test_write_rejects_hal_length_overflow(void)
{
    platform_spi_bus_t bus = PLATFORM_SPI_BUS_INITIALIZER;
    platform_spi_device_t device = PLATFORM_SPI_DEVICE_INITIALIZER;
    platform_spi_device_config_t config = make_config();
    uint8_t data = 0U;

    reset_fake_hal();
    TEST_ASSERT(0 == construct_and_start(&bus));
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_spi_device_init(&device, "device", &bus, NULL,
                                         PLATFORM_GPIO_LEVEL_LOW, &config));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_spi_transaction_begin(&device));
    TEST_ASSERT(PLATFORM_ERR_OVERFLOW ==
                platform_spi_write(&device, &data, 0x10000U));
    TEST_ASSERT(0U == g_fakeHalSpiTransmitCount);
    TEST_ASSERT(PLATFORM_ERR_OK == platform_spi_transaction_end(&device));

    return 0;
}

static int test_stop_and_deinit_reject_active_transaction(void)
{
    platform_spi_bus_t bus = PLATFORM_SPI_BUS_INITIALIZER;
    platform_spi_device_t device = PLATFORM_SPI_DEVICE_INITIALIZER;
    platform_spi_device_config_t config = make_config();

    reset_fake_hal();
    TEST_ASSERT(0 == construct_and_start(&bus));
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_spi_device_init(&device, "device", &bus, NULL,
                                         PLATFORM_GPIO_LEVEL_LOW, &config));
    TEST_ASSERT(PLATFORM_ERR_OK == platform_spi_transaction_begin(&device));
    TEST_ASSERT(PLATFORM_ERR_BUSY == bus.device.lifecycle->stop(&bus));
    bus.device.object.state = PLATFORM_OBJECT_STOPPED;
    TEST_ASSERT(PLATFORM_ERR_BUSY == bus.device.lifecycle->deinit(&bus));
    bus.device.object.state = PLATFORM_OBJECT_STARTED;
    TEST_ASSERT(PLATFORM_ERR_OK == platform_spi_transaction_end(&device));

    return 0;
}

int main(void)
{
    int result = test_construct_and_lifecycle_follow_platform_state_model();

    if (result != 0) {
        return result;
    }
    result = test_initialized_bus_can_deinit_for_start_failure_rollback();
    if (result != 0) {
        return result;
    }
    result = test_apply_config_accepts_actual_mode_order_bits_and_clock();
    if (result != 0) {
        return result;
    }
    result = test_apply_config_rejects_each_fixed_hardware_mismatch();
    if (result != 0) {
        return result;
    }
    result = test_clock_derivation_covers_all_hal_prescalers();
    if (result != 0) {
        return result;
    }
    result = test_write_uses_finite_timeout_and_maps_hal_status();
    if (result != 0) {
        return result;
    }
    result = test_write_rejects_hal_length_overflow();
    if (result != 0) {
        return result;
    }

    return test_stop_and_deinit_reject_active_transaction();
}
