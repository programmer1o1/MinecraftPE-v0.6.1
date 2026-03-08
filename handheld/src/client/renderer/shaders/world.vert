#version 120

void main() {
    gl_Position    = gl_ModelViewProjectionMatrix * gl_Vertex;
    gl_FrontColor  = gl_Color;
    gl_TexCoord[0] = gl_MultiTexCoord0;
    // Store eye-space depth for linear fog (absolute value of the z/w clip-space)
    gl_FogFragCoord = abs(gl_Position.z / gl_Position.w);
}
