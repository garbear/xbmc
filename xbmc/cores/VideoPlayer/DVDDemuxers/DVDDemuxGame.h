/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "DVDDemux.h"
#include "DVDInputStreams/DVDInputStream.h"

class CDVDDemuxGame : public CDVDDemux
{
public:
  CDVDDemuxGame(CDVDInputStream *pInput);

  virtual ~CDVDDemuxGame();

  bool Open();
  void Dispose();

  // implementation of CDVDDemux
  virtual void Reset() override;
  virtual void Abort() override;
  virtual void Flush() override;
  virtual DemuxPacket* Read() override;
  virtual bool SeekTime(int time, bool backwords = false, double* startpts = NULL) override;
  virtual void SetSpeed(int iSpeed) override;
  virtual int GetStreamLength() override;
  virtual std::vector<CDemuxStream*> GetStreams() const override;
  virtual int GetNrOfStreams() const override;
  virtual std::string GetFileName() override;
  virtual std::string GetStreamCodecName(int iStreamId) override;
  virtual void EnableStream(int id, bool enable) override;

protected:
  virtual CDemuxStream* GetStream(int iStreamId) const override;

private:
  CDVDInputStream* const m_pInput;
  CDVDInputStream::IDemux* m_IDemux;
};
