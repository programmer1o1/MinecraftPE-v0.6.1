#ifndef NET_MINECRAFT_CLIENT_RENDERER__MatrixStack_H__
#define NET_MINECRAFT_CLIENT_RENDERER__MatrixStack_H__

// Software matrix stack for GLES2 migration.
// Replaces glMatrixMode/glPushMatrix/glPopMatrix/glLoadIdentity/
// glTranslatef/glRotatef/glScalef/glOrthof/glMultMatrixf
// with a CPU-side implementation that feeds shader uniforms.

#include <cmath>
#include <cstring>

class MatrixStack {
public:
    enum Mode { MODELVIEW = 0, PROJECTION = 1 };

    static void init() {
        _mode = MODELVIEW;
        _mvDepth = 0;
        _projDepth = 0;
        loadIdentity(_mvStack[0]);
        loadIdentity(_projStack[0]);
        _mvpDirty = true;
    }

    static void matrixMode(int mode) {
        _mode = (mode == 0x1701) ? PROJECTION : MODELVIEW; // GL_PROJECTION=0x1701
    }

    static void pushMatrix() {
        if (_mode == MODELVIEW && _mvDepth < MAX_DEPTH - 1) {
            memcpy(_mvStack[_mvDepth + 1], _mvStack[_mvDepth], 64);
            _mvDepth++;
        } else if (_mode == PROJECTION && _projDepth < MAX_DEPTH - 1) {
            memcpy(_projStack[_projDepth + 1], _projStack[_projDepth], 64);
            _projDepth++;
        }
    }

    static void popMatrix() {
        if (_mode == MODELVIEW && _mvDepth > 0) {
            _mvDepth--;
        } else if (_mode == PROJECTION && _projDepth > 0) {
            _projDepth--;
        }
        _mvpDirty = true;
    }

    static void loadIdentityOp() {
        loadIdentity(current());
        _mvpDirty = true;
    }

    static void translatef(float x, float y, float z) {
        float t[16];
        loadIdentity(t);
        t[12] = x; t[13] = y; t[14] = z;
        multMatrix(t);
    }

    static void rotatef(float angle, float x, float y, float z) {
        float rad = angle * 3.14159265358979323846f / 180.0f;
        float c = cosf(rad), s = sinf(rad);
        float len = sqrtf(x*x + y*y + z*z);
        if (len < 1e-6f) return;
        x /= len; y /= len; z /= len;
        float nc = 1.0f - c;

        float r[16];
        r[0] = x*x*nc + c;    r[4] = x*y*nc - z*s;  r[8]  = x*z*nc + y*s;  r[12] = 0;
        r[1] = y*x*nc + z*s;  r[5] = y*y*nc + c;     r[9]  = y*z*nc - x*s;  r[13] = 0;
        r[2] = z*x*nc - y*s;  r[6] = z*y*nc + x*s;   r[10] = z*z*nc + c;    r[14] = 0;
        r[3] = 0;             r[7] = 0;               r[11] = 0;             r[15] = 1;
        multMatrix(r);
    }

    static void scalef(float x, float y, float z) {
        float s[16];
        loadIdentity(s);
        s[0] = x; s[5] = y; s[10] = z;
        multMatrix(s);
    }

    static void orthof(float left, float right, float bottom, float top, float nearVal, float farVal) {
        float m[16];
        memset(m, 0, 64);
        m[0]  = 2.0f / (right - left);
        m[5]  = 2.0f / (top - bottom);
        m[10] = -2.0f / (farVal - nearVal);
        m[12] = -(right + left) / (right - left);
        m[13] = -(top + bottom) / (top - bottom);
        m[14] = -(farVal + nearVal) / (farVal - nearVal);
        m[15] = 1.0f;
        multMatrix(m);
    }

    static void multMatrix(const float* m) {
        float tmp[16];
        multiply(tmp, current(), m);
        memcpy(current(), tmp, 64);
        _mvpDirty = true;
    }

    // Get current modelview matrix
    static const float* getModelView() { return _mvStack[_mvDepth]; }

    // Get current projection matrix
    static const float* getProjection() { return _projStack[_projDepth]; }

    // Get combined MVP matrix (computed lazily)
    static const float* getMVP() {
        if (_mvpDirty) {
            multiply(_mvp, _projStack[_projDepth], _mvStack[_mvDepth]);
            _mvpDirty = false;
        }
        return _mvp;
    }

    // For glGetFloatv replacement
    static void getMatrix(int pname, float* out) {
        if (pname == 0x0BA6) // GL_MODELVIEW_MATRIX
            memcpy(out, _mvStack[_mvDepth], 64);
        else if (pname == 0x0BA7) // GL_PROJECTION_MATRIX
            memcpy(out, _projStack[_projDepth], 64);
    }

private:
    static const int MAX_DEPTH = 32;

    static float* current() {
        return (_mode == MODELVIEW) ? _mvStack[_mvDepth] : _projStack[_projDepth];
    }

    static void loadIdentity(float* m) {
        memset(m, 0, 64);
        m[0] = 1; m[5] = 1; m[10] = 1; m[15] = 1;
    }

    static void multiply(float* out, const float* a, const float* b) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                out[j*4+i] = a[0*4+i]*b[j*4+0] + a[1*4+i]*b[j*4+1]
                           + a[2*4+i]*b[j*4+2] + a[3*4+i]*b[j*4+3];
            }
        }
    }

    static Mode _mode;
    static int _mvDepth, _projDepth;
    static float _mvStack[MAX_DEPTH][16];
    static float _projStack[MAX_DEPTH][16];
    static float _mvp[16];
    static bool _mvpDirty;
};

#endif
