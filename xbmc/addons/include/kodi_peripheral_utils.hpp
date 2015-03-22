/*
 *      Copyright (C) 2014 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "kodi_peripheral_types.h"

#include <cstring>
#include <map>
#include <string>
#include <vector>

#define PERIPHERAL_SAFE_DELETE(x)        do { delete   (x); (x) = NULL; } while (0)
#define PERIPHERAL_SAFE_DELETE_ARRAY(x)  do { delete[] (x); (x) = NULL; } while (0)

namespace ADDON
{
  /*!
   * Utility class to manipulate arrays of peripheral types.
   */
  template <class THE_CLASS, typename THE_STRUCT>
  class PeripheralVector
  {
  public:
    static void ToStructs(const std::vector<THE_CLASS>& vecObjects, THE_STRUCT** pStructs)
    {
      if (!pStructs)
        return;

      if (vecObjects.empty())
      {
        *pStructs = NULL;
      }
      else
      {
        (*pStructs) = new THE_STRUCT[vecObjects.size()];
        for (unsigned int i = 0; i < vecObjects.size(); i++)
          vecObjects.at(i).ToStruct((*pStructs)[i]);
      }
    }

    static void ToStructs(const std::vector<THE_CLASS*>& vecObjects, THE_STRUCT** pStructs)
    {
      if (!pStructs)
        return;

      if (vecObjects.empty())
      {
        *pStructs = NULL;
      }
      else
      {
        *pStructs = new THE_STRUCT[vecObjects.size()];
        for (unsigned int i = 0; i < vecObjects.size(); i++)
          vecObjects.at(i)->ToStruct((*pStructs)[i]);
      }
    }

    static void FreeStructs(unsigned int structCount, THE_STRUCT* structs)
    {
      if (structs)
      {
        for (unsigned int i = 0; i < structCount; i++)
          THE_CLASS::FreeStruct(structs[i]);
      }
      PERIPHERAL_SAFE_DELETE_ARRAY(structs);
    }
  };

  /*!
   * ADDON::Peripheral
   *
   * Wrapper class providing peripheral information. Classes can extend
   * Peripheral to inherit peripheral properties.
   */
  class Peripheral
  {
  public:
    Peripheral(PERIPHERAL_TYPE type = PERIPHERAL_TYPE_UNKNOWN, const std::string& strName = "")
    : m_type(type),
      m_strName(strName),
      m_vendorId(0),
      m_productId(0),
      m_index(0)
    {
    }

    Peripheral(PERIPHERAL_INFO& info)
    : m_type(info.type),
      m_strName(info.name ? info.name : ""),
      m_vendorId(info.vendor_id),
      m_productId(info.product_id),
      m_index(info.index)
    {
    }

    virtual ~Peripheral(void) { }

    PERIPHERAL_TYPE    Type(void) const      { return m_type; }
    const std::string& Name(void) const      { return m_strName; }
    uint16_t           VendorID(void) const  { return m_vendorId; }
    uint16_t           ProductID(void) const { return m_productId; }
    unsigned int       Index(void) const     { return m_index; }

    void SetType(PERIPHERAL_TYPE type)       { m_type      = type; }
    void SetName(const std::string& strName) { m_strName   = strName; }
    void SetVendorID(uint16_t vendorId)      { m_vendorId  = vendorId; }
    void SetProductID(uint16_t productId)    { m_productId = productId; }
    void SetIndex(unsigned int index)        { m_index     = index; }

    void ToStruct(PERIPHERAL_INFO& info) const
    {
      info.type       = m_type;
      info.name       = new char[m_strName.size() + 1];
      info.vendor_id  = m_vendorId;
      info.product_id = m_productId;
      info.index      = m_index;

      std::strcpy(info.name, m_strName.c_str());
    }

    static void FreeStruct(PERIPHERAL_INFO& info)
    {
      PERIPHERAL_SAFE_DELETE_ARRAY(info.name);
    }

  private:
    PERIPHERAL_TYPE  m_type;
    std::string      m_strName;
    uint16_t         m_vendorId;
    uint16_t         m_productId;
    unsigned int     m_index;
  };

  typedef PeripheralVector<Peripheral, PERIPHERAL_INFO> Peripherals;

  /*!
   * ADDON::JoystickFeature
   *
   * Base class for joystick features. In kodi_peripheral_types.h, the various
   * driver types are stored as an union. Here, we use polymorphism to allow for
   * extra driver information.
   */
  class JoystickFeature
  {
  public:
    JoystickFeature(void)
    : m_id()
    {
    }

    JoystickFeature(JOYSTICK_FEATURE_ID id)
    : m_id(id)
    {
    }

    JoystickFeature(const JOYSTICK_FEATURE& feature)
    : m_id(feature.id)
    {
    }

    virtual ~JoystickFeature(void) { }

    virtual JoystickFeature* Clone(void) const { return new JoystickFeature(*this); }

    virtual JOYSTICK_DRIVER_TYPE Type(void) const { return JOYSTICK_DRIVER_TYPE_UNKNOWN; }
    JOYSTICK_FEATURE_ID          ID(void) const   { return m_id; }

    void SetID(JOYSTICK_FEATURE_ID id) { m_id = id; }

    virtual void ToStruct(JOYSTICK_FEATURE& feature) const
    {
      feature.id          = m_id;
      feature.driver_type = Type();
    }

    static void FreeStruct(JOYSTICK_FEATURE& feature)
    {
    }

  private:
    JOYSTICK_FEATURE_ID m_id;
  };

  typedef PeripheralVector<JoystickFeature, JOYSTICK_FEATURE> JoystickFeatures;

  /*!
   * ADDON::DriverButton
   *
   * Joystick feature is a button that can be read from the driver.
   */
  class DriverButton : public JoystickFeature
  {
  public:
    DriverButton(void) :
      m_index(DRIVER_INDEX_UNKNOWN)
    {
    }

    DriverButton(JOYSTICK_FEATURE_ID id, int index) :
      JoystickFeature(id),
      m_index(index)
    {
    }

    DriverButton(const JOYSTICK_FEATURE& feature) :
      JoystickFeature(feature),
      m_index(feature.driver_button.index)
    {
    }

    virtual JoystickFeature* Clone(void) const { return new DriverButton(*this); }

    virtual JOYSTICK_DRIVER_TYPE Type(void) const  { return JOYSTICK_DRIVER_TYPE_BUTTON; }
    int                          Index(void) const { return m_index; }

    void SetIndex(int index) { m_index = index; }

    virtual void ToStruct(JOYSTICK_FEATURE& feature) const
    {
      JoystickFeature::ToStruct(feature);
      feature.driver_button.index = m_index;
    }

  private:
    int m_index;
  };

  /*!
   * ADDON::DriverHat
   *
   * Joystick feature is a cardinal direction of the driver's hat
   */
  class DriverHat : public JoystickFeature
  {
  public:
    DriverHat(void) :
      m_index(DRIVER_INDEX_UNKNOWN),
      m_direction(JOYSTICK_DRIVER_HAT_UNKNOWN)
    {
    }

    DriverHat(JOYSTICK_FEATURE_ID id, int index, JOYSTICK_DRIVER_HAT_DIRECTION direction) :
      JoystickFeature(id),
      m_index(index),
      m_direction(direction)
    {
    }

    DriverHat(const JOYSTICK_FEATURE& feature) :
      JoystickFeature(feature),
      m_index(feature.driver_hat.index),
      m_direction(feature.driver_hat.direction)
    {
    }

    virtual JoystickFeature* Clone(void) const { return new DriverHat(*this); }

    virtual JOYSTICK_DRIVER_TYPE  Type(void) const      { return JOYSTICK_DRIVER_TYPE_HAT_DIRECTION; }
    int                           Index(void) const     { return m_index; }
    JOYSTICK_DRIVER_HAT_DIRECTION Direction(void) const { return m_direction; }

    void SetIndex(int index)                                   { m_index     = index; }
    void SetDirection(JOYSTICK_DRIVER_HAT_DIRECTION direction) { m_direction = direction; }

    virtual void ToStruct(JOYSTICK_FEATURE& feature) const
    {
      JoystickFeature::ToStruct(feature);
      feature.driver_hat.index     = m_index;
      feature.driver_hat.direction = m_direction;
    }

  private:
    int                           m_index;
    JOYSTICK_DRIVER_HAT_DIRECTION m_direction;
  };

  /*!
   * ADDON::DriverSemiAxis
   *
   * Joystick feature is the positive or negative half of an axis in the driver.
   */
  class DriverSemiAxis : public JoystickFeature
  {
  public:
    DriverSemiAxis(void) :
      m_index(DRIVER_INDEX_UNKNOWN),
      m_direction(JOYSTICK_DRIVER_SEMIAXIS_DIRECTION_UNKNOWN)
    {
    }

    DriverSemiAxis(JOYSTICK_FEATURE_ID id, int index, JOYSTICK_DRIVER_SEMIAXIS_DIRECTION direction) :
      JoystickFeature(id),
      m_index(index),
      m_direction(direction)
    {
    }

    DriverSemiAxis(const JOYSTICK_FEATURE& feature) :
      JoystickFeature(feature),
      m_index(feature.driver_semiaxis.index),
      m_direction(feature.driver_semiaxis.direction)
    {
    }

    virtual JoystickFeature* Clone(void) const { return new DriverSemiAxis(*this); }

    virtual JOYSTICK_DRIVER_TYPE       Type(void) const      { return JOYSTICK_DRIVER_TYPE_SEMIAXIS; }
    int                                Index(void) const     { return m_index; }
    JOYSTICK_DRIVER_SEMIAXIS_DIRECTION Direction(void) const { return m_direction; }

    void SetIndex(int index)                                        { m_index     = index; }
    void SetDirection(JOYSTICK_DRIVER_SEMIAXIS_DIRECTION direction) { m_direction = direction; }

    virtual void ToStruct(JOYSTICK_FEATURE& feature) const
    {
      JoystickFeature::ToStruct(feature);
      feature.driver_semiaxis.index     = m_index;
      feature.driver_semiaxis.direction = m_direction;
    }

  private:
    int                                m_index;
    JOYSTICK_DRIVER_SEMIAXIS_DIRECTION m_direction;
  };

  /*!
   * ADDON::DriverAnalogStick
   *
   * Joystick feature is an analog stick associated with two axes in the driver.
   */
  class DriverAnalogStick : public JoystickFeature
  {
  public:
    DriverAnalogStick(void) :
      m_xIndex(DRIVER_INDEX_UNKNOWN),
      m_xInverted(false),
      m_yIndex(DRIVER_INDEX_UNKNOWN),
      m_yInverted(false)
    {
    }

    DriverAnalogStick(JOYSTICK_FEATURE_ID id, int xIndex, bool xInverted, int yIndex, bool yInverted) :
      JoystickFeature(id),
      m_xIndex(xIndex),
      m_xInverted(xInverted),
      m_yIndex(yIndex),
      m_yInverted(yInverted)
    {
    }

    DriverAnalogStick(const JOYSTICK_FEATURE& feature) :
      JoystickFeature(feature),
      m_xIndex(feature.driver_analog_stick.x_index),
      m_xInverted(feature.driver_analog_stick.x_inverted),
      m_yIndex(feature.driver_analog_stick.y_index),
      m_yInverted(feature.driver_analog_stick.y_inverted)
    {
    }

    virtual JoystickFeature* Clone(void) const { return new DriverAnalogStick(*this); }

    virtual JOYSTICK_DRIVER_TYPE Type(void) const      { return JOYSTICK_DRIVER_TYPE_ANALOG_STICK; }
    int                          XIndex(void) const    { return m_xIndex; }
    bool                         XInverted(void) const { return m_xInverted; }
    int                          YIndex(void) const    { return m_yIndex; }
    bool                         YInverted(void) const { return m_yInverted; }

    void SetXIndex(int xIndex)        { m_xIndex    = xIndex; }
    void SetXInverted(bool xInverted) { m_xInverted = xInverted; }
    void SetYIndex(int yIndex)        { m_yIndex    = yIndex; }
    void SetYInverted(bool yInverted) { m_yInverted = yInverted; }

    virtual void ToStruct(JOYSTICK_FEATURE& feature) const
    {
      JoystickFeature::ToStruct(feature);
      feature.driver_analog_stick.x_index    = m_xIndex;
      feature.driver_analog_stick.x_inverted = m_xInverted;
      feature.driver_analog_stick.y_index    = m_yIndex;
      feature.driver_analog_stick.y_inverted = m_yInverted;
    }

  private:
    int  m_xIndex;
    bool m_xInverted;
    int  m_yIndex;
    bool m_yInverted;
  };

  /*!
   * ADDON::DriverAnalogStick
   *
   * Joystick feature is an accelerometer associated with three axes in the
   * driver.
   */
  class DriverAccelerometer : public JoystickFeature
  {
  public:
    DriverAccelerometer(void) :
      m_xIndex(DRIVER_INDEX_UNKNOWN),
      m_xInverted(false),
      m_yIndex(DRIVER_INDEX_UNKNOWN),
      m_yInverted(false),
      m_zIndex(DRIVER_INDEX_UNKNOWN),
      m_zInverted(false)
    {
    }

    DriverAccelerometer(JOYSTICK_FEATURE_ID id, int xIndex, bool xInverted, int yIndex, bool yInverted, int zIndex, bool zInverted) :
      JoystickFeature(id),
      m_xIndex(xIndex),
      m_xInverted(xInverted),
      m_yIndex(yIndex),
      m_yInverted(yInverted),
      m_zIndex(zIndex),
      m_zInverted(zInverted)
    {
    }

    DriverAccelerometer(const JOYSTICK_FEATURE& feature) :
      JoystickFeature(feature),
      m_xIndex(feature.driver_accelerometer.x_index),
      m_xInverted(feature.driver_accelerometer.x_inverted),
      m_yIndex(feature.driver_accelerometer.y_index),
      m_yInverted(feature.driver_accelerometer.y_inverted),
      m_zIndex(feature.driver_accelerometer.z_index),
      m_zInverted(feature.driver_accelerometer.z_inverted)
    {
    }

    virtual JoystickFeature* Clone(void) const { return new DriverAccelerometer(*this); }

    virtual JOYSTICK_DRIVER_TYPE Type(void) const      { return JOYSTICK_DRIVER_TYPE_ACCELEROMETER; }
    int                          XIndex(void) const    { return m_xIndex; }
    bool                         XInverted(void) const { return m_xInverted; }
    int                          YIndex(void) const    { return m_yIndex; }
    bool                         YInverted(void) const { return m_yInverted; }
    int                          ZIndex(void) const    { return m_zIndex; }
    bool                         ZInverted(void) const { return m_zInverted; }

    void SetXIndex(int xIndex)        { m_xIndex    = xIndex; }
    void SetXInverted(bool xInverted) { m_xInverted = xInverted; }
    void SetYIndex(int yIndex)        { m_yIndex    = yIndex; }
    void SetYInverted(bool yInverted) { m_yInverted = yInverted; }
    void SetZIndex(int zIndex)        { m_zIndex    = zIndex; }
    void SetZInverted(bool zInverted) { m_zInverted = zInverted; }

    virtual void ToStruct(JOYSTICK_FEATURE& feature) const
    {
      JoystickFeature::ToStruct(feature);
      feature.driver_accelerometer.x_index    = m_xIndex;
      feature.driver_accelerometer.x_inverted = m_xInverted;
      feature.driver_accelerometer.y_index    = m_yIndex;
      feature.driver_accelerometer.y_inverted = m_yInverted;
      feature.driver_accelerometer.z_index    = m_zIndex;
      feature.driver_accelerometer.z_inverted = m_zInverted;
    }

  private:
    int  m_xIndex;
    bool m_xInverted;
    int  m_yIndex;
    bool m_yInverted;
    int  m_zIndex;
    bool m_zInverted;
  };

  /*!
   * ADDON::JoystickFeatureFactory
   *
   * Utility class to create joystick features polymorphically
   */
  class JoystickFeatureFactory
  {
  public:
    static JoystickFeature* Create(const JOYSTICK_FEATURE& feature)
    {
      switch (feature.driver_type)
      {
      case JOYSTICK_DRIVER_TYPE_BUTTON:
        return new DriverButton(feature);
        break;
      case JOYSTICK_DRIVER_TYPE_HAT_DIRECTION:
        return new DriverHat(feature);
        break;
      case JOYSTICK_DRIVER_TYPE_SEMIAXIS:
        return new DriverSemiAxis(feature);
        break;
      case JOYSTICK_DRIVER_TYPE_ANALOG_STICK:
        return new DriverAnalogStick(feature);
        break;
      case JOYSTICK_DRIVER_TYPE_ACCELEROMETER:
        return new DriverAccelerometer(feature);
        break;
      default:
        break;
      }
      return NULL;
    }
  };

  /*!
   * ADDON::Joystick
   *
   * Wrapper class providing additional joystick information not provided by
   * ADDON::Peripheral. Classes can extend Joystick to inherit peripheral and
   * joystick properties.
   */
  class Joystick : public Peripheral
  {
  public:
    Joystick(const std::string& provider = "", const std::string& strName = "")
    : Peripheral(PERIPHERAL_TYPE_JOYSTICK, strName),
      m_provider(provider),
      m_requestedPort(0),
      m_buttonCount(0),
      m_hatCount(0),
      m_axisCount(0)
    {
    }

    Joystick(const Joystick& other)
    : Peripheral(other)
    {
      *this = other;
    }

    Joystick(JOYSTICK_INFO& info)
    : Peripheral(info.peripheral_info),
      m_provider(info.provider ? info.provider : ""),
      m_requestedPort(info.requested_port_num),
      m_buttonCount(info.driver.button_count),
      m_hatCount(info.driver.hat_count),
      m_axisCount(info.driver.axis_count)
    {
      if (info.features)
      {
        m_features.reserve(info.feature_count);
        for (unsigned int i = 0; i < info.feature_count; i++)
          m_features.push_back(JoystickFeatureFactory::Create(info.features[i]));
      }
    }

    virtual ~Joystick(void)
    {
      for (std::vector<JoystickFeature*>::iterator it = m_features.begin(); it != m_features.end(); ++it)
        delete *it;
    }

    Joystick& operator=(const Joystick& rhs)
    {
      if (this != &rhs)
      {
        Peripheral::operator=(rhs);
        m_provider      = rhs.m_provider;
        m_requestedPort = rhs.m_requestedPort;
        m_buttonCount   = rhs.m_buttonCount;
        m_hatCount      = rhs.m_hatCount;
        m_axisCount     = rhs.m_axisCount;

        m_features.reserve(rhs.m_features.size());
        for (std::vector<JoystickFeature*>::const_iterator it = rhs.m_features.begin(); it != rhs.m_features.end(); ++it)
          m_features.push_back((*it)->Clone());
      }
      return *this;
    }

    const std::string& Provider(void) const      { return m_provider; }
    unsigned int       RequestedPort(void) const { return m_requestedPort; }
    unsigned int       ButtonCount(void) const   { return m_buttonCount; }
    unsigned int       HatCount(void) const      { return m_hatCount; }
    unsigned int       AxisCount(void) const     { return m_axisCount; }
    const std::vector<JoystickFeature*>& Features(void) const { return m_features; }

    void SetProvider(const std::string& provider)     { m_provider      = provider; }
    void SetRequestedPort(unsigned int requestedPort) { m_requestedPort = requestedPort; }
    void SetButtonCount(unsigned int buttonCount)     { m_buttonCount   = buttonCount; }
    void SetHatCount(unsigned int hatCount)           { m_hatCount      = hatCount; }
    void SetAxisCount(unsigned int axisCount)         { m_axisCount     = axisCount; }
    std::vector<JoystickFeature*>& Features(void)     { return m_features; }

    void ToStruct(JOYSTICK_INFO& info) const
    {
      Peripheral::ToStruct(info.peripheral_info);

      info.provider            = new char[m_provider.size() + 1];
      info.requested_port_num  = m_requestedPort;
      info.driver.button_count = m_buttonCount;
      info.driver.hat_count    = m_hatCount;
      info.driver.axis_count   = m_axisCount;
      info.feature_count       = m_features.size();
      JoystickFeatures::ToStructs(m_features, &info.features);

      std::strcpy(info.provider, m_provider.c_str());
    }

    static void FreeStruct(JOYSTICK_INFO& info)
    {
      Peripheral::FreeStruct(info.peripheral_info);
      JoystickFeatures::FreeStructs(info.feature_count, info.features);
      info.features = NULL;
    }

  private:
    std::string                   m_provider;
    unsigned int                  m_requestedPort;
    unsigned int                  m_buttonCount;
    unsigned int                  m_hatCount;
    unsigned int                  m_axisCount;
    std::vector<JoystickFeature*> m_features;
  };

  typedef PeripheralVector<Joystick, JOYSTICK_INFO> Joysticks;

  /*!
   * ADDON::PeripheralEvent
   *
   * Wrapper class for peripheral events.
   */
  class PeripheralEvent
  {
  public:
    PeripheralEvent(void)
    : m_event()
    {
    }

    PeripheralEvent(unsigned int peripheralIndex, unsigned int buttonIndex, JOYSTICK_STATE_BUTTON state)
    : m_event()
    {
      SetType(PERIPHERAL_EVENT_TYPE_DRIVER_BUTTON);
      SetPeripheralIndex(peripheralIndex);
      SetDriverIndex(buttonIndex);
      SetButtonState(state);
    }

    PeripheralEvent(unsigned int peripheralIndex, unsigned int hatIndex, JOYSTICK_STATE_HAT state)
    : m_event()
    {
      SetType(PERIPHERAL_EVENT_TYPE_DRIVER_HAT);
      SetPeripheralIndex(peripheralIndex);
      SetDriverIndex(hatIndex);
      SetHatState(state);
    }

    PeripheralEvent(unsigned int peripheralIndex, unsigned int axisIndex, JOYSTICK_STATE_AXIS state)
    : m_event()
    {
      SetType(PERIPHERAL_EVENT_TYPE_DRIVER_AXIS);
      SetPeripheralIndex(peripheralIndex);
      SetDriverIndex(axisIndex);
      SetAxisState(state);
    }

    PeripheralEvent(const PERIPHERAL_EVENT& event)
    : m_event(event)
    {
    }

    PERIPHERAL_EVENT_TYPE Type(void) const            { return m_event.type; }
    unsigned int          PeripheralIndex(void) const { return m_event.peripheral_index; }
    unsigned int          DriverIndex(void) const     { return m_event.driver_index; }
    JOYSTICK_STATE_BUTTON ButtonState(void) const     { return m_event.driver_button_state; }
    JOYSTICK_STATE_HAT    HatState(void) const        { return m_event.driver_hat_state; }
    JOYSTICK_STATE_AXIS   AxisState(void) const       { return m_event.driver_axis_state; }

    void SetType(PERIPHERAL_EVENT_TYPE type)         { m_event.type                = type; }
    void SetPeripheralIndex(unsigned int index)      { m_event.peripheral_index    = index; }
    void SetDriverIndex(unsigned int index)          { m_event.driver_index        = index; }
    void SetButtonState(JOYSTICK_STATE_BUTTON state) { m_event.driver_button_state = state; }
    void SetHatState(JOYSTICK_STATE_HAT state)       { m_event.driver_hat_state    = state; }
    void SetAxisState(JOYSTICK_STATE_AXIS state)     { m_event.driver_axis_state   = state; }

    void ToStruct(PERIPHERAL_EVENT& event) const
    {
      event = m_event;
    }

    static void FreeStruct(PERIPHERAL_EVENT& event)
    {
      (void)event;
    }

  private:
    PERIPHERAL_EVENT m_event;
  };

  typedef PeripheralVector<PeripheralEvent, PERIPHERAL_EVENT> PeripheralEvents;
}
