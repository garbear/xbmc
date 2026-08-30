# RetroPlayer asynchronous shader compilation and cache design

Date: 2026-08-29
Status: Approved for implementation

## Summary

RetroPlayer currently creates a video shader preset synchronously on the render
path. On Windows, this reaches `CShaderDX::Create()`, which asks `CD3DEffect` to
compile HLSL/FX source and create an Effects11 runtime object in one operation.
Focusing filters in the Video Filter dialog changes the active render settings,
so the resulting source compilation is observable as scrolling stutter.

The selected design separates immutable CPU-side compilation from mutable
render-instance creation. A Game Services-owned compile service prepares and
compiles individual shader passes on Kodi jobs, deduplicates work by effective
content, and stores Windows FX bytecode in a persistent content-addressed cache.
The render thread later creates an independent Effects11 object from that
bytecode and performs the existing GPU-resource setup.

The same service warms exactly the presets exposed by the current Video Filter
catalog. GL and GLES retain their existing synchronous behavior while gaining a
small backend interface through which equivalent support can be added later.

## Goals

- Remove HLSL source compilation from filter focus and render interaction.
- Cache individual shader passes rather than whole presets.
- Reuse compiled passes across presets, games, cores, dialog openings, and Kodi
  processes.
- Deduplicate concurrent requests, including requests from different paths that
  resolve to identical effective compilation input.
- Invalidate on source or transitive include content changes, not timestamps.
- Keep Effects11 objects and every mutable rendering resource per instance.
- Warm only presets exposed by the active backend's manifest.
- Keep a selected-but-not-ready preset pending without blocking the GUI or
  permanently marking it failed.
- Preserve GL and GLES behavior and expose an explicit future extension point.

## Non-goals

- Persistent GL or GLES program binaries.
- Secondary or shared GL contexts.
- Driver-specific cache keys or GPU-driver binary artifacts.
- Whole-preset caching, a cache database, or an LRU policy.
- Changes to shader source or to the `game.shader.presets` catalog unless a
  concrete compatibility defect is found during qualification.
- Seamless visual blending between the old and new filter while compilation is
  pending.

## Existing flow and constraints

`CDialogGameVideoFilter::InitVideoFilters()` locates and parses the current
backend's XML catalog. Focusing an entry writes the selected path to game
settings. The render manager observes the new settings and normally creates a
renderer for them. On its first render, `CRPBaseRenderer::Updateshaders()` calls
`CShaderPreset::SetShaderPreset()`.

`CShaderPreset::SetShaderPreset()` currently performs all of these operations
synchronously:

1. Parse the preset through `CShaderPresetFactory` and the shader add-on.
2. Create every backend shader.
3. Create input layouts and vertex buffers.
4. Create constant buffers and samplers.
5. Create intermediate textures on the following update.

For DX, `CShaderPresetDX::CreateShaders()` constructs one `CShaderDX` per pass.
`CShaderDX::Create()` supplies the `HLSL_4`, `HLSL_FX`, and
`PARAMETER_UNIFORM` defines, adds the shader file's directory to the include
search, and calls `CD3DEffect::Create()`. `CD3DEffect` currently invokes
`D3DX11CompileEffectFromMemory()`, which combines source compilation and effect
creation.

Effects11 11.29 implements that convenience function as `D3DCompile()` with
target `fx_5_0`, followed by effect loading and binding to the supplied D3D11
device. It also exposes `D3DX11CreateEffectFromMemory()`, which creates a new
effect from compiled bytecode. This gives the required safe split:

```text
source + includes + defines
        |
        | worker thread, CPU only
        v
immutable FX bytecode
        |
        | render thread, per shader instance
        v
ID3DX11Effect + layouts + buffers + resources
```

`ID3DX11Effect` cannot be shared between unrelated shaders because resource
bindings, variables, selected techniques, pass state, and parameter values are
mutable. Only immutable bytecode is shared.

## Component design

### Preset data

Preset parsing will produce a backend-neutral data object containing the
existing `ShaderPass` vector. `IShaderPresetLoader` and
`CShaderPresetAddon::TranslateShaderPreset()` will populate this data object
instead of requiring a live renderer-facing `IShaderPreset` merely as a pass
sink.

`CShaderPreset::ReadPresetFile()` will load the same data and move its passes
into the preset instance. Background warmup can use the same factory and loader
without constructing a render context or GPU objects. This refactor changes no
parsing semantics for DX, GL, or GLES.

Loader registration and lookup will be synchronized. Loader snapshots will use
shared ownership so an add-on disable or reinstall cannot destroy a loader
while a background preset parse is using it. The add-on wrapper will serialize
calls into its binary instance. Add-on events can publish a new loader snapshot
without racing UI, render, or warmup reads.

### Generic compile service

`CShaderCompileService` will live for the same duration as
`CShaderPresetFactory`, which is owned by `CGameServices`. It therefore outlives
dialogs, individual games, renderers, and cores. The factory exposes the compile
service to backend preset implementations and owns manifest warmup entry
points.

The generic API contains:

- `ShaderCompileState`: unknown, queued, compiling, ready, or failed.
- An immutable backend compile request and diagnostic context.
- A shared compile handle for state, result, error text, and completion
  notification.
- An immutable compiled artifact represented as bytes plus its content key and
  provenance.
- `IShaderCompiler`, which prepares a backend request, derives its effective
  content key, and compiles a prepared unit.
- Request-group support for a preset containing multiple passes.
- A persistent artifact store used only by backends that opt in.
- Asynchronous warmup of a supplied set of preset paths.

The generic service does not know HLSL syntax or graphics APIs. Backend-specific
request and prepared data remain immutable compiler-owned values. Common state,
deduplication, cache publication, diagnostics, and completion behavior remain
generic.

On GL and GLES builds no asynchronous compiler is registered initially.
Requests report unsupported and those backends continue through their existing
synchronous creation path. This keeps the later extension point explicit
without adding current GL behavior or driver assumptions.

### Job ownership and lifetime

Warmup enumeration uses a single low-priority job. Missing compile units use a
bounded FIFO `CJobQueue` with at most two low-priority workers. Low-pausable jobs
will not be used because Kodi pauses them during active playback, precisely when
the Video Filter dialog is used.

The service owns shared internal state. Jobs own only shared control blocks,
immutable compiler inputs, and a shared compiler implementation; they never
capture a raw dialog, renderer, factory, or service pointer. Shutdown marks the
state as stopping and cancels queued work. An already-running job may finish,
but it can then update only its retained control block and cannot call a
destroyed owner.

Completion listeners execute outside service locks. The renderer listener only
sets a generation-checked atomic wake token. It never performs graphics work on
a worker thread.

## Request and deduplication flow

The final content key cannot be known until HLSL includes and preprocessing are
resolved. A request therefore has two identities:

1. A provisional request identity used only to merge equivalent work while
   preparation is in flight.
2. The canonical SHA-256 compile key derived from effective compilation input.

The provisional identity includes root source contents, the include base,
backend options, and compiler settings. It is removed after the request becomes
terminal. This is important: retaining it permanently would hide an include
file edit whose root shader source did not change.

Preparation also records a dependency transcript. It contains the raw root
source and the raw bytes returned by every include-resolution call, in
resolution order, with explicit length framing. Resolved filesystem paths are
retained for diagnostics and change validation but are not part of a successful
content key. This makes comment-only and inactive-branch edits inside a loaded
dependency invalidate the cache while allowing identical content stored at
different locations to deduplicate.

After preparation, the job performs a second locked lookup by canonical key.
If another request already owns that key, the later handle resolves to the
existing entry and does not compile or publish another artifact. Consequently,
two different paths with identical effective code may preprocess separately in
a race, but only one `D3DCompile()` invocation can occur.

The canonical map remains for the process lifetime. Ready entries provide
in-memory reuse. Failed entries also remain so unchanged bad input is not
continuously requeued. A source or transitive include content change generates
a different canonical key and is eligible for a new request.

A preprocessing failure has no successful canonical key. It instead receives a
preparation-failure fingerprint derived from the compiler settings, raw root
source, include requests, successfully resolved dependency bytes, missing-file
status, and error result. A later request must resolve its dependencies again so
that a repaired include can recover, but an unchanged result joins the existing
failed entry, is not passed to `D3DCompile()`, and is not logged again. Successful
source-compilation failures use the normal canonical key and require no such
special handling.

State transitions are:

```text
unknown -> queued -> compiling -> ready
                            \----> failed

queued -> ready  (validated persistent-cache hit)
queued -> failed (preparation or cache-independent validation failure)

ready(disk, generation N) -> queued(generation N+1) -> compiling
                          -> ready or failed  (one cache-rejection retry)
```

No code waits synchronously for these transitions on the GUI or render thread.

## Windows compiler

The DX compiler will use a short-lived include resolver that reproduces
`CD3DEffect`'s current behavior:

- Start with `special://xbmc/system/shaders/`.
- Add the source shader's base directory.
- Search the ordered include-path set currently used by `CD3DEffect`.
- Load through Kodi VFS.
- Add the base directory of each resolved include for subsequent nested
  includes.
- Preserve the existing treatment of parent include data.

Preparation calls `D3DPreprocess()` with an empty source name, the same define
set, and that resolver. The preprocessed output captures conditional include and
macro semantics without a second hand-written HLSL parser. The include resolver
simultaneously records the raw dependency transcript so changes discarded by
preprocessing, such as comments, still produce a miss.

The canonical key is SHA-256 over explicitly length-framed fields in a fixed
encoding:

- Kodi shader-cache ABI version.
- Backend identity (`d3d11-fx`).
- Compiler identity (`d3dcompiler_47`).
- Effects11 version (`1129`).
- Target (`fx_5_0`).
- HLSL and FX compiler flags.
- The ordered define names and values.
- The length-framed raw root source.
- The length-framed raw contents of each resolved include, in resolution order.
- The complete preprocessed source bytes.

Lengths and integers are serialized explicitly; native structs, `size_t`, naked
string concatenation, and `std::hash` are not used.

Compilation calls `D3DCompile()` on the prepared source with entry point `""`,
target `fx_5_0`, and the exact flags currently passed through Effects11:

- Debug builds: zero, matching current behavior.
- Non-debug builds:
  `D3DCOMPILE_ENABLE_BACKWARDS_COMPATIBILITY |
  D3DCOMPILE_OPTIMIZATION_LEVEL3`.
- FX flags: zero.

Preprocessor and compiler error blobs are retained. Diagnostics include the
preset path when known, pass index, alias, shader path, and compiler message.

## Persistent cache

Windows artifacts are stored under:

```text
special://temp/retroplayer/shaders/dx11/v1/<sha256>.fxc
```

On Windows, `special://temp` maps to Kodi's application cache, including in
portable mode. It survives normal Kodi restarts and is shared across games. Kodi
startup clears its archive-cache child, not unrelated cache children.

The filename contains only a validated lowercase SHA-256 value, so shader names
and paths cannot introduce traversal. Each file contains fields serialized in a
fixed wire format:

- Magic.
- Format version.
- 32-byte compile key.
- Fixed-width payload length.
- 32-byte SHA-256 payload digest.
- FX bytecode payload.

Loading requires exact magic, version, key, length, total file size, and payload
digest matches before an artifact becomes ready. Entries larger than 64 MiB are
rejected before payload allocation. Empty, truncated, oversized, or inconsistent
entries are deleted best-effort and treated as misses.

Publishing uses a unique sibling temporary filename in the same directory:

1. Create the cache directory recursively.
2. Write the complete envelope, handling short writes.
3. Flush and close the temporary file.
4. Rename it to the content-addressed final name.
5. Delete the temporary file on every failure path.

Within one process, canonical-key deduplication permits one writer. Across Kodi
processes, immutable writers may race. If rename loses because a final entry
already exists, the loser validates that entry and discards its temporary file.
The final file is never truncated or updated in place.

A validated disk artifact records disk provenance. If
`D3DX11CreateEffectFromMemory()` rejects it, the cache entry is invalidated and
the service may recompile it once. Invalidation is performed under the canonical
entry lock: one caller increments the entry generation, marks the disk retry as
spent, removes the disk artifact, clears the shared artifact, and transitions
the existing handle back to queued. Other callers attach to that generation
instead of starting another compile. Request groups re-arm their completion
notification, return the preset to pending, and wake the render thread after the
new generation is terminal. Freshly compiled artifacts and repeated failures do
not enter an invalidation loop. A missing `TEQ` technique is a valid compiled
result with an unsupported shader contract, so it remains an ordinary terminal
preset failure rather than a cache-corruption retry.

The cache ABI is incremented whenever the wire format, key framing or fields,
define/include semantics, compiler target or flags, or bytecode consumer
contract changes.

## Render-time consumption and pending presets

`CD3DEffect` gains a bytecode creation path alongside its existing source path,
because non-RetroPlayer callers may still require synchronous source creation.
The new path calls `D3DX11CreateEffectFromMemory()` on the current D3D device and
stores shared immutable bytecode for device restoration. Every `CD3DEffect`
still owns its own `ID3DX11Effect`, selected technique, current pass, and mutable
bindings.

`CShaderPresetDX::CreateShaders()` requests one compile handle per pass. It does
not create LUTs, effects, layouts, buffers, samplers, or textures until every
pass is ready. When ready, it constructs a distinct `CShaderDX` and
`CD3DEffect` from each artifact, then continues the existing render-thread GPU
setup.

Preset activation becomes tri-state:

- Ready: all passes and GPU resources were created successfully.
- Pending: at least one compile unit is queued or compiling.
- Failed: parsing, compilation, effect contract, or resource creation failed.

`CShaderPreset` retains parsed pass data while pending and does not add the path
to `m_failedPaths`. A genuinely failed preset remains terminal under the
existing policy.

While pending, the newly selected renderer follows the simplest safe fallback:
it renders the game without the video shader. Its requested preset path remains
associated with the renderer, preventing repeated renderer allocation.

The request group invokes a completion callback only after all passes are
terminal. That callback locks a weak generation token and sets one atomic wake
flag. The next render-thread update consumes the flag and retries preset
realization. A stale completion from an older selection cannot wake a newer
generation, and destroying a renderer drops the last strong token rather than
leaving a callback into freed state.

If render-time creation rejects a disk artifact and the service accepts its
single retry, the preset remains pending on the same generation-aware request
group. It is not entered into `m_failedPaths` unless the refreshed compile or
subsequent render-thread realization fails terminally.

The steady pending cost is a single atomic check; there is no repeated preset
parse, filesystem scan, service lookup, compiler request, or GPU call per frame.

## Manifest-driven warmup

`CDialogGameVideoFilter::InitVideoFilters()` remains the single catalog
availability path. After it parses the active backend's manifest and removes
entries that `CShaderPresetFactory::CanLoadPreset()` cannot load, it supplies
the exact exposed preset paths to the factory's warmup entry point.

The warmup entry point first checks whether the active backend registered an
asynchronous compiler. GL and GLES return immediately before parsing any preset,
leaving their runtime and add-on behavior unchanged.

This produces the same trigger in both cases:

- If `game.shader.presets` is already installed, initial dialog population
  submits the catalog.
- After "Get more..." installs or enables it, the existing dialog refresh runs
  `InitVideoFilters()` again and submits the newly available catalog.

The warmup job:

1. Parses only the supplied preset paths through the existing loader.
2. Collects the passes referenced by those presets.
3. Lets the active backend prepare canonical compile units.
4. Deduplicates them by canonical key.
5. Loads validated memory or disk hits.
6. Queues only unique misses.

It never recursively enumerates the add-on resource tree.

A hash of the submitted path set coalesces identical warmups while one is
active. The signature is removed when enumeration completes, allowing a later
dialog opening to revalidate content changes while still producing only memory
or disk hits for unchanged shaders.

The existing "Get more..." worker's raw `[this]` capture will be removed while
this flow is touched. Installation work will own only immutable install data.
Loader availability will be published through the synchronized factory state,
and GUI refresh will be posted by window/control ID. No install or warmup job
will dereference a dialog object, and closing the dialog cannot leave a worker
callback into freed dialog state.

One DEBUG summary is emitted after preparation, for example:

```text
Video shader warmup: 45 presets, 87 passes, 53 unique, 30 memory, 20 disk, 3 queued, 0 failed
```

Normal per-frame or per-hit logging is not added. Compiler failures retain
specific diagnostic context.

## Error handling

- Preset parse failure is logged against the preset and does not abort unrelated
  warmup entries.
- Preprocess failure records the preparation-failure fingerprint, while compile
  failure marks the canonical unit failed for the session.
- A pending preset is never added to permanent failed-path state.
- Cache read errors are misses; corrupt entries are removed best-effort.
- Cache write errors do not discard a successfully compiled in-memory artifact.
- Effect creation from a disk artifact receives one cache-eviction/recompile
  opportunity.
- Technique, input-layout, buffer, sampler, LUT, or texture failures remain
  render-thread preset failures with existing cleanup behavior.
- Service shutdown cancels queued jobs and prevents late callbacks into owners.

## Automated tests

Generic tests use a deterministic fake compiler, shared control gates, Kodi's
job manager, and an injected temporary cache directory. Windows integration
tests use WARP and Effects11 where a device is required.

Required coverage:

1. Identical requests deduplicate.
2. Concurrent requests produce one compiler invocation.
3. One pass referenced by multiple presets maps to one compile unit.
4. Root source-content change produces a miss.
5. Transitive include-content change produces a miss.
6. Unchanged effective content produces a hit.
7. Pending and failed states remain distinguishable.
8. An unchanged failed item is not continuously requeued.
9. A persistent hit avoids `D3DCompile()`.
10. A corrupt persistent entry is rejected and safely recompiled.
11. Warmup processes only supplied exposed presets and their referenced passes.
12. Repeated concurrent catalog warmup is idempotent.

Additional Windows tests verify:

- `D3DCompile()` bytecode creates an effect through
  `D3DX11CreateEffectFromMemory()`.
- Two effects from one artifact do not share mutable parameter state.
- Named `TEQ` reflection and pass-zero input-layout creation still work.
- Device recreation uses retained bytecode without source compilation.
- A rejected disk artifact is retried at most once.
- Defines and nested includes produce equivalent effects through the old
  one-step Effects11 path and the new preprocess/compile/load path.
- A stale selection completion cannot activate or wake a newer selection.
- Renderer destruction before completion leaves no callback into freed state.
- Compile-service shutdown with queued and active work is lifetime-safe.

Tests will avoid unbounded sleeps. Worker tests use manual-reset events and
bounded waits only as failure timeouts.

## Runtime qualification

After the Windows Debug build succeeds, qualification will use a real
RetroPlayer game and the deployed local preset add-on.

- Cold cache: verify manifest-only enumeration, pass deduplication, responsive
  scrolling, sensible summary counts, and one compile per unique miss.
- Pending selection: select unfinished work, verify unfiltered rendering,
  terminal activation without restart, and no permanent failure mark.
- Same process: reopen the dialog and switch filters with zero source
  recompilations.
- Cross-game: launch another game/core and reuse ready passes with zero source
  recompilations.
- Persistent cache: restart Kodi and verify validated bytecode hits with zero
  source recompilations.
- Invalidation: temporarily change one source or include, verify only affected
  keys miss, then restore without committing the edit.
- Regression: exercise a single-pass filter, multipass filter, LUT filter, and a
  recently added Windows preset; inspect the complete log for compiler, effect,
  layout, resource, cache, and lifetime errors.

Temporary counters may be used during development for preset, pass, unique-key,
deduplication, disk-hit, and compile counts. Only the useful DEBUG summary and
actionable failures remain in committed code.

## Commit structure

Existing branch history will not be reset, rebased, amended, squashed, dropped,
or otherwise rewritten. Nothing will be pushed.

1. Design note: document this approved architecture.
2. `[games] Cache compiled RetroPlayer shaders on Windows`
   - Generic compile state, handles, cache, and backend interface.
   - Windows preparation/compiler and persistent FX bytecode store.
   - Runtime bytecode consumption, independent effect instances, pending
     activation, and automated tests.
3. `[games] Precompile video shaders from the preset manifest`
   - Manifest-only warmup, dialog integration, catalog accounting,
     idempotence, loader-snapshot safety, and automated tests.

Implementation commit bodies will describe behavior and rationale, wrap at 76
columns, and avoid development-history narration.
