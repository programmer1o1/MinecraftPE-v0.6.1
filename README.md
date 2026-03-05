# Minecraft PE v0.6.1

A leaked source code of **Minecraft Pocket Edition v0.6.1**. Now ported to Mac!

**NOT FOR COMMERCIAL PURPOSES.**

---

## Platforms

| Platform | Status | Build system | Notes |
|---|---|---|---|
| Android | Original + Modernized | NDK (`handheld/project/android/`) | armeabi-v7a / arm64-v8a, immersive fullscreen, modern multitouch |
| iOS | Original | Xcode (`handheld/project/ios/`) | Requires code signing |
| Win32 | Original | Visual Studio (`handheld/project/win32/`) | MSVC, OpenGL ES emulator (libs bundled) |
| Raspberry Pi | Original | Makefile (`handheld/project/rpi/`) | GLES 1.x, SDL 1.2 |
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

Open `handheld/project/win32/minecraft.sln` in **Visual Studio** (2010 or newer recommended).

Dependencies expected by the project (install via [vcpkg](https://vcpkg.io) or manually):
- [SDL2](https://libsdl.org) — windowing and input
- [GLEW](https://glew.sourceforge.net) — OpenGL extension loading
- [OpenAL Soft](https://openal-soft.org) — audio

Build the `minecraft` configuration in Release or Debug mode.

---

### Raspberry Pi

Uses SDL 1.2 and GLES 1.x via the Broadcom VideoCore headers (`/opt/vc/`).

```
cd handheld/project/rpi
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

## Android modernizations

The Android port has been updated for modern devices (Android 10-14+):

- **Immersive fullscreen** — navigation bar and status bar are hidden using `SYSTEM_UI_FLAG_IMMERSIVE_STICKY`. Automatically restored on focus regain. No more UI elements hidden behind system bars.
- **Correct GUI scaling** — screen dimensions account for system UI insets so touch targets and UI elements scale properly on all screen sizes.
- **Unified multitouch system** — touch input feeds exclusively through `Multitouch`. Legacy `Mouse` is auto-mirrored from pointer 0 via `Multitouch::feed()` so GUI code (menus, sliders, scrolling lists) works transparently.
- **Accurate touch positions** — `MouseDevice::feed()` updates position on all event types (DOWN, UP, MOVE), not just MOVE. Fixes stale coordinates when tapping after holding.
- **Secondary finger tap support** — `Mouse::feedEventOnly()` adds secondary finger taps to the Mouse event queue without corrupting button state, so tapping inventory while holding movement works correctly.
- **Gesture cancellation** — `AMOTION_EVENT_ACTION_CANCEL` releases all pointers to prevent phantom stuck touches on dialog open or focus loss.
- **Storage permissions** — runtime permission request for `WRITE_EXTERNAL_STORAGE` (API 23-29) and `MANAGE_EXTERNAL_STORAGE` (API 30+). Manifest includes `requestLegacyExternalStorage` for Android 10.
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

## Known issues

| Issue | Status |
|---|---|
| Entity frustum culling edge pop-out | Fixed — entity BBs expanded by 0.5 before frustum test |
| Renderer singleton memory leaks | Fixed — cleanup enabled on all non-Android platforms |
| Lighting inaccuracies | Open — inherent to the reconstruction |
| Water horizon ignores head-bob offset | Open — underwater overlay disabled due to missing asset |

---

## CI

GitHub Actions CI builds four targets: macOS, Linux, Android (NDK r16b), and dedicated server. See `.github/workflows/ci.yml`.
