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
#include <aws/cryptosdk/kms_keyring.h>

#include <stddef.h>
#include <stdlib.h>
#include <aws/common/byte_buf.h>
#include <aws/core/utils/Outcome.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/memory/stl/AWSAllocator.h>
#include <aws/core/utils/memory/MemorySystemInterface.h>
#include <aws/kms/KMSClient.h>
#include <aws/kms/model/EncryptResult.h>
#include <aws/kms/model/DecryptResult.h>
#include <aws/kms/model/GenerateDataKeyResult.h>
#include <aws/cryptosdk/cipher.h>
#include <aws/cryptosdk/error.h>
#include <aws/cryptosdk/materials.h>
#include <aws/cryptosdk/private/cpputils.h>

namespace Aws {
namespace Cryptosdk {

using Private::aws_utils_byte_buffer_from_c_aws_byte_buf;
using Private::aws_byte_buf_dup_from_aws_utils;
using Private::aws_map_from_c_aws_hash_table;
using Private::append_key_dup_to_edks;

static const char *AWS_CRYPTO_SDK_KMS_CLASS_TAG = "KmsKeyring";
static const char *KEY_PROVIDER_STR = "aws-kms";

void KmsKeyring::DestroyAwsCryptoKeyring(struct aws_cryptosdk_keyring *keyring) {
    auto keyring_data_ptr = static_cast<Aws::Cryptosdk::KmsKeyring *>(keyring);
    Aws::Delete(keyring_data_ptr);
}

int KmsKeyring::OnDecrypt(struct aws_cryptosdk_keyring *keyring,
                          struct aws_allocator *request_alloc,
                          struct aws_byte_buf *unencrypted_data_key,
                          const struct aws_array_list *edks,
                          const struct aws_hash_table *enc_context,
                          enum aws_cryptosdk_alg_id alg) {
    auto self = static_cast<Aws::Cryptosdk::KmsKeyring *>(keyring);
    if (!self || !request_alloc || !unencrypted_data_key || !edks || !enc_context) {
        abort();
    }

    Aws::StringStream error_buf;
    const auto enc_context_cpp = aws_map_from_c_aws_hash_table(enc_context);

    size_t num_elems = aws_array_list_length(edks);
    for (unsigned int idx = 0; idx < num_elems; idx++) {
        struct aws_cryptosdk_edk *edk;
        int rv = aws_array_list_get_at_ptr(edks, (void **) &edk, idx);
        if (rv != AWS_OP_SUCCESS) {
            continue;
        }

        if (!aws_byte_buf_eq(&edk->provider_id, &self->key_provider)) {
            // EDK belongs to a different non KMS keyring. Skip.
            continue;
        }

        const Aws::String key_arn = Private::aws_string_from_c_aws_byte_buf(&edk->provider_info);
        if (std::find(self->key_ids.begin(), self->key_ids.end(), key_arn) == self->key_ids.end()) {
            // EDK encrypted by KMS key this keyring does not have access to. Skip.
            continue;
        }
        auto kms_client = self->GetKmsClient(key_arn);
        auto kms_request = self->CreateDecryptRequest(key_arn,
                                                      self->grant_tokens,
                                                      aws_utils_byte_buffer_from_c_aws_byte_buf(&edk->enc_data_key),
                                                      enc_context_cpp);

        Aws::KMS::Model::DecryptOutcome outcome = kms_client->Decrypt(kms_request);
        if (!outcome.IsSuccess()) {
            error_buf << "Error: " << outcome.GetError().GetExceptionName() << " Message:"
                      << outcome.GetError().GetMessage() << " ";
            continue;
        }

        const Aws::String &outcome_key_id = outcome.GetResult().GetKeyId();
        if (outcome_key_id == key_arn) {
            return aws_byte_buf_dup_from_aws_utils(request_alloc,
                                                   unencrypted_data_key,
                                                   outcome.GetResult().GetPlaintext());
        }
    }

    AWS_LOGSTREAM_ERROR(AWS_CRYPTO_SDK_KMS_CLASS_TAG,
                        "Could not find any data key that can be decrypted by KMS. Errors:" << error_buf.str());
    // According to materials.h we should return success when no key was found
    return AWS_OP_SUCCESS;
}

int KmsKeyring::OnEncrypt(struct aws_cryptosdk_keyring *keyring,
                          struct aws_allocator *request_alloc,
                          struct aws_byte_buf *unencrypted_data_key,
                          struct aws_array_list *edk_list,
                          const struct aws_hash_table *enc_context,
                          enum aws_cryptosdk_alg_id alg) {
    // Class that prevents memory leak of aws_list (even if a function throws)
    // When the object will be destroyed it will call aws_cryptosdk_edk_list_clean_up
    class EdksRaii {
      public:
        ~EdksRaii() {
            if (initialized) {
                aws_cryptosdk_edk_list_clean_up(&aws_list);
                initialized = false;
            }
        }
        int Create(struct aws_allocator *alloc) {
            auto rv = aws_cryptosdk_edk_list_init(alloc, &aws_list);
            initialized = (rv == AWS_OP_SUCCESS);
            return rv;
        }
        bool initialized = false;
        struct aws_array_list aws_list;
    };
    
    if (!keyring || !request_alloc || !unencrypted_data_key || !edk_list || !enc_context) {
            abort();
    }
    auto self = static_cast<Aws::Cryptosdk::KmsKeyring *>(keyring);

    EdksRaii edks;
    int rv = edks.Create(request_alloc);
    if (rv != AWS_OP_SUCCESS) return rv;

    const auto enc_context_cpp = aws_map_from_c_aws_hash_table(enc_context);

    bool generated_new_data_key = false;
    Aws::String generating_key_id = self->key_ids.front();
    if (!unencrypted_data_key->buffer) {
        const struct aws_cryptosdk_alg_properties *alg_prop = aws_cryptosdk_alg_props(alg);
        if (alg_prop == NULL) {
            AWS_LOGSTREAM_ERROR(AWS_CRYPTO_SDK_KMS_CLASS_TAG, "Invalid encryption materials algorithm properties");
            return aws_raise_error(AWS_CRYPTOSDK_ERR_CRYPTO_UNKNOWN);
        }
        auto kms_client = self->GetKmsClient(generating_key_id);
        auto kms_request = self->CreateGenerateDataKeyRequest(generating_key_id,
                                                              self->grant_tokens,
                                                              (int)alg_prop->data_key_len,
                                                              enc_context_cpp);

        Aws::KMS::Model::GenerateDataKeyOutcome outcome = kms_client->GenerateDataKey(kms_request);
        if (!outcome.IsSuccess()) {
            AWS_LOGSTREAM_ERROR(AWS_CRYPTO_SDK_KMS_CLASS_TAG, "Invalid encryption materials algorithm properties");
            return aws_raise_error(AWS_CRYPTOSDK_ERR_KMS_FAILURE);
        }

        rv = append_key_dup_to_edks(request_alloc,
                                    &edks.aws_list,
                                    &outcome.GetResult().GetCiphertextBlob(),
                                    &outcome.GetResult().GetKeyId(),
                                    &self->key_provider);
        if (rv != AWS_OP_SUCCESS) return rv;

        rv = aws_byte_buf_dup_from_aws_utils(request_alloc,
                                             unencrypted_data_key,
                                             outcome.GetResult().GetPlaintext());
        if (rv != AWS_OP_SUCCESS) return rv;
        generated_new_data_key = true;

    }

    const auto unencrypted_data_key_cpp = aws_utils_byte_buffer_from_c_aws_byte_buf(unencrypted_data_key);

    for (auto kms_cmk_name : self->key_ids) {
        /* When we make generate data key call above, KMS also encrypts the data
         * key once. In that case, do not reencrypt with the same CMK.
         */
        if (generated_new_data_key && kms_cmk_name == generating_key_id) continue;
        auto kms_client = self->GetKmsClient(kms_cmk_name);
        auto kms_client_request = self->CreateEncryptRequest(
            kms_cmk_name,
            self->grant_tokens,
            unencrypted_data_key_cpp,
            enc_context_cpp);

        Aws::KMS::Model::EncryptOutcome outcome = kms_client->Encrypt(kms_client_request);
        if (!outcome.IsSuccess()) {
            AWS_LOGSTREAM_ERROR(AWS_CRYPTO_SDK_KMS_CLASS_TAG,
                                "KMS encryption error : " << outcome.GetError().GetExceptionName() << " Message: "
                                                          << outcome.GetError().GetMessage());
            rv = aws_raise_error(AWS_CRYPTOSDK_ERR_KMS_FAILURE);
            goto out;
        }

        rv = append_key_dup_to_edks(
            request_alloc,
            &edks.aws_list,
            &outcome.GetResult().GetCiphertextBlob(),
            &outcome.GetResult().GetKeyId(),
            &self->key_provider);
        if (rv != AWS_OP_SUCCESS) {
            goto out;
        }
    }
    rv = aws_cryptosdk_transfer_edk_list(edk_list, &edks.aws_list);
out:
    if (rv != AWS_OP_SUCCESS && generated_new_data_key) {
        aws_byte_buf_clean_up(unencrypted_data_key);
    }
    return rv;
}

Aws::Cryptosdk::KmsKeyring::~KmsKeyring() {
}

Aws::Cryptosdk::KmsKeyring::KmsKeyring(const Aws::Vector<Aws::String> &key_ids,
                                       const String &default_region,
                                       const Aws::Vector<Aws::String> &grant_tokens,
                                       std::shared_ptr<ClientSupplier> client_supplier)
    : key_provider(aws_byte_buf_from_c_str(KEY_PROVIDER_STR)),
      kms_client_supplier(client_supplier),
      default_region(default_region),
      grant_tokens(grant_tokens),
      key_ids(key_ids) {

    static const aws_cryptosdk_keyring_vt kms_keyring_vt = {
        sizeof(struct aws_cryptosdk_keyring_vt),  // size
        KEY_PROVIDER_STR,  // name
        &KmsKeyring::DestroyAwsCryptoKeyring,  // destroy callback
        &KmsKeyring::OnEncrypt, // on_encrypt callback
        &KmsKeyring::OnDecrypt  // on_decrypt callback
    };

    aws_cryptosdk_keyring_base_init(this, &kms_keyring_vt);
}

Aws::KMS::Model::EncryptRequest KmsKeyring::CreateEncryptRequest(const Aws::String &key_id,
                                                                 const Aws::Vector<Aws::String> &grant_tokens,
                                                                 const Utils::ByteBuffer &plaintext,
                                                                 const Aws::Map<Aws::String,
                                                                                Aws::String> &encryption_context) const {
    KMS::Model::EncryptRequest encryption_request;
    encryption_request.SetKeyId(key_id);
    encryption_request.SetPlaintext(plaintext);

    encryption_request.SetEncryptionContext(encryption_context);
    encryption_request.SetGrantTokens(grant_tokens);

    return encryption_request;
}

Aws::KMS::Model::DecryptRequest KmsKeyring::CreateDecryptRequest(const Aws::String &key_id,
                                                                 const Aws::Vector<Aws::String> &grant_tokens,
                                                                 const Utils::ByteBuffer &ciphertext,
                                                                 const Aws::Map<Aws::String,
                                                                                Aws::String> &encryption_context) const {
    KMS::Model::DecryptRequest request;
    request.SetCiphertextBlob(ciphertext);

    request.SetEncryptionContext(encryption_context);
    request.SetGrantTokens(grant_tokens);

    return request;
}

Aws::KMS::Model::GenerateDataKeyRequest KmsKeyring::CreateGenerateDataKeyRequest(
    const Aws::String &key_id,
    const Aws::Vector<Aws::String> &grant_tokens,
    int number_of_bytes,
    const Aws::Map<Aws::String, Aws::String> &encryption_context) const {

    KMS::Model::GenerateDataKeyRequest request;
    request.SetKeyId(key_id);
    request.SetNumberOfBytes(number_of_bytes);

    request.SetGrantTokens(grant_tokens);
    request.SetEncryptionContext(encryption_context);

    return request;
}

std::shared_ptr<KMS::KMSClient> KmsKeyring::GetKmsClient(const Aws::String &key_id) const {
    auto region = Private::parse_region_from_kms_key_arn(key_id);
    if (region.empty()) {
	region = default_region;
    }

    return kms_client_supplier->UnlockedGetClient(region);
}

std::shared_ptr<KMS::KMSClient> KmsKeyring::CreateDefaultKmsClient(const Aws::String &region) {
    Aws::Client::ClientConfiguration client_configuration;
    client_configuration.region = region;
#ifdef VALGRIND_TESTS
    // When running under valgrind, the default timeouts are too slow
    client_configuration.requestTimeoutMs = 10000;
    client_configuration.connectTimeoutMs = 10000;
#endif
    return Aws::MakeShared<Aws::KMS::KMSClient>(AWS_CRYPTO_SDK_KMS_CLASS_TAG, client_configuration);

}

void KmsKeyring::CachingClientSupplier::PutClient(const Aws::String &region, std::shared_ptr<KMS::KMSClient> kms_client) {
    std::unique_lock<std::mutex> lock(cache_mutex);
    if (cache.find(region) != cache.end()) {
        // Already have a cached client for this region. Do nothing.
        return;
    }

    if (!kms_client) {
        kms_client = CreateDefaultKmsClient(region);
    }
    cache[region] = kms_client;
}

KmsKeyring::SingleClientSupplier::SingleClientSupplier(const std::shared_ptr<KMS::KMSClient> &kms_client)
    : kms_client(kms_client) {
}

std::shared_ptr<KMS::KMSClient> KmsKeyring::SingleClientSupplier::UnlockedGetClient(const Aws::String &region) const {
    return kms_client;
}


std::shared_ptr<KMS::KMSClient> KmsKeyring::CachingClientSupplier::UnlockedGetClient(const Aws::String &region) const {
    if (cache.find(region) != cache.end()) {
        return cache.at(region);
    }
    return nullptr;
}

std::shared_ptr<KMS::KMSClient> KmsKeyring::CachingClientSupplier::LockedGetClient(const Aws::String &region) const {
    std::unique_lock<std::mutex> lock(cache_mutex);
    if (cache.find(region) != cache.end()) {
        return cache.at(region);
    }
    return nullptr;
}

std::shared_ptr<KmsKeyring::ClientSupplier> KmsKeyring::Builder::BuildClientSupplier() const {
    /* Presence of default region when needed has already been verified by ValidParameters()
     * so we will not recheck for it here.
     */

    if (kms_client) {
        return Aws::MakeShared<SingleClientSupplier>(AWS_CRYPTO_SDK_KMS_CLASS_TAG, kms_client);
    }

    if (key_ids.size() == 1) {
        Aws::String region = Private::parse_region_from_kms_key_arn(key_ids.front());
        if (region.empty()) {
            region = default_region;
        }
        return Aws::MakeShared<SingleClientSupplier>(AWS_CRYPTO_SDK_KMS_CLASS_TAG, CreateDefaultKmsClient(region));
    }

    auto my_client_supplier = client_supplier ? client_supplier :
        Aws::MakeShared<CachingClientSupplier>(AWS_CRYPTO_SDK_KMS_CLASS_TAG);

    for (auto &key : key_ids) {
        Aws::String region = Private::parse_region_from_kms_key_arn(key);
        if (region.empty()) {
            region = default_region;
        }
        my_client_supplier->PutClient(region);
    }
    return my_client_supplier;
}

bool KmsKeyring::Builder::ValidParameters() const {
    if (key_ids.size() == 0) {
        AWS_LOGSTREAM_ERROR(AWS_CRYPTO_SDK_KMS_CLASS_TAG, "No key IDs were provided");
        return false;
    }

    bool need_full_arns = kms_client == nullptr && default_region == "";

    for (auto &key : key_ids) {
        if (key.empty()) {
            AWS_LOGSTREAM_ERROR(AWS_CRYPTO_SDK_KMS_CLASS_TAG, "A key that was provided is empty");
            return false;
        }
        if (need_full_arns && Private::parse_region_from_kms_key_arn(key).empty()) {
            AWS_LOGSTREAM_ERROR(AWS_CRYPTO_SDK_KMS_CLASS_TAG, "Default region or KMS client needed for non-ARN key IDs");
            return false;
        }
    }

    return true;
}

aws_cryptosdk_keyring *KmsKeyring::Builder::Build() const {
    if (!ValidParameters()) {
        aws_raise_error(AWS_CRYPTOSDK_ERR_KMS_FAILURE);
        return NULL;
    }

    /**
     * We want to allow construction of a KmsKeyring object only through the builder, which is why
     * KmsKeyring has a protected constructor. Doing this allows us to guarantee that it is always
     * allocated with Aws::New. However, Aws::New only allocates memory for classes that have public
     * constructors or for which Aws::New is a friend function. Making Aws::New a friend function
     * would allow creation of a KmsKeyring without the builder. The solution was to make a nested
     * class in the builder which is just the KmsKeyring with a public constructor.
     */
    class KmsKeyringWithPublicConstructor : public KmsKeyring {
    public:
        KmsKeyringWithPublicConstructor(const Aws::Vector<Aws::String> &key_ids,
                                        const String &default_region,
                                        const Aws::Vector<Aws::String> &grant_tokens,
                                        std::shared_ptr<ClientSupplier> client_supplier) :
            KmsKeyring(key_ids,
                       default_region,
                       grant_tokens,
                       client_supplier) {}
    };

    return Aws::New<KmsKeyringWithPublicConstructor>(AWS_CRYPTO_SDK_KMS_CLASS_TAG,
                                                     std::move(key_ids),
                                                     std::move(default_region),
                                                     std::move(grant_tokens),
                                                     BuildClientSupplier());
}

KmsKeyring::Builder &KmsKeyring::Builder::WithDefaultRegion(const String &default_region) {
    this->default_region = default_region;
    return *this;
}

KmsKeyring::Builder &KmsKeyring::Builder::WithKeyIds(const Aws::Vector<Aws::String> &key_ids) {
    this->key_ids.insert(this->key_ids.end(), key_ids.begin(), key_ids.end());
    return *this;
}

KmsKeyring::Builder &KmsKeyring::Builder::WithKeyId(const Aws::String &key_id) {
    this->key_ids.push_back(key_id);
    return *this;
}

KmsKeyring::Builder &KmsKeyring::Builder::WithGrantTokens(const Aws::Vector<Aws::String> &grant_tokens) {
    this->grant_tokens.insert(this->grant_tokens.end(), grant_tokens.begin(), grant_tokens.end());
    return *this;
}

KmsKeyring::Builder &KmsKeyring::Builder::WithCachingClientSupplier(
        const std::shared_ptr<CachingClientSupplier> &client_supplier) {
    this->client_supplier = client_supplier;
    return *this;
}

KmsKeyring::Builder &KmsKeyring::Builder::WithKmsClient(std::shared_ptr<KMS::KMSClient> kms_client) {
    this->kms_client = kms_client;
    return *this;
}

}  // namespace Cryptosdk
}  // namespace Aws
