/******************************************************************************
 * Copyright (C) 2026 YaoQian Wang
 *
 * All Rights Reserved.
 *
 * @file firmware_version.c
 * @brief Firmware Version V1 实现
 * @author YaoQian Wang
 * @date 2026-09-13
 * @version V1.0
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include <stddef.h>

#include "firmware_version.h"
//******************************** Includes *********************************//

int32_t firmware_version_compare(const firmware_version_t *left, const firmware_version_t *right)
{
    if (left == NULL) {
        return (right == NULL) ? 0 : -1;
    }

    if (right == NULL) {
        return 1;
    }

    if (left->major != right->major) {
        return (left->major > right->major) ? 1 : -1;
    }

    if (left->minor != right->minor) {
        return (left->minor > right->minor) ? 1 : -1;
    }

    if (left->patch != right->patch) {
        return (left->patch > right->patch) ? 1 : -1;
    }

    return 0;
}

platform_bool_t firmware_version_is_valid(const firmware_version_t *version)
{
    return ((version != NULL) && (version->reserved == 0U)) ? (platform_bool_t)1U : (platform_bool_t)0U;
}
