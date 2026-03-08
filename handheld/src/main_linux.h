#ifndef MAIN_LINUX_H__
#define MAIN_LINUX_H__

/*
 * Linux entry point for Minecraft PE
 *
 * Uses SDL2 for windowing/input and desktop OpenGL (legacy compat, 2.1).
 *
 * Build with: -DLINUX -DPOSIX
 * Dependencies: libsdl2-dev libpng-dev zlib1g-dev libopenal-dev libgl1-mesa-dev
 *   sudo apt-get install libsdl2-dev libpng-dev zlib1g-dev libopenal-dev libgl1-mesa-dev
 */

#include <SDL.h>
#include <unistd.h>
#include <cstdio>
#include <string>

#include "App.h"
#include "platform/input/Mouse.h"
#include "platform/input/Multitouch.h"
#include "platform/input/Keyboard.h"
#include "AppPlatform_linux.h"

static int   g_width  = 854;
static int   g_height = 480;
static bool  g_running = true;
static int   g_mouseX = 0;
static int   g_mouseY = 0;

static SDL_Window*   g_sdlWindow  = NULL;
static SDL_GLContext g_glContext   = NULL;

// ----------------------------------------------------------------------------
// Gamepad state
// ----------------------------------------------------------------------------
static SDL_GameController* g_controller = NULL;

// Deadzone threshold (SDL axis range: -32768 to 32767)
static const int CONTROLLER_DEADZONE = 8000;

// Right stick axis values — updated from SDL_CONTROLLERAXISMOTION events,
// consumed every frame in the main loop to produce smooth camera movement.
static float g_rightStickX = 0.0f;
static float g_rightStickY = 0.0f;

// Sensitivity multiplier for the right stick → mouse-look mapping.
// Tune this value to taste (higher = faster camera).
static const float STICK_LOOK_SCALE = 8.0f;

// Track which virtual keys are currently held down by the gamepad so we can
// release them cleanly when the button/stick is released.
static bool g_padW = false, g_padA = false, g_padS = false, g_padD = false;

// Track trigger press state to generate click down/up pairs.
static bool g_triggerLeft  = false; // left trigger  → left click  (mine/attack)
static bool g_triggerRight = false; // right trigger → right click (place/use)

// Apply deadzone and normalise an axis value to [-1, 1].
// Returns 0 when the axis is within the dead zone.
static float applyDeadzone(Sint16 raw) {
    if (raw >  CONTROLLER_DEADZONE) return  (raw - CONTROLLER_DEADZONE) / (float)(32767 - CONTROLLER_DEADZONE);
    if (raw < -CONTROLLER_DEADZONE) return  (raw + CONTROLLER_DEADZONE) / (float)(32767 - CONTROLLER_DEADZONE);
    return 0.0f;
}

// Press or release a game key only when the state actually changes.
static void setGameKey(unsigned char key, bool& tracked, bool pressed) {
    if (pressed == tracked) return;
    tracked = pressed;
    Keyboard::feed(key, pressed ? 1 : 0);
}

// Map SDL2 keycodes to the game's internal key codes (matching main_rpi.h mapping)
static unsigned char transformKey(SDL_Keycode key) {
    switch (key) {
        case SDLK_LSHIFT:  return Keyboard::KEY_LSHIFT;
        case SDLK_DOWN:    return 40;
        case SDLK_UP:      return 38;
        case SDLK_SPACE:   return Keyboard::KEY_SPACE;
        case SDLK_RETURN:  return 13;
        case SDLK_ESCAPE:  return Keyboard::KEY_ESCAPE;
        case SDLK_TAB:     return 250;
        default:           break;
    }
    if (key >= 'a' && key <= 'z') return (unsigned char)(key - 32);
    if (key >= '0' && key <= '9') return (unsigned char)key;
    return 0;
}

static int handleEvents(App* app) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                return -1;

            case SDL_KEYDOWN: {
                unsigned char k = transformKey(event.key.keysym.sym);
                if (k) Keyboard::feed(k, 1);
                break;
            }
            case SDL_KEYUP: {
                unsigned char k = transformKey(event.key.keysym.sym);
                if (k) Keyboard::feed(k, 0);
                break;
            }
            case SDL_TEXTINPUT: {
                for (const char* p = event.text.text; *p; ++p)
                    if ((unsigned char)*p >= 32)
                        Keyboard::feedText((unsigned char)*p);
                break;
            }
            case SDL_MOUSEBUTTONDOWN: {
                bool left = (event.button.button == SDL_BUTTON_LEFT);
                char btn  = left ? 1 : 2;
                Mouse::feed(btn, 1, event.button.x, event.button.y);
                Multitouch::feed(btn, 1, event.button.x, event.button.y, 0);
                break;
            }
            case SDL_MOUSEBUTTONUP: {
                bool left = (event.button.button == SDL_BUTTON_LEFT);
                char btn  = left ? 1 : 2;
                Mouse::feed(btn, 0, event.button.x, event.button.y);
                Multitouch::feed(btn, 0, event.button.x, event.button.y, 0);
                break;
            }
            case SDL_MOUSEWHEEL: {
                Mouse::feed(3, (event.wheel.y != 0) ? 1 : 0, g_mouseX, g_mouseY, 0, event.wheel.y);
                break;
            }
            case SDL_MOUSEMOTION: {
                float x = (float)event.motion.x;
                float y = (float)event.motion.y;
                g_mouseX = event.motion.x;
                g_mouseY = event.motion.y;
                Mouse::feed(0, 0, x, y, event.motion.xrel, event.motion.yrel);
                Multitouch::feed(0, 0, x, y, 0);
                break;
            }
            case SDL_WINDOWEVENT: {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                    event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    g_width  = event.window.data1;
                    g_height = event.window.data2;
                    if (app) app->setSize(g_width, g_height);
                }
                break;
            }

            // ----------------------------------------------------------------
            // Gamepad device connect / disconnect
            // ----------------------------------------------------------------
            case SDL_CONTROLLERDEVICEADDED: {
                if (!g_controller) {
                    g_controller = SDL_GameControllerOpen(event.cdevice.which);
                    if (g_controller)
                        printf("[gamepad] Controller connected: %s\n",
                               SDL_GameControllerName(g_controller));
                }
                break;
            }
            case SDL_CONTROLLERDEVICEREMOVED: {
                if (g_controller) {
                    SDL_GameController* removed =
                        SDL_GameControllerFromInstanceID(event.cdevice.which);
                    if (removed == g_controller) {
                        printf("[gamepad] Controller disconnected.\n");
                        SDL_GameControllerClose(g_controller);
                        g_controller = NULL;
                        // Release any gamepad-held keys/buttons
                        if (g_padW) { Keyboard::feed(Keyboard::KEY_W, 0); g_padW = false; }
                        if (g_padA) { Keyboard::feed(Keyboard::KEY_A, 0); g_padA = false; }
                        if (g_padS) { Keyboard::feed(Keyboard::KEY_S, 0); g_padS = false; }
                        if (g_padD) { Keyboard::feed(Keyboard::KEY_D, 0); g_padD = false; }
                        if (g_triggerLeft)  { Mouse::feed(1, 0, g_mouseX, g_mouseY); g_triggerLeft  = false; }
                        if (g_triggerRight) { Mouse::feed(2, 0, g_mouseX, g_mouseY); g_triggerRight = false; }
                        g_rightStickX = g_rightStickY = 0.0f;
                    }
                }
                break;
            }

            // ----------------------------------------------------------------
            // Gamepad button press / release
            // ----------------------------------------------------------------
            case SDL_CONTROLLERBUTTONDOWN:
            case SDL_CONTROLLERBUTTONUP: {
                bool down = (event.type == SDL_CONTROLLERBUTTONDOWN);
                int  val  = down ? 1 : 0;
                switch (event.cbutton.button) {
                    // Face buttons
                    case SDL_CONTROLLER_BUTTON_A:
                        Keyboard::feed(Keyboard::KEY_SPACE, val);      // jump
                        break;
                    case SDL_CONTROLLER_BUTTON_B:
                        Keyboard::feed(Keyboard::KEY_ESCAPE, val);     // pause / back
                        break;
                    case SDL_CONTROLLER_BUTTON_X:
                        Mouse::feed(1, (char)val, g_mouseX, g_mouseY); // left click (mine)
                        break;
                    case SDL_CONTROLLER_BUTTON_Y:
                        Mouse::feed(2, (char)val, g_mouseX, g_mouseY); // right click (place)
                        break;

                    // Shoulder buttons — hotbar scroll (fire once on press)
                    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
                        if (down)
                            Mouse::feed(3, 1, g_mouseX, g_mouseY, 0,  1); // scroll left
                        break;
                    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
                        if (down)
                            Mouse::feed(3, 1, g_mouseX, g_mouseY, 0, -1); // scroll right
                        break;

                    // D-pad
                    case SDL_CONTROLLER_BUTTON_DPAD_UP:
                        Keyboard::feed(Keyboard::KEY_W, val);
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                        Keyboard::feed(Keyboard::KEY_S, val);
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
                        Keyboard::feed(Keyboard::KEY_A, val);
                        break;
                    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
                        Keyboard::feed(Keyboard::KEY_D, val);
                        break;

                    // Menu buttons
                    case SDL_CONTROLLER_BUTTON_START:
                        Keyboard::feed(Keyboard::KEY_ESCAPE, val);     // pause
                        break;
                    case SDL_CONTROLLER_BUTTON_BACK:
                        Keyboard::feed(Keyboard::KEY_E, val);          // inventory
                        break;

                    default:
                        break;
                }
                break;
            }

            // ----------------------------------------------------------------
            // Gamepad axis motion
            // ----------------------------------------------------------------
            case SDL_CONTROLLERAXISMOTION: {
                Sint16 raw = event.caxis.value;
                float  v   = applyDeadzone(raw);

                switch (event.caxis.axis) {
                    // Left stick X → A/D (strafe)
                    case SDL_CONTROLLER_AXIS_LEFTX:
                        setGameKey(Keyboard::KEY_A, g_padA, v < -0.3f);
                        setGameKey(Keyboard::KEY_D, g_padD, v >  0.3f);
                        break;

                    // Left stick Y → W/S (forward/back) — axis positive = down
                    case SDL_CONTROLLER_AXIS_LEFTY:
                        setGameKey(Keyboard::KEY_W, g_padW, v < -0.3f);
                        setGameKey(Keyboard::KEY_S, g_padS, v >  0.3f);
                        break;

                    // Right stick — accumulate for smooth per-frame camera input
                    case SDL_CONTROLLER_AXIS_RIGHTX:
                        g_rightStickX = v;
                        break;
                    case SDL_CONTROLLER_AXIS_RIGHTY:
                        g_rightStickY = v;
                        break;

                    // Left trigger (axis 4) → left click (mine/attack)
                    case SDL_CONTROLLER_AXIS_TRIGGERLEFT: {
                        // Trigger range: 0..32767; treat >25% as pressed
                        bool pressed = (raw > 8192);
                        if (pressed != g_triggerLeft) {
                            g_triggerLeft = pressed;
                            Mouse::feed(1, (char)(pressed ? 1 : 0), g_mouseX, g_mouseY);
                        }
                        break;
                    }

                    // Right trigger (axis 5) → right click (place/use)
                    case SDL_CONTROLLER_AXIS_TRIGGERRIGHT: {
                        bool pressed = (raw > 8192);
                        if (pressed != g_triggerRight) {
                            g_triggerRight = pressed;
                            Mouse::feed(2, (char)(pressed ? 1 : 0), g_mouseX, g_mouseY);
                        }
                        break;
                    }

                    default:
                        break;
                }
                break;
            }

            default:
                break;
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) < 0) {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        return -1;
    }

    // Request a legacy/compatibility OpenGL 2.1 context so the fixed-function
    // pipeline (glMatrixMode, glVertexPointer, etc.) is available.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   16);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE,     8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,   8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,    8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,   8);

    g_sdlWindow = SDL_CreateWindow(
        "Minecraft PE (Linux)",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        g_width, g_height,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN
    );
    if (!g_sdlWindow) {
        printf("Couldn't create SDL window: %s\n", SDL_GetError());
        SDL_Quit();
        return -2;
    }

    g_glContext = SDL_GL_CreateContext(g_sdlWindow);
    if (!g_glContext) {
        printf("Couldn't create OpenGL context: %s\n", SDL_GetError());
        SDL_DestroyWindow(g_sdlWindow);
        SDL_Quit();
        return -3;
    }

    SDL_GL_SetSwapInterval(1); // vsync

    // glInit() is a no-op on LINUX (skips glewInit)
    glInit();

    // Change working directory to the binary's location so relative asset
    // paths like "data/images/" resolve correctly.
    std::string binPath = argv[0];
    size_t slash = binPath.rfind('/');
    if (slash != std::string::npos) {
        binPath = binPath.substr(0, slash);
        chdir(binPath.c_str());
    }

    // Open any already-connected controllers (plugged in before launch)
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i) && !g_controller) {
            g_controller = SDL_GameControllerOpen(i);
            if (g_controller)
                printf("[gamepad] Controller opened at startup: %s\n",
                       SDL_GameControllerName(g_controller));
        }
    }

    MAIN_CLASS* app = new MAIN_CLASS();

    // Store saves in ~/.local/share/minecraft/ (XDG base dir standard)
    const char* xdgData = getenv("XDG_DATA_HOME");
    std::string storagePath;
    if (xdgData) {
        storagePath = std::string(xdgData) + "/minecraft/";
    } else {
        const char* home = getenv("HOME");
        storagePath = home
            ? std::string(home) + "/.local/share/minecraft/"
            : ".minecraft/";
    }
    app->externalStoragePath      = storagePath;
    app->externalCacheStoragePath = storagePath;

    AppContext    context;
    AppPlatform_linux platform;
    context.doRender  = true;
    context.platform  = &platform;

    // Call through base App* to avoid name hiding by NinecraftApp::init()
    static_cast<App*>(app)->init(context);
    app->setSize(g_width, g_height);

    while (g_running && !app->wantToQuit()) {
        if (handleEvents(app) != 0)
            break;

        // Right stick → per-frame camera look (smooth, continuous)
        if (g_controller && (g_rightStickX != 0.0f || g_rightStickY != 0.0f)) {
            short dx = (short)(g_rightStickX * STICK_LOOK_SCALE);
            short dy = (short)(g_rightStickY * STICK_LOOK_SCALE);
            if (dx != 0 || dy != 0) {
                // Feed as a mouse-move event with relative deltas only;
                // keep the reported absolute position at the screen centre so
                // the crosshair stays fixed while the controller is in use.
                short cx = (short)(g_width  / 2);
                short cy = (short)(g_height / 2);
                Mouse::feed(0, 0, cx, cy, dx, dy);
            }
        }

        app->update();

        // On Linux with NO_EGL, App::swapBuffers() is a no-op, so we swap here.
        SDL_GL_SwapWindow(g_sdlWindow);
    }

    delete app;
    context.platform->finish();

    if (g_controller) {
        SDL_GameControllerClose(g_controller);
        g_controller = NULL;
    }

    SDL_GL_DeleteContext(g_glContext);
    SDL_DestroyWindow(g_sdlWindow);
    SDL_Quit();

    return 0;
}

#endif /* MAIN_LINUX_H__ */
