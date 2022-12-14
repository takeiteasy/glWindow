#include "glWindow.h"
#include <stdio.h>  // printf
#include <string.h> // strcat

#if defined(GL_WIN_WINDOWS) && !defined(_CRT_SECURE_NO_WARNINGS)
#define _CRT_SECURE_NO_WARNINGS
#endif

#if defined(GL_WIN_MAC) && !defined(GL_SILENCE_DEPRECATION)
#define GL_SILENCE_DEPRECATION
#endif

#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif
#ifndef GLAPI
#define GLAPI extern
#endif

typedef float GLfloat;
typedef unsigned int GLbitfield;
#define GL_COLOR_BUFFER_BIT 0x00004000

typedef void (APIENTRYP PFNGLCLEARPROC)(GLbitfield mask);
#define glClear __glClear
typedef void (APIENTRYP PFNGLCLEARCOLORPROC)(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
#define glClearColor __glClearColor

#define GL_FUNCTIONS           \
    X(PFNGLCLEARPROC, glClear) \
    X(PFNGLCLEARCOLORPROC, glClearColor)
#define X(T, N) extern T __##N;
GL_FUNCTIONS
#undef X
#define X(T, N) T __##N = (T)((void*)0);
GL_FUNCTIONS
#undef X

#if defined(_WIN32) || defined(__CYGWIN__)
#ifndef _WINDOWS_
#undef APIENTRY
#endif
#include <windows.h>
typedef void* (APIENTRYP PFNWGLGETPROCADDRESSPROC_PRIVATE)(const char*);
static PFNWGLGETPROCADDRESSPROC_PRIVATE glLoaderGetProcAddressPtr;
static HMODULE libGL = NULL;

#ifdef _MSC_VER
#ifdef __has_include
#if __has_include(<winapifamily.h>)
#define HAVE_WINAPIFAMILY 1
#endif
#elif _MSC_VER >= 1700 && !_USING_V110_SDK71_
#define HAVE_WINAPIFAMILY 1
#endif
#endif

#ifdef HAVE_WINAPIFAMILY
#include <winapifamily.h>
#if !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) && WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP)
#define IS_UWP 1
#endif
#endif

static int LoadGLLibrary(void) {
#ifndef IS_UWP
    if ((libGL = LoadLibraryW(L"opengl32.dll"))) {
        void(*tmp)(void);
        tmp = (void(*)(void)) GetProcAddress(libGL, "wglGetProcAddress");
        glLoaderGetProcAddressPtr = (PFNWGLGETPROCADDRESSPROC_PRIVATE)tmp;
        return glLoaderGetProcAddressPtr != NULL;
    }
#endif
    return 0;
}

static void CloseGLLibrary(void) {
    if (libGL) {
        FreeLibrary((HMODULE)libGL);
        libGL = NULL;
    }
}
#else
#include <dlfcn.h>
#if !defined(__APPLE__) && !defined(__HAIKU__)
typedef void* (APIENTRYP PFNGLXGETPROCADDRESSPROC_PRIVATE)(const char*);
static PFNGLXGETPROCADDRESSPROC_PRIVATE glLoaderGetProcAddressPtr;
#endif
static void* libGL = NULL;

static int LoadGLLibrary(void) {
#ifdef __APPLE__
    static const char *NAMES[] = {
        "../Frameworks/OpenGL.framework/OpenGL",
        "/Library/Frameworks/OpenGL.framework/OpenGL",
        "/System/Library/Frameworks/OpenGL.framework/OpenGL",
        "/System/Library/Frameworks/OpenGL.framework/Versions/Current/OpenGL"
    };
#else
    static const char *NAMES[] = {"libGL.so.1", "libGL.so"};
#endif
    
    unsigned int index = 0;
    for (index = 0; index < (sizeof(NAMES) / sizeof(NAMES[0])); index++) {
        if ((libGL = dlopen(NAMES[index], RTLD_NOW | RTLD_GLOBAL))) {
#if defined(__APPLE__) || defined(__HAIKU__)
            return 1;
#else
            glLoaderGetProcAddressPtr = (PFNGLXGETPROCADDRESSPROC_PRIVATE)dlsym(libGL,
                "glXGetProcAddressARB");
            return glLoaderGetProcAddressPtr != NULL;
#endif
        }
    }
    return 0;
}

static void CloseGLLibrary(void) {
    if (libGL) {
        dlclose(libGL);
        libGL = NULL;
    }
}
#endif

static void* LoadGLProc(const char *namez) {
    void* result = NULL;
    if (libGL == NULL)
        return NULL;

#if !defined(__APPLE__) && !defined(__HAIKU__)
    if (glLoaderGetProcAddressPtr)
        result = glLoaderGetProcAddressPtr(namez);
#endif
    if (!result) {
#if defined(_WIN32) || defined(__CYGWIN__)
        result = (void*)GetProcAddress((HMODULE)libGL, namez);
#else
        result = dlsym(libGL, namez);
#endif
    }

    return result;
}

#define X(T, N)                       \
    if (!(__##N = (T)LoadGLProc(#N))) \
        failures++;
static int failures = 0;
static int InitOpenGL(void) {
    int result = 0;
    if (LoadGLLibrary()) {
        GL_FUNCTIONS
        result = failures == 0;
        CloseGLLibrary();
    }
    return result;
}
#undef X

static const char* ModifierString(GLmod modifier) {
    static char ret[512];
    ret[0] = '\0';
    char *or = "";
    
#define MODS     \
    X(SHIFT)     \
    X(CONTROL)   \
    X(ALT)       \
    X(SUPER)     \
    X(CAPS_LOCK) \
    X(NUM_LOCK)
#define X(N)                      \
    if (modifier & KEY_MOD_##N) { \
        strcat(ret, or);          \
        strcat(ret, #N);          \
        or = ", ";                \
    }
    MODS
#undef X
    return ret;
}

static void KeyboardCallback(void *userdata, GLkey key, GLmod modifier, int isDown) {
    const char *modidierString = ModifierString(modifier);
    printf("KEYBOARD: Key %d is %s, modifier: %s\n", (int)key, isDown ? "down" : "up", strlen(modidierString) ? modidierString : "None");
}

static void MouseButtonCallback(void *userdata, int button, GLmod modifier, int isDown) {
    const char *modidierString = ModifierString(modifier);
    printf("MOUSE BUTTON: Button %d is %s, modifier: %s\n", button, isDown ? "down" : "up", strlen(modidierString) ? modidierString : "None");
}

static void MouseMoveCallback(void *userdata, int x, int y, float dx, float dy) {
    printf("MOUSE MOVED: Mouse position is %dx%d, moved by %fx%f\n", x, y, dx, dy);
}

static void MouseScrollCallback(void *userdata, float dx, float dy, GLmod modifier) {
    const char *modidierString = ModifierString(modifier);
    printf("MOUSE SCROLL: Scroll delta: %fx%f, modifier: %s\n", dx, dy, strlen(modidierString) ? modidierString : "None");
}

static void ResizedCallback(void *userdata, int w, int h) {
    printf("WINDOW RESIZED: Window size is %dx%d\n", w, h);
}

static void FocusCallback(void *userdata, int isFocused) {
    printf("WINDOW FOCUS: Window is %s\n", isFocused ? "focused" : "blurred");
}

static void ClosedCallback(void *userdata) {
    printf("WINDOW CLOSED! Goodbye...\n");
}

int main(int argc, const char *argv[]) {
    if (!InitOpenGL())
        return 1;
    if (!glWindow(640, 480, "glWindow", glResizable))
        return 1;
#define X(NAME, _) NAME##Callback,
    glWindowCallbacks(GL_WIN_CALLBACKS NULL);
#undef X
    
    while (glPollWindow()) {
        glClearColor(1.f, 0.f, 0.f, 1.f);
        glClear(GL_COLOR_BUFFER_BIT);
        glFlushWindow();
    }
    glWindowQuit();
    return 0;
}
