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
#include <aws/cryptosdk/default_cmm.h>
#include <aws/cryptosdk/cipher.h>
#include <aws/common/string.h>

#include <assert.h>

#define DEFAULT_ALG AES_128_GCM_IV12_AUTH16_KDSHA256_SIGNONE

AWS_STATIC_STRING_FROM_LITERAL(EC_PUBLIC_KEY_FIELD, "aws-crypto-public-key");

struct default_cmm {
    const struct aws_cryptosdk_cmm_vt * vt;
    struct aws_allocator * alloc;
    struct aws_cryptosdk_keyring * kr;
    enum aws_cryptosdk_alg_id alg_id;
};

static int default_cmm_generate_encryption_materials(struct aws_cryptosdk_cmm * cmm,
                                                     struct aws_cryptosdk_encryption_materials ** output,
                                                     struct aws_cryptosdk_encryption_request * request) {
    struct aws_cryptosdk_encryption_materials * enc_mat = NULL;
    struct default_cmm * self = (struct default_cmm *) cmm;

    *output = NULL;
    if (request->requested_alg && request->requested_alg != self->alg_id) {
        return aws_raise_error(AWS_CRYPTOSDK_ERR_UNSUPPORTED_FORMAT);
    }

    request->requested_alg = self->alg_id;

    enc_mat = aws_cryptosdk_encryption_materials_new(request->alloc, request->requested_alg);
    if (!enc_mat) goto err;

    enc_mat->enc_context = request->enc_context;

    if (aws_cryptosdk_keyring_generate_data_key(self->kr, enc_mat)) goto err;

// TODO: implement trailing signatures

    *output = enc_mat;
    return AWS_OP_SUCCESS;

err:
    aws_cryptosdk_encryption_materials_destroy(enc_mat);
    return AWS_OP_ERR;
}

static int default_cmm_decrypt_materials(struct aws_cryptosdk_cmm * cmm,
                                          struct aws_cryptosdk_decryption_materials ** output,
                                          struct aws_cryptosdk_decryption_request * request) {
    struct aws_cryptosdk_decryption_materials * dec_mat;
    struct default_cmm * self = (struct default_cmm *) cmm;

    dec_mat = aws_cryptosdk_decryption_materials_new(request->alloc, request->alg);
    if (!dec_mat) goto err;

    if (aws_cryptosdk_keyring_decrypt_data_key(self->kr, dec_mat, request)) goto err;

    if (!dec_mat->unencrypted_data_key.buffer) {
        aws_raise_error(AWS_CRYPTOSDK_ERR_CANNOT_DECRYPT);
        goto err;
    }

    const struct aws_cryptosdk_alg_properties *props = aws_cryptosdk_alg_props(request->alg);
    if (props->signature_len) {
        struct aws_hash_element *pElement = NULL;

        if (aws_hash_table_find(request->enc_context, EC_PUBLIC_KEY_FIELD, &pElement)
            || !pElement || !pElement->key
        ) {
            aws_raise_error(AWS_CRYPTOSDK_ERR_BAD_CIPHERTEXT);
            goto err;
        }

        if (aws_cryptosdk_sig_verify_start(&dec_mat->signctx, request->alloc, pElement->value, props)) {
            goto err;
        }
    }

    *output = dec_mat;
    return AWS_OP_SUCCESS;

err:
    *output = NULL;
    aws_cryptosdk_decryption_materials_destroy(dec_mat);
    return AWS_OP_ERR;
}

static void default_cmm_destroy(struct aws_cryptosdk_cmm * cmm) {
    struct default_cmm * self = (struct default_cmm *) cmm;
    aws_mem_release(self->alloc, self);
}

static const struct aws_cryptosdk_cmm_vt default_cmm_vt = {
    .vt_size = sizeof(struct aws_cryptosdk_cmm_vt),
    .name = "default cmm",
    .destroy = default_cmm_destroy,
    .generate_encryption_materials = default_cmm_generate_encryption_materials,
    .decrypt_materials = default_cmm_decrypt_materials
};

struct aws_cryptosdk_cmm * aws_cryptosdk_default_cmm_new(struct aws_allocator * alloc, struct aws_cryptosdk_keyring * kr) {
    struct default_cmm * cmm;
    cmm = aws_mem_acquire(alloc, sizeof(struct default_cmm));
    if (!cmm) return NULL;

    cmm->vt = &default_cmm_vt;
    cmm->alloc = alloc;
    cmm->kr = kr;
    cmm->alg_id = DEFAULT_ALG;

    return (struct aws_cryptosdk_cmm *) cmm;
}

int aws_cryptosdk_default_cmm_set_alg_id(struct aws_cryptosdk_cmm *cmm, enum aws_cryptosdk_alg_id alg_id) {
    struct default_cmm * self = (struct default_cmm *) cmm;

    assert(self->vt == &default_cmm_vt);

    if (!aws_cryptosdk_alg_props(alg_id)) {
        return aws_raise_error(AWS_CRYPTOSDK_ERR_UNSUPPORTED_FORMAT);
    }

    self->alg_id = alg_id;

    return AWS_OP_SUCCESS;
}
