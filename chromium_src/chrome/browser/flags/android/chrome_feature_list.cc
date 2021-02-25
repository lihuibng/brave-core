/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/flags/android/chrome_feature_list.h"

#include "brave/common/brave_features.h"
#include "third_party/blink/public/common/features.h"

namespace chrome {
namespace android {

extern const base::Feature kTabGroupsAndroidDummy;

namespace {

const base::Feature* kBraveFeaturesExposedToJava[] = {
    &features::kBraveRewards,
    &blink::features::kForceWebContentsDarkMode,
    &kTabGroupsAndroid,
};

const base::Feature* BraveFindFeatureExposedToJava(
    const std::string& feature_name) {
  for (size_t i = 0; i < base::size(kBraveFeaturesExposedToJava); ++i) {
    if (kBraveFeaturesExposedToJava[i]->name == feature_name)
      return kBraveFeaturesExposedToJava[i];
  }

  return nullptr;
}

}  // namespace

}  // namespace android
}  // namespace chrome

#define BRAVE_FIND_FEATURE_EXPOSED_TO_JAVA                                    \
  const base::Feature* feature = BraveFindFeatureExposedToJava(feature_name); \
  if (feature)                                                                \
    return feature;

#define kTabGroupsAndroid kTabGroupsAndroidDummy
#include "../../../../../../chrome/browser/flags/android/chrome_feature_list.cc"
#undef kTabGroupsAndroid
#undef BRAVE_FIND_FEATURE_EXPOSED_TO_JAVA

namespace chrome {
namespace android {

const base::Feature kTabGroupsAndroid{"TabGroupsAndroid",
                                      base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace android
}  // namespace chrome
