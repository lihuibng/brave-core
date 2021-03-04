/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_FILE_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_FILE_UTIL_H_

#include <string>

#include "base/files/file.h"
#include "base/memory/weak_ptr.h"

namespace brave_rewards {

bool TailFile(
    base::File* file,
    const int num_lines);

bool TailFileAsString(
    base::File* file,
    const int num_lines,
    std::string* value);

std::string GetLastFileError(
    base::File* file);

class WeakWrapperFile : public base::File,
                        public base::SupportsWeakPtr<WeakWrapperFile> {
 public:
  WeakWrapperFile() {}
  ~WeakWrapperFile() {}

  WeakWrapperFile(WeakWrapperFile&& other)
      : base::File::File(std::move(other)) {}
};

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_BROWSER_FILE_UTIL_H_
