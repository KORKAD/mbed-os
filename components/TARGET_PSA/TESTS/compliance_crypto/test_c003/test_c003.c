/** @file
 * Copyright (c) 2018-2019, Arm Limited or its affiliates. All rights reserved.
 * SPDX-License-Identifier : Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
**/

#include "val_interfaces.h"
#include "val_target.h"
#include "test_c003.h"
#include "test_data.h"

client_test_t test_c003_crypto_list[] = {
    NULL,
    psa_export_key_test,
    psa_export_key_negative_test,
    NULL,
};

static int      g_test_count = 1;
static uint8_t  data[BUFFER_SIZE];

int32_t psa_export_key_test(security_t caller)
{
    uint32_t         length, i;
    const uint8_t    *key_data;
    psa_key_policy_t policy;
    psa_key_type_t   key_type;
    size_t           bits;
    int              num_checks = sizeof(check1)/sizeof(check1[0]);
    int32_t          status;

    /* Initialize the PSA crypto library*/
    status = val->crypto_function(VAL_CRYPTO_INIT);
    TEST_ASSERT_EQUAL(status, PSA_SUCCESS, TEST_CHECKPOINT_NUM(1));

    /* Set the key data buffer to the input base on algorithm */
    for (i = 0; i < num_checks; i++)
    {
        val->print(PRINT_TEST, "[Check %d] ", g_test_count++);
        val->print(PRINT_TEST, check1[i].test_desc, 0);

        /* Initialize a key policy structure to a default that forbids all
         * usage of the key
         */
        val->crypto_function(VAL_CRYPTO_KEY_POLICY_INIT, &policy);

        /* Setting up the watchdog timer for each check */
        status = val->wd_reprogram_timer(WD_CRYPTO_TIMEOUT);
        TEST_ASSERT_EQUAL(status, VAL_STATUS_SUCCESS, TEST_CHECKPOINT_NUM(2));

        if (PSA_KEY_TYPE_IS_RSA(check1[i].key_type))
        {
            if (check1[i].key_type == PSA_KEY_TYPE_RSA_KEYPAIR)
            {
                if (check1[i].expected_bit_length == BYTES_TO_BITS(384))
                    key_data = rsa_384_keypair;
                else if (check1[i].expected_bit_length == BYTES_TO_BITS(256))
                    key_data = rsa_256_keypair;
                else
                    return VAL_STATUS_INVALID;
            }
            else
            {
                if (check1[i].expected_bit_length == BYTES_TO_BITS(384))
                    key_data = rsa_384_keydata;
                else if (check1[i].expected_bit_length == BYTES_TO_BITS(256))
                    key_data = rsa_256_keydata;
                else
                    return VAL_STATUS_INVALID;
            }
        }
        else if (PSA_KEY_TYPE_IS_ECC(check1[i].key_type))
        {
            if (PSA_KEY_TYPE_IS_ECC_KEYPAIR(check1[i].key_type))
                key_data = ec_keypair;
            else
                key_data = ec_keydata;
        }
        else
            key_data = check1[i].key_data;

        /* Set the standard fields of a policy structure */
        val->crypto_function(VAL_CRYPTO_KEY_POLICY_SET_USAGE, &policy, check1[i].usage,
                                                                          check1[i].key_alg);

        /* Allocate a key slot for a transient key */
        status = val->crypto_function(VAL_CRYPTO_ALLOCATE_KEY, &check1[i].key_handle);
        TEST_ASSERT_EQUAL(status, PSA_SUCCESS, TEST_CHECKPOINT_NUM(3));

        /* Set the usage policy on a key slot */
        status = val->crypto_function(VAL_CRYPTO_SET_KEY_POLICY, check1[i].key_handle, &policy);
        TEST_ASSERT_EQUAL(status, PSA_SUCCESS, TEST_CHECKPOINT_NUM(4));

        /* Import the key data into the key slot */
        status = val->crypto_function(VAL_CRYPTO_IMPORT_KEY, check1[i].key_handle,
                                            check1[i].key_type, key_data, check1[i].key_length);
        TEST_ASSERT_EQUAL(status, PSA_SUCCESS, TEST_CHECKPOINT_NUM(5));

        /* Get basic metadata about a key */
        status = val->crypto_function(VAL_CRYPTO_GET_KEY_INFORMATION, check1[i].key_handle,
                                       &key_type, &bits);
        TEST_ASSERT_EQUAL(status, PSA_SUCCESS, TEST_CHECKPOINT_NUM(6));

        TEST_ASSERT_EQUAL(key_type, check1[i].key_type, TEST_CHECKPOINT_NUM(7));

        TEST_ASSERT_EQUAL(bits, check1[i].expected_bit_length, TEST_CHECKPOINT_NUM(8));

        /* Export a key in binary format */
        status = val->crypto_function(VAL_CRYPTO_EXPORT_KEY, check1[i].key_handle, data,
                                      check1[i].buffer_size, &length);
        TEST_ASSERT_EQUAL(status, check1[i].expected_status, TEST_CHECKPOINT_NUM(9));

        if (check1[i].expected_status != PSA_SUCCESS)
            continue;

        TEST_ASSERT_EQUAL(length, check1[i].expected_key_length, TEST_CHECKPOINT_NUM(10));

        /* Check if original key data matches with the exported data */
        if (PSA_KEY_TYPE_IS_UNSTRUCTURED(check1[i].key_type))
        {
            TEST_ASSERT_MEMCMP(check1[i].key_data, data, length, TEST_CHECKPOINT_NUM(11));
        }
        else if (PSA_KEY_TYPE_IS_RSA(check1[i].key_type) || PSA_KEY_TYPE_IS_ECC(check1[i].key_type))
        {
            TEST_ASSERT_MEMCMP(key_data, data, length, TEST_CHECKPOINT_NUM(12));
        }
        else
        {
            return VAL_STATUS_INVALID;
        }
    }

    return VAL_STATUS_SUCCESS;
}

int32_t psa_export_key_negative_test(security_t caller)
{
    int              num_checks = sizeof(check2)/sizeof(check2[0]);
    uint32_t         i, length;
    int32_t          status;
    psa_key_policy_t policy;

    /* Initialize the PSA crypto library*/
    status = val->crypto_function(VAL_CRYPTO_INIT);
    TEST_ASSERT_EQUAL(status, PSA_SUCCESS, TEST_CHECKPOINT_NUM(1));

    for (i = 0; i < num_checks; i++)
    {
        /* Setting up the watchdog timer for each check */
        status = val->wd_reprogram_timer(WD_CRYPTO_TIMEOUT);
        TEST_ASSERT_EQUAL(status, VAL_STATUS_SUCCESS, TEST_CHECKPOINT_NUM(2));

        val->print(PRINT_TEST, "[Check %d] Test psa_export_key with unallocated key handle\n",
                                                                                 g_test_count++);
        /* Export a key in binary format */
        status = val->crypto_function(VAL_CRYPTO_EXPORT_KEY, check2[i].key_handle, data,
                                       check2[i].key_length, &length);
        TEST_ASSERT_EQUAL(status, PSA_ERROR_INVALID_HANDLE, TEST_CHECKPOINT_NUM(3));

        val->print(PRINT_TEST, "[Check %d] Test psa_export_key with empty key handle\n",
                                                                                 g_test_count++);
        /* Allocate a key slot for a transient key */
        status = val->crypto_function(VAL_CRYPTO_ALLOCATE_KEY, &check2[i].key_handle);
        TEST_ASSERT_EQUAL(status, PSA_SUCCESS, TEST_CHECKPOINT_NUM(4));

        /* Export a key in binary format */
        status = val->crypto_function(VAL_CRYPTO_EXPORT_KEY, check2[i].key_handle, data,
                                       check2[i].key_length, &length);
        TEST_ASSERT_EQUAL(status, PSA_ERROR_EMPTY_SLOT, TEST_CHECKPOINT_NUM(5));

        val->print(PRINT_TEST, "[Check %d] Test psa_export_key with zero as key handle\n",
                                                                                 g_test_count++);
        /* Export a key in binary format */
        status = val->crypto_function(VAL_CRYPTO_EXPORT_KEY, 0, data,
                                       check2[i].key_length, &length);
        TEST_ASSERT_EQUAL(status, PSA_ERROR_INVALID_HANDLE, TEST_CHECKPOINT_NUM(6));

        val->print(PRINT_TEST, "[Check %d] Test psa_export_key with destroyed key handle\n",
                                                                                 g_test_count++);
        /* Initialize a key policy structure to a default that forbids all
         * usage of the key
         */
        val->crypto_function(VAL_CRYPTO_KEY_POLICY_INIT, &policy);

        /* Set the standard fields of a policy structure */
        val->crypto_function(VAL_CRYPTO_KEY_POLICY_SET_USAGE, &policy, check2[i].usage,
                                                                          check2[i].key_alg);

        /* Set the usage policy on a key slot */
        status = val->crypto_function(VAL_CRYPTO_SET_KEY_POLICY, check2[i].key_handle, &policy);
        TEST_ASSERT_EQUAL(status, PSA_SUCCESS, TEST_CHECKPOINT_NUM(7));

        /* Import the key data into the key slot */
        status = val->crypto_function(VAL_CRYPTO_IMPORT_KEY, check2[i].key_handle,
                                     check2[i].key_type, check2[i].key_data, check2[i].key_length);
        TEST_ASSERT_EQUAL(status, PSA_SUCCESS, TEST_CHECKPOINT_NUM(8));

        /* Destroy the key handle */
        status = val->crypto_function(VAL_CRYPTO_DESTROY_KEY, check2[i].key_handle);
        TEST_ASSERT_EQUAL(status, PSA_SUCCESS, TEST_CHECKPOINT_NUM(9));

        /* Export a key in binary format */
        status = val->crypto_function(VAL_CRYPTO_EXPORT_KEY, check2[i].key_handle, data,
                                       check2[i].key_length, &length);
        TEST_ASSERT_EQUAL(status, PSA_ERROR_INVALID_HANDLE, TEST_CHECKPOINT_NUM(10));
    }

    return VAL_STATUS_SUCCESS;
}
