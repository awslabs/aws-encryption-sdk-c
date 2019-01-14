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
#include "raw_rsa_keyring_test_vectors.h"
#include <aws/cryptosdk/raw_rsa_keyring.h>
#include "testutil.h"

AWS_STATIC_STRING_FROM_LITERAL(raw_rsa_keyring_tv_provider_id, "asoghis");
AWS_STATIC_STRING_FROM_LITERAL(raw_rsa_keyring_tv_master_key_id, "asdfhasiufhiasuhviawurhgiuawrhefiuawhf");

static const char raw_rsa_keyring_tv_public_key[] =
    "-----BEGIN PUBLIC KEY-----\n"
    "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC78fCDSGwNcJFXtP6XQ/48kDzU\n"
    "FUFT22YiJZJDVOkaj9Sq0xEeiuB3CVuDv41+Cc4bacijKRwbW5ZsXRLDjEwsPxLD\n"
    "coCVtVdA+APPNpaOiGOOBMdZA9vubMHBqq+vPwRAtksbWb7H+EyStManM2SalZCz\n"
    "CRBKg64VmYdZng/XZwIDAQAB\n"
    "-----END PUBLIC KEY-----\n";

static const char raw_rsa_keyring_tv_private_key[] =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIICdgIBADANBgkqhkiG9w0BAQEFAASCAmAwggJcAgEAAoGBALvx8INIbA1wkVe0\n"
    "/pdD/jyQPNQVQVPbZiIlkkNU6RqP1KrTER6K4HcJW4O/jX4JzhtpyKMpHBtblmxd\n"
    "EsOMTCw/EsNygJW1V0D4A882lo6IY44Ex1kD2+5swcGqr68/BEC2SxtZvsf4TJK0\n"
    "xqczZJqVkLMJEEqDrhWZh1meD9dnAgMBAAECgYByskygIcNnVEoup1MzhxgRZ8jn\n"
    "eO08OsmSjzE6jAgR4LLdaR+qbwBbRMenmG/F+j/g9Oavw/fWLkeXbBl2YxlcXlQk\n"
    "R2u21OVVgDaChJx1LJM+qwthvqRANm1qTWh2/aVUyV4phsuNjnmpDkveaCklffIc\n"
    "r2J6ppUXCuETvniv2QJBAPFISRh/cLGqdrqQSX9bZEUU8Wsu8Xj5fJBYXkKg/+8u\n"
    "B0/Rrwk6ktpkwzOChk8OC3Gvv9pwNbFv0UqRdLwqxOsCQQDHaMWxGabRlMwZY5ry\n"
    "SPkV2jFBt+q9VEv2wn7d9irNQih8Iou0Te0r8Opej7yUEy/BPQ9PesVV8p/kKzcT\n"
    "9Yh1AkAq4i4brIrbCPERN5PYjuXDYXWHF1DTr4P0I8CdFwBmAkhKZ3o0qbRwHHiV\n"
    "Lx2v708ZZaMzr73bS4RnPHMC/pcBAkBYpuu84HqZkl1qrC2mqWqTnH1piiqCIYfk\n"
    "HHPqmhZNSqxVA8a4Uiyu7FxFzgE4k48Xid3Up/AzVbpf5haGeRJBAkEAnVB7Icg/\n"
    "osxu+ddL3m5V9uF81vxfFWjLN2CIOVM0xSzbh8ygyZFe+QftPHt0QGamVo4kWZQl\n"
    "PvsBm2PORwzpAg==\n"
    "-----END PRIVATE KEY-----\n";

/**
 * An incorrect rsa public key used to test for encryption failure.
 * This key is incorrect as it is not in pem format.
 */
static const char wrong_raw_rsa_keyring_tv_public_key[] =
    "MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQC78fCDSGwNcJFXtP6XQ/48kDzU\n"
    "coCVtVdA+APPNpaOiGOOBMdZA9vubMHBqq+vPwRAtksbWb7H+EyStManM2SalZCz\n"
    "CRBKg64VmYdZng/XZwIDAQAB\n";

/**
 * An incorrect rsa private key used to test for decryption failure.
 * It contains only partial key bytes from the above correct private key.
 */
static const char wrong_raw_rsa_keyring_tv_private_key[] =
    "-----BEGIN PRIVATE KEY-----\n"
    "MIICdgIBADANBgkqhkiG9w0BAQEFAASCAmAwggJcAgEAAoGBALvx8INIbA1wkVe0\n"
    "/pdD/jyQPNQVQVPbZiIlkkNU6RqP1KrTER6K4HcJW4O/jX4JzhtpyKMpHBtblmxd\n"
    "EsOMTCw/EsNygJW1V0D4A882lo6IY44Ex1kD2+5swcGqr68/BEC2SxtZvsf4TJK0\n"
    "xqczZJqVkLMJEEqDrhWZh1meD9dnAgMBAAECgYByskygIcNnVEoup1MzhxgRZ8jn\n"
    "eO08OsmSjzE6jAgR4LLdaR+qbwBbRMenmG/F+j/g9Oavw/fWLkeXbBl2YxlcXlQk\n"
    "R2u21OVVgDaChJx1LJM+qwthvqRANm1qTWh2/aVUyV4phsuNjnmpDkveaCklffIc\n"
    "r2J6ppUXCuETvniv2QJBAPFISRh/cLGqdrqQSX9bZEUU8Wsu8Xj5fJBYXkKg/+8u\n"
    "B0/Rrwk6ktpkwzOChk8OC3Gvv9pwNbFv0UqRdLwqxOsCQQDHaMWxGabRlMwZY5ry\n"
    "PvsBm2PORwzpAg==\n"
    "-----END PRIVATE KEY-----\n";

struct aws_cryptosdk_keyring *raw_rsa_keyring_tv_new(
    struct aws_allocator *alloc, enum aws_cryptosdk_rsa_padding_mode rsa_padding_mode) {
    return aws_cryptosdk_raw_rsa_keyring_new(
        alloc,
        raw_rsa_keyring_tv_provider_id,
        raw_rsa_keyring_tv_master_key_id,
        raw_rsa_keyring_tv_private_key,
        raw_rsa_keyring_tv_public_key,
        rsa_padding_mode);
}

struct aws_cryptosdk_keyring *raw_rsa_keyring_tv_new_with_wrong_key(
    struct aws_allocator *alloc, enum aws_cryptosdk_rsa_padding_mode rsa_padding_mode) {
    return aws_cryptosdk_raw_rsa_keyring_new(
        alloc,
        raw_rsa_keyring_tv_provider_id,
        raw_rsa_keyring_tv_master_key_id,
        wrong_raw_rsa_keyring_tv_private_key,
        wrong_raw_rsa_keyring_tv_public_key,
        rsa_padding_mode);
}

int raw_rsa_keyring_tv_trace_updated_properly(struct aws_array_list *trace, uint32_t flags) {
    return assert_keyring_trace_record(
        trace,
        aws_array_list_length(trace) - 1,
        (const char *)raw_rsa_keyring_tv_provider_id->bytes,
        (const char *)raw_rsa_keyring_tv_master_key_id->bytes,
        flags);
}

struct aws_cryptosdk_edk edk_init(const uint8_t *edk_bytes, size_t edk_len) {
    struct aws_cryptosdk_edk edk;
    edk.ciphertext = aws_byte_buf_from_array(edk_bytes, edk_len);
    edk.provider_id =
        aws_byte_buf_from_array(raw_rsa_keyring_tv_provider_id->bytes, raw_rsa_keyring_tv_provider_id->len);
    edk.provider_info =
        aws_byte_buf_from_array(raw_rsa_keyring_tv_master_key_id->bytes, raw_rsa_keyring_tv_master_key_id->len);
    return edk;
}

// Test vector 0: 128 bit data key, RSA wrapping key with PKCS1 padding, empty encryption context
static const uint8_t tv_0_data_key[] = { 0x2c, 0x30, 0xad, 0x18, 0x0a, 0xeb, 0xb0, 0x74,
                                         0x29, 0x1b, 0xf7, 0x33, 0x2a, 0x3c, 0x2c, 0x96 };
// 128 encrypted data key
static const uint8_t tv_0_edk_bytes[] = {
    0x14, 0xcc, 0xd2, 0x20, 0x0a, 0x7c, 0x62, 0x84, 0x53, 0x4e, 0x7d, 0x52, 0xff, 0x07, 0x27, 0x67, 0x03, 0x99, 0x21,
    0xf3, 0x79, 0xd1, 0xa3, 0x08, 0xd7, 0xa0, 0x84, 0x67, 0x44, 0xbe, 0x0b, 0xb4, 0x4e, 0xe1, 0x5f, 0x79, 0x11, 0xc3,
    0x89, 0xfe, 0xca, 0x3c, 0xde, 0x3c, 0x7f, 0xa5, 0xa2, 0x7a, 0xba, 0x86, 0x12, 0x0a, 0x64, 0x9c, 0xe4, 0xeb, 0xc7,
    0xc6, 0xcb, 0x1f, 0xf7, 0x48, 0x1d, 0xde, 0xce, 0xf6, 0x32, 0xed, 0x13, 0xba, 0x50, 0x7c, 0x57, 0x8f, 0xa8, 0x5f,
    0x69, 0x49, 0xc7, 0xfe, 0xff, 0x39, 0xd1, 0xa9, 0x51, 0x6a, 0x6c, 0xe7, 0x5e, 0xd9, 0xba, 0x0c, 0x9c, 0xf8, 0x6b,
    0x2f, 0x1f, 0xf2, 0xd1, 0x5f, 0xb9, 0x1b, 0xb9, 0x48, 0x59, 0xee, 0x46, 0xd0, 0xbb, 0x3e, 0x34, 0x39, 0x84, 0xa0,
    0xb6, 0x1b, 0xf0, 0x39, 0xae, 0x5c, 0x51, 0x31, 0x2a, 0x81, 0x51, 0x6e, 0x8f, 0x0e
};

// Test vector 1: 192 bit data key, RSA wrapping key with PKCS1 Padding, empty encryption context
static const uint8_t tv_1_data_key[] = { 0x22, 0xae, 0xd1, 0x6d, 0x2e, 0xb5, 0x93, 0x80, 0x7f, 0x00, 0x43, 0x90,
                                         0xef, 0xc2, 0x99, 0x1f, 0xca, 0x24, 0x19, 0x56, 0x7d, 0xfb, 0xb1, 0xc0 };
// 128 encrypted data key
static const uint8_t tv_1_edk_bytes[] = {
    0x18, 0x9d, 0x7d, 0x81, 0xe5, 0x55, 0x22, 0x3c, 0x5f, 0x93, 0x19, 0xab, 0x27, 0x3f, 0xee, 0x08, 0x48, 0x0d, 0x1e,
    0x5a, 0x00, 0xc2, 0x64, 0x7f, 0xdd, 0x4e, 0x44, 0xd5, 0x1c, 0xd3, 0x87, 0x02, 0x3e, 0xdb, 0xf3, 0x7c, 0x62, 0xa0,
    0x4b, 0x62, 0x42, 0x19, 0x60, 0xc5, 0xa2, 0x8d, 0xa1, 0xf5, 0x4e, 0x88, 0xe9, 0x08, 0x2a, 0x13, 0xf2, 0x96, 0x5a,
    0x55, 0xd0, 0xf8, 0x81, 0xbb, 0x4d, 0x15, 0x86, 0xcc, 0x68, 0x49, 0xb8, 0x97, 0xbd, 0x70, 0x6c, 0x4b, 0xa6, 0xe1,
    0x82, 0xe5, 0xf8, 0xce, 0x49, 0x95, 0x04, 0x87, 0xbd, 0xd0, 0x41, 0x18, 0xf0, 0x33, 0x71, 0x90, 0xb6, 0x56, 0x89,
    0x17, 0x32, 0xd5, 0x15, 0xe2, 0xac, 0x7b, 0x47, 0xd4, 0xc3, 0x6d, 0x92, 0x48, 0xa2, 0xfc, 0xb4, 0x63, 0xbe, 0x1c,
    0x84, 0x74, 0xa2, 0xf1, 0x46, 0xeb, 0x33, 0xbb, 0x54, 0x64, 0x9d, 0x76, 0x57, 0x95
};

// Test vector 2: 256 bit data key,  RSA wrapping key with PKCS1 Padding, empty encryption context
static const uint8_t tv_2_data_key[] = { 0xfb, 0x4d, 0x51, 0xe7, 0xe9, 0x8c, 0x98, 0xef, 0x16, 0x39, 0x1f,
                                         0x53, 0xbf, 0x6a, 0xa0, 0x33, 0x12, 0x78, 0x66, 0xfc, 0x4b, 0x35,
                                         0xcb, 0xa9, 0x5f, 0xe1, 0x74, 0xee, 0xdf, 0x14, 0x2b, 0x79 };
// 128 encrypted data key
static const uint8_t tv_2_edk_bytes[] = {
    0x9b, 0xa2, 0x8d, 0x1b, 0xfc, 0xea, 0xbd, 0x0d, 0x4c, 0x00, 0xbd, 0x4a, 0xcb, 0x5d, 0xcf, 0xd3, 0xbd, 0xce, 0x96,
    0x58, 0xa2, 0xad, 0x38, 0xb3, 0x3b, 0xa6, 0x28, 0xd8, 0x50, 0x0c, 0xe9, 0xe9, 0xf6, 0xa6, 0x85, 0x59, 0x78, 0x22,
    0xf3, 0xec, 0x4d, 0x7c, 0xcf, 0x7a, 0x85, 0xa5, 0x9e, 0x88, 0x18, 0xb3, 0x83, 0x4d, 0xb1, 0x8d, 0xd7, 0xe3, 0x91,
    0xd0, 0x7a, 0x63, 0x73, 0xfb, 0x44, 0x06, 0xf5, 0x18, 0xec, 0x3a, 0x33, 0x60, 0xa7, 0xfc, 0x07, 0xaf, 0xde, 0xaf,
    0x3a, 0x01, 0x63, 0xc1, 0x6e, 0x9f, 0x09, 0x55, 0x3f, 0x05, 0x40, 0xcb, 0xda, 0xb4, 0xf4, 0x8d, 0x7b, 0x06, 0x53,
    0x0b, 0x22, 0x57, 0x18, 0x09, 0xe9, 0xdb, 0x06, 0xf9, 0x41, 0xf3, 0x0c, 0x83, 0xfb, 0x8b, 0xf8, 0x05, 0x2b, 0x02,
    0x76, 0xe1, 0x39, 0x6e, 0x91, 0xf0, 0x72, 0x6d, 0x03, 0xc5, 0x19, 0xea, 0xff, 0x68
};

// Test vector 3: 128 bit data key, RSA wrapping key with OAEP_SHA1_MGF1 padding, empty encryption context
static const uint8_t tv_3_data_key[] = { 0xec, 0xf8, 0xc8, 0x1d, 0xd1, 0x0d, 0xd2, 0xf0,
                                         0xe8, 0xfd, 0x2b, 0x62, 0xbe, 0x54, 0xe4, 0x73 };

// 128 encrypted data key
static const uint8_t tv_3_edk_bytes[] = {
    0x79, 0x11, 0x57, 0xb2, 0xf4, 0xb5, 0x94, 0xd1, 0xfa, 0xdc, 0xe4, 0xae, 0x97, 0x9e, 0x9c, 0xe1, 0xa4, 0x58, 0x82,
    0x8c, 0x02, 0x77, 0xf7, 0x40, 0x68, 0x45, 0xe5, 0xa7, 0x82, 0xc3, 0x4f, 0x0c, 0x7d, 0x4a, 0x88, 0xba, 0xc5, 0xb6,
    0x63, 0x1b, 0x63, 0xd4, 0x58, 0xd3, 0xd2, 0x97, 0x25, 0x33, 0x5a, 0x0c, 0x96, 0xa7, 0xcf, 0x90, 0xd6, 0x03, 0x35,
    0x88, 0xc7, 0xdb, 0x34, 0x76, 0xd4, 0x76, 0xaf, 0xbb, 0x40, 0xaf, 0x98, 0x02, 0x0e, 0x0c, 0x9d, 0x9b, 0x63, 0x88,
    0xee, 0xbc, 0x4c, 0x70, 0xc7, 0xf6, 0xbd, 0x74, 0x52, 0x17, 0x73, 0xd0, 0xff, 0x0b, 0x06, 0x23, 0xb6, 0x9a, 0x93,
    0xbd, 0xc9, 0x33, 0x86, 0xcb, 0xf2, 0xea, 0x01, 0xb8, 0x90, 0xdc, 0xdc, 0x7b, 0x71, 0xaa, 0xab, 0xfa, 0x4f, 0x64,
    0xd2, 0xda, 0xda, 0xf2, 0xf2, 0xc8, 0xaf, 0x51, 0x54, 0xf4, 0x24, 0x3d, 0x5d, 0x8a
};

// Test vector 4: 192 bit data key, RSA wrapping key with RSA_OAEP_SHA1_MGF1 padding, empty encryption context
static const uint8_t tv_4_data_key[] = { 0xf2, 0xd0, 0x56, 0x69, 0xd0, 0x68, 0x09, 0x3a, 0x5a, 0x73, 0x94, 0x7d,
                                         0x81, 0x83, 0x24, 0x3f, 0xe0, 0xf3, 0xd1, 0x37, 0x44, 0x4e, 0xe7, 0x3d };

// 128 encrypted data key
static const uint8_t tv_4_edk_bytes[] = {
    0x56, 0x44, 0x98, 0x53, 0x76, 0x0c, 0x3b, 0x77, 0x53, 0xa5, 0xb5, 0x1f, 0xd8, 0x3c, 0xd6, 0xd0, 0xc4, 0x61, 0x28,
    0xa4, 0x8e, 0x3c, 0x86, 0x0e, 0x91, 0x90, 0xd9, 0xb7, 0x75, 0xfd, 0xc8, 0xfc, 0x59, 0xd2, 0xc9, 0xcd, 0x9a, 0x2c,
    0x38, 0x2d, 0xd7, 0xfe, 0x8d, 0x08, 0x51, 0x37, 0xb3, 0x50, 0x2a, 0xa2, 0xe6, 0xd6, 0xf1, 0x7c, 0xde, 0xc7, 0xae,
    0x8d, 0xa4, 0x15, 0x96, 0xae, 0x02, 0xfb, 0x2d, 0xdb, 0xed, 0xe5, 0x0b, 0xa8, 0x35, 0x4d, 0xc9, 0xf2, 0x58, 0xa4,
    0xf3, 0x32, 0x5f, 0x9f, 0xf9, 0x4a, 0x4c, 0x36, 0x95, 0x17, 0x17, 0x22, 0xe1, 0x50, 0x2c, 0x56, 0x29, 0xb1, 0xb4,
    0x8d, 0x00, 0x51, 0x4a, 0x95, 0x59, 0x34, 0xc8, 0xb9, 0x8d, 0xce, 0xa8, 0x96, 0xcb, 0x4d, 0x37, 0x98, 0x3b, 0x1e,
    0xc8, 0x8a, 0x82, 0x93, 0xda, 0xaf, 0x43, 0x95, 0x5b, 0xa6, 0xac, 0x1b, 0x9c, 0x60
};

// Test vector 5: 256 bit data key, RSA wrapping key with RSA_OAEP_SHA1_MGF1 padding, empty encryption context
static const uint8_t tv_5_data_key[] = { 0x62, 0x59, 0x04, 0xb2, 0xc0, 0x68, 0xd0, 0x8a, 0xe3, 0x46, 0x1b,
                                         0xa9, 0x8d, 0xa4, 0xcb, 0x90, 0x8b, 0x3f, 0x6b, 0x5c, 0x7f, 0x2d,
                                         0x9e, 0x07, 0xc8, 0x50, 0xa2, 0x97, 0x2a, 0xb6, 0x3a, 0x63 };

// 128 encrypted data key
static const uint8_t tv_5_edk_bytes[] = {
    0x2b, 0x6e, 0xae, 0xe8, 0x85, 0x19, 0xe6, 0x93, 0xfe, 0x98, 0x7d, 0x34, 0x1a, 0x44, 0x09, 0xfb, 0x9f, 0x01, 0xab,
    0xc5, 0xa9, 0x84, 0xba, 0xdd, 0x5b, 0x80, 0xd1, 0xa4, 0xc5, 0x0d, 0x3e, 0x33, 0xcb, 0xbe, 0xd2, 0xa0, 0x10, 0x4e,
    0x98, 0x44, 0x02, 0xa3, 0x70, 0x08, 0xac, 0x64, 0x7d, 0x70, 0x5e, 0x79, 0x45, 0xb5, 0x5a, 0x7c, 0x4f, 0x97, 0xd1,
    0x5c, 0xe0, 0xbb, 0x7a, 0xb0, 0xe0, 0xfb, 0x06, 0x02, 0x7e, 0x3d, 0x1e, 0x97, 0x3c, 0x94, 0xe1, 0xf6, 0xf3, 0xad,
    0x4f, 0x62, 0x44, 0x1e, 0x1e, 0x21, 0x1f, 0xfb, 0x0b, 0xa6, 0x63, 0xd9, 0xfe, 0x4b, 0x15, 0x92, 0x65, 0x61, 0x50,
    0x86, 0x34, 0xd5, 0x3e, 0x72, 0x35, 0x7b, 0x6c, 0x82, 0xab, 0x69, 0xc1, 0x5f, 0x0d, 0x97, 0xde, 0xb6, 0x95, 0x9b,
    0x8d, 0x66, 0xdc, 0xd7, 0x84, 0x58, 0x80, 0x33, 0x95, 0x1f, 0xd3, 0xe9, 0x64, 0x0e
};

// Test vector 6: 128 bit data key, RSA wrapping key with RSA_OAEP_SHA256_MGF1 padding, empty encryption context
static const uint8_t tv_6_data_key[] = { 0xe6, 0x7c, 0xc1, 0xfb, 0x0a, 0x91, 0x7b, 0xcf,
                                         0x4e, 0x48, 0xf6, 0x91, 0x9b, 0xb0, 0x81, 0xf7 };

// 128 encrypted data key
static const uint8_t tv_6_edk_bytes[] = {
    0x51, 0xe8, 0x50, 0x66, 0x22, 0x6b, 0xd3, 0x2d, 0x83, 0x11, 0xa9, 0x62, 0x8e, 0xa5, 0x9e, 0x9a, 0x36, 0x6c, 0x8d,
    0xbd, 0x0a, 0xf6, 0x07, 0x83, 0x8a, 0x7c, 0xf4, 0xd6, 0x39, 0xdf, 0x76, 0x88, 0xa4, 0x52, 0x98, 0xc2, 0xfa, 0xf7,
    0x3d, 0xcf, 0x98, 0x87, 0xbd, 0xcf, 0xcb, 0x7a, 0xbc, 0x9e, 0xf8, 0x52, 0x20, 0x86, 0xe7, 0x37, 0xee, 0xe1, 0xac,
    0x12, 0x22, 0xe5, 0xd5, 0x55, 0x47, 0x80, 0xe9, 0xc0, 0x31, 0x3e, 0x09, 0xc2, 0xe6, 0x77, 0x77, 0xaf, 0x1a, 0x39,
    0x5d, 0xc2, 0xf4, 0xb2, 0xd1, 0x7d, 0x6e, 0x34, 0x0a, 0x13, 0x80, 0x2b, 0x26, 0xb1, 0x75, 0xdd, 0x13, 0x6f, 0xcb,
    0x7c, 0x6c, 0x62, 0x9d, 0xc8, 0xf7, 0x6a, 0x30, 0xbe, 0x18, 0x61, 0xb1, 0x89, 0xa5, 0xfb, 0x2b, 0x0a, 0x09, 0x5d,
    0x4e, 0x57, 0xfe, 0x12, 0x9b, 0x2b, 0x50, 0x5e, 0x15, 0x3e, 0x5b, 0x80, 0xa7, 0x8b
};

// Test vector 7: 192 bit data key, RSA wrapping key with RSA_OAEP_SHA256_MGF1 padding, empty encryption context
static const uint8_t tv_7_data_key[] = { 0xfa, 0xf4, 0xe6, 0x49, 0xeb, 0x16, 0x39, 0x75, 0xe6, 0x93, 0x7a, 0x1a,
                                         0xc0, 0x09, 0x9c, 0x44, 0xde, 0x12, 0x0b, 0xc4, 0x2c, 0xc1, 0x8b, 0x97 };
// 128 encrypted data key
static const uint8_t tv_7_edk_bytes[] = {
    0x6b, 0x4c, 0x76, 0x9b, 0xbd, 0xd7, 0x0e, 0x47, 0x51, 0xc6, 0xe3, 0x7a, 0xda, 0x65, 0xf7, 0x18, 0x58, 0xd3, 0xec,
    0x86, 0xf9, 0xf4, 0x8f, 0x6d, 0x0a, 0x21, 0x22, 0x8d, 0x08, 0x40, 0xe9, 0x82, 0xed, 0xf0, 0x1c, 0x7a, 0x64, 0x99,
    0xf2, 0xcd, 0x85, 0x06, 0xa7, 0xb3, 0x0c, 0x93, 0xc0, 0x4f, 0xfe, 0x8f, 0xa7, 0xbf, 0x79, 0xff, 0xb7, 0xfb, 0x7f,
    0xb3, 0x6e, 0xf2, 0xc0, 0x51, 0xbf, 0x08, 0xff, 0x78, 0x1a, 0x7d, 0x0e, 0x6d, 0x46, 0x88, 0x2d, 0x73, 0x8f, 0x84,
    0x08, 0xc1, 0xb5, 0x69, 0x7d, 0x74, 0xec, 0xb0, 0xd4, 0xac, 0xf7, 0x77, 0xeb, 0xb4, 0x42, 0x4d, 0x0b, 0xb3, 0x8c,
    0x8b, 0x55, 0xed, 0x01, 0xb5, 0xb4, 0x21, 0x35, 0xa0, 0x49, 0xef, 0x12, 0x6e, 0x70, 0x80, 0xcf, 0x1e, 0xec, 0x0f,
    0xa1, 0x5f, 0xfd, 0x05, 0xb0, 0xa2, 0x20, 0x4e, 0x21, 0x5e, 0x4d, 0x91, 0x9d, 0xdc
};

// Test vector 8: 256 bit data key, RSA wrapping key with RSA_OAEP_SHA256_MGF1 padding, empty encryption context
static const uint8_t tv_8_data_key[] = { 0xc0, 0xe9, 0x66, 0x7e, 0xef, 0xee, 0x55, 0xb0, 0x48, 0x72, 0xe7,
                                         0x1d, 0x2b, 0xaa, 0xd1, 0xdc, 0x21, 0x14, 0x06, 0xb1, 0xa1, 0xb3,
                                         0x73, 0xd5, 0xa7, 0x3b, 0x10, 0x8e, 0x56, 0xd3, 0x56, 0xc6 };
// 128 encrypted data key
static const uint8_t tv_8_edk_bytes[] = {
    0x6b, 0x55, 0x91, 0xd8, 0xa8, 0xdd, 0x80, 0x28, 0x4a, 0x31, 0x78, 0x96, 0x22, 0x9f, 0x6d, 0x41, 0x1c, 0xad, 0xab,
    0x50, 0xeb, 0xdd, 0x2b, 0x53, 0x8b, 0xf8, 0x7b, 0xa0, 0xd1, 0xfd, 0x69, 0x23, 0xd4, 0x84, 0x22, 0xcd, 0x95, 0x33,
    0xe3, 0xae, 0xa0, 0x4b, 0x89, 0x81, 0x37, 0x5a, 0xde, 0x26, 0x52, 0x04, 0x5e, 0x6e, 0x85, 0x6c, 0xac, 0x71, 0x72,
    0xaa, 0xc5, 0xdd, 0x28, 0xec, 0x65, 0x03, 0x2d, 0xcb, 0x53, 0x44, 0x51, 0x8c, 0x5d, 0x28, 0xe8, 0xa1, 0xf6, 0x3c,
    0x4b, 0x83, 0xc5, 0xbc, 0xd0, 0xc7, 0x0f, 0xe9, 0xed, 0x65, 0x59, 0xd1, 0x92, 0x95, 0x9b, 0xdd, 0x1f, 0x12, 0x05,
    0xdb, 0xef, 0x6a, 0x55, 0x23, 0x24, 0x5d, 0x21, 0x2e, 0x67, 0x71, 0x6d, 0x33, 0xb5, 0x7c, 0x5d, 0x11, 0xb7, 0x20,
    0x3c, 0x8d, 0xc6, 0xf7, 0xc1, 0x16, 0x1a, 0xee, 0x2a, 0xb0, 0x6f, 0x12, 0xb2, 0x26
};

struct raw_rsa_keyring_test_vector raw_rsa_keyring_test_vectors[] = {
    {
        .rsa_padding_mode = AWS_CRYPTOSDK_RSA_PKCS1,
        .alg              = ALG_AES128_GCM_IV12_TAG16_NO_KDF,
        .data_key         = tv_0_data_key,
        .data_key_len     = sizeof(tv_0_data_key),
        .edk_bytes        = tv_0_edk_bytes,
        .edk_bytes_len    = sizeof(tv_0_edk_bytes),
    },
    {
        .rsa_padding_mode = AWS_CRYPTOSDK_RSA_PKCS1,
        .alg              = ALG_AES192_GCM_IV12_TAG16_NO_KDF,
        .data_key         = tv_1_data_key,
        .data_key_len     = sizeof(tv_1_data_key),
        .edk_bytes        = tv_1_edk_bytes,
        .edk_bytes_len    = sizeof(tv_1_edk_bytes),
    },
    {
        .rsa_padding_mode = AWS_CRYPTOSDK_RSA_PKCS1,
        .alg              = ALG_AES256_GCM_IV12_TAG16_NO_KDF,
        .data_key         = tv_2_data_key,
        .data_key_len     = sizeof(tv_2_data_key),
        .edk_bytes        = tv_2_edk_bytes,
        .edk_bytes_len    = sizeof(tv_2_edk_bytes),
    },
    {
        .rsa_padding_mode = AWS_CRYPTOSDK_RSA_OAEP_SHA1_MGF1,
        .alg              = ALG_AES128_GCM_IV12_TAG16_NO_KDF,
        .data_key         = tv_3_data_key,
        .data_key_len     = sizeof(tv_3_data_key),
        .edk_bytes        = tv_3_edk_bytes,
        .edk_bytes_len    = sizeof(tv_3_edk_bytes),
    },
    {
        .rsa_padding_mode = AWS_CRYPTOSDK_RSA_OAEP_SHA1_MGF1,
        .alg              = ALG_AES192_GCM_IV12_TAG16_NO_KDF,
        .data_key         = tv_4_data_key,
        .data_key_len     = sizeof(tv_4_data_key),
        .edk_bytes        = tv_4_edk_bytes,
        .edk_bytes_len    = sizeof(tv_4_edk_bytes),
    },
    {
        .rsa_padding_mode = AWS_CRYPTOSDK_RSA_OAEP_SHA1_MGF1,
        .alg              = ALG_AES256_GCM_IV12_TAG16_NO_KDF,
        .data_key         = tv_5_data_key,
        .data_key_len     = sizeof(tv_5_data_key),
        .edk_bytes        = tv_5_edk_bytes,
        .edk_bytes_len    = sizeof(tv_5_edk_bytes),
    },
    {
        .rsa_padding_mode = AWS_CRYPTOSDK_RSA_OAEP_SHA256_MGF1,
        .alg              = ALG_AES128_GCM_IV12_TAG16_NO_KDF,
        .data_key         = tv_6_data_key,
        .data_key_len     = sizeof(tv_6_data_key),
        .edk_bytes        = tv_6_edk_bytes,
        .edk_bytes_len    = sizeof(tv_6_edk_bytes),
    },
    {
        .rsa_padding_mode = AWS_CRYPTOSDK_RSA_OAEP_SHA256_MGF1,
        .alg              = ALG_AES192_GCM_IV12_TAG16_NO_KDF,
        .data_key         = tv_7_data_key,
        .data_key_len     = sizeof(tv_7_data_key),
        .edk_bytes        = tv_7_edk_bytes,
        .edk_bytes_len    = sizeof(tv_7_edk_bytes),
    },
    {
        .rsa_padding_mode = AWS_CRYPTOSDK_RSA_OAEP_SHA256_MGF1,
        .alg              = ALG_AES256_GCM_IV12_TAG16_NO_KDF,
        .data_key         = tv_8_data_key,
        .data_key_len     = sizeof(tv_8_data_key),
        .edk_bytes        = tv_8_edk_bytes,
        .edk_bytes_len    = sizeof(tv_8_edk_bytes),
    },
    { 0 }
};

struct aws_cryptosdk_edk edk_init_test_vector(struct raw_rsa_keyring_test_vector *tv) {
    return edk_init(tv->edk_bytes, tv->edk_bytes_len);
}

struct aws_cryptosdk_edk edk_init_test_vector_idx(int idx) {
    return edk_init(raw_rsa_keyring_test_vectors[idx].edk_bytes, raw_rsa_keyring_test_vectors[idx].edk_bytes_len);
}
