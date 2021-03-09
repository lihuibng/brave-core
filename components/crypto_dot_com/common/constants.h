/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_CRYPTO_DOT_COM_COMMON_CONSTANTS_H_
#define BRAVE_COMPONENTS_CRYPTO_DOT_COM_COMMON_CONSTANTS_H_

// TODO(simonhong): Use prod url.
// prod url: https://auth.crypto.com/exchange/widget/sign_in
constexpr char kCryptoDotComAuthURL[] =
    "https://st.mona.co/exchange/widget/sign_in/";
// prod url: https://auth.crypto.com/exchange/widget/sign_in
constexpr char kCryptoDotComGetAccountBalanceURL[] =
    "https://xdev-www.3ona.co/fe-ex-api/widget/get-account-summary";
// prod url: https://crypto.com/fe-ex-api/widget/get-deposit-address
constexpr char kCryptoDotComGetDepositAddressURL[] =
    "https://xdev-www.3ona.co/fe-ex-api/widget/get-deposit-address";
// prod url: https://crypto.com/fe-ex-api/widget/create-order
constexpr char kCryptoDotComCreateMarketOrderURL[] =
    "https://xdev-www.3ona.co/fe-ex-api/widget/create-order";
// prod url: https://crypto.com/fe-ex-api/widget/get-events
constexpr char kCryptoDotComGetNewsEventsURL[] =
    "https://xdev-www.3ona.co/fe-ex-api/widget/get-events";
#endif  // BRAVE_COMPONENTS_CRYPTO_DOT_COM_COMMON_CONSTANTS_H_
