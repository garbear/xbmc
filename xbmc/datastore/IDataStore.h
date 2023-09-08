/*
 *  Copyright (C) 2018-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>
#include <vector>

namespace KODI
{
namespace DATASTORE
{
class IDataStore
{
public:
  virtual ~IDataStore();

  /*!
   * \brief Open and connect to the data store
   *
   * \param dataStorePath The directory dedicated to this data store. Formed
   *                      by adding the name to the data store profile path.
   *
   * \return True on success, false on failure
   */
  virtual bool Open(const std::string& dataStorePath) = 0;

  /*!
   * \brief Close the data store and release all resources
   */
  virtual void Close() = 0;

  /*!
   * \brief Determine if the key can be found
   *
   * \param key The key to look for
   * \param keySize The size of the key, in bytes
   *
   * \return True if data with the given key was found, false otherwise
   */
  virtual bool Has(const uint8_t* key, size_t keySize) = 0;

  /*!
   * \brief Get data from the data store
   *
   * \param key The key used to look up the data
   * \param keySize The key size, in bytes
   * \param[out] data The data, or unmodified if this function returns false
   *
   * \return True if the data was retrieved, false for missing key or error
   */
  virtual bool Get(const uint8_t* key, size_t keySize, std::vector<uint8_t>& data) = 0;

  /*!
   * \brief Reserve a buffer in the data store for the given key, saving an
   *        extra memcpy
   *
   * Duplicates are not allowed, so this replaces any previously existing
   * key.
   *
   * \param key The key used to add the data
   * \param keySize The key size, in bytes
   * \param dataSize The size of the buffer to reserve
   *
   * \return A pointer to the reserved space which the caller can write to,
   *         or nullptr if a buffer of the requested size could not be
   *         reserved
   */
  virtual uint8_t* Reserve(const uint8_t* key, size_t keySize, size_t dataSize) = 0;

  /*!
   * \brief Commit reserved memory to the data store
   *
   * \param data The data returned by Reserve()
   */
  virtual void Commit(const uint8_t* data) = 0;

  /*!
   * \brief Add a key/value pair to the data store
   *
   * Duplicates are not allowed, so this replaces any previously existing
   * key.
   *
   * \param key The key used to add the data
   * \param keySize The key size, in bytes
   * \param data The data to store
   * \param dataSize The size of the data to store
   *
   * \return True if the data was added, false on error
   */
  virtual bool Put(const uint8_t* key, size_t keySize, const uint8_t* data, size_t dataSize) = 0;

  /*!
   * \brief Delete data based on its key
   *
   * \param key The key to look for
   * \param keySize The size of the key, in bytes
   *
   * \return True on success, false on error
   */
  virtual bool Delete(const uint8_t* key, size_t keySize) = 0;
};
} // namespace DATASTORE
} // namespace KODI
