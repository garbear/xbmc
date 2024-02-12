/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/joysticks/DriverPrimitive.h"
#include "input/joysticks/JoystickTypes.h"
#include "input/joysticks/interfaces/IButtonMap.h"

#include <map>
#include <string>
#include <vector>

namespace PERIPHERALS
{
class CPeripheral;
}

namespace KODI
{
namespace JOYSTICK
{

class CGenericButtonMap : public IButtonMap
{
public:
  CGenericButtonMap(const std::string& deviceLocation, const std::string& controllerId);

  ~CGenericButtonMap() override;

  // Implementation of IButtonMap
  std::string ControllerID() const override { return m_controllerId; }
  std::string Location() const override { return m_deviceLocation; }
  bool Load() override { return true; }
  void Reset() override;
  bool IsEmpty() const override;
  std::string GetAppearance() const override { return m_controllerAppearance; }
  bool SetAppearance(const std::string& controllerId) override;
  bool GetFeature(const CDriverPrimitive& primitive, FeatureName& feature) override;
  FEATURE_TYPE GetFeatureType(const FeatureName& feature) override;
  bool GetScalar(const FeatureName& feature, CDriverPrimitive& primitive) override;
  void AddScalar(const FeatureName& feature, const CDriverPrimitive& primitive) override;
  bool GetAnalogStick(const FeatureName& feature,
                      ANALOG_STICK_DIRECTION direction,
                      CDriverPrimitive& primitive) override;
  void AddAnalogStick(const FeatureName& feature,
                      ANALOG_STICK_DIRECTION direction,
                      const CDriverPrimitive& primitive) override;
  bool GetRelativePointer(const FeatureName& feature,
                          RELATIVE_POINTER_DIRECTION direction,
                          CDriverPrimitive& primitive) override;
  void AddRelativePointer(const FeatureName& feature,
                          RELATIVE_POINTER_DIRECTION direction,
                          const CDriverPrimitive& primitive) override;
  bool GetAccelerometer(const FeatureName& feature,
                        CDriverPrimitive& positiveX,
                        CDriverPrimitive& positiveY,
                        CDriverPrimitive& positiveZ) override;
  void AddAccelerometer(const FeatureName& feature,
                        const CDriverPrimitive& positiveX,
                        const CDriverPrimitive& positiveY,
                        const CDriverPrimitive& positiveZ) override;
  bool GetWheel(const FeatureName& feature,
                WHEEL_DIRECTION direction,
                CDriverPrimitive& primitive) override;
  void AddWheel(const FeatureName& feature,
                WHEEL_DIRECTION direction,
                const CDriverPrimitive& primitive) override;
  bool GetThrottle(const FeatureName& feature,
                   THROTTLE_DIRECTION direction,
                   CDriverPrimitive& primitive) override;
  void AddThrottle(const FeatureName& feature,
                   THROTTLE_DIRECTION direction,
                   const CDriverPrimitive& primitive) override;
  bool GetKey(const FeatureName& feature, CDriverPrimitive& primitive) override;
  void AddKey(const FeatureName& feature, const CDriverPrimitive& primitive) override;
  void SetIgnoredPrimitives(const std::vector<CDriverPrimitive>& primitives) override;
  bool IsIgnored(const CDriverPrimitive& primitive) override;
  bool GetAxisProperties(unsigned int axisIndex, int& center, unsigned int& range) override;
  void SaveButtonMap() override {}
  void RevertButtonMap() override {}

private:
  typedef std::map<CDriverPrimitive, FeatureName> DriverMap;
  typedef std::vector<CDriverPrimitive> JoystickPrimitiveVector;

  typedef std::vector<CDriverPrimitive> PrimitiveVector;
  typedef std::map<FeatureName, PrimitiveVector> FeatureMap;

  // Utility functions
  static DriverMap CreateLookupTable(const FeatureMap& features);

  // Construction parameters
  const std::string m_deviceLocation;
  const std::string m_controllerId;

  // Button map state
  std::string m_controllerAppearance;
  FeatureMap m_featureMap;
  DriverMap m_driverMap;
  JoystickPrimitiveVector m_ignoredPrimitives;
};
} // namespace JOYSTICK
} // namespace KODI
