#ifndef MAIN_MACOS_H__
#define MAIN_MACOS_H__

/*
 * macOS entry point for Minecraft PE
 *
 * Uses SDL2 for windowing/input and macOS's native OpenGL (legacy compat, 2.1)
 * instead of EGL/GLES which is not available on macOS.
 *
 * Build with: -DMACOS -DPOSIX
 * Dependencies: SDL2, libpng, zlib (all available via Homebrew)
 */

// sdl2-config --cflags adds -I.../SDL2, so headers are accessed as <SDL.h>
#include <SDL.h>
#include <unistd.h>
#include <cstdio>
#include <string>

#include "App.h"
#include "platform/input/Mouse.h"
#include "platform/input/Multitouch.h"
#include "platform/input/Keyboard.h"
#include "AppPlatform_macos.h"

static int   g_width  = 854;
static int   g_height = 480;
static bool  g_running = true;
static int   g_mouseX = 0;
static int   g_mouseY = 0;

static SDL_Window*   g_sdlWindow  = NULL;
static SDL_GLContext g_glContext   = NULL;

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
                // Feed printable characters for text screens
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
                // Use last known mouse position to avoid corrupting mouse coords
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
            default:
                break;
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Couldn't initialize SDL: %s\n", SDL_GetError());
        return -1;
    }

    // Request a legacy/compatibility OpenGL 2.1 context so the fixed-function
    // pipeline (glMatrixMode, glVertexPointer, glBegin, etc.) is available.
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
        "Minecraft PE (macOS)",
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

    // glInit() is a no-op on MACOS (skips glewInit)
    glInit();

    // Change working directory to the binary's location so relative asset
    // paths like "data/images/" resolve correctly.
    std::string binPath = argv[0];
    size_t slash = binPath.rfind('/');
    if (slash != std::string::npos) {
        binPath = binPath.substr(0, slash);
        chdir(binPath.c_str());
    }

    MAIN_CLASS* app = new MAIN_CLASS();

    // Store saves in ~/Library/Application Support/minecraft/
    const char* home = getenv("HOME");
    std::string storagePath = home
        ? std::string(home) + "/Library/Application Support/minecraft/"
        : ".minecraft/";
    app->externalStoragePath      = storagePath;
    app->externalCacheStoragePath = storagePath;

    AppContext    context;
    AppPlatform_macos platform;
    context.doRender  = true;
    context.platform  = &platform;

    // Call through base App* to avoid name hiding by NinecraftApp::init()
    static_cast<App*>(app)->init(context);
    app->setSize(g_width, g_height);

    while (g_running && !app->wantToQuit()) {
        if (handleEvents(app) != 0)
            break;

        app->update();

        // On macOS with NO_EGL, App::swapBuffers() is a no-op, so we swap here.
        SDL_GL_SwapWindow(g_sdlWindow);
    }

    delete app;
    context.platform->finish();

    SDL_GL_DeleteContext(g_glContext);
    SDL_DestroyWindow(g_sdlWindow);
    SDL_Quit();

    return 0;
}

#endif /* MAIN_MACOS_H__ */
