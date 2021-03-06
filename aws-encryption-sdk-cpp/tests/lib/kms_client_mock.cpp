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

#include "kms_client_mock.h"
#include <stdexcept>

namespace Aws {
namespace Cryptosdk {
namespace Testing {
using std::logic_error;

const char *CLASS_TAG = "KMS_CLIENT_MOCK_CTAG";

KmsClientMock::KmsClientMock() : Aws::KMS::KMSClient(), expect_generate_dk(false) {}

KmsClientMock::~KmsClientMock() {
    // there shouldn't be any other expecting calls
    if (ExpectingOtherCalls() == true) {
        std::cerr << "KmsClientMock destroyed but other calls were expected" << std::endl;
        abort();
    }
}

Model::EncryptOutcome KmsClientMock::Encrypt(const Model::EncryptRequest &request) const {
    if (expected_encrypt_values.size() == 0) {
        throw logic_error("Unexpected call to encrypt");
    }

    ExpectedEncryptValues eev = expected_encrypt_values.front();
    expected_encrypt_values.pop_front();

    if (request.GetKeyId() != eev.expected_enc_request.GetKeyId()) {
        throw logic_error(
            std::string("Got :") + request.GetKeyId().c_str() +
            " expecting: " + eev.expected_enc_request.GetKeyId().c_str());
    }

    if (request.GetPlaintext() != eev.expected_enc_request.GetPlaintext()) {
        throw logic_error(
            std::string("Got :") + reinterpret_cast<const char *>(request.GetPlaintext().GetUnderlyingData()) +
            " expecting: " +
            reinterpret_cast<const char *>(eev.expected_enc_request.GetPlaintext().GetUnderlyingData()));
    }

    if (request.GetGrantTokens() != grant_tokens) {
        throw logic_error("Got other grant tokens than expected");
    }

    if (request.GetEncryptionContext() != eev.expected_enc_request.GetEncryptionContext()) {
        throw logic_error("Got other encryption context than expected");
    }

    return eev.encrypt_return;
}
void KmsClientMock::ExpectEncryptAccumulator(
    const Model::EncryptRequest &request, Model::EncryptOutcome encrypt_return) {
    ExpectedEncryptValues eev = { request, encrypt_return };
    this->expected_encrypt_values.push_back(eev);
}

Model::DecryptOutcome KmsClientMock::Decrypt(const Model::DecryptRequest &request) const {
    if (expected_decrypt_values.size() == 0) {
        throw std::exception();
    }
    ExpectedDecryptValues edv = expected_decrypt_values.front();
    expected_decrypt_values.pop_front();

    if (edv.expected_dec_request.GetCiphertextBlob() != request.GetCiphertextBlob()) {
        throw std::exception();
    }

    if (request.GetGrantTokens() != grant_tokens) {
        throw logic_error("Got other grant tokens than expected");
    }

    if (request.GetEncryptionContext() != edv.expected_dec_request.GetEncryptionContext()) {
        throw logic_error("Got other encryption context than expected");
    }

    if (request.GetKeyId() != edv.expected_dec_request.GetKeyId()) {
        throw logic_error("Got other key ID than expected");
    }

    return edv.return_decrypt;
}

void KmsClientMock::ExpectDecryptAccumulator(
    const Model::DecryptRequest &request, Model::DecryptOutcome decrypt_return) {
    ExpectedDecryptValues edv = { request, decrypt_return };
    this->expected_decrypt_values.push_back(edv);
}

Model::GenerateDataKeyOutcome KmsClientMock::GenerateDataKey(const Model::GenerateDataKeyRequest &request) const {
    if (!expect_generate_dk) {
        throw std::exception();
    }

    if (request.GetKeyId() != expected_generate_dk_request.GetKeyId()) {
        throw std::exception();
    }

    if (request.GetNumberOfBytes() != expected_generate_dk_request.GetNumberOfBytes()) {
        throw std::exception();
    }

    if (request.GetGrantTokens() != grant_tokens) {
        throw logic_error("Got other grant tokens than expected");
    }

    if (request.GetEncryptionContext() != expected_generate_dk_request.GetEncryptionContext()) {
        throw logic_error("Got other encryption context than expected");
    }

    expect_generate_dk = false;
    return generate_dk_return;
}

void KmsClientMock::ExpectGenerateDataKey(
    const Model::GenerateDataKeyRequest &request, Model::GenerateDataKeyOutcome generate_dk_return) {
    expect_generate_dk           = true;
    expected_generate_dk_request = request;
    this->generate_dk_return     = generate_dk_return;
}

bool KmsClientMock::ExpectingOtherCalls() {
    return (expected_decrypt_values.size() != 0) || (expected_encrypt_values.size() != 0) || expect_generate_dk;
}

void KmsClientMock::ExpectGrantTokens(const Aws::Vector<Aws::String> &grant_tokens) {
    this->grant_tokens = grant_tokens;
}

std::shared_ptr<KMS::KMSClient> KmsClientSupplierMock::GetClient(
    const Aws::String &region, std::function<void()> &report_success) {
    const auto client_mock_iter = kms_client_mocks.find(region);
    if (client_mock_iter != kms_client_mocks.end()) {
        return client_mock_iter->second;
    }

    const auto client_mock = Aws::MakeShared<KmsClientMock>(CLASS_TAG);
    kms_client_mocks.insert({ region, client_mock });

    report_success = [] {};
    return client_mock;
}

const std::shared_ptr<KmsClientMock> KmsClientSupplierMock::GetClientMock(const Aws::String &region) const {
    const auto client_mock_iter = kms_client_mocks.find(region);
    return (client_mock_iter == kms_client_mocks.end()) ? nullptr : client_mock_iter->second;
}

const Aws::Map<Aws::String, std::shared_ptr<KmsClientMock>> &KmsClientSupplierMock::GetClientMocksMap() const {
    return kms_client_mocks;
}

}  // namespace Testing
}  // namespace Cryptosdk
}  // namespace Aws
