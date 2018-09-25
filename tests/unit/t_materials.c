/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"). You may not use
 * this file except in compliance with the License. A copy of the License is
 * located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <aws/cryptosdk/materials.h>
#include <aws/cryptosdk/default_cmm.h>
#include "testing.h"
#include "zero_keyring.h"
#include "bad_cmm.h"
#include "test_keyring.h"

int default_cmm_zero_keyring_enc_mat() {
    struct aws_hash_table enc_context;
    struct aws_allocator * alloc = aws_default_allocator();
    struct aws_cryptosdk_keyring * kr = aws_cryptosdk_zero_keyring_new(alloc);
    struct aws_cryptosdk_cmm * cmm = aws_cryptosdk_default_cmm_new(alloc, kr);

    struct aws_cryptosdk_encryption_request req;
    req.enc_context = &enc_context; // this is uninitialized; we just want to see if it gets passed along
    req.requested_alg = AES_256_GCM_IV12_AUTH16_KDNONE_SIGNONE;
    req.alloc = aws_default_allocator();

    struct aws_cryptosdk_encryption_materials * enc_mat;
    TEST_ASSERT_INT_EQ(AWS_OP_SUCCESS,
                       aws_cryptosdk_cmm_generate_encryption_materials(cmm, &enc_mat, &req));

    TEST_ASSERT_ADDR_EQ(enc_mat->enc_context, &enc_context);
    TEST_ASSERT_INT_EQ(enc_mat->alg, AES_256_GCM_IV12_AUTH16_KDNONE_SIGNONE);

    TEST_ASSERT_BUF_EQ(enc_mat->unencrypted_data_key,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

    TEST_ASSERT_INT_EQ(enc_mat->encrypted_data_keys.length, 1);
    struct aws_cryptosdk_edk * edk;
    TEST_ASSERT_INT_EQ(AWS_OP_SUCCESS,
                       aws_array_list_get_at_ptr(&enc_mat->encrypted_data_keys, (void **)&edk, 0));

    TEST_ASSERT_BUF_EQ(edk->enc_data_key, 'n', 'u', 'l', 'l');
    TEST_ASSERT_BUF_EQ(edk->provider_id, 'n', 'u', 'l', 'l');
    TEST_ASSERT_BUF_EQ(edk->provider_info, 'n', 'u', 'l', 'l');

    aws_cryptosdk_encryption_materials_destroy(enc_mat);
    aws_cryptosdk_cmm_destroy(cmm);
    aws_cryptosdk_keyring_destroy(kr);

    return 0;
}

int default_cmm_zero_keyring_dec_mat() {
    struct aws_allocator * alloc = aws_default_allocator();
    struct aws_cryptosdk_keyring * kr = aws_cryptosdk_zero_keyring_new(alloc);
    struct aws_cryptosdk_cmm * cmm = aws_cryptosdk_default_cmm_new(alloc, kr);

    struct aws_cryptosdk_decryption_request req;
    req.alg = AES_192_GCM_IV12_AUTH16_KDNONE_SIGNONE;
    req.alloc = aws_default_allocator();

    TEST_ASSERT_SUCCESS(aws_cryptosdk_edk_list_init(alloc, &req.encrypted_data_keys));
    struct aws_cryptosdk_edk edk;
    aws_cryptosdk_literally_null_edk(&edk);

    TEST_ASSERT_SUCCESS(aws_array_list_push_back(&req.encrypted_data_keys, (void *) &edk));

    struct aws_cryptosdk_decryption_materials * dec_mat;
    TEST_ASSERT_INT_EQ(AWS_OP_SUCCESS, aws_cryptosdk_cmm_decrypt_materials(cmm, &dec_mat, &req));

    TEST_ASSERT_BUF_EQ(dec_mat->unencrypted_data_key,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

    aws_cryptosdk_decryption_materials_destroy(dec_mat);
    aws_cryptosdk_cmm_destroy(cmm);
    aws_cryptosdk_keyring_destroy(kr);
    aws_array_list_clean_up(&req.encrypted_data_keys);
    return 0;
}

int zero_size_cmm_does_not_run_vfs() {
    struct aws_cryptosdk_cmm * cmm = aws_cryptosdk_zero_size_cmm_new();
    TEST_ASSERT_ERROR(AWS_ERROR_UNIMPLEMENTED,
                      aws_cryptosdk_cmm_generate_encryption_materials(cmm, NULL, NULL));

    TEST_ASSERT_ERROR(AWS_ERROR_UNIMPLEMENTED,
                      aws_cryptosdk_cmm_decrypt_materials(cmm, NULL, NULL));

    TEST_ASSERT_ERROR(AWS_ERROR_UNIMPLEMENTED,
                      aws_cryptosdk_cmm_destroy_with_failed_return_value(cmm));

    return 0;
}

int null_cmm_fails_vf_calls_cleanly() {
    struct aws_cryptosdk_cmm * cmm = aws_cryptosdk_null_cmm_new();
    TEST_ASSERT_ERROR(AWS_ERROR_UNIMPLEMENTED,
                      aws_cryptosdk_cmm_generate_encryption_materials(cmm, NULL, NULL));

    TEST_ASSERT_ERROR(AWS_ERROR_UNIMPLEMENTED,
                      aws_cryptosdk_cmm_decrypt_materials(cmm, NULL, NULL));

    TEST_ASSERT_ERROR(AWS_ERROR_UNIMPLEMENTED,
                      aws_cryptosdk_cmm_destroy_with_failed_return_value(cmm));
    return 0;
}

int null_materials_destroy_is_noop() {
    aws_cryptosdk_cmm_destroy(NULL);
    aws_cryptosdk_keyring_destroy(NULL);

    return 0;
}

static struct test_keyring test_kr;
static struct aws_cryptosdk_keyring *kr;

static void reset_test_keyring() {
    memset(&test_kr, 0, sizeof(test_kr));
    test_kr.vt = &test_keyring_vt;
    kr = (struct aws_cryptosdk_keyring *)&test_kr;
}

int on_encrypt_precondition_violation() {
    /* No data key but at least one EDK -> raise error and do not make virtual call */
    reset_test_keyring();

    struct aws_byte_buf unencrypted_data_key = {0};
    struct aws_array_list edks;
    struct aws_cryptosdk_edk edk = {0};
    TEST_ASSERT_SUCCESS(aws_cryptosdk_edk_list_init(aws_default_allocator(), &edks));
    TEST_ASSERT_SUCCESS(aws_array_list_push_back(&edks, &edk));

    TEST_ASSERT_ERROR(AWS_CRYPTOSDK_ERR_BAD_STATE,
                      aws_cryptosdk_keyring_on_encrypt(kr, &unencrypted_data_key, &edks, NULL, 0));

    TEST_ASSERT(!test_kr.on_encrypt_called);

    aws_cryptosdk_edk_list_clean_up(&edks);
    return 0;
}

int on_encrypt_postcondition_violation() {
    /* Generate data key of wrong length -> raise error after virtual call */
    reset_test_keyring();
    test_kr.generated_data_key_to_return = aws_byte_buf_from_c_str("wrong data key length");

    struct aws_byte_buf unencrypted_data_key = {0};
    struct aws_array_list edks;
    TEST_ASSERT_SUCCESS(aws_cryptosdk_edk_list_init(aws_default_allocator(), &edks));

    TEST_ASSERT_ERROR(AWS_CRYPTOSDK_ERR_BAD_STATE,
                      aws_cryptosdk_keyring_on_encrypt(kr, &unencrypted_data_key, &edks, NULL,
                                                       AES_256_GCM_IV12_AUTH16_KDSHA384_SIGEC384));

    TEST_ASSERT(test_kr.on_encrypt_called);

    aws_cryptosdk_edk_list_clean_up(&edks);
    return 0;
}

int on_decrypt_precondition_violation() {
    /* Unencrypted data key buffer already set -> raise error and do not make virtual call */
    reset_test_keyring();

    struct aws_byte_buf unencrypted_data_key = aws_byte_buf_from_c_str("Oops, already set!");
    TEST_ASSERT_ERROR(AWS_CRYPTOSDK_ERR_BAD_STATE,
                      aws_cryptosdk_keyring_on_decrypt(kr, &unencrypted_data_key, NULL, NULL, 0));

    TEST_ASSERT(!test_kr.on_decrypt_called);
    return 0;
}

int on_decrypt_postcondition_violation() {
    /* Decrypt data key of wrong length -> raise error after virtual call */
    reset_test_keyring();

    struct aws_byte_buf unencrypted_data_key = {0};
    struct aws_array_list edks;
    TEST_ASSERT_SUCCESS(aws_cryptosdk_edk_list_init(aws_default_allocator(), &edks));

    test_kr.decrypted_data_key_to_return = aws_byte_buf_from_c_str("wrong data key length");

    TEST_ASSERT_ERROR(AWS_CRYPTOSDK_ERR_BAD_CIPHERTEXT,
                      aws_cryptosdk_keyring_on_decrypt(kr, &unencrypted_data_key, &edks, NULL,
                                                       AES_256_GCM_IV12_AUTH16_KDSHA384_SIGEC384));

    aws_cryptosdk_edk_list_clean_up(&edks);
    return 0;
}

struct test_case materials_test_cases[] = {
    { "materials", "default_cmm_zero_keyring_enc_mat", default_cmm_zero_keyring_enc_mat },
    { "materials", "default_cmm_zero_keyring_dec_mat", default_cmm_zero_keyring_dec_mat },
    { "materials", "zero_size_cmm_does_not_run_vfs", zero_size_cmm_does_not_run_vfs },
    { "materials", "null_cmm_fails_vf_calls_cleanly", null_cmm_fails_vf_calls_cleanly },
    { "materials", "null_materials_destroy_is_noop", null_materials_destroy_is_noop },
    { "materials", "on_encrypt_precondition_violation", on_encrypt_precondition_violation },
    { "materials", "on_encrypt_postcondition_violation", on_encrypt_postcondition_violation },
    { "materials", "on_decrypt_precondition_violation", on_decrypt_precondition_violation },
    { "materials", "on_decrypt_postcondition_violation", on_decrypt_postcondition_violation },
    { NULL }
};
