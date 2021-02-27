/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_URL_FETCHER_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_URL_FETCHER_H_

#include "bat/ledger/internal/core/async_result.h"
#include "bat/ledger/internal/core/bat_ledger_context.h"
#include "bat/ledger/public/interfaces/ledger.mojom.h"

namespace ledger {

class URLFetcher : public BATLedgerContext::Component {
 public:
  static const BATLedgerContext::ComponentKey kComponentKey;

  explicit URLFetcher(BATLedgerContext* context);

  struct FetchOptions {
    bool log_response_body = false;
    bool disable_logging = false;
  };

  using FetchResult = AsyncResult<mojom::UrlResponsePtr>;
  FetchResult Fetch(mojom::UrlRequestPtr request);
  FetchResult Fetch(mojom::UrlRequestPtr request, FetchOptions options);

 protected:
  virtual FetchResult FetchImpl(mojom::UrlRequestPtr request,
                                FetchOptions options);

 private:
  void LogURLRequest(const mojom::UrlRequest& request, FetchOptions options);
  void LogURLResponse(const mojom::UrlResponse& response, FetchOptions options);
  void LogURLResponseDetails(const mojom::UrlResponse& response,
                             FetchOptions options);
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_CORE_URL_FETCHER_H_
