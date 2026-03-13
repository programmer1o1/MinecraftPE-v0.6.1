#include "MatrixStack.h"

MatrixStack::Mode MatrixStack::_mode = MatrixStack::MODELVIEW;
int MatrixStack::_mvDepth = 0;
int MatrixStack::_projDepth = 0;
float MatrixStack::_mvStack[MAX_DEPTH][16] = {};
float MatrixStack::_projStack[MAX_DEPTH][16] = {};
float MatrixStack::_mvp[16] = {};
bool MatrixStack::_mvpDirty = true;
