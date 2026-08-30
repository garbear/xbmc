/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ServiceBroker.h"
#include "Util.h"
#include "cores/RetroPlayer/shaders/ShaderArtifactStore.h"
#include "cores/RetroPlayer/shaders/ShaderCompileService.h"
#include "cores/RetroPlayer/shaders/ShaderTypes.h"
#include "cores/RetroPlayer/shaders/windows/ShaderCompilerDX.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "guilib/D3DResource.h"
#include "jobs/JobManager.h"
#include "threads/Event.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"

#include <d3d11.h>
#include <d3dx11effect.h>
#include <gtest/gtest.h>
#include <wrl/client.h>

using namespace std::chrono_literals;

using namespace KODI::SHADER;

namespace
{
class CompilerRoot
{
public:
  CompilerRoot()
    : path(URIUtils::AddFileToFolder("special://temp/retroplayer-compiler-tests/",
                                     StringUtils::CreateUUID()))
  {
    EXPECT_TRUE(XFILE::CDirectory::Create(path));
  }
  ~CompilerRoot() { XFILE::CDirectory::RemoveRecursive(path); }
  std::string File(std::string_view name) const
  {
    return URIUtils::AddFileToFolder(path, std::string{name});
  }
  void Write(std::string_view name, std::string_view value) const
  {
    XFILE::CFile file;
    ASSERT_TRUE(file.OpenForWrite(File(name), true));
    ASSERT_EQ(static_cast<ssize_t>(value.size()), file.Write(value.data(), value.size()));
    file.Close();
  }
  std::string path;
};

ShaderPass Pass(const CompilerRoot& root, std::string source, std::string name = "main.fx")
{
  ShaderPass pass;
  pass.sourcePath = root.File(name);
  pass.vertexSource = std::move(source);
  return pass;
}

ShaderPrepareResult Prepare(CShaderCompilerDX& compiler, const ShaderPass& pass)
{
  auto request = compiler.CreateRequest(pass, {});
  return compiler.Prepare(*request.input);
}

constexpr std::string_view EFFECT = R"(
float Value;
float4 VS(float4 p : POSITION) : SV_POSITION { return p; }
float4 PS() : SV_Target { return float4(Value, 0, 0, 1); }
technique11 TEQ { pass P0 { SetVertexShader(CompileShader(vs_5_0, VS())); SetPixelShader(CompileShader(ps_5_0, PS())); } }
)";

void WaitTerminal(const std::shared_ptr<CShaderCompileHandle>& handle)
{
  auto event = std::make_shared<CEvent>();
  handle->AddCompletionCallback([event] { event->Set(); });
  if (handle->GetState() != ShaderCompileState::READY &&
      handle->GetState() != ShaderCompileState::FAILED)
    ASSERT_TRUE(event->Wait(5s));
}
} // namespace

TEST(TestShaderCompilerDX, UnchangedBytesProduceSameCanonicalKey)
{
  CompilerRoot root;
  CShaderCompilerDX compiler;
  const auto first = Prepare(compiler, Pass(root, std::string{EFFECT}));
  const auto second = Prepare(compiler, Pass(root, std::string{EFFECT}));
  ASSERT_TRUE(first.canonicalKey);
  ASSERT_TRUE(second.canonicalKey);
  EXPECT_EQ(first.canonicalKey->hex, second.canonicalKey->hex);
}

TEST(TestShaderCompilerDX, RootContentChangeProducesMiss)
{
  CompilerRoot root;
  CShaderCompilerDX compiler;
  const auto first = Prepare(compiler, Pass(root, std::string{EFFECT}));
  const auto second = Prepare(compiler, Pass(root, std::string{EFFECT} + "\n// changed"));
  ASSERT_TRUE(first.canonicalKey);
  ASSERT_TRUE(second.canonicalKey);
  EXPECT_NE(first.canonicalKey->hex, second.canonicalKey->hex);
}

TEST(TestShaderCompilerDX, NestedIncludeContentChangeProducesMiss)
{
  CompilerRoot root;
  root.Write("outer.inc", "#include \"inner.inc\"\n");
  root.Write("inner.inc", "float Included;\n");
  CShaderCompilerDX compiler;
  const std::string source = "#include \"outer.inc\"\n" + std::string{EFFECT};
  const auto first = Prepare(compiler, Pass(root, source));
  root.Write("inner.inc", "float Included2;\n");
  const auto second = Prepare(compiler, Pass(root, source));
  ASSERT_TRUE(first.canonicalKey);
  ASSERT_TRUE(second.canonicalKey);
  EXPECT_NE(first.canonicalKey->hex, second.canonicalKey->hex);
}

TEST(TestShaderCompilerDX, TimestampOnlyChangeDoesNotChangeKey)
{
  CompilerRoot root;
  root.Write("value.inc", "float Included;\n");
  CShaderCompilerDX compiler;
  const std::string source = "#include \"value.inc\"\n" + std::string{EFFECT};
  const auto first = Prepare(compiler, Pass(root, source));
  root.Write("value.inc", "float Included;\n");
  const auto second = Prepare(compiler, Pass(root, source));
  ASSERT_TRUE(first.canonicalKey);
  ASSERT_TRUE(second.canonicalKey);
  EXPECT_EQ(first.canonicalKey->hex, second.canonicalKey->hex);
}

TEST(TestShaderCompilerDX, IdenticalContentAtDifferentPathsDeduplicates)
{
  CompilerRoot root;
  CShaderCompilerDX compiler;
  const auto first = Prepare(compiler, Pass(root, std::string{EFFECT}, "a.fx"));
  const auto second = Prepare(compiler, Pass(root, std::string{EFFECT}, "b.fx"));
  ASSERT_TRUE(first.canonicalKey);
  ASSERT_TRUE(second.canonicalKey);
  EXPECT_EQ(first.canonicalKey->hex, second.canonicalKey->hex);
}

TEST(TestShaderCompilerDX, PreparationFailureFingerprintChangesAfterIncludeRepair)
{
  CompilerRoot root;
  CShaderCompilerDX compiler;
  const auto missing = Prepare(compiler, Pass(root, "#include \"broken.inc\"\n"));
  root.Write("broken.inc", "#error repaired\n");
  const auto repaired = Prepare(compiler, Pass(root, "#include \"broken.inc\"\n"));
  EXPECT_FALSE(missing.failureFingerprint.empty());
  EXPECT_FALSE(repaired.failureFingerprint.empty());
  EXPECT_NE(missing.failureFingerprint, repaired.failureFingerprint);
}

TEST(TestShaderCompilerDX, DefinesAndNestedIncludesMatchLegacyEffects11Compilation)
{
  CompilerRoot root;
  root.Write("outer.inc", "#include \"inner.inc\"\n");
  root.Write("inner.inc", "#define INCLUDED 1\n");
  const std::string source = "#include \"outer.inc\"\n" + std::string{EFFECT};
  auto compileCount = std::make_shared<std::atomic_uint>(0);
  CShaderCompilerDX compiler(compileCount);
  const auto prepared = Prepare(compiler, Pass(root, source));
  ASSERT_TRUE(prepared.prepared) << prepared.error;
  const auto compiled = compiler.Compile(*prepared.prepared);
  ASSERT_FALSE(compiled.bytecode.empty()) << compiled.error;
  ASSERT_EQ(1u, *compileCount);

  Microsoft::WRL::ComPtr<ID3D11Device> device;
  ASSERT_TRUE(
      SUCCEEDED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0,
                                  D3D11_SDK_VERSION, device.GetAddressOf(), nullptr, nullptr)));
  CD3DEffect bytecodeEffect(device.Get());
  auto bytecode = std::make_shared<const EffectBytecode>(compiled.bytecode);
  ASSERT_TRUE(bytecodeEffect.Create(std::move(bytecode)));

  DefinesMap defines{{"HLSL_4", ""}, {"HLSL_FX", ""}, {"PARAMETER_UNIFORM", ""}};
  CD3DEffect legacyEffect(device.Get());
  legacyEffect.AddIncludePath(root.path);
  ASSERT_TRUE(legacyEffect.Create(source, &defines));

  ASSERT_EQ(1u, *compileCount);
  ASSERT_TRUE(bytecodeEffect.Get()->GetTechniqueByName("TEQ")->IsValid());
  ASSERT_TRUE(legacyEffect.Get()->GetTechniqueByName("TEQ")->IsValid());
  EXPECT_TRUE(bytecodeEffect.Get()->GetTechniqueByName("TEQ")->GetPassByName("P0")->IsValid());
  EXPECT_TRUE(legacyEffect.Get()->GetTechniqueByName("TEQ")->GetPassByName("P0")->IsValid());
}

TEST(TestShaderCompilerDX, PersistentHitSkipsD3DCompileAfterServiceReconstruction)
{
  CompilerRoot root;
  const ShaderPass pass = Pass(root, std::string{EFFECT});
  auto counter = std::make_shared<std::atomic_uint>(0);
  auto store =
      std::make_shared<CShaderArtifactStore>(root.path, ".fxc", 1, 64ULL * 1024ULL * 1024ULL);
  CServiceBroker::RegisterJobManager(std::make_shared<CJobManager>());

  {
    CShaderCompileService service;
    service.RegisterCompiler(std::make_shared<CShaderCompilerDX>(counter), store);
    const auto first = service.Request(CShaderCompilerDX::BACKEND_ID, pass, {});
    WaitTerminal(first);
    ASSERT_EQ(ShaderCompileState::READY, first->GetState());
    EXPECT_EQ(1u, *counter);
    CServiceBroker::GetJobManager()->CancelJobs();
    CServiceBroker::GetJobManager()->Restart();
  }
  {
    CShaderCompileService service;
    service.RegisterCompiler(std::make_shared<CShaderCompilerDX>(counter), store);
    const auto second = service.Request(CShaderCompilerDX::BACKEND_ID, pass, {});
    WaitTerminal(second);
    ASSERT_EQ(ShaderCompileState::READY, second->GetState());
    EXPECT_EQ(ShaderArtifactOrigin::DISK, second->GetArtifact()->origin);
    EXPECT_EQ(1u, *counter);
    CServiceBroker::GetJobManager()->CancelJobs();
    CServiceBroker::GetJobManager()->Restart();
  }
  CServiceBroker::UnregisterJobManager();
}
