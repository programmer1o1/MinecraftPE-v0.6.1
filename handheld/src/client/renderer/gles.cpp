#include "gles.h"

// Un-define macros inside this translation unit so we can call real GL functions
#if defined(__APPLE__) && !defined(MACOS)
#undef glEnable
#undef glDisable
#undef glGetFloatv
#undef glGetIntegerv
#undef glIsEnabled
#undef glDrawArrays
// Provide the real glGetIntegerv wrapper that gles.h's inline calls
extern "C" void glGetIntegerv(GLenum pname, GLint* params);
void _real_glGetIntegerv(GLenum pname, GLint* params) {
    glGetIntegerv(pname, params);
}
extern "C" void glEnable(GLenum cap);
extern "C" void glDisable(GLenum cap);
extern "C" GLboolean glIsEnabled(GLenum cap);
extern "C" void glDrawArrays(GLenum mode, GLint first, GLsizei count);
extern "C" void glGetFloatv(GLenum pname, GLfloat* params);
#endif

#if defined(MACOS) || defined(LINUX)
#undef glAlphaFunc
#endif

#include <cmath>
#include <cstdio>
#include <cstdlib>

// ── GLSL Shader support ──────────────────────────────────────────────────
#if defined(MACOS) || defined(LINUX) || (defined(__APPLE__) && !defined(MACOS))

// Attribute locations (bound before linking)
#define ATTR_POSITION  0
#define ATTR_TEXCOORD  1
#define ATTR_COLOR     2

// ---------------------------------------------------------------------------
// Embedded GLSL sources
// ---------------------------------------------------------------------------
#if defined(__APPLE__) && !defined(MACOS)
// GLSL ES 1.00 for iOS (MetalANGLE GLES2)
static const char* s_vertSrc =
    "precision highp float;\n"
    "attribute vec4 a_position;\n"
    "attribute vec2 a_texCoord;\n"
    "attribute vec4 a_color;\n"
    "uniform mat4 u_mvp;\n"
    "uniform mat4 u_mv;\n"
    "varying vec2 v_texCoord;\n"
    "varying vec4 v_color;\n"
    "varying float v_fogDist;\n"
    "void main() {\n"
    "    gl_Position = u_mvp * a_position;\n"
    "    v_texCoord  = a_texCoord;\n"
    "    v_color     = a_color;\n"
    "    v_fogDist   = abs((u_mv * a_position).z);\n"
    "}\n";

static const char* s_fragSrc =
    "precision mediump float;\n"
    "uniform sampler2D u_tex;\n"
    "uniform int  u_useTexture;\n"
    "uniform int  u_useAlphaTest;\n"
    "uniform float u_alphaTestRef;\n"
    "uniform int  u_useFog;\n"
    "uniform float u_fogStart;\n"
    "uniform float u_fogEnd;\n"
    "uniform vec4  u_fogColor;\n"
    "uniform int   u_fogMode;\n"
    "uniform float u_fogDensity;\n"
    "uniform int   u_useVertexColor;\n"
    "uniform vec4  u_color;\n"
    "varying vec2 v_texCoord;\n"
    "varying vec4 v_color;\n"
    "varying float v_fogDist;\n"
    "void main() {\n"
    "    vec4 color = (u_useVertexColor != 0) ? v_color : u_color;\n"
    "    if (u_useTexture != 0) {\n"
    "        color *= texture2D(u_tex, v_texCoord);\n"
    "    }\n"
    "    if (u_useAlphaTest != 0 && color.a < u_alphaTestRef) {\n"
    "        discard;\n"
    "    }\n"
    "    if (u_useFog != 0) {\n"
    "        float fogFactor;\n"
    "        if (u_fogMode == 2048) {\n"  // GL_EXP = 0x0800
    "            fogFactor = exp(-u_fogDensity * v_fogDist);\n"
    "        } else if (u_fogMode == 2049) {\n"  // GL_EXP2 = 0x0801
    "            fogFactor = exp(-u_fogDensity * u_fogDensity * v_fogDist * v_fogDist);\n"
    "        } else {\n"  // GL_LINEAR
    "            float denom = u_fogEnd - u_fogStart;\n"
    "            fogFactor = clamp((u_fogEnd - v_fogDist) / (denom + 0.001), 0.0, 1.0);\n"
    "        }\n"
    "        fogFactor = clamp(fogFactor, 0.0, 1.0);\n"
    "        color = mix(u_fogColor, color, fogFactor);\n"
    "    }\n"
    "    gl_FragColor = color;\n"
    "}\n";

#else
// GLSL 1.20 for macOS/Linux desktop
static const char* s_vertSrc =
    "#version 120\n"
    "void main() {\n"
    "    gl_Position    = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
    "    gl_FrontColor  = gl_Color;\n"
    "    gl_TexCoord[0] = gl_MultiTexCoord0;\n"
    "    gl_FogFragCoord = abs(gl_Position.z / gl_Position.w);\n"
    "}\n";

static const char* s_fragSrc =
    "#version 120\n"
    "uniform sampler2D tex;\n"
    "uniform bool      useTexture;\n"
    "uniform bool      useAlphaTest;\n"
    "uniform float     alphaTestRef;\n"
    "uniform bool      useFog;\n"
    "void main() {\n"
    "    vec4 color = gl_Color;\n"
    "    if (useTexture) {\n"
    "        color *= texture2D(tex, gl_TexCoord[0].xy);\n"
    "    }\n"
    "    if (useAlphaTest && color.a < alphaTestRef) {\n"
    "        discard;\n"
    "    }\n"
    "    if (useFog) {\n"
    "        float denom = gl_Fog.end - gl_Fog.start;\n"
    "        float fogFactor = clamp((gl_Fog.end - gl_FogFragCoord) / (denom + 0.001), 0.0, 1.0);\n"
    "        gl_FragColor = mix(gl_Fog.color, color, fogFactor);\n"
    "    } else {\n"
    "        gl_FragColor = color;\n"
    "    }\n"
    "}\n";
#endif

// ---------------------------------------------------------------------------
// Shader globals
// ---------------------------------------------------------------------------
GLuint g_shader_program = 0;
GLuint g_vert_shader    = 0;
GLuint g_frag_shader    = 0;

GLint  g_loc_mvp          = -1;
GLint  g_loc_mv           = -1;
GLint  g_loc_useTexture   = -1;
GLint  g_loc_useAlphaTest = -1;
GLint  g_loc_alphaTestRef = -1;
GLint  g_loc_useFog       = -1;
GLint  g_loc_fogStart     = -1;
GLint  g_loc_fogEnd       = -1;
GLint  g_loc_fogColor     = -1;
GLint  g_loc_fogMode      = -1;
GLint  g_loc_fogDensity   = -1;
GLint  g_loc_tex          = -1;
GLint  g_loc_useVertexColor = -1;
GLint  g_loc_uColor       = -1;

bool  g_shaderEnabled    = false;
bool  g_texture2DEnabled = false;
bool  g_alphaTestEnabled = false;
float g_alphaTestRef     = 0.1f;
bool  g_fogEnabled       = false;
float g_fogStart         = 0.0f;
float g_fogEnd           = 1.0f;
float g_fogColor[4]      = {1,1,1,1};
int   g_fogMode          = 0x0801; // GL_LINEAR
float g_fogDensity       = 1.0f;
float g_currentColor[4]  = {1,1,1,1};

// ---------------------------------------------------------------------------
// Helper: compile a single shader stage and print errors if any
// ---------------------------------------------------------------------------
static GLuint compileShader(GLenum type, const char* src) {
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    GLint ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetShaderInfoLog(id, sizeof(log), nullptr, log);
        const char* typeName = (type == GL_VERTEX_SHADER) ? "VERTEX" : "FRAGMENT";
        LOGI("[shader] %s compile error:\n%s\n", typeName, log);
    }
    return id;
}

// ---------------------------------------------------------------------------
// shaderInit — call once from glInit()
// ---------------------------------------------------------------------------
void shaderInit() {
    g_vert_shader = compileShader(GL_VERTEX_SHADER,   s_vertSrc);
    g_frag_shader = compileShader(GL_FRAGMENT_SHADER, s_fragSrc);

    g_shader_program = glCreateProgram();
    glAttachShader(g_shader_program, g_vert_shader);
    glAttachShader(g_shader_program, g_frag_shader);

#if defined(__APPLE__) && !defined(MACOS)
    // Bind attribute locations before linking (GLES2)
    glBindAttribLocation(g_shader_program, ATTR_POSITION, "a_position");
    glBindAttribLocation(g_shader_program, ATTR_TEXCOORD, "a_texCoord");
    glBindAttribLocation(g_shader_program, ATTR_COLOR,    "a_color");
#endif

    glLinkProgram(g_shader_program);

    GLint ok = 0;
    glGetProgramiv(g_shader_program, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[2048];
        glGetProgramInfoLog(g_shader_program, sizeof(log), nullptr, log);
        LOGI("[shader] link error:\n%s\n", log);
        return;
    }

    // Cache uniform locations
#if defined(__APPLE__) && !defined(MACOS)
    g_loc_mvp          = glGetUniformLocation(g_shader_program, "u_mvp");
    g_loc_mv           = glGetUniformLocation(g_shader_program, "u_mv");
    g_loc_useTexture   = glGetUniformLocation(g_shader_program, "u_useTexture");
    g_loc_useAlphaTest = glGetUniformLocation(g_shader_program, "u_useAlphaTest");
    g_loc_alphaTestRef = glGetUniformLocation(g_shader_program, "u_alphaTestRef");
    g_loc_useFog       = glGetUniformLocation(g_shader_program, "u_useFog");
    g_loc_fogStart     = glGetUniformLocation(g_shader_program, "u_fogStart");
    g_loc_fogEnd       = glGetUniformLocation(g_shader_program, "u_fogEnd");
    g_loc_fogColor     = glGetUniformLocation(g_shader_program, "u_fogColor");
    g_loc_fogMode      = glGetUniformLocation(g_shader_program, "u_fogMode");
    g_loc_fogDensity   = glGetUniformLocation(g_shader_program, "u_fogDensity");
    g_loc_tex          = glGetUniformLocation(g_shader_program, "u_tex");
    g_loc_useVertexColor = glGetUniformLocation(g_shader_program, "u_useVertexColor");
    g_loc_uColor       = glGetUniformLocation(g_shader_program, "u_color");
#else
    g_loc_useTexture   = glGetUniformLocation(g_shader_program, "useTexture");
    g_loc_useAlphaTest = glGetUniformLocation(g_shader_program, "useAlphaTest");
    g_loc_alphaTestRef = glGetUniformLocation(g_shader_program, "alphaTestRef");
    g_loc_useFog       = glGetUniformLocation(g_shader_program, "useFog");
    g_loc_tex          = glGetUniformLocation(g_shader_program, "tex");
#endif

    // Bind shader and seed all uniforms
    glUseProgram(g_shader_program);
    g_shaderEnabled = true;

    if (g_loc_tex          >= 0) glUniform1i(g_loc_tex,          0);
    if (g_loc_useTexture   >= 0) glUniform1i(g_loc_useTexture,   0);
    if (g_loc_useAlphaTest >= 0) glUniform1i(g_loc_useAlphaTest, 0);
    if (g_loc_alphaTestRef >= 0) glUniform1f(g_loc_alphaTestRef, g_alphaTestRef);
    if (g_loc_useFog       >= 0) glUniform1i(g_loc_useFog,       0);
    if (g_loc_useVertexColor >= 0) glUniform1i(g_loc_useVertexColor, 1);
    if (g_loc_uColor       >= 0) glUniform4f(g_loc_uColor, 1, 1, 1, 1);

#if defined(__APPLE__) && !defined(MACOS)
    if (g_loc_fogStart   >= 0) glUniform1f(g_loc_fogStart, g_fogStart);
    if (g_loc_fogEnd     >= 0) glUniform1f(g_loc_fogEnd, g_fogEnd);
    if (g_loc_fogColor   >= 0) glUniform4fv(g_loc_fogColor, 1, g_fogColor);
    if (g_loc_fogMode    >= 0) glUniform1i(g_loc_fogMode, g_fogMode);
    if (g_loc_fogDensity >= 0) glUniform1f(g_loc_fogDensity, g_fogDensity);
#endif

    LOGI("[shader] compiled and active (program=%u)\n", g_shader_program);
}

// ---------------------------------------------------------------------------
void shaderBind() {
    if (g_shader_program) {
        glUseProgram(g_shader_program);
        g_shaderEnabled = true;
    }
}

void shaderUnbind() {
    glUseProgram(0);
    g_shaderEnabled = false;
}

// ---------------------------------------------------------------------------
// shaderSyncUniforms — called before every glDrawArrays on iOS
// ---------------------------------------------------------------------------
void shaderSyncUniforms() {
#if defined(__APPLE__) && !defined(MACOS)
    if (!g_shaderEnabled) return;

    // Upload MVP and MV matrices from software stack
    if (g_loc_mvp >= 0) glUniformMatrix4fv(g_loc_mvp, 1, GL_FALSE, MatrixStack::getMVP());
    if (g_loc_mv  >= 0) glUniformMatrix4fv(g_loc_mv,  1, GL_FALSE, MatrixStack::getModelView());
#endif
}

// ---------------------------------------------------------------------------
// State management functions
// ---------------------------------------------------------------------------
void glEnable_shader(GLenum cap) {
#if defined(__APPLE__) && !defined(MACOS)
    // In GLES2, GL_ALPHA_TEST and GL_FOG don't exist as real caps
    if (cap == GL_ALPHA_TEST) {
        g_alphaTestEnabled = true;
        if (g_shaderEnabled && g_loc_useAlphaTest >= 0)
            glUniform1i(g_loc_useAlphaTest, 1);
        return;
    }
    if (cap == GL_FOG) {
        g_fogEnabled = true;
        if (g_shaderEnabled && g_loc_useFog >= 0)
            glUniform1i(g_loc_useFog, 1);
        return;
    }
    if (cap == GL_TEXTURE_2D) {
        g_texture2DEnabled = true;
        if (g_shaderEnabled && g_loc_useTexture >= 0)
            glUniform1i(g_loc_useTexture, 1);
        return;
    }
    if (cap == GL_LIGHTING || cap == GL_COLOR_MATERIAL) {
        return; // no-op in GLES2
    }
    // Real GL caps (GL_DEPTH_TEST, GL_CULL_FACE, GL_BLEND, etc.)
    glEnable(cap);
#else
    glEnable(cap);
    if (!g_shaderEnabled) return;
    if (cap == GL_TEXTURE_2D) {
        g_texture2DEnabled = true;
        if (g_loc_useTexture >= 0) glUniform1i(g_loc_useTexture, 1);
    } else if (cap == GL_ALPHA_TEST) {
        g_alphaTestEnabled = true;
        if (g_loc_useAlphaTest >= 0) glUniform1i(g_loc_useAlphaTest, 1);
        if (g_loc_alphaTestRef >= 0) glUniform1f(g_loc_alphaTestRef, g_alphaTestRef);
    } else if (cap == GL_FOG) {
        g_fogEnabled = true;
        if (g_loc_useFog >= 0) glUniform1i(g_loc_useFog, 1);
    }
#endif
}

void glDisable_shader(GLenum cap) {
#if defined(__APPLE__) && !defined(MACOS)
    if (cap == GL_ALPHA_TEST) {
        g_alphaTestEnabled = false;
        if (g_shaderEnabled && g_loc_useAlphaTest >= 0)
            glUniform1i(g_loc_useAlphaTest, 0);
        return;
    }
    if (cap == GL_FOG) {
        g_fogEnabled = false;
        if (g_shaderEnabled && g_loc_useFog >= 0)
            glUniform1i(g_loc_useFog, 0);
        return;
    }
    if (cap == GL_TEXTURE_2D) {
        g_texture2DEnabled = false;
        if (g_shaderEnabled && g_loc_useTexture >= 0)
            glUniform1i(g_loc_useTexture, 0);
        return;
    }
    if (cap == GL_LIGHTING || cap == GL_COLOR_MATERIAL) {
        return; // no-op
    }
    glDisable(cap);
#else
    glDisable(cap);
    if (!g_shaderEnabled) return;
    if (cap == GL_TEXTURE_2D) {
        g_texture2DEnabled = false;
        if (g_loc_useTexture >= 0) glUniform1i(g_loc_useTexture, 0);
    } else if (cap == GL_ALPHA_TEST) {
        g_alphaTestEnabled = false;
        if (g_loc_useAlphaTest >= 0) glUniform1i(g_loc_useAlphaTest, 0);
    } else if (cap == GL_FOG) {
        g_fogEnabled = false;
        if (g_loc_useFog >= 0) glUniform1i(g_loc_useFog, 0);
    }
#endif
}

void glAlphaFunc_shader(GLenum func, GLclampf ref) {
#if defined(MACOS) || defined(LINUX)
    extern void glAlphaFunc(GLenum, GLclampf);
    glAlphaFunc(func, ref);
#endif
    g_alphaTestRef = (float)ref;
    if (g_shaderEnabled && g_loc_alphaTestRef >= 0) {
        glUniform1f(g_loc_alphaTestRef, g_alphaTestRef);
    }
}

void glColor4f_shader(float r, float g, float b, float a) {
    g_currentColor[0] = r;
    g_currentColor[1] = g;
    g_currentColor[2] = b;
    g_currentColor[3] = a;
    if (g_shaderEnabled && g_loc_uColor >= 0) {
        glUniform4f(g_loc_uColor, r, g, b, a);
    }
}

void glFogf_shader(GLenum pname, GLfloat param) {
    if (pname == GL_FOG_START) {
        g_fogStart = param;
        if (g_shaderEnabled && g_loc_fogStart >= 0) glUniform1f(g_loc_fogStart, param);
    } else if (pname == GL_FOG_END) {
        g_fogEnd = param;
        if (g_shaderEnabled && g_loc_fogEnd >= 0) glUniform1f(g_loc_fogEnd, param);
    } else if (pname == GL_FOG_DENSITY) {
        g_fogDensity = param;
        if (g_shaderEnabled && g_loc_fogDensity >= 0) glUniform1f(g_loc_fogDensity, param);
    }
}

void glFogfv_shader(GLenum pname, const GLfloat* params) {
    if (pname == GL_FOG_COLOR) {
        g_fogColor[0] = params[0];
        g_fogColor[1] = params[1];
        g_fogColor[2] = params[2];
        g_fogColor[3] = params[3];
        if (g_shaderEnabled && g_loc_fogColor >= 0)
            glUniform4fv(g_loc_fogColor, 1, params);
    }
}

void glFogx_shader(GLenum pname, GLint param) {
    if (pname == GL_FOG_MODE) {
        g_fogMode = param;
        if (g_shaderEnabled && g_loc_fogMode >= 0) glUniform1i(g_loc_fogMode, param);
    }
}

#endif // shader platforms

static const float __glPi = 3.14159265358979323846f;

static void __gluMakeIdentityf(GLfloat m[16]);


void gluPerspective(GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar) {
    GLfloat m[4][4];
    GLfloat sine, cotangent, deltaZ;
    GLfloat radians=(GLfloat)(fovy/2.0f*__glPi/180.0f);

    deltaZ=zFar-zNear;
    sine=(GLfloat)sin(radians);
    if ((deltaZ==0.0f) || (sine==0.0f) || (aspect==0.0f))
    {
        return;
    }
    cotangent=(GLfloat)(cos(radians)/sine);

    __gluMakeIdentityf(&m[0][0]);
    m[0][0] = cotangent / aspect;
    m[1][1] = cotangent;
    m[2][2] = -(zFar + zNear) / deltaZ;
    m[2][3] = -1.0f;
    m[3][2] = -2.0f * zNear * zFar / deltaZ;
    m[3][3] = 0;
#if defined(__APPLE__) && !defined(MACOS)
    MatrixStack::multMatrix(&m[0][0]);
#else
    glMultMatrixf(&m[0][0]);
#endif
}

void __gluMakeIdentityf(GLfloat m[16]) {
    m[0] = 1;  m[4] = 0;  m[8]  = 0;  m[12] = 0;
    m[1] = 0;  m[5] = 1;  m[9]  = 0;  m[13] = 0;
    m[2] = 0;  m[6] = 0;  m[10] = 1;  m[14] = 0;
    m[3] = 0;  m[7] = 0;  m[11] = 0;  m[15] = 1;
}

void glInit()
{
#if !defined(OPENGL_ES) && !defined(MACOS) && !defined(LINUX)
	GLenum err = glewInit();
	printf("Err: %d\n", err);
#endif

#if defined(__APPLE__) && !defined(MACOS)
    // iOS GLES2: initialize matrix stack and shader
    MatrixStack::init();
    shaderInit();

    // Enable vertex attrib arrays permanently
    glEnableVertexAttribArray(ATTR_POSITION);
    glEnableVertexAttribArray(ATTR_TEXCOORD);
    glEnableVertexAttribArray(ATTR_COLOR);
#elif defined(MACOS) || defined(LINUX)
    // Desktop: compile GLSL 1.20 shader for texture/alpha/fog uniform control
    shaderInit();
#endif
}

void anGenBuffers(GLsizei n, GLuint* buffers) {
	glGenBuffers(n, buffers);
}

#ifdef USE_VBO

#if defined(__APPLE__) && !defined(MACOS)
// iOS GLES2: use vertex attrib pointers instead of fixed-function

void drawArrayVT(int bufferId, int vertices, int vertexSize /* = 24 */, unsigned int mode /* = GL_TRIANGLES */) {
    glBindBuffer(GL_ARRAY_BUFFER, bufferId);
    glVertexAttribPointer(ATTR_POSITION, 3, GL_FLOAT, GL_FALSE, vertexSize, 0);
    glVertexAttribPointer(ATTR_TEXCOORD, 2, GL_FLOAT, GL_FALSE, vertexSize, (GLvoid*)(3*4));
    // No per-vertex color; use u_color uniform
    if (g_loc_useVertexColor >= 0) glUniform1i(g_loc_useVertexColor, 0);
    glDisableVertexAttribArray(ATTR_COLOR);
    shaderSyncUniforms();
    glDrawArrays(mode, 0, vertices);
    glEnableVertexAttribArray(ATTR_COLOR);
    if (g_loc_useVertexColor >= 0) glUniform1i(g_loc_useVertexColor, 1);
}

void drawArrayVTC(int bufferId, int vertices, int vertexSize /* = 24 */) {
    glBindBuffer(GL_ARRAY_BUFFER, bufferId);
    glVertexAttribPointer(ATTR_POSITION, 3, GL_FLOAT, GL_FALSE, vertexSize, 0);
    glVertexAttribPointer(ATTR_TEXCOORD, 2, GL_FLOAT, GL_FALSE, vertexSize, (GLvoid*)(3*4));
    glVertexAttribPointer(ATTR_COLOR,    4, GL_UNSIGNED_BYTE, GL_TRUE, vertexSize, (GLvoid*)(5*4));
    if (g_loc_useVertexColor >= 0) glUniform1i(g_loc_useVertexColor, 1);
    shaderSyncUniforms();
    glDrawArrays(GL_TRIANGLES, 0, vertices);
}

#else
// Non-iOS: original fixed-function implementation

void drawArrayVT(int bufferId, int vertices, int vertexSize /* = 24 */, unsigned int mode /* = GL_TRIANGLES */) {
	glBindBuffer2(GL_ARRAY_BUFFER, bufferId);
	glTexCoordPointer2(2, GL_FLOAT, vertexSize, (GLvoid*) (3 * 4));
	glEnableClientState2(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer2(3, GL_FLOAT, vertexSize, 0);
	glEnableClientState2(GL_VERTEX_ARRAY);
	glDrawArrays2(mode, 0, vertices);
	glDisableClientState2(GL_VERTEX_ARRAY);
	glDisableClientState2(GL_TEXTURE_COORD_ARRAY);
}

#ifndef drawArrayVT_NoState
void drawArrayVT_NoState(int bufferId, int vertices, int vertexSize /* = 24 */) {
	glBindBuffer2(GL_ARRAY_BUFFER, bufferId);
	glTexCoordPointer2(2, GL_FLOAT, vertexSize, (GLvoid*) (3 * 4));
	glVertexPointer2(3, GL_FLOAT, vertexSize, 0);
	glDrawArrays2(GL_TRIANGLES, 0, vertices);
}
#endif

void drawArrayVTC(int bufferId, int vertices, int vertexSize /* = 24 */) {
	glEnableClientState2(GL_VERTEX_ARRAY);
	glEnableClientState2(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState2(GL_COLOR_ARRAY);

	glBindBuffer2(GL_ARRAY_BUFFER, bufferId);

	glVertexPointer2(  3, GL_FLOAT, vertexSize, 0);
	glTexCoordPointer2(2, GL_FLOAT, vertexSize, (GLvoid*) (3 * 4));
	glColorPointer2(4, GL_UNSIGNED_BYTE, vertexSize, (GLvoid*) (5*4));

	glDrawArrays2(GL_TRIANGLES, 0, vertices);

	glDisableClientState2(GL_VERTEX_ARRAY);
	glDisableClientState2(GL_TEXTURE_COORD_ARRAY);
	glDisableClientState2(GL_COLOR_ARRAY);
}

#ifndef drawArrayVTC_NoState
void drawArrayVTC_NoState(int bufferId, int vertices, int vertexSize /* = 24 */) {
	glBindBuffer2(GL_ARRAY_BUFFER, bufferId);

	glVertexPointer2(  3, GL_FLOAT, vertexSize, 0);
	glTexCoordPointer2(2, GL_FLOAT, vertexSize, (GLvoid*) (3 * 4));
	glColorPointer2(4, GL_UNSIGNED_BYTE, vertexSize, (GLvoid*) (5*4));

	glDrawArrays2(GL_TRIANGLES, 0, vertices);
}
#endif

#endif // iOS vs non-iOS

#endif // USE_VBO


//
// Code borrowed from OpenGL.org
//
void MultiplyMatrices4by4OpenGL_FLOAT(float *result, float *matrix1, float *matrix2)
{
	result[0]=matrix1[0]*matrix2[0]+matrix1[4]*matrix2[1]+matrix1[8]*matrix2[2]+matrix1[12]*matrix2[3];
	result[4]=matrix1[0]*matrix2[4]+matrix1[4]*matrix2[5]+matrix1[8]*matrix2[6]+matrix1[12]*matrix2[7];
	result[8]=matrix1[0]*matrix2[8]+matrix1[4]*matrix2[9]+matrix1[8]*matrix2[10]+matrix1[12]*matrix2[11];
	result[12]=matrix1[0]*matrix2[12]+matrix1[4]*matrix2[13]+matrix1[8]*matrix2[14]+matrix1[12]*matrix2[15];
	result[1]=matrix1[1]*matrix2[0]+matrix1[5]*matrix2[1]+matrix1[9]*matrix2[2]+matrix1[13]*matrix2[3];
	result[5]=matrix1[1]*matrix2[4]+matrix1[5]*matrix2[5]+matrix1[9]*matrix2[6]+matrix1[13]*matrix2[7];
	result[9]=matrix1[1]*matrix2[8]+matrix1[5]*matrix2[9]+matrix1[9]*matrix2[10]+matrix1[13]*matrix2[11];
	result[13]=matrix1[1]*matrix2[12]+matrix1[5]*matrix2[13]+matrix1[9]*matrix2[14]+matrix1[13]*matrix2[15];
	result[2]=matrix1[2]*matrix2[0]+matrix1[6]*matrix2[1]+matrix1[10]*matrix2[2]+matrix1[14]*matrix2[3];
	result[6]=matrix1[2]*matrix2[4]+matrix1[6]*matrix2[5]+matrix1[10]*matrix2[6]+matrix1[14]*matrix2[7];
	result[10]=matrix1[2]*matrix2[8]+matrix1[6]*matrix2[9]+matrix1[10]*matrix2[10]+matrix1[14]*matrix2[11];
	result[14]=matrix1[2]*matrix2[12]+matrix1[6]*matrix2[13]+matrix1[10]*matrix2[14]+matrix1[14]*matrix2[15];
	result[3]=matrix1[3]*matrix2[0]+matrix1[7]*matrix2[1]+matrix1[11]*matrix2[2]+matrix1[15]*matrix2[3];
	result[7]=matrix1[3]*matrix2[4]+matrix1[7]*matrix2[5]+matrix1[11]*matrix2[6]+matrix1[15]*matrix2[7];
	result[11]=matrix1[3]*matrix2[8]+matrix1[7]*matrix2[9]+matrix1[11]*matrix2[10]+matrix1[15]*matrix2[11];
	result[15]=matrix1[3]*matrix2[12]+matrix1[7]*matrix2[13]+matrix1[11]*matrix2[14]+matrix1[15]*matrix2[15];
}

void MultiplyMatrixByVector4by4OpenGL_FLOAT(float *resultvector, const float *matrix, const float *pvector)
{
	resultvector[0]=matrix[0]*pvector[0]+matrix[4]*pvector[1]+matrix[8]*pvector[2]+matrix[12]*pvector[3];
	resultvector[1]=matrix[1]*pvector[0]+matrix[5]*pvector[1]+matrix[9]*pvector[2]+matrix[13]*pvector[3];
	resultvector[2]=matrix[2]*pvector[0]+matrix[6]*pvector[1]+matrix[10]*pvector[2]+matrix[14]*pvector[3];
	resultvector[3]=matrix[3]*pvector[0]+matrix[7]*pvector[1]+matrix[11]*pvector[2]+matrix[15]*pvector[3];
}

#define SWAP_ROWS_DOUBLE(a, b) { double *_tmp = a; (a)=(b); (b)=_tmp; }
#define SWAP_ROWS_FLOAT(a, b) { float *_tmp = a; (a)=(b); (b)=_tmp; }
#define MAT(m,r,c) (m)[(c)*4+(r)]

int glhInvertMatrixf2(float *m, float *out)
{
	float wtmp[4][8];
	float m0, m1, m2, m3, s;
	float *r0, *r1, *r2, *r3;
	r0 = wtmp[0], r1 = wtmp[1], r2 = wtmp[2], r3 = wtmp[3];
	r0[0] = MAT(m, 0, 0), r0[1] = MAT(m, 0, 1),
		r0[2] = MAT(m, 0, 2), r0[3] = MAT(m, 0, 3),
		r0[4] = 1.0f, r0[5] = r0[6] = r0[7] = 0.0f,
		r1[0] = MAT(m, 1, 0), r1[1] = MAT(m, 1, 1),
		r1[2] = MAT(m, 1, 2), r1[3] = MAT(m, 1, 3),
		r1[5] = 1.0f, r1[4] = r1[6] = r1[7] = 0.0f,
		r2[0] = MAT(m, 2, 0), r2[1] = MAT(m, 2, 1),
		r2[2] = MAT(m, 2, 2), r2[3] = MAT(m, 2, 3),
		r2[6] = 1.0f, r2[4] = r2[5] = r2[7] = 0.0f,
		r3[0] = MAT(m, 3, 0), r3[1] = MAT(m, 3, 1),
		r3[2] = MAT(m, 3, 2), r3[3] = MAT(m, 3, 3),
		r3[7] = 1.0f, r3[4] = r3[5] = r3[6] = 0.0f;
	if (fabsf(r3[0]) > fabsf(r2[0])) SWAP_ROWS_FLOAT(r3, r2);
	if (fabsf(r2[0]) > fabsf(r1[0])) SWAP_ROWS_FLOAT(r2, r1);
	if (fabsf(r1[0]) > fabsf(r0[0])) SWAP_ROWS_FLOAT(r1, r0);
	if (0.0f == r0[0]) return 0;
	m1 = r1[0] / r0[0]; m2 = r2[0] / r0[0]; m3 = r3[0] / r0[0];
	s = r0[1]; r1[1] -= m1 * s; r2[1] -= m2 * s; r3[1] -= m3 * s;
	s = r0[2]; r1[2] -= m1 * s; r2[2] -= m2 * s; r3[2] -= m3 * s;
	s = r0[3]; r1[3] -= m1 * s; r2[3] -= m2 * s; r3[3] -= m3 * s;
	s = r0[4]; if (s != 0.0f) { r1[4] -= m1*s; r2[4] -= m2*s; r3[4] -= m3*s; }
	s = r0[5]; if (s != 0.0f) { r1[5] -= m1*s; r2[5] -= m2*s; r3[5] -= m3*s; }
	s = r0[6]; if (s != 0.0f) { r1[6] -= m1*s; r2[6] -= m2*s; r3[6] -= m3*s; }
	s = r0[7]; if (s != 0.0f) { r1[7] -= m1*s; r2[7] -= m2*s; r3[7] -= m3*s; }
	if (fabsf(r3[1]) > fabsf(r2[1])) SWAP_ROWS_FLOAT(r3, r2);
	if (fabsf(r2[1]) > fabsf(r1[1])) SWAP_ROWS_FLOAT(r2, r1);
	if (0.0f == r1[1]) return 0;
	m2 = r2[1] / r1[1]; m3 = r3[1] / r1[1];
	r2[2] -= m2 * r1[2]; r3[2] -= m3 * r1[2];
	r2[3] -= m2 * r1[3]; r3[3] -= m3 * r1[3];
	s = r1[4]; if (0.0f != s) { r2[4] -= m2*s; r3[4] -= m3*s; }
	s = r1[5]; if (0.0f != s) { r2[5] -= m2*s; r3[5] -= m3*s; }
	s = r1[6]; if (0.0f != s) { r2[6] -= m2*s; r3[6] -= m3*s; }
	s = r1[7]; if (0.0f != s) { r2[7] -= m2*s; r3[7] -= m3*s; }
	if (fabsf(r3[2]) > fabsf(r2[2])) SWAP_ROWS_FLOAT(r3, r2);
	if (0.0f == r2[2]) return 0;
	m3 = r3[2] / r2[2];
	r3[3] -= m3 * r2[3], r3[4] -= m3 * r2[4],
		r3[5] -= m3 * r2[5], r3[6] -= m3 * r2[6], r3[7] -= m3 * r2[7];
	if (0.0f == r3[3]) return 0;
	s = 1.0f / r3[3];
	r3[4] *= s; r3[5] *= s; r3[6] *= s; r3[7] *= s;
	m2 = r2[3]; s = 1.0f / r2[2];
	r2[4] = s * (r2[4] - r3[4] * m2), r2[5] = s * (r2[5] - r3[5] * m2),
		r2[6] = s * (r2[6] - r3[6] * m2), r2[7] = s * (r2[7] - r3[7] * m2);
	m1 = r1[3];
	r1[4] -= r3[4] * m1, r1[5] -= r3[5] * m1, r1[6] -= r3[6] * m1, r1[7] -= r3[7] * m1;
	m0 = r0[3];
	r0[4] -= r3[4] * m0, r0[5] -= r3[5] * m0, r0[6] -= r3[6] * m0, r0[7] -= r3[7] * m0;
	m1 = r1[2]; s = 1.0f / r1[1];
	r1[4] = s * (r1[4] - r2[4] * m1), r1[5] = s * (r1[5] - r2[5] * m1),
		r1[6] = s * (r1[6] - r2[6] * m1), r1[7] = s * (r1[7] - r2[7] * m1);
	m0 = r0[2];
	r0[4] -= r2[4] * m0, r0[5] -= r2[5] * m0, r0[6] -= r2[6] * m0, r0[7] -= r2[7] * m0;
	m0 = r0[1]; s = 1.0f / r0[0];
	r0[4] = s * (r0[4] - r1[4] * m0), r0[5] = s * (r0[5] - r1[5] * m0),
		r0[6] = s * (r0[6] - r1[6] * m0), r0[7] = s * (r0[7] - r1[7] * m0);
	MAT(out, 0, 0) = r0[4]; MAT(out, 0, 1) = r0[5], MAT(out, 0, 2) = r0[6];
	MAT(out, 0, 3) = r0[7], MAT(out, 1, 0) = r1[4]; MAT(out, 1, 1) = r1[5], MAT(out, 1, 2) = r1[6];
	MAT(out, 1, 3) = r1[7], MAT(out, 2, 0) = r2[4]; MAT(out, 2, 1) = r2[5], MAT(out, 2, 2) = r2[6];
	MAT(out, 2, 3) = r2[7], MAT(out, 3, 0) = r3[4]; MAT(out, 3, 1) = r3[5], MAT(out, 3, 2) = r3[6];
	MAT(out, 3, 3) = r3[7];
	return 1;
}

int glhUnProjectf(float winx, float winy, float winz,
                  float *modelview, float *projection,
                  int *viewport, float *objectCoordinate)
{
	float m[16], A[16];
	float in[4], out[4];
	MultiplyMatrices4by4OpenGL_FLOAT(A, projection, modelview);
	if(glhInvertMatrixf2(A, m)==0)
		return 0;
	in[0]=(winx-(float)viewport[0])/(float)viewport[2]*2.0f-1.0f;
	in[1]=(winy-(float)viewport[1])/(float)viewport[3]*2.0f-1.0f;
	in[2]=2.0f*winz-1.0f;
	in[3]=1.0f;
	MultiplyMatrixByVector4by4OpenGL_FLOAT(out, m, in);
	if(out[3]==0.0f)
		return 0;
	out[3]=1.0f/out[3];
	objectCoordinate[0]=out[0]*out[3];
	objectCoordinate[1]=out[1]*out[3];
	objectCoordinate[2]=out[2]*out[3];
	return 1;
}
