/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_LIFETIME_MANAGER_H_
#define BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_LIFETIME_MANAGER_H_

#include <map>
#include <memory>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/scoped_observation.h"
#include "base/util/timer/wall_clock_timer.h"
#include "components/content_settings/core/browser/content_settings_observer.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/permissions/permission_util.h"

namespace user_prefs {
class PrefRegistrySyncable;
}

class PrefService;

namespace permissions {

// Keeps permission expirations and resets permissions when a lifetime is
// expired.
class PermissionLifetimeManager : public KeyedService,
                                  public content_settings::Observer {
 public:
  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  PermissionLifetimeManager(HostContentSettingsMap* host_content_settings_map,
                            PrefService* prefs);
  PermissionLifetimeManager(const PermissionLifetimeManager&) = delete;
  PermissionLifetimeManager& operator=(const PermissionLifetimeManager&) =
      delete;
  ~PermissionLifetimeManager() override;

  // KeyedService:
  void Shutdown() override;

  // Saves permission lifetime to prefs and restarts expiration timer if
  // required.
  void PermissionDecided(const PermissionRequest& permission_request,
                         const GURL& requesting_origin,
                         const GURL& embedding_origin,
                         ContentSetting content_setting,
                         bool is_one_time);

  // content_settings::Observer:
  void OnContentSettingChanged(const ContentSettingsPattern& primary_pattern,
                               const ContentSettingsPattern& secondary_pattern,
                               ContentSettingsType content_type) override;

 private:
  friend class PermissionLifetimeManagerTest;

  struct ExpiringPermission {
    GURL requesting_origin;
    GURL embedding_origin;
  };

  using ExpiringPermissions = std::vector<ExpiringPermission>;
  using TimeExpirationsMap = std::map<base::Time, ExpiringPermissions>;
  using Expirations = base::flat_map<ContentSettingsType, TimeExpirationsMap>;

  // Update value in prefs, |time_items| used to update only selected items.
  void UpdateTimeExpirationsPref(ContentSettingsType content_type,
                                 const base::flat_set<base::Time>& time_items);
  // Restarts/stops Expiration timer if required.
  void UpdateExpirationTimer();
  // Stops Expiration timer.
  void StopExpirationTimer();
  // Resets permission, updates prefs and restarts Expiration timer if required.
  void OnExpirationTimer();
  void ResetExpiredPermissionsAndUpdateTimer(
      base::Time current_expiration_time);

  void ReadExpirationsFromPrefs();
  ExpiringPermissions ParseExpiringPermissions(
      const base::Value& expirations_val);
  base::Value ExpiringPermissionsToValue(
      const std::vector<ExpiringPermission>& expiring_permissions) const;

  HostContentSettingsMap* const host_content_settings_map_ = nullptr;
  PrefService* const prefs_ = nullptr;
  base::ScopedObservation<HostContentSettingsMap, content_settings::Observer>
      host_content_settings_map_observation_{this};

  // Expirations data from prefs used at runtime. Kept in sync with prefs.
  Expirations expirations_;
  // If an expiration timer is running, here is stored an expiration of the most
  // recent permission to expire.
  base::Time current_scheduled_expiration_time_;
  // WallClockTimer to reset permissions properly even if a machine was put in
  // a long sleep/wake cycle.
  util::WallClockTimer expiration_timer_;
  // Flag to ignore notifications from HostContentSettingsMap when a permission
  // reset is in progress.
  bool is_currently_removing_permissions_ = false;
};

}  // namespace permissions

#endif  // BRAVE_COMPONENTS_PERMISSIONS_PERMISSION_LIFETIME_MANAGER_H_
