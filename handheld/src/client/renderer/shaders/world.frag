#version 120

uniform sampler2D tex;
uniform bool      useTexture;
uniform bool      useAlphaTest;
uniform float     alphaTestRef;
uniform bool      useFog;

void main() {
    vec4 color = gl_Color;

    if (useTexture) {
        color *= texture2D(tex, gl_TexCoord[0].xy);
    }

    if (useAlphaTest && color.a < alphaTestRef) {
        discard;
    }

    if (useFog) {
        // GL_LINEAR fog: factor = (end - dist) / (end - start), clamped 0..1
        float denom = gl_Fog.end - gl_Fog.start;
        float fogFactor = clamp((gl_Fog.end - gl_FogFragCoord) / (denom + 0.001), 0.0, 1.0);
        gl_FragColor = mix(gl_Fog.color, color, fogFactor);
    } else {
        gl_FragColor = color;
    }
}
