/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <aws/cryptosdk/cipher.h>
#include <aws/cryptosdk/private/cipher.h>
#include <make_common_data_structures.h>

#define MSG_ID_LEN 16

void aws_cryptosdk_aes_gcm_encrypt_harness() {
    struct aws_byte_buf cipher;
    struct aws_byte_buf tag;
    struct aws_byte_cursor plain;
    struct aws_byte_cursor iv;
    struct aws_byte_cursor aad;
    struct aws_string *key;

    __CPROVER_assume(aws_byte_buf_is_bounded(&cipher, MAX_BUFFER_SIZE));
    ensure_byte_buf_has_allocated_buffer_member(&cipher);
    __CPROVER_assume(aws_byte_buf_is_valid(&cipher));

    __CPROVER_assume(aws_byte_buf_is_bounded(&tag, MAX_BUFFER_SIZE));
    ensure_byte_buf_has_allocated_buffer_member(&tag);
    __CPROVER_assume(aws_byte_buf_is_valid(&tag));

    __CPROVER_assume(aws_byte_cursor_is_bounded(&plain, MAX_BUFFER_SIZE));
    ensure_byte_cursor_has_allocated_buffer_member(&plain);
    __CPROVER_assume(aws_byte_cursor_is_valid(&plain));

    __CPROVER_assume(aws_byte_cursor_is_bounded(&iv, MAX_BUFFER_SIZE));
    ensure_byte_cursor_has_allocated_buffer_member(&iv);
    __CPROVER_assume(aws_byte_cursor_is_valid(&iv));

    __CPROVER_assume(aws_byte_cursor_is_bounded(&aad, MAX_BUFFER_SIZE));
    ensure_byte_cursor_has_allocated_buffer_member(&aad);
    __CPROVER_assume(aws_byte_cursor_is_valid(&aad));

    key = ensure_string_is_allocated_bounded_length(256);

    aws_cryptosdk_aes_gcm_encrypt(&cipher, &tag, plain, iv, aad, key);
}
