# Copyright (C) 2018-2024 Intel Corporation
# SPDX-License-Identifier: Apache-2.0
#

add_subdirectory(logger)
if(ENABLE_ZEROAPI_BACKEND)
    add_subdirectory(zero)
endif()
