#-------------------------------------------------------------------------------
# Copyright (c) 2022, Arm Limited. All rights reserved.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#-------------------------------------------------------------------------------

add_library(psa_adac_sdm STATIC)

target_include_directories(psa_adac_sdm
    PUBLIC
        ${PSA_ADAC_ROOT}/psa-adac/sdm/include
)

list(APPEND PSA_ADAC_SDM_SRC
    ${PSA_ADAC_ROOT}/psa-adac/sdm/src/psa_adac_sdm.c
    ${PSA_ADAC_ROOT}/psa-adac/sdm/src/sdm_token.c
)

target_sources(psa_adac_sdm
    PUBLIC
        ${PSA_ADAC_SDM_SRC}
)

target_link_libraries(psa_adac_sdm
    PUBLIC
        psa_adac_core
)
