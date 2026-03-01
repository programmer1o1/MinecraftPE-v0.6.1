# Minecraft PE v0.6.1

An leaked source code of very early **Minecraft Pocket Edition** (0.6.1 version, circa 2012).

This repository includes a **macOS port** (arm64 / Apple Silicon) built on top of the Raspberry Pi target, using SDL2 for windowing and input, and the system OpenGL/OpenAL frameworks.

---

## Dependencies

Install via [Homebrew](https://brew.sh):

```
brew install sdl2 libpng zlib
```

The following system frameworks are used directly (no extra install needed):
- `OpenGL.framework` — legacy compatibility profile (2.1)
- `OpenAL.framework` — audio playback
- `Foundation.framework` — file utilities

---

## Building

```
cd handheld/project/macos
make -j8
```

The output binary is `handheld/project/macos/minecraftpe`. Run it from that directory so relative asset paths (`data/`) resolve correctly:

```
cd handheld/project/macos
./minecraftpe
```

To do a clean rebuild:

```
make clean && make -j8
```

---

## Controls

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

## Save location

Worlds and options are stored in:

```
~/Library/Application Support/minecraft/
```

---

## macOS port — files changed

The port targets macOS (arm64) with `clang++`. The following source files were added or modified to enable macOS support:

| File | Change |
|---|---|
| `handheld/src/main_macos.h` | SDL2 entry point — window creation, event loop, input feeding |
| `handheld/src/AppPlatform_macos.h/cpp` | Platform abstraction for file I/O, screen size |
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

## Known issues (upstream)

From `docs/bugs-handheld.txt`:

- Frustum culler is not 100% correct for entities
- Lighting has minor inaccuracies
- A few small static memory leaks (marked `@memleak` in source)
- Water horizon ignores head-bob offset
