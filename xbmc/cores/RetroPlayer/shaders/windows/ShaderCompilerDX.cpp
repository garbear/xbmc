/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "ShaderCompilerDX.h"

#include "cores/RetroPlayer/shaders/ShaderTypes.h"
#include "filesystem/File.h"
#include "utils/Digest.h"
#include "utils/URIUtils.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <set>
#include <span>
#include <utility>

#include <d3dcompiler.h>
#include <wrl/client.h>

namespace KODI::SHADER
{
namespace
{
using Microsoft::WRL::ComPtr;

void Frame(std::vector<std::uint8_t>& bytes, std::span<const std::uint8_t> value)
{
  const std::uint64_t size = value.size();
  for (unsigned int shift = 0; shift < 64; shift += 8)
    bytes.emplace_back(static_cast<std::uint8_t>(size >> shift));
  bytes.insert(bytes.end(), value.begin(), value.end());
}

void Frame(std::vector<std::uint8_t>& bytes, std::string_view value)
{
  Frame(bytes, std::span{reinterpret_cast<const std::uint8_t*>(value.data()), value.size()});
}

ShaderCompileKey Hash(const std::vector<std::uint8_t>& bytes)
{
  KODI::UTILITY::CDigest digest{KODI::UTILITY::CDigest::Type::SHA256};
  digest.Update(bytes.data(), bytes.size());
  const std::string raw = digest.FinalizeRaw();

  ShaderCompileKey key;
  std::copy_n(reinterpret_cast<const std::uint8_t*>(raw.data()), key.raw.size(), key.raw.begin());
  constexpr char HEX[] = "0123456789abcdef";
  key.hex.reserve(key.raw.size() * 2);
  for (const std::uint8_t byte : key.raw)
  {
    key.hex.push_back(HEX[byte >> 4]);
    key.hex.push_back(HEX[byte & 0x0f]);
  }
  return key;
}

std::string BlobText(ID3DBlob* blob)
{
  if (!blob)
    return {};
  return {static_cast<const char*>(blob->GetBufferPointer()), blob->GetBufferSize()};
}

std::vector<D3D_SHADER_MACRO> MakeMacros(const DefinesMap& defines)
{
  std::vector<D3D_SHADER_MACRO> macros;
  macros.reserve(defines.size() + 1);
  for (const auto& [name, definition] : defines)
    macros.push_back({name.c_str(), definition.empty() ? nullptr : definition.c_str()});
  macros.push_back({nullptr, nullptr});
  return macros;
}

class CIncludeResolver final : public ID3DInclude
{
public:
  explicit CIncludeResolver(std::string sourcePath)
  {
    m_paths.insert("special://xbmc/system/shaders/");
    m_paths.insert(URIUtils::GetBasePath(sourcePath));
  }

  HRESULT __stdcall Open(
      D3D_INCLUDE_TYPE, LPCSTR fileName, LPCVOID, LPCVOID* data, UINT* bytes) override
  {
    for (const auto& includePath : m_paths)
    {
      std::string base = includePath;
      std::string scheme;
      if (URIUtils::IsURL(base))
      {
        const std::size_t end = base.find(":") + 3;
        scheme = base.substr(0, end);
        base.erase(0, end);
      }
      std::string candidate =
          URIUtils::CanonicalizePath(URIUtils::AddFileToFolder(base, std::string{fileName}));
      candidate.insert(0, scheme);

      XFILE::CFile file;
      if (!file.Open(candidate))
        continue;
      const int64_t length = file.GetLength();
      if (length < 0 || length > UINT_MAX)
        return E_FAIL;
      std::vector<std::uint8_t> content(static_cast<std::size_t>(length));
      std::size_t offset = 0;
      while (offset < content.size())
      {
        const ssize_t read = file.Read(content.data() + offset, content.size() - offset);
        if (read <= 0)
          return E_FAIL;
        offset += static_cast<std::size_t>(read);
      }

      void* allocation = std::malloc(std::max<std::size_t>(1, content.size()));
      if (!allocation)
        return E_OUTOFMEMORY;
      if (!content.empty())
        std::memcpy(allocation, content.data(), content.size());
      m_dependencies.emplace_back(content);
      m_paths.insert(URIUtils::GetBasePath(candidate));
      *data = allocation;
      *bytes = static_cast<UINT>(content.size());
      return S_OK;
    }
    return E_FAIL;
  }

  HRESULT __stdcall Close(LPCVOID data) override
  {
    std::free(const_cast<void*>(data));
    return S_OK;
  }

  const std::vector<std::vector<std::uint8_t>>& Dependencies() const { return m_dependencies; }

private:
  std::set<std::string> m_paths;
  std::vector<std::vector<std::uint8_t>> m_dependencies;
};

constexpr UINT CompileFlags()
{
#ifdef _DEBUG
  return 0;
#else
  return D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY | D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
}

std::vector<std::uint8_t> KeyMaterial(const ShaderCompileInputDX& input,
                                      std::string_view preprocessed,
                                      const std::vector<std::vector<std::uint8_t>>& dependencies)
{
  std::vector<std::uint8_t> material;
  Frame(material, CShaderCompilerDX::BACKEND_ID);
  Frame(material, CShaderCompilerDX::COMPILER_ID);
  Frame(material, CShaderCompilerDX::EFFECTS11_VERSION);
  Frame(material, CShaderCompilerDX::TARGET);
  std::array<std::uint8_t, 4> abi{};
  for (unsigned int shift = 0; shift < 32; shift += 8)
    abi[shift / 8] = static_cast<std::uint8_t>(CShaderCompilerDX::CACHE_ABI >> shift);
  Frame(material, abi);
  std::array<std::uint8_t, 4> flags{};
  for (unsigned int shift = 0; shift < 32; shift += 8)
    flags[shift / 8] = static_cast<std::uint8_t>(CompileFlags() >> shift);
  Frame(material, flags);
  for (const auto& [name, definition] : input.defines)
  {
    Frame(material, name);
    Frame(material, definition);
  }
  Frame(material, input.source);
  Frame(material, preprocessed);
  for (const auto& dependency : dependencies)
    Frame(material, dependency);
  return material;
}
} // namespace

CShaderCompilerDX::CShaderCompilerDX(std::shared_ptr<std::atomic_uint> compileCounter)
  : m_compileCounter(std::move(compileCounter))
{
}

ShaderCompileRequest CShaderCompilerDX::CreateRequest(const ShaderPass& pass,
                                                      ShaderCompileContext context) const
{
  auto input = std::make_shared<ShaderCompileInputDX>();
  input->sourcePath = pass.sourcePath;
  input->source = pass.vertexSource;
  input->defines = {{"HLSL_4", ""}, {"HLSL_FX", ""}, {"PARAMETER_UNIFORM", ""}};

  std::vector<std::uint8_t> provisional;
  Frame(provisional, input->sourcePath);
  Frame(provisional, input->source);
  for (const auto& [name, definition] : input->defines)
  {
    Frame(provisional, name);
    Frame(provisional, definition);
  }
  return {Hash(provisional).hex, std::move(input), std::move(context)};
}

ShaderPrepareResult CShaderCompilerDX::Prepare(const IShaderCompileInput& opaque) const
{
  const auto& input = static_cast<const ShaderCompileInputDX&>(opaque);
  const auto macros = MakeMacros(input.defines);
  CIncludeResolver resolver(input.sourcePath);
  ComPtr<ID3DBlob> preprocessed;
  ComPtr<ID3DBlob> errors;
  const HRESULT result =
      D3DPreprocess(input.source.data(), input.source.size(), "", macros.data(), &resolver,
                    preprocessed.GetAddressOf(), errors.GetAddressOf());
  if (FAILED(result) || !preprocessed)
  {
    std::vector<std::uint8_t> failure;
    Frame(failure, input.source);
    for (const auto& dependency : resolver.Dependencies())
      Frame(failure, dependency);
    Frame(failure, BlobText(errors.Get()));
    return {{}, Hash(failure).hex, {}, BlobText(errors.Get())};
  }

  auto prepared = std::make_shared<ShaderPreparedUnitDX>();
  prepared->preprocessedSource.assign(static_cast<const char*>(preprocessed->GetBufferPointer()),
                                      preprocessed->GetBufferSize());
  prepared->dependencies = resolver.Dependencies();
  ShaderCompileKey key =
      Hash(KeyMaterial(input, prepared->preprocessedSource, prepared->dependencies));
  return {std::move(key), {}, std::move(prepared), {}};
}

ShaderCompileResult CShaderCompilerDX::Compile(const IShaderPreparedUnit& opaque) const
{
  const auto& prepared = static_cast<const ShaderPreparedUnitDX&>(opaque);
  if (m_compileCounter)
    ++*m_compileCounter;

  ComPtr<ID3DBlob> bytecode;
  ComPtr<ID3DBlob> errors;
  const HRESULT result = D3DCompile(
      prepared.preprocessedSource.data(), prepared.preprocessedSource.size(), "", nullptr, nullptr,
      "", TARGET.data(), CompileFlags(), 0, bytecode.GetAddressOf(), errors.GetAddressOf());
  if (FAILED(result) || !bytecode)
    return {{}, BlobText(errors.Get())};
  const auto* begin = static_cast<const std::uint8_t*>(bytecode->GetBufferPointer());
  return {{begin, begin + bytecode->GetBufferSize()}, {}};
}
} // namespace KODI::SHADER
