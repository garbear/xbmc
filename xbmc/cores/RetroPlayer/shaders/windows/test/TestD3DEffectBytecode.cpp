/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "guilib/D3DResource.h"

#include <atomic>
#include <memory>
#include <string>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <d3dx11effect.h>
#include <gtest/gtest.h>
#include <wrl/client.h>

namespace
{
constexpr const char* SOURCE = R"(
float Value;
float4 VS(float4 p : POSITION) : SV_POSITION { return p; }
float4 PS() : SV_Target { return float4(Value, 0, 0, 1); }
technique11 TEQ { pass P0 { SetVertexShader(CompileShader(vs_5_0, VS())); SetPixelShader(CompileShader(ps_5_0, PS())); } }
)";

class TestD3DEffectBytecode : public testing::Test
{
protected:
  void SetUp() override
  {
    ASSERT_TRUE(
        SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0,
                                    D3D11_SDK_VERSION, device.GetAddressOf(), nullptr, nullptr)));
    Microsoft::WRL::ComPtr<ID3DBlob> blob;
    Microsoft::WRL::ComPtr<ID3DBlob> errors;
    ++compileCount;
    const HRESULT result = D3DCompile(SOURCE, strlen(SOURCE), "", nullptr, nullptr, "", "fx_5_0", 0,
                                      0, blob.GetAddressOf(), errors.GetAddressOf());
    ASSERT_TRUE(SUCCEEDED(result));
    const auto* begin = static_cast<const std::uint8_t*>(blob->GetBufferPointer());
    bytecode = std::make_shared<const EffectBytecode>(begin, begin + blob->GetBufferSize());
  }

  Microsoft::WRL::ComPtr<ID3D11Device> device;
  std::shared_ptr<const EffectBytecode> bytecode;
  std::atomic_uint compileCount{0};
};
} // namespace

TEST_F(TestD3DEffectBytecode, CreatesNamedTechniqueFromD3DCompileBytecode)
{
  CD3DEffect effect(device.Get());
  ASSERT_TRUE(effect.Create(bytecode));
  ASSERT_NE(nullptr, effect.Get());
  ASSERT_TRUE(effect.SetTechnique("TEQ"));
  EXPECT_TRUE(effect.Get()->GetTechniqueByName("TEQ")->GetPassByName("P0")->IsValid());
}

TEST_F(TestD3DEffectBytecode, TwoEffectsFromOneArtifactKeepIndependentScalars)
{
  CD3DEffect first(device.Get());
  CD3DEffect second(device.Get());
  ASSERT_TRUE(first.Create(bytecode));
  ASSERT_TRUE(second.Create(bytecode));
  ASSERT_TRUE(first.SetScalar("Value", 1.0f));
  ASSERT_TRUE(second.SetScalar("Value", 2.0f));
  float firstValue{};
  float secondValue{};
  ASSERT_TRUE(
      SUCCEEDED(first.Get()->GetVariableByName("Value")->AsScalar()->GetFloat(&firstValue)));
  ASSERT_TRUE(
      SUCCEEDED(second.Get()->GetVariableByName("Value")->AsScalar()->GetFloat(&secondValue)));
  EXPECT_FLOAT_EQ(1.0f, firstValue);
  EXPECT_FLOAT_EQ(2.0f, secondValue);
}

TEST_F(TestD3DEffectBytecode, DeviceRecreationUsesRetainedBytecodeWithoutCompile)
{
  CD3DEffect effect(device.Get());
  ASSERT_TRUE(effect.Create(bytecode));
  bytecode.reset();

  effect.OnDestroyDevice(false);
  EXPECT_EQ(nullptr, effect.Get());
  effect.OnCreateDevice();
  ASSERT_NE(nullptr, effect.Get());

  effect.OnDestroyDevice(false);
  EXPECT_EQ(nullptr, effect.Get());
  effect.OnCreateDevice();

  EXPECT_EQ(1u, compileCount);
  ASSERT_NE(nullptr, effect.Get());
  EXPECT_TRUE(effect.SetTechnique("TEQ"));
  EXPECT_TRUE(effect.Get()->GetTechniqueByName("TEQ")->GetPassByName("P0")->IsValid());
}
