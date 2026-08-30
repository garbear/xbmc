# RetroPlayer Shader Compilation Cache Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Compile Windows RetroPlayer HLSL passes asynchronously, cache immutable FX11 bytecode by effective content, and warm exactly the Video Filter catalog without blocking filter focus or render interaction.

**Architecture:** A `CShaderPresetFactory`-owned compile service accepts backend-neutral pass requests, prepares them on a bounded Kodi job queue, converges provisional requests on a canonical SHA-256 key, and publishes immutable artifacts through stable handles. The Windows compiler preserves Kodi's current HLSL defines, include search, target, and flags; it persists portable FX bytecode in an atomic content-addressed store, while each render instance creates its own mutable Effects11 object on the render thread. The dialog submits its already-filtered catalog to the same service after either initial population or add-on refresh; GL/GLES keep their existing synchronous paths.

**Tech Stack:** C++20, Kodi `CJobQueue`/`CJobManager`, Kodi VFS, `KODI::UTILITY::CDigest` SHA-256, D3DCompiler 47, Effects11 11.29, Direct3D 11/WARP, GoogleTest, CMake/Visual Studio Debug builds.

**Spec:** `docs/superpowers/specs/2026-08-29-retroplayer-shader-compilation-cache-design.md`

## Global Constraints

- Work only on the current local `retroplayer-22beta2` history; do not reset, rebase, amend, squash, drop, rewrite, or push existing work.
- Do not modify `C:\Users\garrett\Documents\game.shader.presets` or Kodi's deployed add-on copy unless qualification proves a concrete source compatibility defect.
- Cache individual shader passes, never whole presets, games, ROMs, cores, resolutions, runtime parameters, or frame state.
- A successful Windows key must include ABI `1`, backend `d3d11-fx`, compiler `d3dcompiler_47`, Effects11 `1129`, target `fx_5_0`, exact flags/defines, raw root bytes, ordered raw transitive include bytes, and preprocessed bytes with explicit fixed-width length framing.
- Successful keys exclude filesystem paths so identical effective content at different locations deduplicates; source or loaded-include content changes miss, while timestamp-only changes hit.
- Compile CPU-side HLSL on at most two low-priority FIFO Kodi jobs; never create an `ID3DX11Effect`, D3D device resource, renderer object, or GUI object on those jobs.
- Share only immutable bytecode. Every `CShaderDX` owns an independent `CD3DEffect`, technique/pass state, bindings, parameters, buffers, LUT resources, textures, FBOs, and runtime dimensions.
- Persist only portable Effects11 bytecode at `special://temp/retroplayer/shaders/dx11/v1/<sha256>.fxc`; never include driver identity or driver-specific binaries.
- Cache files use a validated 64-character lowercase hex filename, a fixed envelope, a 64 MiB pre-allocation cap, unique same-directory temporary files, looped I/O, close-before-rename publication, and best-effort cleanup/removal.
- A pending preset renders unfiltered, remains outside `m_failedPaths`, and wakes render-thread realization through a weak generation token plus one atomic flag; stale selections and destroyed renderers receive no callback.
- Only a disk-provenance effect-load rejection may rearm a canonical entry, and it may do so exactly once; missing named `TEQ` or pass `P0` remains a terminal preset contract failure.
- Warm only exact paths already exposed by `CDialogGameVideoFilter::InitVideoFilters()` for the current backend; never recursively scan shader resources.
- Preinstalled and newly installed preset add-ons must reach the same catalog-ready warmup call; warmup ownership is independent of the dialog lifetime and concurrent identical catalogs coalesce.
- GL and GLES retain current synchronous compilation and runtime behavior; add only backend-neutral extension points, no persistent program binaries, no secondary contexts, and no Windows implementation guesses for those backends.
- Add deterministic automated tests using events and bounded failure waits; do not use timing sleeps as correctness conditions.
- Keep one useful DEBUG warmup summary and actionable failure diagnostics; remove temporary per-item/per-frame instrumentation before committing.

---

## File and responsibility map

- Create `xbmc/cores/RetroPlayer/shaders/ShaderCompileTypes.h` for generic states, diagnostic context, immutable artifacts, opaque backend inputs, and preparation/compile results.
- Create `xbmc/cores/RetroPlayer/shaders/IShaderCompiler.h` for the backend compiler and optional artifact-store contracts.
- Create `xbmc/cores/RetroPlayer/shaders/ShaderArtifactStore.h/.cpp` for validated, atomic, content-addressed VFS envelopes.
- Create `xbmc/cores/RetroPlayer/shaders/ShaderCompileService.h/.cpp` for stable handles, bounded jobs, provisional/canonical/failure maps, groups, listeners, shutdown, and disk-rejection retry.
- Create `xbmc/cores/RetroPlayer/shaders/windows/ShaderCompilerDX.h/.cpp` for DX inputs, current include semantics, dependency transcripts, explicit key framing, `D3DPreprocess`, and `D3DCompile`.
- Modify `xbmc/guilib/D3DResource.h/.cpp` to add retained-bytecode effect creation without changing existing source creation.
- Modify `xbmc/cores/RetroPlayer/shaders/windows/ShaderDX.h/.cpp` to create a per-instance effect from a compiled artifact and distinguish effect-load rejection from invalid `TEQ`.
- Modify `xbmc/cores/RetroPlayer/shaders/{IShaderPreset,ShaderPreset}.{h,cpp}` and `windows/ShaderPresetDX.{h,cpp}` for ready/pending/failed activation and request-group realization.
- Modify `xbmc/cores/RetroPlayer/rendering/VideoRenderers/RPBaseRenderer.{h,cpp}` for generation-safe render-thread activation after completion.
- Modify `xbmc/cores/RetroPlayer/shaders/{ShaderTypes,IShaderPresetLoader,ShaderPresetFactory}.{h,cpp}` and `xbmc/addons/ShaderPreset.{h,cpp}` for data-only parsing and shared synchronized loader snapshots.
- Modify `xbmc/games/dialogs/osd/DialogGameVideoFilter.{h,cpp}` to submit exact catalog paths and replace raw-dialog polling with GUI-thread refresh notification.
- Create generic tests in `xbmc/cores/RetroPlayer/shaders/test/`; create Windows tests in the existing `xbmc/cores/RetroPlayer/shaders/windows/test/`; register them in the corresponding CMake and treedata files.

### Task 1: Generic Compile Service and Complete Windows Bytecode Cache

**Files:**
- Create: `xbmc/cores/RetroPlayer/shaders/ShaderCompileTypes.h`
- Create: `xbmc/cores/RetroPlayer/shaders/IShaderCompiler.h`
- Create: `xbmc/cores/RetroPlayer/shaders/ShaderArtifactStore.h`
- Create: `xbmc/cores/RetroPlayer/shaders/ShaderArtifactStore.cpp`
- Create: `xbmc/cores/RetroPlayer/shaders/ShaderCompileService.h`
- Create: `xbmc/cores/RetroPlayer/shaders/ShaderCompileService.cpp`
- Create: `xbmc/cores/RetroPlayer/shaders/windows/ShaderCompilerDX.h`
- Create: `xbmc/cores/RetroPlayer/shaders/windows/ShaderCompilerDX.cpp`
- Create: `xbmc/cores/RetroPlayer/shaders/test/CMakeLists.txt`
- Create: `xbmc/cores/RetroPlayer/shaders/test/TestShaderArtifactStore.cpp`
- Create: `xbmc/cores/RetroPlayer/shaders/test/TestShaderCompileService.cpp`
- Create: `xbmc/cores/RetroPlayer/shaders/test/TestShaderPresetAsync.cpp`
- Create: `xbmc/cores/RetroPlayer/shaders/windows/test/TestShaderCompilerDX.cpp`
- Create: `xbmc/cores/RetroPlayer/shaders/windows/test/TestD3DEffectBytecode.cpp`
- Modify: `xbmc/cores/RetroPlayer/shaders/CMakeLists.txt`
- Modify: `xbmc/cores/RetroPlayer/shaders/windows/CMakeLists.txt`
- Modify: `xbmc/cores/RetroPlayer/shaders/windows/test/CMakeLists.txt`
- Modify: `cmake/treedata/common/tests.txt`
- Modify: `xbmc/guilib/D3DResource.h`
- Modify: `xbmc/guilib/D3DResource.cpp`
- Modify: `xbmc/cores/RetroPlayer/shaders/IShaderPreset.h`
- Modify: `xbmc/cores/RetroPlayer/shaders/ShaderPreset.h`
- Modify: `xbmc/cores/RetroPlayer/shaders/ShaderPreset.cpp`
- Modify: `xbmc/cores/RetroPlayer/shaders/ShaderPresetFactory.h`
- Modify: `xbmc/cores/RetroPlayer/shaders/ShaderPresetFactory.cpp`
- Modify: `xbmc/cores/RetroPlayer/shaders/gl/ShaderPresetGL.h`
- Modify: `xbmc/cores/RetroPlayer/shaders/gl/ShaderPresetGL.cpp`
- Modify: `xbmc/cores/RetroPlayer/shaders/gles/ShaderPresetGLES.h`
- Modify: `xbmc/cores/RetroPlayer/shaders/gles/ShaderPresetGLES.cpp`
- Modify: `xbmc/cores/RetroPlayer/shaders/windows/ShaderDX.h`
- Modify: `xbmc/cores/RetroPlayer/shaders/windows/ShaderDX.cpp`
- Modify: `xbmc/cores/RetroPlayer/shaders/windows/ShaderPresetDX.h`
- Modify: `xbmc/cores/RetroPlayer/shaders/windows/ShaderPresetDX.cpp`
- Modify: `xbmc/cores/RetroPlayer/rendering/VideoRenderers/RPBaseRenderer.h`
- Modify: `xbmc/cores/RetroPlayer/rendering/VideoRenderers/RPBaseRenderer.cpp`

**Interfaces:**
- Consumes: Existing `ShaderPass`, Kodi VFS, `CJobQueue`, `CDigest`, `CD3DEffect`, `CShaderPreset` lifecycle, and Windows render-thread/device ownership.
- Produces: The following stable API for Task 2 and runtime use:

```cpp
enum class ShaderCompileState { UNKNOWN, QUEUED, COMPILING, READY, FAILED };
enum class ShaderArtifactOrigin { DISK, COMPILED };
enum class ShaderRequestDisposition { MEMORY_HIT, DISK_HIT, QUEUED };
enum class ShaderPresetState { READY, PENDING, FAILED };

struct ShaderCompileKey
{
  std::array<std::uint8_t, 32> raw;
  std::string hex;
};

struct ShaderCompileContext
{
  std::string presetPath;
  unsigned int passIndex{0};
  std::string passAlias;
  std::string shaderPath;
};

struct ShaderCompiledArtifact
{
  ShaderCompileKey key;
  std::shared_ptr<const std::vector<std::uint8_t>> bytecode;
  ShaderArtifactOrigin origin{ShaderArtifactOrigin::COMPILED};
};

enum class ShaderCacheLoadState { MISS, HIT, CORRUPT };

struct ShaderCacheLoadResult
{
  ShaderCacheLoadState state{ShaderCacheLoadState::MISS};
  std::shared_ptr<const std::vector<std::uint8_t>> bytecode;
};

class IShaderCompileInput { public: virtual ~IShaderCompileInput() = default; };
class IShaderPreparedUnit { public: virtual ~IShaderPreparedUnit() = default; };

struct ShaderCompileRequest
{
  std::string provisionalKey;
  std::shared_ptr<const IShaderCompileInput> input;
  ShaderCompileContext context;
};

struct ShaderPrepareResult
{
  std::optional<ShaderCompileKey> canonicalKey;
  std::string failureFingerprint;
  std::shared_ptr<const IShaderPreparedUnit> prepared;
  std::string error;
};

struct ShaderCompileResult
{
  std::vector<std::uint8_t> bytecode;
  std::string error;
};

class IShaderCompiler
{
public:
  virtual ~IShaderCompiler() = default;
  virtual std::string_view GetBackendId() const = 0;
  virtual ShaderCompileRequest CreateRequest(const ShaderPass& pass,
                                             ShaderCompileContext context) const = 0;
  virtual ShaderPrepareResult Prepare(const IShaderCompileInput& input) const = 0;
  virtual ShaderCompileResult Compile(const IShaderPreparedUnit& prepared) const = 0;
};

class IShaderArtifactStore
{
public:
  virtual ~IShaderArtifactStore() = default;
  virtual ShaderCacheLoadResult Load(const ShaderCompileKey& key) = 0;
  virtual bool Store(const ShaderCompileKey& key,
                     std::span<const std::uint8_t> payload) = 0;
  virtual void Remove(const ShaderCompileKey& key) = 0;
};

class CShaderCompileHandle
{
public:
  ShaderCompileState GetState() const;
  std::uint64_t GetGeneration() const;
  ShaderRequestDisposition GetDisposition() const;
  std::shared_ptr<const ShaderCompiledArtifact> GetArtifact() const;
  std::string GetError() const;
  void AddCompletionCallback(std::function<void()> callback) const;
};

class CShaderCompileService
{
public:
  void RegisterCompiler(std::shared_ptr<IShaderCompiler> compiler,
                        std::shared_ptr<IShaderArtifactStore> store = {});
  bool SupportsAsyncCompilation(std::string_view backendId) const;
  std::shared_ptr<CShaderCompileHandle> Request(std::string_view backendId,
                                               const ShaderPass& pass,
                                               ShaderCompileContext context);
  std::shared_ptr<CShaderCompileGroup> RequestGroup(
      std::string_view backendId,
      const std::vector<ShaderPass>& passes,
      std::string presetPath,
      std::function<void()> completion);
  bool RejectDiskArtifact(const std::shared_ptr<CShaderCompileHandle>& handle,
                          std::uint64_t generation);
};

class CShaderCompileGroup
{
public:
  ShaderPresetState GetState() const;
  std::vector<std::shared_ptr<CShaderCompileHandle>> GetHandles() const;
  void AddCompletionCallback(std::function<void()> callback) const;
};
```

- `CShaderCompileGroup::GetState()` returns `PENDING` while any member is queued/compiling/unknown, `FAILED` when all are terminal and at least one failed, and `READY` only when every member owns an artifact. It rearms once when `RejectDiskArtifact()` advances a member generation.
- Artifact origin is immutable (`DISK` or `COMPILED`). Each handle separately records whether this request was a memory hit, disk hit, or queued miss so warmup accounting remains truthful when a later request reuses an existing canonical entry.
- A canonical entry retains its prepared unit and compiler after a disk hit. `RejectDiskArtifact()` marks the one retry spent, advances generation, clears the artifact, and transitions to queued under lock; it removes the VFS entry and submits compilation after unlocking.
- The service is owned by `CShaderPresetFactory`, which exposes `CompileService()` in Task 1. Task 2 adds the warmup entry point on the same owner.

- [ ] **Step 1: Re-run the current shader baseline before changing production code**

Run:

```powershell
build\Debug\kodi-test.exe --gtest_filter=TestShaderVertexDX.*:TestRPWinOutputShaderDX.*:TestShaderDX.*:TestShaderUtilsDX.*
```

Expected: 14 tests pass and no test fails. Record the exact output in the task report.

- [ ] **Step 2: Register the new generic and Windows test targets without production implementations**

Add `xbmc/cores/RetroPlayer/shaders/test/CMakeLists.txt`:

```cmake
set(SOURCES TestShaderArtifactStore.cpp
            TestShaderCompileService.cpp
            TestShaderPresetAsync.cpp)

core_add_test_library(retroplayer_shaders_test)
```

Append the generic directory to `cmake/treedata/common/tests.txt`, and add `TestShaderCompilerDX.cpp` plus `TestD3DEffectBytecode.cpp` to the existing Windows test CMake list. Add the new production source/header names to the generic and Windows shader CMake lists.

- [ ] **Step 3: Write failing persistent-store envelope tests**

Use an injected UUID-named child under `special://temp/retroplayer-shader-tests/`. Define `KEY_A` as 64 lowercase `a` characters, `BYTES_A` as `{0x01, 0x02, 0x03}`, and helpers that write exact envelope bytes. Assert these concrete cases:

```cpp
TEST(TestShaderArtifactStore, RoundTripValidEntry)
{
  CShaderArtifactStore store(root, ".fxc", 1, MAX_PAYLOAD_SIZE);
  ASSERT_TRUE(store.Store(KEY_A, BYTES_A));
  const ShaderCacheLoadResult result = store.Load(KEY_A);
  ASSERT_EQ(ShaderCacheLoadState::HIT, result.state);
  EXPECT_EQ(BYTES_A, result.bytecode);
}

TEST(TestShaderArtifactStore, RejectsTruncatedOversizedWrongKeyAndWrongDigest)
{
  WriteTruncatedEnvelope(root, KEY_A);
  EXPECT_EQ(ShaderCacheLoadState::CORRUPT, store.Load(KEY_A).state);
  WriteOversizedEnvelope(root, KEY_A, MAX_PAYLOAD_SIZE + 1);
  EXPECT_EQ(ShaderCacheLoadState::CORRUPT, store.Load(KEY_A).state);
  WriteEnvelopeWithWrongEmbeddedKey(root, KEY_A, BYTES_A);
  EXPECT_EQ(ShaderCacheLoadState::CORRUPT, store.Load(KEY_A).state);
  WriteEnvelopeWithWrongDigest(root, KEY_A, BYTES_A);
  EXPECT_EQ(ShaderCacheLoadState::CORRUPT, store.Load(KEY_A).state);
}

TEST(TestShaderArtifactStore, RejectsNonLowercaseOrNonSha256Key)
{
  EXPECT_EQ(ShaderCacheLoadState::MISS, store.Load("../bad").state);
  EXPECT_FALSE(store.Store("ABC", BYTES_A));
}
```

- [ ] **Step 4: Run the artifact-store tests and observe the expected red state**

Run:

```powershell
cmake --build build --config Debug --target kodi-test -- /m
build\Debug\kodi-test.exe --gtest_filter=TestShaderArtifactStore.*
```

Expected: build/test failure because `CShaderArtifactStore` and its envelope behavior do not exist yet. A failure caused only by an unrelated stale build must be repaired before proceeding.

- [ ] **Step 5: Implement the fixed persistent-store wire format and atomic publication**

Implement constants and explicit little-endian helpers rather than serializing native structs:

```cpp
constexpr std::array<std::uint8_t, 8> MAGIC{'K', 'R', 'P', 'F', 'X', 'C', 0, 0};
constexpr std::uint32_t FORMAT_VERSION{1};
constexpr std::uint64_t MAX_PAYLOAD_SIZE{64ULL * 1024ULL * 1024ULL};

// Envelope: magic | u32 version | 32 raw key bytes | u64 payload length |
//           32 raw payload SHA-256 bytes | payload.
```

Validate the key before `URIUtils::AddFileToFolder()`. Use `CFile::GetLength()` before allocation, loop `Read()`/`Write()` for short I/O, write `<key>.<uuid>.tmp` in the same directory, flush/close, and rename without replacing. If rename loses a race, validate the winner and delete the temporary file. Delete corrupt final entries best-effort; a store failure must not invalidate the in-memory artifact.

- [ ] **Step 6: Run artifact-store tests green, including concurrent immutable publication**

Add a test where two store objects publish identical key/payload concurrently and both callers finish with one valid final envelope and no sibling `.tmp` files. Then run:

```powershell
cmake --build build --config Debug --target kodi-test -- /m
build\Debug\kodi-test.exe --gtest_filter=TestShaderArtifactStore.*
```

Expected: all artifact-store tests pass.

- [ ] **Step 7: Write failing generic service tests for state, deduplication, failure memoization, cache hits, retry, and shutdown**

Implement a deterministic fake compiler with atomic prepare/compile counters and `CEvent` gates. Cover these exact assertions:

```cpp
TEST_F(TestShaderCompileService, IdenticalAndConcurrentRequestsCompileOnce);
TEST_F(TestShaderCompileService, DifferentProvisionalRequestsConvergeOnOneCanonicalCompile);
TEST_F(TestShaderCompileService, SharedPassAcrossPresetContextsCompilesOnce);
TEST_F(TestShaderCompileService, PendingAndFailedRemainDistinct);
TEST_F(TestShaderCompileService, UnchangedFailureIsNotRequeuedOrRelogged);
TEST_F(TestShaderCompileService, ValidPersistentHitDoesNotInvokeCompiler);
TEST_F(TestShaderCompileService, CorruptPersistentEntryCompilesAndRepublishes);
TEST_F(TestShaderCompileService, DiskArtifactRejectionRecompilesExactlyOnce);
TEST_F(TestShaderCompileService, GroupCompletesOncePerGeneration);
TEST_F(TestShaderCompileService, DestructionWithQueuedAndRunningJobsIsSafe);
```

The concurrent test must hold preparation/compile on manual-reset events, issue requests from two callers, assert `QUEUED`/`COMPILING`, release the gate, bounded-wait for terminal state, and assert `compileCount == 1`. Do not override `CJob::Equals()`; deduplication belongs exclusively to service maps.

- [ ] **Step 8: Run the compile-service tests and observe the expected red state**

Run:

```powershell
cmake --build build --config Debug --target kodi-test -- /m
build\Debug\kodi-test.exe --gtest_filter=TestShaderCompileService.*
```

Expected: build/test failure because the generic request state, service, and group do not exist.

- [ ] **Step 9: Implement stable handles and the bounded compile pipeline**

Use these internal ownership rules:

```cpp
struct ServiceState
{
  CCriticalSection mutex;
  bool stopping{false};
  std::map<std::string, std::weak_ptr<RequestState>, std::less<>> provisional;
  std::map<std::string, std::shared_ptr<CanonicalEntry>, std::less<>> canonical;
  std::map<std::string, std::shared_ptr<CanonicalEntry>, std::less<>> preparationFailures;
  std::map<std::string, CompilerRegistration, std::less<>> compilers;
};

struct Dispatcher
{
  CCriticalSection mutex;
  CJobQueue* queue{nullptr};
  bool stopping{false};
};
```

`CShaderCompileService` owns `CJobQueue(false, 2, CJob::PRIORITY_LOW)` and a shared dispatcher, but queued jobs capture only `ServiceState`, dispatcher, immutable input, compiler, store, and request state. On destruction set dispatcher stopping/null before `CancelJobs()`; running jobs may finish only into retained state. Remove provisional identities at terminal, retain canonical ready/failed entries for process lifetime, perform the second canonical lookup under the state lock, copy listeners under lock, and invoke them after unlock. Finalize every SHA-256 once to the raw 32-byte value and derive the lowercase hex string from that raw value; never finalize one `CDigest` twice.

- [ ] **Step 10: Make all generic service tests pass and run them repeatedly**

Run twice to catch retained-state/order defects:

```powershell
cmake --build build --config Debug --target kodi-test -- /m
build\Debug\kodi-test.exe --gtest_repeat=2 --gtest_filter=TestShaderCompileService.*:TestShaderArtifactStore.*
```

Expected: both repetitions pass; compile counters match the assertions; the process exits normally.

- [ ] **Step 11: Write failing DX preparation/key/compiler tests**

Create VFS-backed root and nested include fixtures and cover:

```cpp
TEST(TestShaderCompilerDX, UnchangedBytesProduceSameCanonicalKey);
TEST(TestShaderCompilerDX, RootContentChangeProducesMiss);
TEST(TestShaderCompilerDX, NestedIncludeContentChangeProducesMiss);
TEST(TestShaderCompilerDX, TimestampOnlyChangeDoesNotChangeKey);
TEST(TestShaderCompilerDX, IdenticalContentAtDifferentPathsDeduplicates);
TEST(TestShaderCompilerDX, PreparationFailureFingerprintChangesAfterIncludeRepair);
TEST(TestShaderCompilerDX, DefinesAndNestedIncludesMatchLegacyEffects11Compilation);
```

The equivalence fixture must use `HLSL_4`, `HLSL_FX`, and `PARAMETER_UNIFORM` with empty definitions; nested includes must exercise the current ordered include-path expansion; the resulting new-path bytecode and legacy one-step effect must both expose valid named `TEQ` and pass `P0` on WARP.

- [ ] **Step 12: Run the DX compiler tests and observe the expected red state**

Run:

```powershell
cmake --build build --config Debug --target kodi-test -- /m
build\Debug\kodi-test.exe --gtest_filter=TestShaderCompilerDX.*
```

Expected: build/test failure because `CShaderCompilerDX` does not exist.

- [ ] **Step 13: Implement exact DX include resolution, preprocessing, key framing, and compilation**

Define the DX input/prepared types and fixed settings in `ShaderCompilerDX`:

```cpp
struct ShaderCompileInputDX final : IShaderCompileInput
{
  std::string sourcePath;
  std::string source;
  DefinesMap defines;
};

struct ShaderPreparedUnitDX final : IShaderPreparedUnit
{
  std::string preprocessedSource;
  std::vector<std::vector<std::uint8_t>> dependencies;
};

constexpr std::string_view BACKEND_ID{"d3d11-fx"};
constexpr std::string_view COMPILER_ID{"d3dcompiler_47"};
constexpr std::string_view EFFECTS11_VERSION{"1129"};
constexpr std::string_view TARGET{"fx_5_0"};
constexpr std::uint32_t CACHE_ABI{1};
```

Start the resolver with `special://xbmc/system/shaders/` and `URIUtils::GetBasePath(sourcePath)`, search its `std::set` in lexical order, read through Kodi VFS, ignore `pParentData`, and insert each resolved include's base path. Record each returned include's raw bytes in resolution order. Run `D3DPreprocess(source, "", macros, resolver, ...)`, frame every key field with explicit fixed-endian length values, then run `D3DCompile(preprocessed, "", nullptr, nullptr, "", "fx_5_0", flags, 0, ...)`. Debug flags are zero; Release flags are backwards compatibility plus optimization level 3. Return compiler blobs verbatim in diagnostic error strings.

- [ ] **Step 14: Make DX compiler tests pass and verify persistent hits skip `D3DCompile`**

Inject a compile-call counter seam used only by tests, request once into an empty store, reconstruct the service/store, request unchanged content again, and assert the second service reaches `READY` from `DISK` while the counter remains unchanged. Run:

```powershell
cmake --build build --config Debug --target kodi-test -- /m
build\Debug\kodi-test.exe --gtest_filter=TestShaderCompilerDX.*:TestShaderCompileService.*:TestShaderArtifactStore.*
```

Expected: all selected tests pass.

- [ ] **Step 15: Write failing retained-bytecode Effects11 tests**

Use the existing WARP device fixture and a minimal `TEQ` effect:

```cpp
TEST_F(TestD3DEffectBytecode, CreatesNamedTechniqueFromD3DCompileBytecode);
TEST_F(TestD3DEffectBytecode, TwoEffectsFromOneArtifactKeepIndependentScalars);
TEST_F(TestD3DEffectBytecode, DeviceRecreationUsesRetainedBytecodeWithoutCompile);
```

The independence test creates two `CD3DEffect` objects from the same shared byte vector, writes different scalar values, reads each underlying effect variable, and asserts they differ. The recreation test calls the resource destroy/create path twice and asserts the injected compile count stays one.

- [ ] **Step 16: Run retained-bytecode tests red, then add the bytecode creation overload**

First run and record the missing-overload failure:

```powershell
cmake --build build --config Debug --target kodi-test -- /m
build\Debug\kodi-test.exe --gtest_filter=TestD3DEffectBytecode.*
```

Then add:

```cpp
using EffectBytecode = std::vector<std::uint8_t>;

bool CD3DEffect::Create(std::shared_ptr<const EffectBytecode> effectBytecode);
bool CD3DEffect::CreateEffectFromBytecode();
```

Source `Create()` clears retained bytecode; bytecode `Create()` clears source/defines, retains the shared bytes, calls `D3DX11CreateEffectFromMemory(bytes, size, 0, device, ..., "")`, and registers the resource. `OnDestroyDevice()` drops only effect/technique/pass; `OnCreateDevice()` recreates from retained bytecode. Leave the existing source overload and its callers unchanged.

- [ ] **Step 17: Run all bytecode/compiler tests green**

Run:

```powershell
cmake --build build --config Debug --target kodi-test -- /m
build\Debug\kodi-test.exe --gtest_filter=TestD3DEffectBytecode.*:TestShaderCompilerDX.*:TestShaderUtilsDX.*
```

Expected: all selected tests pass, including legacy source-path coverage.

- [ ] **Step 18: Write failing preset pending/lifetime tests**

Add deterministic tests around a fake request group and generation token:

```cpp
TEST(TestShaderPresetAsync, QueuedPassReturnsPendingWithoutFailedPath);
TEST(TestShaderPresetAsync, CompletionWakesRenderThreadAndRealizesOnce);
TEST(TestShaderPresetAsync, StaleSelectionCannotWakeNewSelection);
TEST(TestShaderPresetAsync, RendererDestructionBeforeCompletionIsSafe);
TEST(TestShaderPresetAsync, MissingTechniqueIsTerminalWithoutDiskRetry);
```

Expose only narrow protected/test helpers if required: a fake backend may report `PENDING`, `READY`, or `FAILED`, and the test may query `HasPathFailed()` through the derived fixture. Do not add public test-only APIs.

- [ ] **Step 19: Run pending/lifetime tests and observe the expected red state**

Run:

```powershell
cmake --build build --config Debug --target kodi-test -- /m
build\Debug\kodi-test.exe --gtest_filter=TestShaderPresetAsync.*
```

Expected: build/test failure because preset activation is still boolean/synchronous.

- [ ] **Step 20: Implement tri-state preset activation and render-thread bytecode realization**

Change the protected backend creation contract and DX-only result:

```cpp
virtual ShaderPresetState CreateShaders() = 0;
virtual ShaderPresetState SetShaderPreset(const std::string& path) = 0;

enum class ShaderCreateResult
{
  READY,
  EFFECT_CREATION_FAILED,
  INVALID_TECHNIQUE,
};

ShaderCreateResult CShaderDX::CreateFromBytecode(
    unsigned int passIdx,
    std::string passAlias,
    std::string shaderPath,
    std::shared_ptr<const EffectBytecode> bytecode,
    ShaderParameterMap parameters,
    std::vector<std::shared_ptr<IShaderLut>> luts,
    unsigned int frameCountMod = 0);
```

Keep the existing common `IShader::Create(source...)` implementation available for unchanged GL/GLES and legacy callers. `CShaderPresetDX` registers/uses `CShaderCompilerDX`, requests all pass handles, returns `PENDING` before creating LUTs or GPU objects, and only after group readiness creates one independent `CShaderDX` per artifact plus existing layouts/buffers/samplers. Split GPU cleanup from pass-data cleanup so pending retains `m_passes`. Only a disk artifact returning `EFFECT_CREATION_FAILED` calls `RejectDiskArtifact(handle, generation)`; `INVALID_TECHNIQUE` is terminal.

`CRPBaseRenderer` owns a fresh shared generation token for each requested path:

```cpp
struct ShaderWakeToken
{
  const std::uint64_t generation;
  std::atomic_bool ready{false};
};
```

Completion captures only `weak_ptr<ShaderWakeToken>`. `Updateshaders()` consumes the atomic on the render thread, verifies the current generation, and retries realization; pending keeps `m_bUseShaderPreset` false and does not set `m_failedPaths`. Selection replacement drops the old strong token; renderer destruction drops the last token.

- [ ] **Step 21: Preserve GL/GLES behavior explicitly**

For `CShaderPresetGL` and `CShaderPresetGLES`, adapt the return type only:

```cpp
ShaderPresetState CShaderPresetGL::CreateShaders()
{
  const auto numPasses = static_cast<unsigned int>(m_passes.size());
  for (unsigned int shaderIdx = 0; shaderIdx < numPasses; ++shaderIdx)
  {
    const ShaderPass& pass = m_passes[shaderIdx];
    auto videoShader = std::make_unique<CShaderGL>(m_presetPath);
    if (!videoShader->Create(shaderIdx, pass.alias, pass.sourcePath, pass.vertexSource,
                             GetShaderParameters(pass.parameters, pass.vertexSource),
                             presetLUTsGL, pass.frameCountMod))
      return ShaderPresetState::FAILED;
    m_pShaders.push_back(std::move(videoShader));
  }
  return ShaderPresetState::READY;
}
```

Retain the existing LUT creation and logging around this pass loop; change only the declaration and each current `return false`/final `return true` to `FAILED`/`READY`. Apply the same mechanical return-type adaptation to GLES. Do not register an async compiler, parse in background, add persistent artifacts, or change graphics-context calls. Add/retain a generic assertion that an unregistered backend reports unsupported and follows the synchronous path.

- [ ] **Step 22: Run the complete task test gate**

Run:

```powershell
cmake --build build --config Debug --target kodi-test -- /m
build\Debug\kodi-test.exe --gtest_filter=TestShaderCompileService.*:TestShaderArtifactStore.*:TestShaderCompilerDX.*:TestD3DEffectBytecode.*:TestShaderPresetAsync.*:TestShaderVertexDX.*:TestRPWinOutputShaderDX.*:TestShaderDX.*:TestShaderUtilsDX.*
```

Expected: every selected test passes; no process hang; all original 14 baseline tests remain green.

- [ ] **Step 23: Inspect Task 1 for ownership, blocking, and cache-contract defects**

Run:

```powershell
git diff --check
git diff --stat
git status --short
rg -n "D3DCompile|D3DPreprocess|D3DX11CreateEffectFromMemory|D3DX11CompileEffectFromMemory" xbmc/cores/RetroPlayer xbmc/guilib/D3DResource.cpp
rg -n "std::thread|LOW_PAUSABLE|Sleep\(|m_failedPaths" xbmc/cores/RetroPlayer/shaders xbmc/cores/RetroPlayer/rendering/VideoRenderers/RPBaseRenderer.*
```

Confirm source compilation appears only in the worker compiler or preserved legacy `CD3DEffect` source path; the async jobs have no raw renderer/factory/dialog/service capture; no mutable effect is shared; cache keys/path handling match Global Constraints; pending has no permanent-failure insertion; GL/GLES contain no new async graphics work.

- [ ] **Step 24: Commit Task 1 with the approved message**

Stage only Task 1 production/tests/CMake plus this plan file, verify the staged diff, and commit:

```text
[games] Cache compiled RetroPlayer shaders on Windows

Compile HLSL effect source on bounded worker jobs and cache immutable FX
bytecode by effective content. Reuse artifacts across presets, games, device
recreation, and Kodi processes while keeping each Effects11 object and its
mutable bindings per shader instance.

Keep pending presets out of the permanent failure path and wake the render
thread only after all requested passes become terminal.
```

### Task 2: Manifest-Only Warmup and Safe Video Filter Refresh

**Files:**
- Create: `xbmc/cores/RetroPlayer/shaders/test/TestShaderPresetLoaderRegistry.cpp`
- Create: `xbmc/cores/RetroPlayer/shaders/test/TestShaderWarmup.cpp`
- Modify: `xbmc/cores/RetroPlayer/shaders/test/CMakeLists.txt`
- Modify: `xbmc/cores/RetroPlayer/shaders/ShaderTypes.h`
- Modify: `xbmc/cores/RetroPlayer/shaders/IShaderPresetLoader.h`
- Modify: `xbmc/cores/RetroPlayer/shaders/IShaderPreset.h`
- Modify: `xbmc/cores/RetroPlayer/shaders/ShaderPreset.h`
- Modify: `xbmc/cores/RetroPlayer/shaders/ShaderPreset.cpp`
- Modify: `xbmc/cores/RetroPlayer/shaders/ShaderPresetFactory.h`
- Modify: `xbmc/cores/RetroPlayer/shaders/ShaderPresetFactory.cpp`
- Modify: `xbmc/addons/ShaderPreset.h`
- Modify: `xbmc/addons/ShaderPreset.cpp`
- Modify: `xbmc/games/dialogs/osd/DialogGameVideoFilter.h`
- Modify: `xbmc/games/dialogs/osd/DialogGameVideoFilter.cpp`

**Interfaces:**
- Consumes: Task 1 `CShaderCompileService`, `IShaderCompiler`, handles/groups, and DX backend registration.
- Produces: Data-only preset parsing, synchronized loader snapshots, exact-catalog warmup, catalog summary, and lifetime-safe GUI refresh:

```cpp
struct ShaderPresetDefinition
{
  std::vector<ShaderPass> passes;
};

class IShaderPresetLoader
{
public:
  virtual ~IShaderPresetLoader() = default;
  virtual bool LoadPreset(std::string_view presetPath,
                          ShaderPresetDefinition& definition) = 0;
};

struct ShaderWarmupSummary
{
  std::size_t presets{0};
  std::size_t passes{0};
  std::size_t unique{0};
  std::size_t memoryHits{0};
  std::size_t diskHits{0};
  std::size_t queued{0};
  std::size_t failed{0};
};

class CShaderPresetFactory
{
public:
  bool LoadPreset(std::string_view presetPath, ShaderPresetDefinition& definition) const;
  CShaderCompileService& CompileService();
  void WarmupPresets(std::string backendId, std::vector<std::string> presetPaths);
};
```

- Loader lookup copies a `shared_ptr<IShaderPresetLoader>` from an immutable snapshot under a shared lock and invokes it after unlocking; the owning `CShaderPresetAddon` remains alive for the call and serializes its binary instance.
- Warmup signatures exist only while enumeration is active. A later dialog open reparses/revalidates contents, while unchanged passes resolve to memory/disk hits without source compilation.

- [ ] **Step 1: Write failing data-only loader snapshot tests**

Add fake loaders and replacement gates:

```cpp
TEST(TestShaderPresetLoaderRegistry, LoadsIntoDefinitionWithoutRendererObject);
TEST(TestShaderPresetLoaderRegistry, InFlightLoadSurvivesAddonSnapshotReplacement);
TEST(TestShaderPresetLoaderRegistry, ReinstallReplacesOldAndFailedSameIdInstance);
TEST(TestShaderPresetLoaderRegistry, ConcurrentLookupAndPublicationAreSafe);
```

The in-flight test holds the fake loader, publishes a replacement snapshot, releases the old load, and verifies both definitions are valid and the old loader is destroyed only after its call returns.

- [ ] **Step 2: Run loader tests and observe the expected red state**

Run:

```powershell
cmake --build build --config Debug --target kodi-test -- /m
build\Debug\kodi-test.exe --gtest_filter=TestShaderPresetLoaderRegistry.*
```

Expected: build/test failure because loaders still require a mutable renderer-facing `IShaderPreset` and registry maps are raw/unsynchronized.

- [ ] **Step 3: Refactor parsing to `ShaderPresetDefinition` and publish shared snapshots**

Move add-on translation to `definition.passes`:

```cpp
bool CShaderPresetAddon::LoadPreset(std::string_view presetPath,
                                    ShaderPresetDefinition& definition)
{
  std::unique_lock lock(m_dllSection);
  // Invoke the add-on and translate its returned ABI struct directly into
  // definition.passes while the binary instance remains serialized.
}
```

Use `shared_ptr<CShaderPresetAddon>` ownership and a locked immutable loader snapshot. A reinstall event for an add-on ID must force a fresh wrapper and retry a formerly failed entry. `CShaderPreset::ReadPresetFile()` loads into a local definition and moves passes into the instance only on success, preventing stale append behavior. Remove mutable `IShaderPreset::GetPasses()` if no reader still requires it; otherwise expose only `const std::vector<ShaderPass>&`.

- [ ] **Step 4: Run loader tests and existing parser/backend tests green**

Run:

```powershell
cmake --build build --config Debug --target kodi-test -- /m
build\Debug\kodi-test.exe --gtest_filter=TestShaderPresetLoaderRegistry.*:TestShaderPresetAsync.*:TestShaderCompilerDX.*:TestShaderUtilsDX.*
```

Expected: all selected tests pass.

- [ ] **Step 5: Write failing manifest-only and idempotent warmup tests**

Use a fake loader whose definitions reference shared and unique passes, a fake compiler, and exact supplied paths:

```cpp
TEST_F(TestShaderWarmup, CompilesOnlyPassesFromSuppliedExposedPresetPaths)
{
  factory.WarmupPresets("fake", {EXPOSED_A, EXPOSED_B});
  WaitForSummary();
  EXPECT_EQ(2u, summary.presets);
  EXPECT_EQ(4u, summary.passes);
  EXPECT_EQ(3u, summary.unique);
  EXPECT_FALSE(loader.WasAskedFor(UNEXPOSED_PRESET));
  EXPECT_EQ(3u, compiler.compileCount);
}

TEST_F(TestShaderWarmup, ConcurrentIdenticalCatalogsCoalesce);
TEST_F(TestShaderWarmup, LaterReopenRevalidatesAndUsesReadyEntries);
TEST_F(TestShaderWarmup, UnsupportedBackendReturnsBeforeParsing);
TEST_F(TestShaderWarmup, ParseFailureDoesNotAbortOtherPresets);
TEST_F(TestShaderWarmup, SummarySeparatesMemoryDiskQueuedAndFailed);
```

The concurrent test gates enumeration, submits the same normalized path set twice, and asserts one parse per path. The reopen test submits after terminal completion and asserts paths are parsed again but compiler invocations remain unchanged for identical content.

- [ ] **Step 6: Run warmup tests and observe the expected red state**

Run:

```powershell
cmake --build build --config Debug --target kodi-test -- /m
build\Debug\kodi-test.exe --gtest_filter=TestShaderWarmup.*
```

Expected: build/test failure because the factory has no manifest warmup entry point.

- [ ] **Step 7: Implement service-owned catalog enumeration and accounting**

Normalize/sort/deduplicate only the caller-supplied paths, hash that path set for an active signature, and queue one low-priority enumeration job only when the backend supports async compilation. The job captures shared factory-loader state rather than a raw factory, parses each supplied path, creates pass contexts with preset/pass/alias/shader path, requests the same compile service used by runtime, and counts canonical uniqueness by terminal handle/key association. Remove the active signature when enumeration completes.

Emit exactly one normal summary per completed catalog:

```cpp
CLog::Log(LOGDEBUG,
          "Video shader warmup: {} presets, {} passes, {} unique, {} memory, "
          "{} disk, {} queued, {} failed",
          summary.presets, summary.passes, summary.unique, summary.memoryHits,
          summary.diskHits, summary.queued, summary.failed);
```

Compiler failures retain `ShaderCompileContext`; one preset parse failure logs that preset and enumeration continues. Do not log routine per-hit or per-frame lines.

- [ ] **Step 8: Make warmup tests pass, including exact-path exclusion**

Run twice:

```powershell
cmake --build build --config Debug --target kodi-test -- /m
build\Debug\kodi-test.exe --gtest_repeat=2 --gtest_filter=TestShaderWarmup.*:TestShaderCompileService.*:TestShaderPresetLoaderRegistry.*
```

Expected: every repetition passes; `UNEXPOSED_PRESET` is never parsed; duplicate paths and passes do not add compilation.

- [ ] **Step 9: Trigger warmup from the single catalog-ready path**

In the existing manifest filename selection block, assign the backend ID beside the filename: `gles` for `HAS_GLES`, `gl` for `HAS_GL`, and `d3d11-fx` for the existing HLSL `#else`. In `CDialogGameVideoFilter::InitVideoFilters()`, collect each absolute `ListItem.Property(game.videofilter)` only after current-backend XML parsing and `VideoShaders().CanLoadPreset(path)` filtering. After the list is complete, call:

```cpp
CServiceBroker::GetGameServices().VideoShaders().WarmupPresets(
    shaderBackendId, std::move(presetPaths));
```

This extends the dialog's existing three-way manifest `#if`, rather than adding a new backend-specific control-flow block. On GL/GLES, `SupportsAsyncCompilation()` returns before preset parsing.

- [ ] **Step 10: Remove the raw-dialog Get More callback and polling loop**

Replace the current worker capture:

```cpp
[this] { OnGetMoreComplete(...); }
```

with a job that captures only immutable add-on ID/install data and invokes install/enable APIs. Subscribe the dialog while loaded to the factory's loader-snapshot event; the event handler posts `GUI_MSG_REFRESH_LIST` to the known dialog/control ID and never touches `m_items`, `m_regenerateList`, or controls off the GUI thread. `OnMessage()` handles that refresh on the GUI thread, sets regeneration state, and delegates to the base. Unsubscribe in `OnWindowUnload()` and the destructor. Delete `OnGetMoreComplete()` and every 50 ms polling/sleep path.

- [ ] **Step 11: Run dialog/warmup/lifetime tests green**

Run:

```powershell
cmake --build build --config Debug --target kodi-test -- /m
build\Debug\kodi-test.exe --gtest_filter=TestShaderWarmup.*:TestShaderPresetLoaderRegistry.*:TestShaderCompileService.*:TestShaderPresetAsync.*
```

Expected: all selected tests pass; no raw dialog capture exists; the process exits normally.

- [ ] **Step 12: Run the complete shader regression suite and a full Debug Kodi build**

If Visual Studio holds build outputs, close only `devenv.exe` as the user authorized, then run:

```powershell
cmake --build build --config Debug --target kodi-test -- /m
build\Debug\kodi-test.exe --gtest_filter=*Shader*
cmake --build build --config Debug --target kodi -- /m
```

Expected: all shader tests pass and the Debug `kodi` target succeeds.

- [ ] **Step 13: Inspect Task 2 for manifest scope and lifetime defects**

Run:

```powershell
git diff --check
git diff --stat HEAD
git status --short
rg -n "GetDirectory|recursive|resources.*hlsl|Sleep\(|\[this\]" xbmc/games/dialogs/osd/DialogGameVideoFilter.* xbmc/cores/RetroPlayer/shaders xbmc/addons/ShaderPreset.*
rg -n "WarmupPresets|Video shader warmup|LoadPreset" xbmc/games/dialogs/osd/DialogGameVideoFilter.* xbmc/cores/RetroPlayer/shaders xbmc/addons/ShaderPreset.*
```

Confirm warmup receives only filtered catalog paths; the install and preinstalled flows both call the same `InitVideoFilters()` submission; no recursive resource scan exists; identical active catalogs coalesce; later opens revalidate; loader invocation holds shared lifetime and binary serialization; no job captures a dialog, renderer, or factory raw pointer.

- [ ] **Step 14: Commit Task 2 with the approved message**

Stage only Task 2 files, verify the staged diff, and commit:

```text
[games] Precompile video shaders from the preset manifest

Warm only the preset paths exposed by the active Video Filter catalog and
deduplicate their referenced passes through the runtime shader cache.

Use the same catalog-ready path after add-on installation, synchronize loader
snapshots for background parsing, and remove dialog-lifetime polling
callbacks.
```

### Task 3: Windows Build and Real RetroPlayer Qualification

**Files:**
- Inspect: `build/Debug/kodi.exe`
- Inspect: the portable Kodi profile/cache used by the existing shader runtime harness
- Inspect: `kodi.log`, warmup summaries, screenshots, and process exit state
- Temporarily modify and restore: one deployed exposed HLSL source or loaded include for invalidation only
- Do not commit: runtime profiles, cache entries, logs, screenshots, temporary counters, or deployed source edits

**Interfaces:**
- Consumes: Both implementation commits and the deployed local `game.shader.presets` add-on.
- Produces: Measured cold/reopen/cross-game/restart/invalidation counts and evidence for the final report; no source commit.

- [ ] **Step 1: Verify repository identity and clean implementation state**

Run:

```powershell
git branch --show-current
git log -3 --format=fuller
git status --short --branch
git -C C:\Users\garrett\Documents\game.shader.presets branch --show-current
git -C C:\Users\garrett\Documents\game.shader.presets status --short --branch
```

Expected: Kodi is on `retroplayer-22beta2` with the design and two new implementation commits; shader add-on is on `retroplayer-piers` and unchanged; no source edit is pending.

- [ ] **Step 2: Run fresh automated verification immediately before runtime work**

Run:

```powershell
cmake --build build --config Debug --target kodi-test -- /m
build\Debug\kodi-test.exe --gtest_filter=TestShaderCompileService.*:TestShaderArtifactStore.*:TestShaderCompilerDX.*:TestD3DEffectBytecode.*:TestShaderPresetAsync.*:TestShaderPresetLoaderRegistry.*:TestShaderWarmup.*:TestShaderVertexDX.*:TestRPWinOutputShaderDX.*:TestShaderDX.*:TestShaderUtilsDX.*
cmake --build build --config Debug --target kodi -- /m
```

Expected: all selected tests and the full Debug build pass from the committed tree.

- [ ] **Step 3: Cold-cache manifest warmup and aggressive-scroll test**

Close Kodi, resolve the active portable/profile mapping for `special://temp`, and delete only the new `retroplayer/shaders/dx11/v1` cache directory after verifying its absolute resolved path remains under that profile's cache directory. Start Debug Kodi with the existing real RetroPlayer game harness, open the Video Filter dialog, and scroll aggressively before warmup completes.

Record the exact integer following each named field in the DEBUG summary and the worker compilation counter retained for qualification:

```text
manifest presets = summary field "presets"
referenced passes = summary field "passes"
unique compile units = summary field "unique"
deduplicated passes = passes - unique
disk hits = summary field "disk"
source compilations = worker compile counter delta for this run
```

Expected: dialog interaction remains responsive; one summary appears; no non-catalog resource scan occurs; each unique miss compiles once on workers; no shader/effect/layout/resource/lifetime errors occur.

- [ ] **Step 4: Pending-selection activation test**

During a fresh cold warmup, focus/select a pass known not yet terminal. Capture the exact selected absolute path, log state, and full-screen image before and after completion.

Expected: selection call returns without a compile stall; video remains unfiltered rather than black/corrupt; path is not logged as permanently failed; completion wakes the current generation; the shader activates on the render thread without restarting Kodi.

- [ ] **Step 5: Same-process reopen and switching test**

After cold warmup completes, close/reopen the dialog and switch among a single-pass, multipass, LUT, and recently added Windows preset.

Expected: the later catalog revalidates, reports memory hits, queues no source compilation for unchanged units, and each tested image/log remains acceptable. Record `warm reopen source compilations = 0`.

- [ ] **Step 6: Cross-game/core reuse test**

Stop the first game, launch a different available game/core, open the same filter catalog, and select already compiled presets.

Expected: shared passes use memory artifacts and source compilation count remains unchanged. Record `cross-game source compilations = 0` for already compiled units.

- [ ] **Step 7: Normal restart persistent-cache test**

Exit Kodi normally, confirm process termination, restart it, launch a game, and open the dialog.

Expected: unchanged units validate from disk, create new per-instance Effects11 objects, and invoke no source compilation. Record `restart source compilations = 0` and the disk-hit count.

- [ ] **Step 8: Content invalidation and restoration test**

Choose one exposed shader or one transitive include with a narrow dependency set. Save its exact original bytes and repository status, make a temporary one-byte semantic-neutral content change, reopen/restart as needed, then restore the original bytes before any Git operation.

Expected: affected content keys miss and compile; unrelated units remain hits; restoring exact bytes returns to the original key/artifact behavior; both Kodi and shader repository end clean. Do not stage or commit the temporary edit.

- [ ] **Step 9: Full regression log and shutdown inspection**

For cold, pending, reopen, cross-game, restart, and invalidation evidence, scan complete logs for:

```text
shader compilation error
effect creation failure
input-layout error
resource/LUT/texture error
cache corruption/write error
thread/lifetime assertion
unhandled exception or crash
```

Expected: no unexplained signature, every run uses the intended exact active filter path, images are recognizable/acceptable, and bounded shutdown is normal. Any real crash or failed test returns to systematic debugging before completion claims.

- [ ] **Step 10: Final whole-branch verification and status capture**

Run:

```powershell
git diff --check HEAD~2..HEAD
git show --stat --oneline HEAD~1
git show --stat --oneline HEAD
git status --short --branch
git -C C:\Users\garrett\Documents\game.shader.presets status --short --branch
```

Expected: exactly the intended two implementation commits follow the design commit, Kodi and shader add-on worktrees are clean, no temporary runtime edits remain, and nothing was pushed.
