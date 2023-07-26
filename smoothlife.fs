#version 330

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

uniform vec2 resolution;

// o = outer, i = inner
float ro = 21;
float ri = ro/3.0;

#define PI 3.14159265359
// Area = πr^2
float ai = PI*ri*ri;
float ao = PI*ro*ro - ai; // actually the area inbetween the ro and ri

#if 1
// Stolen from https://www.shadertoy.com/view/XtdSDn
float b1 = 0.257;
float b2 = 0.336;
float d1 = 0.365;
float d2 = 0.549;
float alpha_n = 0.028;
float alpha_m = 0.147;
#else
float b1 = 0.278;
float b2 = 0.365;
float d1 = 0.267;
float d2 = 0.445;
float alpha_n = 0.028;
float alpha_m = 0.147;
#endif

float dt = 0.05;

float sigma(float x, float a, float alpha)
{
    return 1.0/(1.0 + exp(-(x - a)*4.0/alpha));
}

float sigma_n(float x, float a, float b)
{
    return sigma(x, a, alpha_n)*(1.0 - sigma(x, b, alpha_n));
}

float sigma_m(float x, float y, float m)
{
    return x*(1 - sigma(m, 0.5, alpha_m)) + y*sigma(m, 0.5, alpha_m);
}

float s(float n, float m)
{
    return sigma_n(n, sigma_m(b1, d1, m), sigma_m(b2, d2, m));
}

float grid(vec2 pos)
{
    return texture(texture0, pos / resolution).r;
}

void main()
{
    vec2 center = gl_FragCoord.xy;
    float so = 0;
    float si = 0;
    float ro2 = ro*ro;
    float ri2 = ri*ri;

    for (float dy = -ro; dy <= ro; ++dy)
    for (float dx = -ro; dx <= ro; ++dx) {
        vec2 offset = vec2(dx, dy);
        float len2 = dot(offset, offset);

        vec2 pos = center + offset;
        float val = grid(pos);

        if(len2 <= ro2) so += val;
        if(len2 <= ri2) si += val;
    }
    so -= si; // it is actually the sum between ro and ri
    so /= ao;
    si /= ai;
    float q = s(so, si);
    float diff = q * 2 - 1;
    float v = grid(center) + dt * diff;

    finalColor = vec4(v, v, v, 1);
}
