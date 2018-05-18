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
#include <aws/cryptosdk/zero_mk.h>

/**
 * A degenerate Master Key which always returns an all zero data key, just for
 * testing the CMM/MKP/MK infrastructure.
 */

struct zero_mk {
    const struct aws_cryptosdk_mk_vt * vt;
    struct aws_allocator * alloc;
};

static int zero_mk_generate_data_key(struct aws_cryptosdk_mk * mk,
                                     struct aws_cryptosdk_data_key * unencrypted_data_key,
                                     struct aws_cryptosdk_encrypted_data_key * encrypted_data_key,
                                     struct aws_hash_table * enc_context) {
    memset(unencrypted_data_key->keybuf, 0x88, MAX_DATA_KEY_SIZE);
    //aws_cryptosdk_secure_zero(unencrypted_data_key->keybuf, MAX_DATA_KEY_SIZE);

    // normally encrypted data key would be written here too, but it is ignored by this MK
    return AWS_OP_SUCCESS;
}

static int zero_mk_encrypt_data_key(struct aws_cryptosdk_mk * mk,
                                    struct aws_cryptosdk_encrypted_data_key * encrypted_data_key,
                                    const struct aws_cryptosdk_data_key * unencrypted_data_key,
                                    struct aws_hash_table * enc_context) {

    return AWS_OP_SUCCESS;
}

static void zero_mk_destroy(struct aws_cryptosdk_mk * mk) {
    struct zero_mk * self = (struct zero_mk *) mk;
    self->alloc->mem_release(self->alloc, self);
}

static const struct aws_cryptosdk_mk_vt zero_mk_vt = {
    sizeof(struct aws_cryptosdk_mk_vt),
    "zero mk",
    zero_mk_destroy,
    zero_mk_generate_data_key,
    zero_mk_encrypt_data_key
};

struct aws_cryptosdk_mk * aws_cryptosdk_zero_mk_new(struct aws_allocator * alloc) {
    struct zero_mk * mk = alloc->mem_acquire(alloc, sizeof(struct zero_mk));
    if (!mk) {
        aws_raise_error(AWS_ERROR_OOM);
        return NULL;
    }
    mk->vt = &zero_mk_vt;
    mk->alloc = alloc;
    return (struct aws_cryptosdk_mk *) mk;
}
