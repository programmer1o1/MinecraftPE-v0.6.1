# Minecraft PE v0.6.1

Leaked source code of very early **Minecraft Pocket Edition** (v0.6.1, circa 2012).

The codebase originally targeted **Android**, **iOS**, **Win32**, and **Raspberry Pi**. A **macOS port** (arm64 / Apple Silicon) has been added on top of the Raspberry Pi target, using SDL2 for windowing and input.

---

## Platforms

| Platform | Status | Notes |
|---|---|---|
| Android | Original | NDK build system in `handheld/project/android/` |
| iOS | Original | Xcode project in `handheld/project/ios/` |
| Win32 | Original | Visual Studio project in `handheld/project/win32/` |
| Raspberry Pi | Original | Makefile in `handheld/project/rpi/` |
| macOS (arm64) | Added | SDL2 + OpenGL 2.1 + OpenAL, Makefile in `handheld/project/macos/` |

---

## macOS Build

### Dependencies

Install via [Homebrew](https://brew.sh):

```
brew install sdl2 libpng zlib
```

System frameworks used directly (no extra install needed):
- `OpenGL.framework` — legacy compatibility profile (2.1)
- `OpenAL.framework` — audio playback
- `Foundation.framework` — file utilities

### Building

```
cd handheld/project/macos
make -j8
```

Run the binary from that directory so relative asset paths (`data/`) resolve correctly:

```
./minecraftpe
```

Clean rebuild:

```
make clean && make -j8
```

---

## Controls (macOS / desktop)

| Key / Action | Function |
|---|---|
| W A S D | Move |
| Space | Jump |
| Left Shift | Sneak |
| Left click | Mine / Attack mob |
| Right click | Place block / Interact |
| 1 – 9 | Select hotbar slot |
| Scroll wheel | Scroll hotbar |
| E | Open inventory |
| F | Toggle fly (Creative) |
| T | Toggle third-person view |
| Escape | Pause / back |

The mouse is captured on entering a world. Move the mouse to look around.

---

## Save location (macOS)

```
~/Library/Application Support/minecraft/
```

---

## macOS port — files changed

| File | Change |
|---|---|
| `handheld/src/main_macos.h` | SDL2 entry point — window creation, event loop, input feeding |
| `handheld/src/AppPlatform_macos.h` | Platform abstraction for file I/O, screen size |
| `handheld/project/macos/Makefile` | Build system for macOS |
| `handheld/src/client/Minecraft.cpp` | Input path, key bindings, `_supportsNonTouchscreen` init ordering |
| `handheld/src/client/Options.cpp` | WASD/Space/Shift bindings, `useMouseForDigging`, `viewDistance` |
| `handheld/src/client/MouseHandler.cpp` | SDL2 `SDL_SetRelativeMouseMode` for mouse capture |
| `handheld/src/client/gui/screens/ScreenChooser.cpp` | Always use touchscreen-style UI screens on macOS |
| `handheld/src/client/player/input/MouseBuildInput.h` | First-click vs hold distinction for attack / block mine |
| `handheld/src/platform/input/Mouse.cpp` | Position update restricted to move events (fixes scroll-wheel camera jump) |
| `handheld/src/client/sound/Sound.h/cpp` | Enabled embedded PCM sound data on macOS |
| `handheld/src/client/sound/SoundEngine.cpp` | Enabled `sounds.add()` registrations on macOS |
| `handheld/src/client/sound/SoundRepository.h` | Deduplicated "sound not found" log spam |
| `handheld/src/platform/audio/SoundSystemAL.cpp` | Removed `sound.destroy()` call (PCM arrays are static, not heap) |

---

## Known issues

From `docs/bugs-handheld.txt` — status of each upstream bug:

| Issue | Status |
|---|---|
| Frustum culler not 100% correct for entities | **Fixed** — entity BBs are expanded by 0.5 units before frustum test to prevent edge-of-screen pop-out |
| Lighting has minor inaccuracies | Open — inherent to the reconstruction; no specific fix known |
| Static memory leaks in renderer singletons | **Fixed** — `ItemRenderer`, `EntityTileRenderer`, `TileEntityRenderDispatcher` are now cleaned up on all non-Android platforms |
| Water horizon ignores head-bob offset | Open — the world-space water horizon plane is not implemented in this PE reconstruction; the underwater screen overlay (`renderWater`) is disabled due to missing `misc/water.png` asset |
