/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/permission_lifetime_manager.h"

#include <algorithm>

#include "base/auto_reset.h"
#include "base/stl_util.h"
#include "base/strings/strcat.h"
#include "brave/components/permissions/permission_lifetime_pref_names.h"
#include "components/content_settings/core/browser/content_settings_utils.h"
#include "components/content_settings/core/browser/website_settings_info.h"
#include "components/content_settings/core/browser/website_settings_registry.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "services/preferences/public/cpp/dictionary_value_update.h"
#include "services/preferences/public/cpp/scoped_pref_update.h"

using content_settings::WebsiteSettingsInfo;
using content_settings::WebsiteSettingsRegistry;

namespace permissions {

namespace {

constexpr base::StringPiece kRequestingOriginKey = "ro";
constexpr base::StringPiece kEmbeddingOriginKey = "eo";

base::Time ParseExpirationTime(const std::string& time_str) {
  int64_t expiration_time = 0;
  if (!base::StringToInt64(time_str, &expiration_time)) {
    return base::Time();
  }
  return base::Time::FromDeltaSinceWindowsEpoch(
      base::TimeDelta::FromMicroseconds(expiration_time));
}

std::string ExpirationTimeToStr(base::Time expiration_time) {
  return std::to_string(
      expiration_time.ToDeltaSinceWindowsEpoch().InMicroseconds());
}

}  // namespace

// static
void PermissionLifetimeManager::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterDictionaryPref(prefs::kPermissionLifetimeRoot);
  registry->RegisterDictionaryPref(prefs::kPermissionLifetimeExpirations);
}

PermissionLifetimeManager::PermissionLifetimeManager(
    HostContentSettingsMap* host_content_settings_map,
    PrefService* prefs)
    : host_content_settings_map_(host_content_settings_map), prefs_(prefs) {
  DCHECK(host_content_settings_map_);
  // In incognito prefs_ is nullptr.

  ReadExpirationsFromPrefs();
  ResetExpiredPermissionsAndUpdateTimer(base::Time::Now());

  host_content_settings_map_observation_.Observe(host_content_settings_map_);
}

PermissionLifetimeManager::~PermissionLifetimeManager() {}

void PermissionLifetimeManager::Shutdown() {
  host_content_settings_map_observation_.Reset();
  StopExpirationTimer();
}

void PermissionLifetimeManager::PermissionDecided(
    const PermissionRequest& permission_request,
    const GURL& requesting_origin,
    const GURL& embedding_origin,
    ContentSetting content_setting,
    bool is_one_time) {
  if (!permission_request.SupportsLifetime() ||
      content_setting != ContentSetting::CONTENT_SETTING_ALLOW || is_one_time) {
    // Only interested in ALLOW and non one-time (Chromium geolocation-specific)
    // decisions.
    return;
  }

  const auto& lifetime = permission_request.GetLifetime();
  if (!lifetime || *lifetime == base::TimeDelta()) {
    // If no lifetime is set, then we don't need to do anything here.
    return;
  }

  const ContentSettingsType content_type =
      permission_request.GetContentSettingsType();
  const base::Time expiration_time = base::Time::Now() + *lifetime;

  DVLOG(1) << "PermissionLifetimeManager::PermissionDecided"
           << "\ntype: "
           << WebsiteSettingsRegistry::GetInstance()->Get(content_type)->name()
           << "\nrequesting_origin: " << requesting_origin
           << "\nembedding_origin: " << embedding_origin
           << "\ncontent_setting: "
           << content_settings::ContentSettingToString(content_setting)
           << "\nlifetime: " << permission_request.GetLifetime()->InSeconds()
           << " seconds";

  expirations_[content_type][expiration_time].push_back(
      ExpiringPermission{.requesting_origin = requesting_origin,
                         .embedding_origin = embedding_origin});
  UpdateTimeExpirationsPref(content_type, {expiration_time});

  UpdateExpirationTimer();
}

void PermissionLifetimeManager::OnContentSettingChanged(
    const ContentSettingsPattern& primary_pattern,
    const ContentSettingsPattern& secondary_pattern,
    ContentSettingsType content_type) {
  DVLOG(1) << "PermissionLifetimeManager::OnContentSettingChanged"
           << "\nis_currently_removing_permissions_ "
           << is_currently_removing_permissions_ << "\ntype: "
           << WebsiteSettingsRegistry::GetInstance()->Get(content_type)->name()
           << "\nprimary_pattern: " << primary_pattern.ToString()
           << "\nsecondary_pattern: " << secondary_pattern.ToString();

  if (is_currently_removing_permissions_) {
    return;
  }

  auto type_expirations_it = expirations_.find(content_type);
  if (type_expirations_it == expirations_.end()) {
    return;
  }

  base::flat_set<base::Time> time_items_to_update_prefs;

  auto& time_expirations_map = type_expirations_it->second;
  for (auto& time_expirations : time_expirations_map) {
    auto& expiring_permissions = time_expirations.second;
    for (auto expiring_permission_it = expiring_permissions.begin();
         expiring_permission_it != expiring_permissions.end();) {
      if (primary_pattern.IsValid() &&
          !primary_pattern.Matches(expiring_permission_it->requesting_origin)) {
        ++expiring_permission_it;
        continue;
      }
      if (secondary_pattern.IsValid() &&
          !secondary_pattern.Matches(
              expiring_permission_it->embedding_origin)) {
        ++expiring_permission_it;
        continue;
      }
      if (host_content_settings_map_->GetContentSetting(
              expiring_permission_it->requesting_origin,
              expiring_permission_it->embedding_origin,
              content_type) == CONTENT_SETTING_ALLOW) {
        ++expiring_permission_it;
        continue;
      }
      expiring_permission_it =
          expiring_permissions.erase(expiring_permission_it);
      time_items_to_update_prefs.insert(time_expirations.first);
    }
  }

  if (time_items_to_update_prefs.empty()) {
    return;
  }

  base::EraseIf(time_expirations_map, [](const auto& time_expirations) {
    return time_expirations.second.empty();
  });
  UpdateTimeExpirationsPref(content_type, time_items_to_update_prefs);
  UpdateExpirationTimer();
}

void PermissionLifetimeManager::UpdateTimeExpirationsPref(
    ContentSettingsType content_type,
    const base::flat_set<base::Time>& time_items) {
  if (!prefs_) {
    return;
  }

  ::prefs::ScopedDictionaryPrefUpdate update(
      prefs_, prefs::kPermissionLifetimeExpirations);
  std::unique_ptr<::prefs::DictionaryValueUpdate> expirations_val =
      update.Get();

  const std::string& content_type_name =
      WebsiteSettingsRegistry::GetInstance()->Get(content_type)->name();

  const auto& type_expirations_it = expirations_.find(content_type);
  if (type_expirations_it == expirations_.end()) {
    expirations_val->RemovePath(content_type_name, nullptr);
    return;
  }

  const auto& time_expirations_map = type_expirations_it->second;
  for (const auto& time_to_update : time_items) {
    const auto& time_expirations_it = time_expirations_map.find(time_to_update);
    if (time_expirations_it == time_expirations_map.end() ||
        time_expirations_it->second.empty()) {
      expirations_val->RemovePath(
          base::StrCat(
              {content_type_name, ".", ExpirationTimeToStr(time_to_update)}),
          nullptr);
    } else {
      expirations_val->SetPath(
          {content_type_name, ExpirationTimeToStr(time_to_update)},
          ExpiringPermissionsToValue(time_expirations_it->second));
    }
  }
}

void PermissionLifetimeManager::UpdateExpirationTimer() {
  base::Time nearest_expiration_time(base::Time::Max());
  for (const auto& type_expirations : expirations_) {
    const auto& time_expirations_map = type_expirations.second;
    if (time_expirations_map.empty()) {
      continue;
    }
    nearest_expiration_time =
        std::min(nearest_expiration_time, time_expirations_map.begin()->first);
  }

  if (nearest_expiration_time == base::Time::Max()) {
    // Nothing to wait for. Stop the timer and return.
    StopExpirationTimer();
    return;
  }

  if (current_scheduled_expiration_time_ == nearest_expiration_time) {
    // Timer is already correct. Do nothing.
    DCHECK(expiration_timer_.IsRunning());
    return;
  }

  current_scheduled_expiration_time_ = nearest_expiration_time;
  expiration_timer_.Start(
      FROM_HERE, current_scheduled_expiration_time_,
      base::BindOnce(&PermissionLifetimeManager::OnExpirationTimer,
                     base::Unretained(this)));
}

void PermissionLifetimeManager::StopExpirationTimer() {
  expiration_timer_.Stop();
  current_scheduled_expiration_time_ = base::Time();
}

void PermissionLifetimeManager::OnExpirationTimer() {
  DCHECK(!current_scheduled_expiration_time_.is_null());
  ResetExpiredPermissionsAndUpdateTimer(current_scheduled_expiration_time_);
}

void PermissionLifetimeManager::ResetExpiredPermissionsAndUpdateTimer(
    base::Time current_expiration_time) {
  for (auto& type_expirations : expirations_) {
    const auto& content_type = type_expirations.first;
    auto& time_expirations_map = type_expirations.second;
    base::flat_set<base::Time> time_items_to_clear;

    for (const auto& time_expirations : time_expirations_map) {
      const base::Time& expiration_time = time_expirations.first;
      auto& expiring_permissions = time_expirations.second;
      if (expiration_time > current_expiration_time) {
        // If we encountered an expiration that is still active, then all next
        // expirations will also be active.
        break;
      }
      for (const auto& expiring_permission : expiring_permissions) {
        base::AutoReset<bool> auto_reset(&is_currently_removing_permissions_,
                                         true);
        host_content_settings_map_->SetContentSettingDefaultScope(
            expiring_permission.requesting_origin,
            expiring_permission.embedding_origin, content_type,
            CONTENT_SETTING_DEFAULT);
      }
      time_items_to_clear.insert(expiration_time);
    }

    for (const auto& time_item_to_clear : time_items_to_clear) {
      time_expirations_map.erase(time_item_to_clear);
    }
    base::EraseIf(time_expirations_map, [](const auto& time_expirations) {
      return time_expirations.second.empty();
    });

    UpdateTimeExpirationsPref(content_type, time_items_to_clear);
  }

  UpdateExpirationTimer();
}

void PermissionLifetimeManager::ReadExpirationsFromPrefs() {
  DCHECK(!expiration_timer_.IsRunning());
  DCHECK(expirations_.empty());

  if (!prefs_) {
    return;
  }

  const base::DictionaryValue* expirations_val =
      prefs_->GetDictionary(prefs::kPermissionLifetimeExpirations);
  if (!expirations_val) {
    return;
  }

  for (const auto& type_expirations_map_val : expirations_val->DictItems()) {
    const auto& type_str = type_expirations_map_val.first;
    const auto& time_expirations_map_val = type_expirations_map_val.second;
    const WebsiteSettingsInfo* website_settings_info =
        WebsiteSettingsRegistry::GetInstance()->GetByName(type_str);
    if (!website_settings_info) {
      continue;
    }
    if (!time_expirations_map_val.is_dict()) {
      continue;
    }
    TimeExpirationsMap time_expirations_map;
    for (const auto& time_expirations_val :
         time_expirations_map_val.DictItems()) {
      const auto& time_str = time_expirations_val.first;
      const auto& expirations_val = time_expirations_val.second;
      base::Time expiration_time = ParseExpirationTime(time_str);
      if (expiration_time.is_null()) {
        continue;
      }
      ExpiringPermissions expiring_permissions =
          ParseExpiringPermissions(expirations_val);
      if (expiring_permissions.empty()) {
        continue;
      }
      time_expirations_map.emplace(expiration_time,
                                   std::move(expiring_permissions));
    }
    if (!time_expirations_map.empty()) {
      expirations_.emplace(website_settings_info->type(),
                           std::move(time_expirations_map));
    }
  }
}

PermissionLifetimeManager::ExpiringPermissions
PermissionLifetimeManager::ParseExpiringPermissions(
    const base::Value& expirations_val) {
  ExpiringPermissions result;
  if (!expirations_val.is_list()) {
    return result;
  }

  result.reserve(expirations_val.GetList().size());
  for (const auto& item : expirations_val.GetList()) {
    if (!item.is_dict()) {
      continue;
    }
    const std::string* requesting_origin =
        item.FindStringKey(kRequestingOriginKey);
    const std::string* embedding_origin =
        item.FindStringKey(kEmbeddingOriginKey);
    if (!requesting_origin) {
      continue;
    }
    result.push_back(ExpiringPermission{
        .requesting_origin = GURL(*requesting_origin),
        .embedding_origin =
            GURL(embedding_origin ? *embedding_origin : *requesting_origin),
    });
  }

  return result;
}

base::Value PermissionLifetimeManager::ExpiringPermissionsToValue(
    const ExpiringPermissions& expiring_permissions) const {
  base::Value::ListStorage items;
  items.reserve(expiring_permissions.size());

  for (const auto& expiring_permission : expiring_permissions) {
    base::Value value(base::Value::Type::DICTIONARY);
    value.SetStringKey(kRequestingOriginKey,
                       expiring_permission.requesting_origin.spec());
    if (expiring_permission.embedding_origin !=
        expiring_permission.requesting_origin) {
      value.SetStringKey(kEmbeddingOriginKey,
                         expiring_permission.embedding_origin.spec());
    }
    items.push_back(std::move(value));
  }

  return base::Value(std::move(items));
}

}  // namespace permissions
