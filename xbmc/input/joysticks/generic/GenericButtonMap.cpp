/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GenericButtonMap.h"

#include "input/joysticks/JoystickUtils.h"

#include <algorithm>

using namespace KODI;
using namespace JOYSTICK;

CGenericButtonMap::CGenericButtonMap(const std::string& deviceLocation,
                                     const std::string& controllerId)
  : m_deviceLocation(deviceLocation), m_controllerId(controllerId)
{
}

CGenericButtonMap::~CGenericButtonMap() = default;

void CGenericButtonMap::Reset()
{
  m_controllerAppearance.clear();
  m_featureMap.clear();
  m_driverMap.clear();
  m_ignoredPrimitives.clear();
}

bool CGenericButtonMap::IsEmpty() const
{
  return m_driverMap.empty();
}

bool CGenericButtonMap::SetAppearance(const std::string& controllerId)
{
  m_controllerAppearance = controllerId;
  return true;
}

bool CGenericButtonMap::GetFeature(const CDriverPrimitive& primitive, FeatureName& feature)
{
  auto it = m_driverMap.find(primitive);
  if (it != m_driverMap.end())
  {
    feature = it->second;
    return true;
  }

  return false;
}

FEATURE_TYPE CGenericButtonMap::GetFeatureType(const FeatureName& feature)
{
  FEATURE_TYPE type = FEATURE_TYPE::UNKNOWN;

  /*
  FeatureMap::const_iterator it = m_featureMap.find(feature);
  if (it != m_featureMap.end())
    type = CPeripheralGenericTranslator::TranslateFeatureType(it->second.Type());
  */

  return type;
}

bool CGenericButtonMap::GetScalar(const FeatureName& feature, CDriverPrimitive& primitive)
{
  /*
  auto it = m_featureMap.find(feature);
  if (it != m_featureMap.end())
  {
    primitive = (*it)->at(0);
    return true;
  }
  */

  return false;
}

void CGenericButtonMap::AddScalar(const FeatureName& feature, const CDriverPrimitive& primitive)
{
  m_featureMap[feature] = {primitive};
}

bool CGenericButtonMap::GetAnalogStick(const FeatureName& feature,
                                       ANALOG_STICK_DIRECTION direction,
                                       CDriverPrimitive& primitive)
{
  bool retVal(false);

  /*
  FeatureMap::const_iterator it = m_featureMap.find(feature);
  if (it != m_featureMap.end())
  {
    const kodi::addon::JoystickFeature& addonFeature = it->second;

    if (addonFeature.Type() == JOYSTICK_FEATURE_TYPE_ANALOG_STICK)
    {
      primitive = CPeripheralGenericTranslator::TranslatePrimitive(
          addonFeature.Primitive(GetAnalogStickIndex(direction)));
      retVal = primitive.IsValid();
    }
  }
  */

  return retVal;
}

void CGenericButtonMap::AddAnalogStick(const FeatureName& feature,
                                       ANALOG_STICK_DIRECTION direction,
                                       const CDriverPrimitive& primitive)
{
  /*
  JOYSTICK_FEATURE_PRIMITIVE primitiveIndex = GetAnalogStickIndex(direction);
  kodi::addon::DriverPrimitive addonPrimitive =
      CPeripheralGenericTranslator::TranslatePrimitive(primitive);

  kodi::addon::JoystickFeature analogStick(feature, JOYSTICK_FEATURE_TYPE_ANALOG_STICK);

  auto it = m_featureMap.find(feature);
  if (it != m_featureMap.end())
    analogStick = it->second;

  const bool bModified = (primitive != CPeripheralGenericTranslator::TranslatePrimitive(
                                           analogStick.Primitive(primitiveIndex)));
  if (bModified)
    analogStick.SetPrimitive(primitiveIndex, addonPrimitive);

  if (auto addon = m_addon.lock())
    addon->MapFeature(m_device, m_strControllerId, analogStick);

  // Because each direction is mapped individually, we need to refresh the
  // feature each time a new direction is mapped.
  if (bModified)
    Load();
  */
}

bool CGenericButtonMap::GetRelativePointer(const FeatureName& feature,
                                           RELATIVE_POINTER_DIRECTION direction,
                                           CDriverPrimitive& primitive)
{
  bool retVal(false);

  /*
  FeatureMap::const_iterator it = m_featureMap.find(feature);
  if (it != m_featureMap.end())
  {
    const kodi::addon::JoystickFeature& addonFeature = it->second;

    if (addonFeature.Type() == JOYSTICK_FEATURE_TYPE_RELPOINTER)
    {
      primitive = CPeripheralGenericTranslator::TranslatePrimitive(
          addonFeature.Primitive(GetRelativePointerIndex(direction)));
      retVal = primitive.IsValid();
    }
  }
  */

  return retVal;
}

void CGenericButtonMap::AddRelativePointer(const FeatureName& feature,
                                           RELATIVE_POINTER_DIRECTION direction,
                                           const CDriverPrimitive& primitive)
{
  /*
  JOYSTICK_FEATURE_PRIMITIVE primitiveIndex = GetRelativePointerIndex(direction);
  kodi::addon::DriverPrimitive addonPrimitive =
      CPeripheralGenericTranslator::TranslatePrimitive(primitive);

  kodi::addon::JoystickFeature relPointer(feature, JOYSTICK_FEATURE_TYPE_RELPOINTER);

  auto it = m_featureMap.find(feature);
  if (it != m_featureMap.end())
    relPointer = it->second;

  const bool bModified = (primitive != CPeripheralGenericTranslator::TranslatePrimitive(
                                           relPointer.Primitive(primitiveIndex)));
  if (bModified)
    relPointer.SetPrimitive(primitiveIndex, addonPrimitive);

  if (auto addon = m_addon.lock())
    addon->MapFeature(m_device, m_strControllerId, relPointer);

  // Because each direction is mapped individually, we need to refresh the
  // feature each time a new direction is mapped.
  if (bModified)
    Load();
  */
}

bool CGenericButtonMap::GetAccelerometer(const FeatureName& feature,
                                         CDriverPrimitive& positiveX,
                                         CDriverPrimitive& positiveY,
                                         CDriverPrimitive& positiveZ)
{
  bool retVal(false);

  /*
  FeatureMap::const_iterator it = m_featureMap.find(feature);
  if (it != m_featureMap.end())
  {
    const kodi::addon::JoystickFeature& addonFeature = it->second;

    if (addonFeature.Type() == JOYSTICK_FEATURE_TYPE_ACCELEROMETER)
    {
      positiveX = CPeripheralGenericTranslator::TranslatePrimitive(
          addonFeature.Primitive(JOYSTICK_ACCELEROMETER_POSITIVE_X));
      positiveY = CPeripheralGenericTranslator::TranslatePrimitive(
          addonFeature.Primitive(JOYSTICK_ACCELEROMETER_POSITIVE_Y));
      positiveZ = CPeripheralGenericTranslator::TranslatePrimitive(
          addonFeature.Primitive(JOYSTICK_ACCELEROMETER_POSITIVE_Z));
      retVal = true;
    }
  }
  */

  return retVal;
}

void CGenericButtonMap::AddAccelerometer(const FeatureName& feature,
                                         const CDriverPrimitive& positiveX,
                                         const CDriverPrimitive& positiveY,
                                         const CDriverPrimitive& positiveZ)
{
  /*
  kodi::addon::JoystickFeature accelerometer(feature, JOYSTICK_FEATURE_TYPE_ACCELEROMETER);

  accelerometer.SetPrimitive(JOYSTICK_ACCELEROMETER_POSITIVE_X,
                             CPeripheralGenericTranslator::TranslatePrimitive(positiveX));
  accelerometer.SetPrimitive(JOYSTICK_ACCELEROMETER_POSITIVE_Y,
                             CPeripheralGenericTranslator::TranslatePrimitive(positiveY));
  accelerometer.SetPrimitive(JOYSTICK_ACCELEROMETER_POSITIVE_Z,
                             CPeripheralGenericTranslator::TranslatePrimitive(positiveZ));

  if (auto addon = m_addon.lock())
    addon->MapFeature(m_device, m_strControllerId, accelerometer);
  */
}

bool CGenericButtonMap::GetWheel(const FeatureName& feature,
                                 WHEEL_DIRECTION direction,
                                 CDriverPrimitive& primitive)
{
  bool retVal(false);

  /*
  FeatureMap::const_iterator it = m_featureMap.find(feature);
  if (it != m_featureMap.end())
  {
    const kodi::addon::JoystickFeature& addonFeature = it->second;

    if (addonFeature.Type() == JOYSTICK_FEATURE_TYPE_WHEEL)
    {
      primitive = CPeripheralGenericTranslator::TranslatePrimitive(
          addonFeature.Primitive(GetPrimitiveIndex(direction)));
      retVal = primitive.IsValid();
    }
  }
  */

  return retVal;
}

void CGenericButtonMap::AddWheel(const FeatureName& feature,
                                 WHEEL_DIRECTION direction,
                                 const CDriverPrimitive& primitive)
{
  /*
  JOYSTICK_FEATURE_PRIMITIVE primitiveIndex = GetPrimitiveIndex(direction);
  kodi::addon::DriverPrimitive addonPrimitive =
      CPeripheralGenericTranslator::TranslatePrimitive(primitive);

  kodi::addon::JoystickFeature joystickFeature(feature, JOYSTICK_FEATURE_TYPE_WHEEL);

  auto it = m_featureMap.find(feature);
  if (it != m_featureMap.end())
    joystickFeature = it->second;

  const bool bModified = (primitive != CPeripheralGenericTranslator::TranslatePrimitive(
                                           joystickFeature.Primitive(primitiveIndex)));
  if (bModified)
    joystickFeature.SetPrimitive(primitiveIndex, addonPrimitive);

  if (auto addon = m_addon.lock())
    addon->MapFeature(m_device, m_strControllerId, joystickFeature);

  // Because each direction is mapped individually, we need to refresh the
  // feature each time a new direction is mapped.
  if (bModified)
    Load();
  */
}

bool CGenericButtonMap::GetThrottle(const FeatureName& feature,
                                    THROTTLE_DIRECTION direction,
                                    CDriverPrimitive& primitive)
{
  bool retVal(false);

  /*
  FeatureMap::const_iterator it = m_featureMap.find(feature);
  if (it != m_featureMap.end())
  {
    const kodi::addon::JoystickFeature& addonFeature = it->second;

    if (addonFeature.Type() == JOYSTICK_FEATURE_TYPE_THROTTLE)
    {
      primitive = CPeripheralGenericTranslator::TranslatePrimitive(
          addonFeature.Primitive(GetPrimitiveIndex(direction)));
      retVal = primitive.IsValid();
    }
  }
  */

  return retVal;
}

void CGenericButtonMap::AddThrottle(const FeatureName& feature,
                                    THROTTLE_DIRECTION direction,
                                    const CDriverPrimitive& primitive)
{
  /*
  JOYSTICK_FEATURE_PRIMITIVE primitiveIndex = GetPrimitiveIndex(direction);
  kodi::addon::DriverPrimitive addonPrimitive =
      CPeripheralGenericTranslator::TranslatePrimitive(primitive);

  kodi::addon::JoystickFeature joystickFeature(feature, JOYSTICK_FEATURE_TYPE_THROTTLE);

  auto it = m_featureMap.find(feature);
  if (it != m_featureMap.end())
    joystickFeature = it->second;

  const bool bModified = (primitive != CPeripheralGenericTranslator::TranslatePrimitive(
                                           joystickFeature.Primitive(primitiveIndex)));
  if (bModified)
    joystickFeature.SetPrimitive(primitiveIndex, addonPrimitive);

  if (auto addon = m_addon.lock())
    addon->MapFeature(m_device, m_strControllerId, joystickFeature);

  // Because each direction is mapped individually, we need to refresh the
  // feature each time a new direction is mapped.
  if (bModified)
    Load();
  */
}

bool CGenericButtonMap::GetKey(const FeatureName& feature, CDriverPrimitive& primitive)
{
  bool retVal(false);

  /*
  FeatureMap::const_iterator it = m_featureMap.find(feature);
  if (it != m_featureMap.end())
  {
    const kodi::addon::JoystickFeature& addonFeature = it->second;

    if (addonFeature.Type() == JOYSTICK_FEATURE_TYPE_KEY)
    {
      primitive = CPeripheralGenericTranslator::TranslatePrimitive(
          addonFeature.Primitive(JOYSTICK_SCALAR_PRIMITIVE));
      retVal = true;
    }
  }
  */

  return retVal;
}

void CGenericButtonMap::AddKey(const FeatureName& feature, const CDriverPrimitive& primitive)
{
  /*
  kodi::addon::JoystickFeature scalar(feature, JOYSTICK_FEATURE_TYPE_KEY);
  scalar.SetPrimitive(JOYSTICK_SCALAR_PRIMITIVE,
                      CPeripheralGenericTranslator::TranslatePrimitive(primitive));

  if (auto addon = m_addon.lock())
    addon->MapFeature(m_device, m_strControllerId, scalar);
  */
}

void CGenericButtonMap::SetIgnoredPrimitives(const std::vector<CDriverPrimitive>& primitives)
{
  m_ignoredPrimitives = primitives;
}

bool CGenericButtonMap::IsIgnored(const CDriverPrimitive& primitive)
{
  return std::find(m_ignoredPrimitives.begin(), m_ignoredPrimitives.end(), primitive) !=
         m_ignoredPrimitives.end();
}

bool CGenericButtonMap::GetAxisProperties(unsigned int axisIndex, int& center, unsigned int& range)
{
  /*
  for (const auto& it : m_driverMap)
  {
    const CDriverPrimitive& primitive = it.first;

    if (primitive.Type() != PRIMITIVE_TYPE::SEMIAXIS)
      continue;

    if (primitive.Index() != axisIndex)
      continue;

    center = primitive.Center();
    range = primitive.Range();
    return true;
  }
  */

  return false;
}

CGenericButtonMap::DriverMap CGenericButtonMap::CreateLookupTable(const FeatureMap& features)
{
  DriverMap driverMap;

  /*
  for (const auto& it : features)
  {
    const PrimitiveVector& primitives = it.second;
    for (const CDriverPrimitive& primitive : primitives)
    {
      driverMap[primitive] = it.first;
      {
        case JOYSTICK_FEATURE_TYPE_SCALAR:
        case JOYSTICK_FEATURE_TYPE_KEY:
        {
          driverMap[CPeripheralGenericTranslator::TranslatePrimitive(
              feature.Primitive(JOYSTICK_SCALAR_PRIMITIVE))] = it.first;
          break;
        }

        case JOYSTICK_FEATURE_TYPE_ANALOG_STICK:
        {
          std::vector<JOYSTICK_FEATURE_PRIMITIVE> primitives = {
              JOYSTICK_ANALOG_STICK_UP,
              JOYSTICK_ANALOG_STICK_DOWN,
              JOYSTICK_ANALOG_STICK_RIGHT,
              JOYSTICK_ANALOG_STICK_LEFT,
          };

          for (auto primitive : primitives)
            driverMap[CPeripheralGenericTranslator::TranslatePrimitive(
                feature.Primitive(primitive))] = it.first;
          break;
        }

        case JOYSTICK_FEATURE_TYPE_ACCELEROMETER:
        {
          std::vector<JOYSTICK_FEATURE_PRIMITIVE> primitives = {
              JOYSTICK_ACCELEROMETER_POSITIVE_X,
              JOYSTICK_ACCELEROMETER_POSITIVE_Y,
              JOYSTICK_ACCELEROMETER_POSITIVE_Z,
          };

          for (auto primitive : primitives)
          {
            CDriverPrimitive translatedPrimitive =
                CPeripheralGenericTranslator::TranslatePrimitive(feature.Primitive(primitive));
            driverMap[translatedPrimitive] = it.first;

            // Map opposite semiaxis
            CDriverPrimitive oppositePrimitive = CDriverPrimitive(
                translatedPrimitive.Index(), 0, translatedPrimitive.SemiAxisDirection() * -1, 1);
            driverMap[oppositePrimitive] = it.first;
          }
          break;
        }

        case JOYSTICK_FEATURE_TYPE_WHEEL:
        {
          std::vector<JOYSTICK_FEATURE_PRIMITIVE> primitives = {
              JOYSTICK_WHEEL_LEFT,
              JOYSTICK_WHEEL_RIGHT,
          };

          for (auto primitive : primitives)
            driverMap[CPeripheralGenericTranslator::TranslatePrimitive(
                feature.Primitive(primitive))] = it.first;
          break;
        }

        case JOYSTICK_FEATURE_TYPE_THROTTLE:
        {
          std::vector<JOYSTICK_FEATURE_PRIMITIVE> primitives = {
              JOYSTICK_THROTTLE_UP,
              JOYSTICK_THROTTLE_DOWN,
          };

          for (auto primitive : primitives)
            driverMap[CPeripheralGenericTranslator::TranslatePrimitive(
                feature.Primitive(primitive))] = it.first;
          break;
        }

        case JOYSTICK_FEATURE_TYPE_RELPOINTER:
        {
          std::vector<JOYSTICK_FEATURE_PRIMITIVE> primitives = {
              JOYSTICK_RELPOINTER_UP,
              JOYSTICK_RELPOINTER_DOWN,
              JOYSTICK_RELPOINTER_RIGHT,
              JOYSTICK_RELPOINTER_LEFT,
          };

          for (auto primitive : primitives)
            driverMap[CPeripheralGenericTranslator::TranslatePrimitive(
                feature.Primitive(primitive))] = it.first;
          break;
        }

        default:
          break;
      }
    }
  }
  */

  return driverMap;
}
