# Minecraft PE v0.6.1

A leaked source code of **Minecraft Pocket Edition v0.6.1**. Ported to macOS and Linux, with Android modernizations and infinite world support.

**NOT FOR COMMERCIAL PURPOSES.**

---

## Platforms

| Platform | Status | Build system | Notes |
|---|---|---|---|
| Android | Original + Modernized | NDK (`handheld/project/android/`) | armeabi-v7a / arm64-v8a, immersive fullscreen, modern multitouch |
| iOS | Original | Xcode (`handheld/project/iosproj/`) | Requires code signing |
| Win32 | Original | Visual Studio (`handheld/project/win32/`) | MSVC, OpenGL ES emulator (libs bundled) |
| Raspberry Pi | Original | Makefile (`handheld/project/raspberry/`) | GLES 1.x, SDL 1.2 |
| macOS (arm64) | Community port | Makefile (`handheld/project/macos/`) | SDL2 + OpenGL 2.1 + OpenAL |
| Linux (x86_64) | Community port | Makefile (`handheld/project/linux/`) | SDL2 + OpenGL 2.1 + OpenAL |

---

## Building

### Android

Requires **NDK r16b** (last release with `stlport_static`).

```
cd handheld/project/android
$NDK_ROOT/ndk-build -j4 NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=jni/Android.mk
```

The APK targets API 28 with `minSdkVersion` 9. Runtime storage permissions are requested automatically on API 23+. On API 30+ (Android 11+), the app requests "All Files Access" on first launch.

---

### macOS

**Dependencies** — install via [Homebrew](https://brew.sh):

```
brew install sdl2 libpng zlib
```

System frameworks (no extra install needed): `OpenGL`, `OpenAL`, `Foundation`.

```
cd handheld/project/macos
make -j8
./minecraftpe
```

---

### Linux

**Dependencies** — install via your package manager:

```
# Ubuntu / Debian
sudo apt-get install libsdl2-dev libpng-dev zlib1g-dev libopenal-dev libgl1-mesa-dev

# Fedora / RHEL
sudo dnf install SDL2-devel libpng-devel zlib-devel openal-soft-devel mesa-libGL-devel

# Arch
sudo pacman -S sdl2 libpng zlib openal mesa
```

```
cd handheld/project/linux
make -j$(nproc)
./minecraftpe
```

---

### Windows

Open `handheld/project/win32/MinecraftWin32.sln` in **Visual Studio** (2010 or newer recommended).

Dependencies expected by the project (install via [vcpkg](https://vcpkg.io) or manually):
- [SDL2](https://libsdl.org) — windowing and input
- [GLEW](https://glew.sourceforge.net) — OpenGL extension loading
- [OpenAL Soft](https://openal-soft.org) — audio

Build the `minecraft` configuration in Release or Debug mode.

---

### Raspberry Pi

Uses SDL 1.2 and GLES 1.x via the Broadcom VideoCore headers (`/opt/vc/`).

```
cd handheld/project/raspberry
make -j4
./minecraftpe
```

Requires the legacy GL driver (`sudo raspi-config` > Advanced > GL Driver > Legacy).

---

## Controls

### Desktop (macOS / Linux / Windows)

| Key / Action | Function |
|---|---|
| W A S D | Move |
| Space | Jump |
| Left Shift | Sneak |
| Left click | Mine / Attack |
| Right click | Place block / Interact |
| 1 - 9 | Select hotbar slot |
| Scroll wheel | Scroll hotbar |
| E | Open inventory |
| F | Toggle fly (Creative) |
| T | Toggle third-person view |
| Escape | Pause / back |

The mouse is captured on entering a world. Move the mouse to look around.

### Android

Standard Minecraft PE touchscreen controls: virtual d-pad for movement, swipe to look, tap to mine/place, on-screen buttons for jump and inventory.

---

## Save locations

| Platform | Path |
|---|---|
| macOS | `~/Library/Application Support/minecraft/` |
| Linux | `~/.local/share/minecraft/` (or `$XDG_DATA_HOME/minecraft/`) |
| Windows | `%APPDATA%\minecraft\` |
| Android | `/sdcard/games/minecraftpe/` |

---

## Infinite world fixes

`WorldType::Infinite`, `RandomLevelSource`, and all the selection UI existed in the original leaked source. The world type was always selectable at world creation — it just had several bugs that made it crash or behave incorrectly. These are bugfixes, not new features. All apply to every platform.

- **Missing faces at the old world border** — `Tile::shouldRenderFace` had hardcoded guards that suppressed faces at x/z = −1 and 256. Removed; finite worlds still suppress those faces correctly via `isSolidRender()` on `EmptyLevelChunk`.
- **Cannot interact with blocks beyond x/z = 255** — `Level::clip()` raycasting had a bounds guard clamped to `LEVEL_WIDTH`. Now bypassed for infinite worlds.
- **Player shaking when crossing x/z = 256** — `Level::tick(Entity*)` skipped the entity physics tick when `hasChunksAt()` returned false for any chunk within a 32-block radius. In infinite worlds those chunks simply hadn't been generated yet, causing alternating freeze/run frames. The check is now bypassed for infinite worlds where chunks are always generated on demand.
- **Player spawning underground or at wrong position** — `setInitialSpawn()` and `validateSpawn()` found a valid x/z but left ySpawn at 64, potentially inside terrain. Both now call `getTopTileY()` to place the spawn on the surface. Infinite worlds start the search at (8, 8) instead of (0, 0) to avoid chunk-corner physics edge cases.
- **Player position reset on world reload** — `readPlayerData` clamped x/z to `[0.5, 255.5]` unconditionally, snapping the player back to the old world boundary on every load. Clamping is now skipped for infinite worlds.
- **Multi-region chunk storage** — The original `ExternalFileLevelStorage` used a single `RegionFile` (one `chunks.dat`). Infinite worlds need one region file per 32×32 chunk area. A `regionFiles` map keyed on region coordinates now routes each chunk to its correct file.
- **Chunk boundary faces between new and existing chunks** — When a newly generated chunk is loaded next to an already-rendered chunk, the shared face wasn't rebuilt. `ChunkCache::getChunk` now calls `level->setTilesDirty()` after loading to mark the boundary dirty.

---

## Android modernizations

The Android port has been updated for modern devices (Android 10–14+):

- **Immersive fullscreen** — navigation bar and status bar are hidden using `SYSTEM_UI_FLAG_IMMERSIVE_STICKY`. Automatically restored on focus regain. No more UI elements hidden behind system bars.
- **Correct GUI scaling** — screen dimensions account for system UI insets so touch targets and UI elements scale properly on all screen sizes.
- **Unified multitouch system** — touch input feeds exclusively through `Multitouch`. Legacy `Mouse` is auto-mirrored from Multitouch pointer 0 via `Multitouch::feed()` so GUI code (menus, sliders, scrolling lists) works transparently.
- **Accurate touch positions** — `MouseDevice::feed()` updates position on all event types (DOWN, UP, MOVE), not just MOVE. Fixes stale coordinates when tapping after holding.
- **Secondary finger tap support** — `Mouse::feedEventOnly()` adds secondary finger taps to the Mouse event queue without corrupting button state, so tapping inventory while holding movement works correctly.
- **Gesture cancellation** — `AMOTION_EVENT_ACTION_CANCEL` releases all pointers to prevent phantom stuck touches on dialog open or focus loss.
- **Storage permissions** — runtime permission request for `WRITE_EXTERNAL_STORAGE` (API 23–29) and `MANAGE_EXTERNAL_STORAGE` (API 30+). Manifest includes `requestLegacyExternalStorage` for Android 10.
- **Thread safety** — detached threads (`PTHREAD_CREATE_DETACHED`) are no longer joined in `CThread` destructor, fixing SIGABRT on world generation.
- **Dialog crash fix** — removed `showSoftInput(getCurrentFocus())` in `onDialogCompleted` that caused NPE when focus was null after dialog dismiss.
- **Touchscreen detection** — `supportsTouchscreen()` returns true for all non-Xperia Play devices.

---

## Architecture notes

### Input system

The input system has two layers:

- **`Multitouch`** — tracks up to 12 independent touch pointers. Each pointer is a `TouchPointer` (typedef of `MouseDevice`). Used by `TouchscreenInput::tick()` for in-game controls (d-pad, look, jump, inventory).
- **`Mouse`** — single-pointer legacy system. On Android, auto-mirrored from Multitouch pointer 0. On desktop, fed directly from SDL2 events. Used by GUI code (`Screen::mouseEvent()`, `Slider`, `ScrollingPane`, `Gui::handleClick`).

On Android, `Multitouch::feed()` for pointer 0 calls `Mouse::feed()`. Button events from other pointers use `Mouse::feedEventOnly()` to add events to the queue without changing button state or position.

### Chunk storage (infinite worlds)

`ExternalFileLevelStorage` maintains a map of `RegionFile*` keyed on `(regionX, regionZ)`. Each region covers 32×32 chunks. `getOrOpenRegion(cx, cz)` computes the region coordinates and opens `r.<rx>.<rz>.mcr` on first access. This mirrors the region file layout used by Minecraft Java Edition alpha/beta.

### Platform abstraction

Each platform has:
- `main_<platform>.h` — entry point, window creation, event loop
- `AppPlatform_<platform>.h` — platform services (texture loading, screen size, paths)

### Preprocessor defines

| Define | Meaning |
|---|---|
| `ANDROID` | Android target |
| `MACOS` | macOS desktop (distinct from iOS, which also defines `__APPLE__`) |
| `LINUX` | Linux desktop |
| `NO_EGL` | Skip EGL calls (auto-defined for `__APPLE__` and `LINUX` in `App.h`) |
| `POSIX` | POSIX APIs available |

---

## Bug fixes

All fixes are in shared source and apply to every platform unless noted.

| Bug | Fix |
|---|---|
| Missing block faces at old world border (x/z = ±256) | Removed hardcoded face suppression guards in `Tile::shouldRenderFace` |
| Cannot mine/place blocks beyond x/z = 255 | `Level::clip()` raycasting guard now bypassed for infinite worlds |
| Player shakes when crossing x/z = 256 | `Level::tick(Entity*)` `hasChunksAt` check skipped for infinite worlds |
| Player spawns underground in infinite world | `setInitialSpawn` / `validateSpawn` now call `getTopTileY()` to find surface Y |
| Player teleports to x/z = 255 on world reload | `readPlayerData` position clamping skipped for infinite worlds |
| Chunk boundary faces missing between old and new chunks | `ChunkCache::getChunk` calls `setTilesDirty()` after loading a new chunk |
| Entity frustum culling pop-out at screen edges | Entity bounding boxes expanded by 0.5 before frustum test in `LevelRenderer` |
| Renderer singleton memory leaks on exit | Cleanup code enabled for all non-Android / non-server platforms in `NinecraftApp` |
| Heap buffer overflow on infinite world creation | Stale object files from header change; Makefile now uses `-MMD -MP` dependency tracking |

---

## CI

GitHub Actions CI builds four targets: macOS, Linux, Android (NDK r16b), and dedicated server. See `.github/workflows/ci.yml`.
