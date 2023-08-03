/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ILibp2p.h"
#include "threads/Thread.h"

#include <memory>

namespace XFILE
{
class IIPFS;
} // namespace XFILE

namespace boost
{
namespace asio
{
class io_context;
} // namespace asio
} // namespace boost

namespace soralog
{
class LoggingSystem;
} // namespace soralog

namespace KODI
{
namespace NETWORK
{
class CLibp2p : public ILibp2p, private CThread
{
public:
  CLibp2p();
  ~CLibp2p() override;

  // Implementation of ILibp2p
  void Start() override;
  bool IsOnline() const override;
  void Stop(bool bWait) override;

  // Implementation of CThread
  void Process() override;

private:
  /*!
   * \brief Initialize libp2p's logging syste
   *
   * @todo Route logging through Kodi's logger instead of the console
   */
  static void InitializeLogging();

  // libp2p parameters
  static std::shared_ptr<soralog::LoggingSystem> m_loggingSystem;
  std::shared_ptr<boost::asio::io_context> m_ioContext;

  // IPFS parameters
  std::shared_ptr<XFILE::IIPFS> m_ipfs;
};
} // namespace NETWORK
} // namespace KODI
