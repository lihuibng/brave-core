/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/hd_key.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_wallet {

TEST(HDKeyUnitTest, TestVector1) {
  std::vector<uint8_t> bytes;
  // path: m
  EXPECT_TRUE(
      base::HexStringToBytes("000102030405060708090a0b0c0d0e0f", &bytes));

  std::unique_ptr<HDKey> m_key = HDKey::GenerateFromSeed(bytes);
  EXPECT_EQ(m_key->GetPublicExtendedKey(),
            "xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESF"
            "jqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8");
  EXPECT_EQ(m_key->GetPrivateExtendedKey(),
            "xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvN"
            "KmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi");
  EXPECT_EQ(m_key->DeriveChildFromPath("m")->GetPublicExtendedKey(),
            "xpub661MyMwAqRbcFtXgS5sYJABqqG9YLmC4Q1Rdap9gSE8NqtwybGhePY2gZ29ESF"
            "jqJoCu1Rupje8YtGqsefD265TMg7usUDFdp6W1EGMcet8");
  EXPECT_EQ(m_key->DeriveChildFromPath("m")->GetPrivateExtendedKey(),
            "xprv9s21ZrQH143K3QTDL4LXw2F7HEK3wJUD2nW2nRk4stbPy6cq3jPPqjiChkVvvN"
            "KmPGJxWUtg6LnF5kejMRNNU3TGtRBeJgk33yuGBxrMPHi");

  // path: m/0'
  std::unique_ptr<HDKey> m_0h_key = m_key->DeriveChild(0x80000000);
  EXPECT_EQ(m_0h_key->GetPrivateExtendedKey(),
            "xprv9uHRZZhk6KAJC1avXpDAp4MDc3sQKNxDiPvvkX8Br5ngLNv1TxvUxt4cV1rGL5"
            "hj6KCesnDYUhd7oWgT11eZG7XnxHrnYeSvkzY7d2bhkJ7");
  EXPECT_EQ(m_0h_key->GetPublicExtendedKey(),
            "xpub68Gmy5EdvgibQVfPdqkBBCHxA5htiqg55crXYuXoQRKfDBFA1WEjWgP6LHhwBZ"
            "eNK1VTsfTFUHCdrfp1bgwQ9xv5ski8PX9rL2dZXvgGDnw");

  // path: m/0'/1
  std::unique_ptr<HDKey> m_0h_1_key = m_0h_key->DeriveChild(1);
  EXPECT_EQ(m_0h_1_key->GetPrivateExtendedKey(),
            "xprv9wTYmMFdV23N2TdNG573QoEsfRrWKQgWeibmLntzniatZvR9BmLnvSxqu53Kw1"
            "UmYPxLgboyZQaXwTCg8MSY3H2EU4pWcQDnRnrVA1xe8fs");
  EXPECT_EQ(m_0h_1_key->GetPublicExtendedKey(),
            "xpub6ASuArnXKPbfEwhqN6e3mwBcDTgzisQN1wXN9BJcM47sSikHjJf3UFHKkNAWbW"
            "MiGj7Wf5uMash7SyYq527Hqck2AxYysAA7xmALppuCkwQ");
}

TEST(HDKeyUnitTest, TestVector2) {
  std::vector<uint8_t> bytes;
  // path: m
  EXPECT_TRUE(base::HexStringToBytes(
      "fffcf9f6f3f0edeae7e4e1dedbd8d5d2cfccc9c6c3c0bdbab7b4b1aeaba8a5a29f9c9996"
      "93908d8a8784817e7b7875726f6c696663605d5a5754514e4b484542",
      &bytes));

  std::unique_ptr<HDKey> m_key = HDKey::GenerateFromSeed(bytes);
  EXPECT_EQ(m_key->GetPrivateExtendedKey(),
            "xprv9s21ZrQH143K31xYSDQpPDxsXRTUcvj2iNHm5NUtrGiGG5e2DtALGdso3pGz6s"
            "srdK4PFmM8NSpSBHNqPqm55Qn3LqFtT2emdEXVYsCzC2U");
  EXPECT_EQ(m_key->GetPublicExtendedKey(),
            "xpub661MyMwAqRbcFW31YEwpkMuc5THy2PSt5bDMsktWQcFF8syAmRUapSCGu8ED9W"
            "6oDMSgv6Zz8idoc4a6mr8BDzTJY47LJhkJ8UB7WEGuduB");

  // path: m/0
  std::unique_ptr<HDKey> m_0_key = m_key->DeriveChild(0);
  EXPECT_EQ(m_0_key->GetPrivateExtendedKey(),
            "xprv9vHkqa6EV4sPZHYqZznhT2NPtPCjKuDKGY38FBWLvgaDx45zo9WQRUT3dKYnjw"
            "ih2yJD9mkrocEZXo1ex8G81dwSM1fwqWpWkeS3v86pgKt");
  EXPECT_EQ(m_0_key->GetPublicExtendedKey(),
            "xpub69H7F5d8KSRgmmdJg2KhpAK8SR3DjMwAdkxj3ZuxV27CprR9LgpeyGmXUbC6wb"
            "7ERfvrnKZjXoUmmDznezpbZb7ap6r1D3tgFxHmwMkQTPH");
}

TEST(HDKeyUnitTest, GenerateFromExtendedKey) {
  // m/0/2147483647'/1/2147483646'/2
  std::unique_ptr<HDKey> hdkey_from_pri = HDKey::GenerateFromExtendedKey(
      "xprvA2nrNbFZABcdryreWet9Ea4LvTJcGsqrMzxHx98MMrotbir7yrKCEXw7nadnHM8Dq38E"
      "GfSh6dqA9QWTyefMLEcBYJUuekgW4BYPJcr9E7j");
  EXPECT_EQ(hdkey_from_pri->depth_, 5u);
  EXPECT_EQ(hdkey_from_pri->parent_fingerprint_, 0x31a507b8u);
  EXPECT_EQ(hdkey_from_pri->index_, 2u);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(hdkey_from_pri->chain_code_)),
            "9452b549be8cea3ecb7a84bec10dcfd94afe4d129ebfd3b3cb58eedf394ed271");
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(hdkey_from_pri->private_key_)),
            "bb7d39bdb83ecf58f2fd82b6d918341cbef428661ef01ab97c28a4842125ac23");
  EXPECT_EQ(
      base::ToLowerASCII(base::HexEncode(hdkey_from_pri->public_key_)),
      "024d902e1a2fc7a8755ab5b694c575fce742c48d9ff192e63df5193e4c7afe1f9c");
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(hdkey_from_pri->identifier_)),
            "26132fdbe7bf89cbc64cf8dafa3f9f88b8666220");

  // m/0/2147483647'/1/2147483646'/2
  std::unique_ptr<HDKey> hdkey_from_pub = HDKey::GenerateFromExtendedKey(
      "xpub6FnCn6nSzZAw5Tw7cgR9bi15UV96gLZhjDstkXXxvCLsUXBGXPdSnLFbdpq8p9HmGsAp"
      "ME5hQTZ3emM2rnY5agb9rXpVGyy3bdW6EEgAtqt");
  EXPECT_EQ(hdkey_from_pub->depth_, 5u);
  EXPECT_EQ(hdkey_from_pub->parent_fingerprint_, 0x31a507b8u);
  EXPECT_EQ(hdkey_from_pub->index_, 2u);
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(hdkey_from_pub->chain_code_)),
            "9452b549be8cea3ecb7a84bec10dcfd94afe4d129ebfd3b3cb58eedf394ed271");
  EXPECT_TRUE(hdkey_from_pub->private_key_.empty());
  EXPECT_EQ(
      base::ToLowerASCII(base::HexEncode(hdkey_from_pub->public_key_)),
      "024d902e1a2fc7a8755ab5b694c575fce742c48d9ff192e63df5193e4c7afe1f9c");
  EXPECT_EQ(base::ToLowerASCII(base::HexEncode(hdkey_from_pub->identifier_)),
            "26132fdbe7bf89cbc64cf8dafa3f9f88b8666220");
}

}  // namespace brave_wallet
