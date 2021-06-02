/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2VideoNode.h"

#include "smarthome/ros2/Ros2Translator.h"
#include "smarthome/streams/SmartHomeStreamSwFramebuffer.h"
#include "smarthome/streams/SmartHomeStreams.h"
#include "utils/log.h"

#include <cstring>

#include <image_transport/transport_hints.hpp>
#include <rclcpp/rclcpp.hpp>

extern "C"
{
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

using namespace KODI;
using namespace SMART_HOME;

namespace
{

// The ROS namespace
constexpr const char* ROS_NAMESPACE = "oasis"; // TODO

// Name of the ROS node
constexpr const char* NODE_NAME = "kodi"; // TODO: Hostname?

// Name of the OS thread
constexpr const char* THREAD_NAME = "ROS2"; // TODO

} // namespace

CRos2VideoNode::CRos2VideoNode(CSmartHomeStreams& streams) : m_streams(streams)
{
}

const char* CRos2VideoNode::NodeName() const
{
  return NODE_NAME;
}

const char* CRos2VideoNode::ThreadName() const
{
  return THREAD_NAME;
}

void CRos2VideoNode::InitializeInternal(std::shared_ptr<rclcpp::Node> node)
{
  m_node = std::move(node);

  m_imgTransport = std::make_unique<image_transport::ImageTransport>(m_node);

  const std::vector<std::string> machines = {"netbook", "lenovo"};
  const std::vector<std::string> topics = {"image_raw"};
  for (const auto& machine : machines)
  {
    for (const auto& topic : topics)
    {
      const std::string topicPath = StringUtils::Format("/{}/{}/{}", ROS_NAMESPACE, machine, topic);
      const auto transportHints = image_transport::TransportHints(m_node.get(), "compressed");

      CLog::Log(LOGDEBUG, "ROS2: Subscribing to {}", topicPath);

      m_imgSubscriber = std::make_unique<image_transport::Subscriber>();
      *m_imgSubscriber = m_imgTransport->subscribe(topicPath, 1, &CRos2VideoNode::ReceiveImage,
                                                   this, &transportHints);

      break; // TODO
    }
    break; // TODO
  }
}

void CRos2VideoNode::DeinitializeInternal()
{
  if (m_stream)
    m_streams.CloseStream(m_stream.release());

  if (m_pixelScaler != nullptr)
  {
    sws_freeContext(m_pixelScaler);
    m_pixelScaler = nullptr;
  }

  m_imgSubscriber.reset();
  m_imgTransport.reset();
  m_node.reset();
}

void CRos2VideoNode::ReceiveImage(const std::shared_ptr<const sensor_msgs::msg::Image>& msg)
{
  const uint32_t width = msg->width;
  const uint32_t height = msg->height;
  const bool isBigEndian = static_cast<bool>(msg->is_bigendian);
  const AVPixelFormat format = CRos2Translator::TranslateEncoding(msg->encoding, isBigEndian);
  const uint32_t stride = msg->step;
  const size_t size = msg->data.size();
  const uint8_t* const data = msg->data.data();

  //CLog::Log(LOGDEBUG, "SMARTHOME: Got frame");

  if (format == AV_PIX_FMT_NONE)
  {
    CLog::Log(LOGERROR, "ROS2: Unknown encoding: {}", msg->encoding);
    return;
  }

  if (stride * height != size)
  {
    CLog::Log(LOGERROR, "ROS2 Video: Invalid stride (width={}, height={}, stride={}, size={}",
              width, height, stride, size);
    return;
  }

  // Create stream if it doesn't exist
  if (!m_stream)
  {
    m_stream.reset(m_streams.OpenStream(format, width, height));
    if (!m_stream)
    {
      CLog::Log(LOGERROR, "ROS2: Failed to create stream of type {} ({}x{})", msg->encoding, width,
                height);
      return;
    }
  }

  AVPixelFormat targetFormat = AV_PIX_FMT_NONE;
  uint8_t* targetData = nullptr;
  size_t targetSize = 0;
  if (!m_stream->GetBuffer(width, height, targetFormat, targetData, targetSize) ||
      targetData == nullptr)
  {
    CLog::Log(LOGERROR, "ROS2: Failed to get a buffer of {}x{} from the stream", width, height);
    return;
  }

  // Pixel conversion
  const unsigned int sourceStride = static_cast<unsigned int>(size / height);
  const unsigned int targetStride = static_cast<unsigned int>(targetSize / height);

  SwsContext*& scalerContext = m_pixelScaler;
  scalerContext = sws_getCachedContext(scalerContext, width, height, format, width, height,
                                       targetFormat, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

  if (scalerContext == nullptr)
  {
    CLog::Log(LOGERROR, "ROS2: Failed to create pixel scaler");
    m_stream->ReleaseBuffer(targetData);
    return;
  }

  uint8_t* src[] = {const_cast<uint8_t*>(data), nullptr, nullptr, nullptr};
  int srcStride[] = {static_cast<int>(sourceStride), 0, 0, 0};
  uint8_t* dst[] = {targetData, nullptr, nullptr, nullptr};
  int dstStride[] = {static_cast<int>(targetStride), 0, 0, 0};

  sws_scale(scalerContext, src, srcStride, 0, height, dst, dstStride);

  m_stream->AddData(width, height, targetData, targetSize);

  m_stream->ReleaseBuffer(targetData);
}
