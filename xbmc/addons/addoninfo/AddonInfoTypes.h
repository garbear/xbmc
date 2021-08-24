/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/AddonVersion.h"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ADDON
{

class CAddonInfo;
using AddonInfoPtr = std::shared_ptr<CAddonInfo>;
using AddonInfos = std::vector<AddonInfoPtr>;

enum class AddonDisabledReason
{
  /// @brief Special reason for returning all disabled addons.
  ///
  /// Only used as an actual value when an addon is enabled.
  NONE = 0,
  USER = 1,
  INCOMPATIBLE = 2,
  PERMANENT_FAILURE = 3
};

enum class AddonOriginType
{
  /// @brief The type of the origin of an addon.
  ///
  /// Represents where an addon was installed from.
  SYSTEM = 0, /// The addon is a system addon
  REPOSITORY = 1, /// The addon origin is a repository
  MANUAL = 2 /// The addon origin is a zip file, package or development build
};

//! @brief Reasons why an addon is not updateable
enum class AddonUpdateRule
{
  ANY = 0, //!< used internally, not to be explicitly set
  USER_DISABLED_AUTO_UPDATE = 1, //!< automatic updates disabled via AddonInfo dialog
  PIN_OLD_VERSION = 2 //!< user downgraded to an older version
};

/*!
 * @brief Add-on state defined within addon.xml to report about the current addon
 * lifecycle state.
 *
 * E.g. the add-on is broken and can no longer be used.
 *
 * XML examples:
 * ~~~~~~~~~~~~~{.xml}
 * <lifecyclestate type="broken" lang="en_GB">SOME TEXT</lifecyclestate>
 * ~~~~~~~~~~~~~
 */
enum class AddonLifecycleState
{
  NORMAL = 0, //!< Used if an add-on has no special lifecycle state which is the default state
  DEPRECATED = 1, //!< the add-on should be marked as deprecated but is still usable
  BROKEN = 2, //!< the add-on should marked as broken in the repository
};

struct DependencyInfo
{
  std::string id;
  AddonVersion versionMin, version;
  bool optional;
  DependencyInfo(std::string id,
                 const AddonVersion& versionMin,
                 const AddonVersion& version,
                 bool optional)
    : id(std::move(id)),
      versionMin(versionMin.empty() ? version : versionMin),
      version(version),
      optional(optional)
  {
  }

  bool operator==(const DependencyInfo& rhs) const
  {
    return id == rhs.id && versionMin == rhs.versionMin && version == rhs.version &&
           optional == rhs.optional;
  }

  bool operator!=(const DependencyInfo& rhs) const
  {
    return !(rhs == *this);
  }
};

using InfoMap = std::map<std::string, std::string>;
using ArtMap = std::map<std::string, std::string>;

} /* namespace ADDON */
