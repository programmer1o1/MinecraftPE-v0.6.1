# Minecraft PE v0.6.1

Leaked source code of very early **Minecraft Pocket Edition** (v0.6.1, circa 2012).

The codebase originally targeted **Android**, **iOS**, **Win32**, and **Raspberry Pi**. Community ports for **macOS** and **Linux** have been added on top of the Raspberry Pi target, using SDL2 for windowing and input.

---

## Platforms

| Platform | Status | Build system | Notes |
|---|---|---|---|
| Android | Original | NDK (`handheld/project/android/`) | armeabi-v7a, NDK r16b |
| iOS | Original | Xcode (`handheld/project/ios/`) | Requires code signing |
| Win32 | Original | Visual Studio (`handheld/project/win32/`) | MSVC, OpenGL ES emulator (libs bundled) |
| Raspberry Pi | Original | Makefile (`handheld/project/rpi/`) | GLES 1.x, SDL 1.2 |
| macOS (arm64) | Added | Makefile (`handheld/project/macos/`) | SDL2 + OpenGL 2.1 + OpenAL |
| Linux (x86_64) | Added | Makefile (`handheld/project/linux/`) | SDL2 + OpenGL 2.1 + OpenAL |

---

## Building

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

Open `handheld/project/win32/minecraft.sln` in **Visual Studio** (2010 or newer recommended).

Dependencies expected by the project (install via [vcpkg](https://vcpkg.io) or manually):
- [SDL2](https://libsdl.org) — windowing and input
- [GLEW](https://glew.sourceforge.net) — OpenGL extension loading
- [OpenAL Soft](https://openal-soft.org) — audio

Build the `minecraft` configuration in Release or Debug mode.

---

### Android

Requires **NDK r16b** (last release with `stlport_static`).

```
cd handheld/project/android
$NDK_ROOT/ndk-build -j4 NDK_PROJECT_PATH=. APP_BUILD_SCRIPT=jni/Android.mk
```

---

### Raspberry Pi

Uses SDL 1.2 and GLES 1.x via the Broadcom VideoCore headers (`/opt/vc/`).

```
cd handheld/project/rpi
make -j4
./minecraftpe
```

Requires the legacy GL driver (`sudo raspi-config` → Advanced → GL Driver → Legacy).

---

## Controls (desktop: macOS / Linux / Windows)

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

## Save locations

| Platform | Path |
|---|---|
| macOS | `~/Library/Application Support/minecraft/` |
| Linux | `~/.local/share/minecraft/` (or `$XDG_DATA_HOME/minecraft/`) |
| Windows | `%APPDATA%\minecraft\` |
| Android | `/sdcard/minecraft/` |

---

## Port notes (macOS / Linux)

These ports share the same SDL2-based input and rendering path. Key implementation files:

| File | Purpose |
|---|---|
| `handheld/src/main_macos.h` | macOS SDL2 entry point — window, event loop, input |
| `handheld/src/main_linux.h` | Linux SDL2 entry point — window, event loop, input |
| `handheld/src/AppPlatform_macos.h` | macOS platform abstraction (libpng texture loading) |
| `handheld/src/AppPlatform_linux.h` | Linux platform abstraction (libpng texture loading) |
| `handheld/project/macos/Makefile` | macOS build system |
| `handheld/project/linux/Makefile` | Linux build system |
| `handheld/src/world/level/storage/MoveFolder_posix.cpp` | POSIX `rename()` replacement for the macOS ObjC `MoveFolder.mm` |
| `handheld/src/client/renderer/gles.h` | OpenGL platform dispatch (GLES → iOS/Android/RPI; desktop GL → macOS/Linux/Win32) |
| `handheld/src/client/Minecraft.cpp` | Input path, key bindings, `_supportsNonTouchscreen` init ordering |
| `handheld/src/client/Options.cpp` | WASD/Space/Shift bindings, `useMouseForDigging`, `viewDistance` |
| `handheld/src/client/MouseHandler.cpp` | SDL2 `SDL_SetRelativeMouseMode` for mouse capture |
| `handheld/src/client/gui/screens/ScreenChooser.cpp` | Forces touchscreen-style UI on desktop (better look) |
| `handheld/src/client/player/input/MouseBuildInput.h` | First-click vs hold for attack / block mine |
| `handheld/src/platform/input/Mouse.cpp` | Position update restricted to move events (fixes scroll-wheel camera jump) |
| `handheld/src/client/sound/Sound.h/cpp` | Embedded PCM sound data enabled on desktop |
| `handheld/src/client/sound/SoundEngine.cpp` | `sounds.add()` registrations enabled on desktop |
| `handheld/src/platform/audio/SoundSystemAL.cpp` | Removed `sound.destroy()` (PCM arrays are static, not heap) |

---

## Known issues

| Issue | Status |
|---|---|
| Frustum culler not 100% correct for entities | **Fixed** — entity BBs expanded by 0.5 units before frustum test to prevent edge-of-screen pop-out |
| Lighting has minor inaccuracies | Open — inherent to the reconstruction |
| Static memory leaks in renderer singletons | **Fixed** — `ItemRenderer`, `EntityTileRenderer`, `TileEntityRenderDispatcher` cleaned up on all non-Android platforms |
| Water horizon ignores head-bob offset | Open — underwater overlay (`renderWater`) disabled due to missing `misc/water.png` asset |
