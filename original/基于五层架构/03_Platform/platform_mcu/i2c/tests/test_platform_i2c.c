/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file test_platform_i2c.c
 * @brief Platform I2C Contract Host Test
 * @author YaoQian Wang
 * @date 2026-09-27
 * @version V2.0
 *
 *****************************************************************************/

#include "platform_i2c.h"

#include <stddef.h>
#include <string.h>

#define TEST_ASSERT(condition)     do { if (!(condition)) { return __LINE__; } } while (0)

typedef struct
{
    uint32_t initCount;
    uint32_t probeCount;
    uint32_t writeCount;
    uint32_t readCount;
    uint32_t writeReadCount;
    uint32_t deinitCount;
    platform_error_t initResult;
    platform_error_t operationResult;
    platform_error_t deinitResult;
} fake_i2c_context_t;

static fake_i2c_context_t *fake_context(platform_i2c_t *i2c)
{
    return (fake_i2c_context_t *)i2c->implContext;
}

static platform_error_t fake_init(platform_i2c_t *i2c)
{
    fake_i2c_context_t *context = fake_context(i2c);
    context->initCount++;
    return context->initResult;
}

static platform_error_t fake_probe(platform_i2c_t *i2c, uint8_t address)
{
    fake_i2c_context_t *context = fake_context(i2c);
    (void)address;
    context->probeCount++;
    return context->operationResult;
}

static platform_error_t fake_write(
    platform_i2c_t *i2c,
    uint8_t address,
    const uint8_t *data,
    uint16_t length)
{
    fake_i2c_context_t *context = fake_context(i2c);
    (void)address;
    (void)data;
    (void)length;
    context->writeCount++;
    return context->operationResult;
}

static platform_error_t fake_read(
    platform_i2c_t *i2c,
    uint8_t address,
    uint8_t *data,
    uint16_t length)
{
    fake_i2c_context_t *context = fake_context(i2c);
    (void)address;
    (void)data;
    (void)length;
    context->readCount++;
    return context->operationResult;
}

static platform_error_t fake_write_read(
    platform_i2c_t *i2c,
    uint8_t address,
    const uint8_t *txData,
    uint16_t txLength,
    uint8_t *rxData,
    uint16_t rxLength)
{
    fake_i2c_context_t *context = fake_context(i2c);
    (void)address;
    (void)txData;
    (void)txLength;
    (void)rxData;
    (void)rxLength;
    context->writeReadCount++;
    return context->operationResult;
}

static platform_error_t fake_deinit(platform_i2c_t *i2c)
{
    fake_i2c_context_t *context = fake_context(i2c);
    context->deinitCount++;
    return context->deinitResult;
}

static const platform_i2c_ops_t g_fakeOps = {
    fake_init,
    fake_probe,
    fake_write,
    fake_read,
    fake_write_read,
    fake_deinit
};

static int init_fixture(
    platform_i2c_t *i2c,
    fake_i2c_context_t *context)
{
    platform_i2c_init_params_t params;

    (void)memset(i2c, 0, sizeof(*i2c));
    (void)memset(context, 0, sizeof(*context));
    context->initResult = PLATFORM_ERR_OK;
    context->operationResult = PLATFORM_ERR_OK;
    context->deinitResult = PLATFORM_ERR_OK;

    params.name = "fake_i2c";
    params.ops = &g_fakeOps;
    params.implContext = context;

    TEST_ASSERT(PLATFORM_ERR_OK == platform_i2c_init(i2c, &params));
    TEST_ASSERT(1U == context->initCount);
    TEST_ASSERT(i2c->initialized == PLATFORM_TRUE);
    return 0;
}

static int test_common_validation_and_dispatch(void)
{
    platform_i2c_t i2c;
    fake_i2c_context_t context;
    uint8_t tx = 0x5AU;
    uint8_t rx = 0U;
    int result = init_fixture(&i2c, &context);

    if (result != 0) {
        return result;
    }

    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                platform_i2c_probe(&i2c, 0x80U));
    TEST_ASSERT(0U == context.probeCount);

    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                platform_i2c_write(&i2c, 0x38U, NULL, 1U));
    TEST_ASSERT(PLATFORM_ERR_INVALID_PARAM ==
                platform_i2c_write(&i2c, 0x38U, &tx, 0U));
    TEST_ASSERT(0U == context.writeCount);

    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_probe(&i2c, 0x38U));
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_write(&i2c, 0x38U, &tx, 1U));
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_read(&i2c, 0x38U, &rx, 1U));
    TEST_ASSERT(PLATFORM_ERR_OK ==
                platform_i2c_write_read(
                    &i2c, 0x38U, &tx, 1U, &rx, 1U));

    TEST_ASSERT(1U == context.probeCount);
    TEST_ASSERT(1U == context.writeCount);
    TEST_ASSERT(1U == context.readCount);
    TEST_ASSERT(1U == context.writeReadCount);

    return 0;
}

static int test_init_failure_clears_binding(void)
{
    platform_i2c_t i2c = PLATFORM_I2C_INITIALIZER;
    fake_i2c_context_t context = {0};
    platform_i2c_init_params_t params;

    context.initResult = PLATFORM_ERR_IO;
    params.name = "fail_i2c";
    params.ops = &g_fakeOps;
    params.implContext = &context;

    TEST_ASSERT(PLATFORM_ERR_IO == platform_i2c_init(&i2c, &params));
    TEST_ASSERT(i2c.initialized == PLATFORM_FALSE);
    TEST_ASSERT(i2c.ops == NULL);
    TEST_ASSERT(i2c.implContext == NULL);
    return 0;
}

static int test_deinit_clears_binding_even_on_backend_error(void)
{
    platform_i2c_t i2c;
    fake_i2c_context_t context;
    int result = init_fixture(&i2c, &context);

    if (result != 0) {
        return result;
    }

    context.deinitResult = PLATFORM_ERR_TIMEOUT;
    TEST_ASSERT(PLATFORM_ERR_TIMEOUT == platform_i2c_deinit(&i2c));
    TEST_ASSERT(1U == context.deinitCount);
    TEST_ASSERT(i2c.initialized == PLATFORM_FALSE);
    TEST_ASSERT(i2c.ops == NULL);
    TEST_ASSERT(i2c.implContext == NULL);
    return 0;
}

int main(void)
{
    int result = test_common_validation_and_dispatch();

    if (result != 0) {
        return result;
    }

    result = test_init_failure_clears_binding();
    if (result != 0) {
        return result;
    }

    return test_deinit_clears_binding_even_on_backend_error();
}
