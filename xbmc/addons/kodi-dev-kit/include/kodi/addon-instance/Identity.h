/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "../c-api/addon-instance/identity.h"

#ifdef __cplusplus

namespace kodi
{
namespace addon
{

//==============================================================================
/// @addtogroup cpp_kodi_addon_identity
///
/// To use on Libretro and for stand-alone identitys or emulators that does not use
/// the Libretro API.
///

//==============================================================================
/// @addtogroup cpp_kodi_addon_identity
/// @brief @cpp_class{ kodi::addon::CInstanceIdentity }
/// **Identity add-on instance**\n
/// This class provides the basic identity processing system for use as an add-on in
/// Kodi.
///
/// This class is created at addon by Kodi.
///
class ATTR_DLL_LOCAL CInstanceIdentity : public IAddonInstance
{
public:
  //============================================================================
  /// @defgroup cpp_kodi_addon_identity_Base 1. Basic functions
  /// @ingroup cpp_kodi_addon_identity
  /// @brief **Functions to manage the addon and get basic information about it**
  ///
  ///@{

  //============================================================================
  /// @brief Identity class constructor
  ///
  /// Used by an add-on that only supports only Identity and only in one instance.
  ///
  /// This class is created at addon by Kodi.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  ///
  /// **Here's example about the use of this:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/addon-instance/Identity.h>
  /// ...
  ///
  /// class ATTR_DLL_LOCAL CIdentityExample
  ///   : public kodi::addon::CAddonBase,
  ///     public kodi::addon::CInstanceIdentity
  /// {
  /// public:
  ///   CIdentityExample()
  ///   {
  ///   }
  ///
  ///   virtual ~CIdentityExample();
  ///   {
  ///   }
  ///
  ///   ...
  /// };
  ///
  /// ADDONCREATOR(CIdentityExample)
  /// ~~~~~~~~~~~~~
  ///
  CInstanceIdentity() : IAddonInstance(IInstanceInfo(CPrivateBase::m_interface->firstKodiInstance))
  {
    if (CPrivateBase::m_interface->globalSingleInstance != nullptr)
      throw std::logic_error("kodi::addon::CInstanceIdentity: Creation of more as one in single "
                             "instance way is not allowed!");

    SetAddonStruct(CPrivateBase::m_interface->firstKodiInstance);
    CPrivateBase::m_interface->globalSingleInstance = this;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Destructor
  ///
  ~CInstanceIdentity() override = default;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// The writable directory of the frontend.
  ///
  /// This directory can be used to store SRAM, memory cards, high scores,
  /// etc, if the identity client cannot use the regular memory interface,
  /// GetMemoryData().
  ///
  /// @return the used profile directory
  ///
  /// @remarks Only called from addon itself
  ///
  std::string ProfileDirectory() const { return m_instanceData->props->profile_directory; }
  //----------------------------------------------------------------------------
  ///@}

  //============================================================================
  ///
  /// @defgroup cpp_kodi_addon_identity_Operation 2. Identity operations
  /// @ingroup cpp_kodi_addon_identity
  /// @brief **Identity operations**
  ///
  /// These are mandatory functions for using this addon to get the available
  /// channels.
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Identity operation parts in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_identity_Operation_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_identity_Operation_source_addon_auto_check
  ///
  ///@{

  //============================================================================
  /// @brief Initialize the identity provider
  ///
  /// @param[in] profileDirectory The path to the user's writable profile
  /// @return True on success, or false is an error occurred
  ///
  virtual bool Initialize(const std::string& profileDirectory) { return false; }
  //----------------------------------------------------------------------------

  //==========================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Requests the frontend to stop the current identity provider
  ///
  /// @remarks Only called from addon itself
  ///
  void Deinitialize() {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Run the identity provier's event loop
  ///
  /// @return True if the event loop was run successfully, false if an error
  /// occurred
  ///
  virtual bool Run() { return false; }
  //----------------------------------------------------------------------------

  ///@}

private:
  void SetAddonStruct(KODI_ADDON_INSTANCE_STRUCT* instance)
  {
    instance->hdl = this;

    instance->identity->toAddon->Initialize = ADDON_Initialize;
    instance->identity->toAddon->Deinitialize = ADDON_Deinitialize;
    instance->identity->toAddon->Run = ADDON_Run;

    m_instanceData = instance->identity;
    m_instanceData->toAddon->addonInstance = this;
  }

  // --- Identity operations ---------------------------------------------------------

  inline static bool ADDON_Initialize(const AddonInstance_Identity* instance,
                                      const char* profilePath)
  {
    return static_cast<CInstanceIdentity*>(instance->toAddon->addonInstance)
        ->Initialize(profilePath);
  }

  inline static void ADDON_Deinitialize(const AddonInstance_Identity* instance)
  {
    static_cast<CInstanceIdentity*>(instance->toAddon->addonInstance)->Deinitialize();
  }

  inline static bool ADDON_Run(const AddonInstance_Identity* instance)
  {
    return static_cast<CInstanceIdentity*>(instance->toAddon->addonInstance)->Run();
  }

  AddonInstance_Identity* m_instanceData;
};

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
