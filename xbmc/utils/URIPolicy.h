/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "URIParser.h"

#include <algorithm>
#include <array>
#include <optional>
#include <string_view>

namespace URIPolicy
{
/*!
 * \brief Kodi operation policy, independent of generic validity and installed handlers.
 * \note Unknown schemes have no filesystem or transport-option semantics by default.
 *       Raw query preservation and full-body redaction are Kodi policies.
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-1.2.2
 */
struct Policy
{
  bool filesystem{false};
  bool queryIdentity{true};
  bool protocolOptions{false};
  bool redactBody{false};
  bool legacy{false};
};

/*!
 * \brief Recognize a scheme in Kodi mixed native-path/URI input, without validating its body.
 * \param input Raw location; alphabetic drive prefixes retain native interpretation.
 * \return A source view without the colon, valid until input changes or is destroyed.
 * \note Bare notes:2026 is an identifier; use ./notes:2026 or CURL::ParseNativePath for
 *       native intent. Explicit URIParser input has no drive-prefix exception.
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3.1
 * \sa https://www.rfc-editor.org/rfc/rfc8089.html#appendix-E.2
 * \sa https://www.rfc-editor.org/rfc/rfc8089.html#appendix-E.3
 */
inline std::optional<std::string_view> GetScheme(std::string_view input)
{
  auto scheme = URIParser::GetScheme(input);
  if (scheme && scheme->size() == 1)
    return std::nullopt;
  return scheme;
}

/*!
 * \brief Compare raw ASCII scheme names case-insensitively, without decoding.
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3.1
 */
inline bool SchemeEquals(std::string_view left, std::string_view right)
{
  return std::ranges::equal(left, right,
                            [](char a, char b)
                            {
                              auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? c + 32 : c; };
                              return lower(a) == lower(b);
                            });
}

/*!
 * \brief Return Kodi's small shared policy for a raw scheme name, case-insensitively.
 * \note A filesystem policy permits operations only on the identified path component.
 *       Established VFS compatibility projections are Kodi policy, not URI syntax.
 *       Magnet member paths require additional shape checks in CURL; they are a Kodi
 *       extension to external magnet syntax. URN NSS slashes never imply file hierarchy.
 * \sa https://www.iana.org/assignments/uri-schemes/prov/magnet
 * \sa https://www.bittorrent.org/beps/bep_0009.html#magnet-uri-format
 * \sa https://www.rfc-editor.org/rfc/rfc8141.html#section-4.3
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3
 */
inline Policy GetPolicy(std::string_view scheme)
{
  if (SchemeEquals(scheme, "data"))
    return {false, true, false, true, false};
  if (SchemeEquals(scheme, "magnet"))
    return {true, true, false, false, false};
  if (scheme.empty())
    return {true, false, false, false, false};

  // Established Kodi adapters, including virtual locations and VFS spellings.
  constexpr std::string_view legacySchemes[] = {
      "http",       "https",    "ftp",     "ftps",        "sftp",     "ssh",       "smb",
      "nfs",        "afp",      "dav",     "davs",        "shout",    "shouts",    "rss",
      "rsss",       "rtsp",     "rtmp",    "rtmpt",       "rtmpe",    "rtmpte",    "rtmps",
      "mms",        "mmst",     "mmsh",    "udp",         "rtp",      "tcp",       "sdp",
      "upnp",       "file",     "special", "resource",    "stack",    "multipath", "virtualpath",
      "zip",        "rar",      "apk",     "xbt",         "archive",  "image",     "udf",
      "iso9660",    "bluray",   "cdda",    "dvd",         "plugin",   "script",    "addons",
      "sources",    "musicdb",  "videodb", "musicsearch", "pvr",      "library",   "favourites",
      "androidapp", "episodes", "pipe",    "win-lib",     "zeroconf", "events",    "sacd"};
  if (std::ranges::any_of(legacySchemes,
                          [scheme](auto candidate) { return SchemeEquals(scheme, candidate); }))
    return {true, false, true, false, true};
  return {};
}

/*!
 * \brief Recognize Kodi's legacy leading scheme:// spelling, not every URI or URL.
 * \note Native drive prefixes take precedence. Body validity and backend support are unchecked.
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-1.1.3
 */
inline bool IsLegacyURL(std::string_view input)
{
  const auto scheme = GetScheme(input);
  return scheme && input.substr(scheme->size() + 1, 2) == "//";
}
} // namespace URIPolicy
