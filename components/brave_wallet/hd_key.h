/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_HD_KEY_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_HD_KEY_H_

#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"

namespace brave_wallet {

FORWARD_DECLARE_TEST(HDKeyUnitTest, GenerateFromExtendedKey);

class HDKey {
 public:
  HDKey();
  HDKey(uint8_t depth, uint32_t parent_fingerprint, uint32_t index);
  ~HDKey();

  static std::unique_ptr<HDKey> GenerateFromSeed(
      const std::vector<uint8_t>& seed);

  static std::unique_ptr<HDKey> GenerateFromExtendedKey(const std::string& key);

  void SetPrivateKey(const std::vector<uint8_t>& value);
  std::string GetPrivateExtendedKey() const;

  void SetPublicKey(const std::vector<uint8_t>& value);
  std::string GetPublicExtendedKey() const;

  void SetChainCode(const std::vector<uint8_t>& value);

  std::unique_ptr<HDKey> DeriveChild(uint32_t index);
  std::unique_ptr<HDKey> DeriveChildFromPath(const std::string& path);

 private:
  FRIEND_TEST_ALL_PREFIXES(HDKeyUnitTest, GenerateFromExtendedKey);
  void GeneratePublicKey();
  const std::vector<uint8_t> Hash160(const std::vector<uint8_t>& input);
  std::string Serialize(uint32_t version,
                        const std::vector<uint8_t>& key) const;

  uint8_t depth_;
  uint32_t fingerprint_;
  uint32_t parent_fingerprint_;
  uint32_t index_;
  std::vector<uint8_t> identifier_;
  std::vector<uint8_t> private_key_;
  std::vector<uint8_t> public_key_;
  std::vector<uint8_t> chain_code_;

  HDKey(const HDKey&) = delete;
  HDKey& operator=(const HDKey&) = delete;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_HD_KEY_H_
