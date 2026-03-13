#ifndef NET_MINECRAFT_CLIENT_RENDERER__gles_H__
#define NET_MINECRAFT_CLIENT_RENDERER__gles_H__

#include "../../platform/log.h"
#include "../Options.h"

// Android and Raspberry Pi always run OPENGL_ES
#if defined(ANDROID) || defined(RPI)
    #define OPENGL_ES
#endif
// iOS runs OPENGL_ES; macOS (MACOS define) uses desktop OpenGL
#if defined(__APPLE__) && !defined(MACOS)
    #define OPENGL_ES
#endif

// Other systems might run it, if they #define OPENGL_ES
#if defined(OPENGL_ES) // || defined(ANDROID)
	#define USE_VBO
	#define GL_QUADS 0x0007
    #if defined(__APPLE__)
        // iOS: Use MetalANGLE GLES2 (Metal backend)
        // Block the system OpenGLES/ES1 headers — our macros would mangle their
        // function declarations.  We only need MetalANGLE's GLES2 headers.
        #define ES1_GL_H_GUARD
        #define ES1_GLEXT_H_GUARD
        #ifndef GL_GLES_PROTOTYPES
            #define GL_GLES_PROTOTYPES 1
        #endif
        #include <MetalANGLE/GLES2/gl2.h>
        #include <MetalANGLE/GLES2/gl2ext.h>

        // GLES2 doesn't have these constants — define them for code that checks state
        #ifndef GL_ALPHA_TEST
            #define GL_ALPHA_TEST           0x0BC0
        #endif
        #ifndef GL_FOG
            #define GL_FOG                  0x0B60
        #endif
        #ifndef GL_MODELVIEW
            #define GL_MODELVIEW            0x1700
        #endif
        #ifndef GL_PROJECTION
            #define GL_PROJECTION           0x1701
        #endif
        #ifndef GL_MODELVIEW_MATRIX
            #define GL_MODELVIEW_MATRIX     0x0BA6
        #endif
        #ifndef GL_PROJECTION_MATRIX
            #define GL_PROJECTION_MATRIX    0x0BA7
        #endif
        #ifndef GL_LIGHTING
            #define GL_LIGHTING             0x0B50
        #endif
        #ifndef GL_COLOR_MATERIAL
            #define GL_COLOR_MATERIAL       0x0B57
        #endif
        #ifndef GL_FLAT
            #define GL_FLAT                 0x1D00
        #endif
        #ifndef GL_SMOOTH
            #define GL_SMOOTH               0x1D01
        #endif
        #ifndef GL_MODULATE
            #define GL_MODULATE             0x2100
        #endif
        #ifndef GL_REPLACE
            #define GL_REPLACE              0x1E01
        #endif
        #ifndef GL_VERTEX_ARRAY
            #define GL_VERTEX_ARRAY         0x8074
        #endif
        #ifndef GL_TEXTURE_COORD_ARRAY
            #define GL_TEXTURE_COORD_ARRAY  0x8078
        #endif
        #ifndef GL_COLOR_ARRAY
            #define GL_COLOR_ARRAY          0x8076
        #endif
        #ifndef GL_NORMAL_ARRAY
            #define GL_NORMAL_ARRAY         0x8075
        #endif
        #ifndef GL_ALPHA_TEST_FUNC
            #define GL_ALPHA_TEST_FUNC      0x0BC1
        #endif
        #ifndef GL_ALPHA_TEST_REF
            #define GL_ALPHA_TEST_REF       0x0BC2
        #endif
        #ifndef GL_FOG_MODE
            #define GL_FOG_MODE             0x0B65
        #endif
        #ifndef GL_FOG_DENSITY
            #define GL_FOG_DENSITY          0x0B62
        #endif
        #ifndef GL_FOG_START
            #define GL_FOG_START            0x0B63
        #endif
        #ifndef GL_FOG_END
            #define GL_FOG_END              0x0B64
        #endif
        #ifndef GL_FOG_COLOR
            #define GL_FOG_COLOR            0x0B66
        #endif
        #ifndef GL_EXP
            #define GL_EXP                  0x0800
        #endif
        #ifndef GL_EXP2
            #define GL_EXP2                 0x0801
        #endif
        #ifndef GL_PERSPECTIVE_CORRECTION_HINT
            #define GL_PERSPECTIVE_CORRECTION_HINT 0x0C50
        #endif

        // Use software matrix stack for iOS GLES2
        #include "MatrixStack.h"

    #else
        #include <GLES/gl.h>
        #if defined(ANDROID)
            #include<GLES/glext.h>
        #endif
    #endif
#elif defined(MACOS)
    // macOS desktop OpenGL — legacy/compatibility profile (2.1)
    #include <OpenGL/gl.h>
    #include <OpenGL/glext.h>
    #define USE_VBO
    #define glFogx(a,b)             glFogi(a,b)
    #define glOrthof(a,b,c,d,e,f)   glOrtho(a,b,c,d,e,f)
    #define glClearDepthf(x)        glClearDepth(x)
    #define glDepthRangef(a,b)      glDepthRange(a,b)
#elif defined(LINUX)
    // Linux desktop OpenGL — legacy/compatibility profile (2.1)
    #include <GL/gl.h>
    #include <GL/glext.h>
    #define USE_VBO
    #define glFogx(a,b)             glFogi(a,b)
    #define glOrthof(a,b,c,d,e,f)   glOrtho(a,b,c,d,e,f)
    #define glClearDepthf(x)        glClearDepth(x)
    #define glDepthRangef(a,b)      glDepthRange(a,b)
#elif defined(STANDALONE_SERVER)
    // Dedicated server: no graphics context at all
    // (nothing to include)
#else
    // Uglyness to fix redeclaration issues
    #ifdef WIN32
	   #include <WinSock2.h>
	   #include <Windows.h>
	#endif
	#include <GL/glew.h>
	#include <GL/GL.h>

	#define USE_VBO
	#define glFogx(a,b)             glFogi(a,b)
	#define glOrthof(a,b,c,d,e,f)   glOrtho(a,b,c,d,e,f)
	#define glClearDepthf(x)        glClearDepth(x)
	#define glDepthRangef(a,b)      glDepthRange(a,b)
#endif


#ifdef STANDALONE_SERVER
// Server build: no OpenGL at all; provide a no-op GLERR so shared headers compile
#define GLERR(x) ((void)0)
#else // !STANDALONE_SERVER

#define GLERRDEBUG 1
#if GLERRDEBUG
#define GLERR(x) do { const int errCode = glGetError(); if (errCode != 0) LOGE("OpenGL ERROR @%d: #%d @ (%s : %d)\n", x, errCode, __FILE__, __LINE__); } while (0)
#else
#define GLERR(x) x
#endif

void anGenBuffers(GLsizei n, GLuint* buffer);

// ── GLSL shader support ──────────────────────────────────────────────────
// Now used on iOS (GLES2) AND desktop (macOS/Linux)
#if defined(MACOS) || defined(LINUX) || (defined(__APPLE__) && !defined(MACOS))

// Shader program & per-shader handles
extern GLuint g_shader_program;
extern GLuint g_vert_shader;
extern GLuint g_frag_shader;

// Cached uniform locations
extern GLint g_loc_mvp;
extern GLint g_loc_mv;
extern GLint g_loc_useTexture;
extern GLint g_loc_useAlphaTest;
extern GLint g_loc_alphaTestRef;
extern GLint g_loc_useFog;
extern GLint g_loc_fogStart;
extern GLint g_loc_fogEnd;
extern GLint g_loc_fogColor;
extern GLint g_loc_fogMode;
extern GLint g_loc_fogDensity;
extern GLint g_loc_tex;
extern GLint g_loc_useVertexColor;
extern GLint g_loc_uColor;

// Current GL state mirrored for uniform updates
extern bool  g_shaderEnabled;
extern bool  g_texture2DEnabled;
extern bool  g_alphaTestEnabled;
extern float g_alphaTestRef;
extern bool  g_fogEnabled;
extern float g_fogStart;
extern float g_fogEnd;
extern float g_fogColor[4];
extern int   g_fogMode;
extern float g_fogDensity;
extern float g_currentColor[4];

void shaderInit();
void shaderBind();
void shaderUnbind();
void shaderSyncUniforms();

// Shader-aware overrides for state management
void glEnable_shader(GLenum cap);
void glDisable_shader(GLenum cap);
void glAlphaFunc_shader(GLenum func, GLclampf ref);
void glColor4f_shader(float r, float g, float b, float a);
void glFogf_shader(GLenum pname, GLfloat param);
void glFogfv_shader(GLenum pname, const GLfloat* params);
void glFogx_shader(GLenum pname, GLint param);

#endif // shader platforms

// glAlphaFunc2 definition
#if defined(MACOS) || defined(LINUX)
#define glAlphaFunc2 glAlphaFunc_shader
#elif defined(__APPLE__) && !defined(MACOS)
// iOS GLES2: no glAlphaFunc in GLES2, route to shader
#define glAlphaFunc2(func, ref)  glAlphaFunc_shader(func, ref)
#define glAlphaFunc(func, ref)   glAlphaFunc_shader(func, ref)
#else
#define glAlphaFunc2 glAlphaFunc
#endif

#ifdef USE_VBO
#define drawArrayVT_NoState drawArrayVT
#define drawArrayVTC_NoState drawArrayVTC
void drawArrayVT(int bufferId, int vertices, int vertexSize = 24, unsigned int mode = GL_TRIANGLES);
#ifndef drawArrayVT_NoState
//void drawArrayVT_NoState(int bufferId, int vertices, int vertexSize = 24);
#endif
void drawArrayVTC(int bufferId, int vertices, int vertexSize = 24);
#ifndef drawArrayVTC_NoState
void drawArrayVTC_NoState(int bufferId, int vertices, int vertexSize = 24);
#endif
#endif

void glInit();
void gluPerspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar);
int glhUnProjectf(	float winx, float winy, float winz,
					float *modelview, float *projection,
					int *viewport, float *objectCoordinate);

// ── Macro redirects ──────────────────────────────────────────────────────
// For iOS GLES2: redirect fixed-function calls to software matrix stack and shader state
#if defined(__APPLE__) && !defined(MACOS) && !defined(GLDEBUG)
    // Matrix operations → software stack
    #define glTranslatef(x,y,z)     MatrixStack::translatef(x,y,z)
    #define glRotatef(a,x,y,z)      MatrixStack::rotatef(a,x,y,z)
    #define glScalef(x,y,z)         MatrixStack::scalef(x,y,z)
    #define glPushMatrix()          MatrixStack::pushMatrix()
    #define glPopMatrix()           MatrixStack::popMatrix()
    #define glLoadIdentity()        MatrixStack::loadIdentityOp()
    #define glMatrixMode(m)         MatrixStack::matrixMode(m)
    #define glOrthof(l,r,b,t,n,f)   MatrixStack::orthof(l,r,b,t,n,f)
    #define glMultMatrixf(m)        MatrixStack::multMatrix(m)

    // Matrix stack macros
    #define glTranslatef2(x,y,z)    MatrixStack::translatef(x,y,z)
    #define glRotatef2(a,x,y,z)     MatrixStack::rotatef(a,x,y,z)
    #define glScalef2(x,y,z)        MatrixStack::scalef(x,y,z)
    #define glPushMatrix2()         MatrixStack::pushMatrix()
    #define glPopMatrix2()          MatrixStack::popMatrix()
    #define glLoadIdentity2()       MatrixStack::loadIdentityOp()

    // State management → shader uniforms
    #define glEnable2(cap)          glEnable_shader(cap)
    #define glDisable2(cap)         glDisable_shader(cap)
    #define glEnable(cap)           glEnable_shader(cap)
    #define glDisable(cap)          glDisable_shader(cap)

    // Color → shader uniform
    #define glColor4f(r,g,b,a)      glColor4f_shader(r,g,b,a)
    #define glColor4f2(r,g,b,a)     glColor4f_shader(r,g,b,a)

    // Fog → shader uniforms
    #define glFogf(p,v)             glFogf_shader(p,v)
    #define glFogfv(p,v)            glFogfv_shader(p,v)
    #define glFogx(p,v)             glFogx_shader(p,v)

    // Fixed-function no-ops in GLES2
    #define glShadeModel(m)         ((void)0)
    #define glShadeModel2(m)        ((void)0)
    #define glTexEnvi(a,b,c)        ((void)0)
    #define glTexEnvf(a,b,c)        ((void)0)
    #define glHint(a,b)             ((void)0)

    // Vertex attribute setup (these are no-ops; actual setup is in draw functions)
    #define glVertexPointer(a,b,c,d)      ((void)0)
    #define glTexCoordPointer(a,b,c,d)    ((void)0)
    #define glColorPointer(a,b,c,d)       ((void)0)
    #define glNormalPointer(a,b,c)        ((void)0)
    #define glNormal3f(x,y,z)            ((void)0)
    #define glEnableClientState(s)        ((void)0)
    #define glDisableClientState(s)       ((void)0)

    #define glVertexPointer2(a,b,c,d)     ((void)0)
    #define glTexCoordPointer2(a,b,c,d)   ((void)0)
    #define glColorPointer2(a,b,c,d)      ((void)0)
    #define glEnableClientState2(s)       ((void)0)
    #define glDisableClientState2(s)      ((void)0)

    // Draw calls → shader-aware draw
    #define glDrawArrays2(m,o,v)    do { shaderSyncUniforms(); glDrawArrays(m,o,v); } while(0)

    // Matrix readback → software stack
    #define glGetFloatv(pname, params) MatrixStack::getMatrix(pname, params)

    // glIsEnabled → check software state
    static inline GLboolean _gles2_isEnabled(GLenum cap) {
        if (cap == GL_TEXTURE_2D) return g_texture2DEnabled ? GL_TRUE : GL_FALSE;
        if (cap == GL_ALPHA_TEST) return g_alphaTestEnabled ? GL_TRUE : GL_FALSE;
        if (cap == GL_FOG) return g_fogEnabled ? GL_TRUE : GL_FALSE;
        // For real GL states, we can't query them easily in GLES2 without tracking.
        // Return true for common states that the game enables at init:
        if (cap == GL_DEPTH_TEST || cap == GL_CULL_FACE) return GL_TRUE;
        if (cap == GL_BLEND) return GL_FALSE;
        if (cap == GL_LIGHTING) return GL_FALSE;
        return GL_FALSE;
    }
    #define glIsEnabled(cap)        _gles2_isEnabled(cap)

    // glGetIntegerv → partial software emulation
    static inline void _gles2_getIntegerv(GLenum pname, GLint* params) {
        // For matrix queries, redirect to getMatrix (they're float)
        // For other queries, pass through to real glGetIntegerv
        if (pname == GL_ALPHA_TEST_FUNC) { *params = 0x204; return; } // GL_GREATER
        if (pname == GL_CULL_FACE_MODE) { *params = 0x405; return; } // GL_BACK
        if (pname == GL_FRONT_FACE) { *params = 0x901; return; } // GL_CCW
        // Viewport and texture binding go to real GL
        // Note: we need the real glGetIntegerv for these
        extern void _real_glGetIntegerv(GLenum pname, GLint* params);
        _real_glGetIntegerv(pname, params);
    }
    #define glGetIntegerv(pname, params) _gles2_getIntegerv(pname, params)

    // glGetTexEnviv no-op
    #define glGetTexEnviv(a,b,c)    ((void)0)

    // Texture calls are same in GLES2
    #define glTexParameteri2  glTexParameteri
    #define glTexImage2D2     glTexImage2D
    #define glTexSubImage2D2  glTexSubImage2D
    #define glGenBuffers2     anGenBuffers
    #define glBindBuffer2     glBindBuffer
    #define glBufferData2     glBufferData
    #define glBindTexture2    glBindTexture
    #define glBlendFunc2      glBlendFunc

#elif !defined(GLDEBUG)
    // Non-iOS, non-debug: simple passthrough macros
	#define glTranslatef2	glTranslatef
	#define glRotatef2		glRotatef
	#define glScalef2		glScalef
	#define glPushMatrix2	glPushMatrix
	#define glPopMatrix2	glPopMatrix
	#define glLoadIdentity2 glLoadIdentity

	#define glVertexPointer2	glVertexPointer
	#define glColorPointer2		glColorPointer
	#define glTexCoordPointer2  glTexCoordPointer
	#define glEnableClientState2  glEnableClientState
	#define glDisableClientState2 glDisableClientState
	#define glDrawArrays2		glDrawArrays

	#define glTexParameteri2 glTexParameteri
	#define glTexImage2D2	 glTexImage2D
	#define glTexSubImage2D2 glTexSubImage2D
	#define glGenBuffers2	 anGenBuffers
	#define glBindBuffer2	 glBindBuffer
	#define glBufferData2	 glBufferData
	#define glBindTexture2	 glBindTexture

	#if defined(MACOS) || defined(LINUX)
	#define glEnable2		glEnable_shader
	#define glDisable2		glDisable_shader
	#else
	#define glEnable2		glEnable
	#define glDisable2		glDisable
	#endif

	#define glColor4f2		glColor4f
	#define glBlendFunc2	glBlendFunc
	#define glShadeModel2	glShadeModel
#else
    // GLDEBUG mode (same as before but not shown here for brevity)
	#define glTranslatef2(x, y, z) do{ if (Options::debugGl) LOGI("glTrans @ %s:%d: %f,%f,%f\n", __FILE__, __LINE__, (float)(x), (float)(y), (float)(z)); glTranslatef(x, y, z); GLERR(0); } while(0)
	#define glRotatef2(a, x, y, z) do{ if (Options::debugGl) LOGI("glRotat @ %s:%d: %f,%f,%f,%f\n", __FILE__, __LINE__, (float)(a), (float)(x), (float)(y), (float)(z)); glRotatef(a, x, y, z); GLERR(1); } while(0)
	#define glScalef2(x, y, z) do{ if (Options::debugGl) LOGI("glScale @ %s:%d: %f,%f,%f\n", __FILE__, __LINE__, (float)(x), (float)(y), (float)(z)); glScalef(x, y, z); GLERR(2); } while(0)
	#define glPushMatrix2() do{ if (Options::debugGl) LOGI("glPushM @ %s:%d\n", __FILE__, __LINE__); glPushMatrix(); GLERR(3); } while(0)
	#define glPopMatrix2() do{ if (Options::debugGl) LOGI("glPopM  @ %s:%d\n", __FILE__, __LINE__); glPopMatrix(); GLERR(4); } while(0)
	#define glLoadIdentity2() do{ if (Options::debugGl) LOGI("glLoadI @ %s:%d\n", __FILE__, __LINE__); glLoadIdentity(); GLERR(5); } while(0)

	#define glVertexPointer2(a, b, c, d) do{ glVertexPointer(a, b, c, d); GLERR(6); } while(0)
	#define glColorPointer2(a, b, c, d) do{ glColorPointer(a, b, c, d); GLERR(7); } while(0)
	#define glTexCoordPointer2(a, b, c, d) do{ glTexCoordPointer(a, b, c, d); GLERR(8); } while(0)
	#define glEnableClientState2(s) do{ glEnableClientState(s); GLERR(9); } while(0)
	#define glDisableClientState2(s) do{ glDisableClientState(s); GLERR(10); } while(0)
	#define glDrawArrays2(m, o, v) do{ glDrawArrays(m,o,v); GLERR(11); } while(0)

	#define glTexParameteri2(m, o, v) do{ glTexParameteri(m,o,v); GLERR(12); } while(0)
	#define glTexImage2D2(a,b,c,d,e,f,g,height,i) do{ glTexImage2D(a,b,c,d,e,f,g,height,i); GLERR(13); } while(0)
	#define glTexSubImage2D2(a,b,c,d,e,f,g,height,i) do{ glTexSubImage2D(a,b,c,d,e,f,g,height,i); GLERR(14); } while(0)
	#define glGenBuffers2(s, id) do{ anGenBuffers(s, id); GLERR(15); } while(0)
	#define glBindBuffer2(s, id) do{ glBindBuffer(s, id); GLERR(16); } while(0)
	#define glBufferData2(a, b, c, d) do{ glBufferData(a, b, c, d); GLERR(17); } while(0)
	#define glBindTexture2(m, z) do{ glBindTexture(m, z); GLERR(18); } while(0)

	#if defined(MACOS) || defined(LINUX)
	#define glEnable2(s) do{ glEnable_shader(s); GLERR(19); } while(0)
	#define glDisable2(s) do{ glDisable_shader(s); GLERR(20); } while(0)
	#else
	#define glEnable2(s) do{ glEnable(s); GLERR(19); } while(0)
	#define glDisable2(s) do{ glDisable(s); GLERR(20); } while(0)
	#endif
	#define glColor4f2(r, g, b, a) do{ glColor4f(r,g,b,a); GLERR(21); } while(0)
	#define glBlendFunc2(src, dst) do{ glBlendFunc(src, dst); GLERR(23); } while(0)
	#define glShadeModel2(s) do{ glShadeModel(s); GLERR(25); } while(0)
#endif

//
// Extensions
//
#ifdef WIN32
	#define glGetProcAddress(a) wglGetProcAddress(a)
#else
	#define glGetProcAddress(a) ((void*)(0))
#endif

#endif // !STANDALONE_SERVER


#endif /*NET_MINECRAFT_CLIENT_RENDERER__gles_H__ */
