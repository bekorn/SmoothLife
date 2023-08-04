#version 400

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

uniform vec2 resolution;

#define PARAM_SET 2 // 0 = from the paper, 1 = from stackoverflow, 2 = inspired by the constants of the universe
// o = outer, i = inner

#if PARAM_SET == 0
const float ro = 21;
const float ri = ro/3.0;
const float b1 = 0.278;
const float b2 = 0.365;
const float d1 = 0.267;
const float d2 = 0.445;
const float alpha_n = 0.028;
const float alpha_m = 0.147;

#elif PARAM_SET == 1
const float ro = 21;
const float ri = ro/3.0;
// Stolen from https://www.shadertoy.com/view/XtdSDn
const float b1 = 0.257;
const float b2 = 0.336;
const float d1 = 0.365;
const float d2 = 0.549;
const float alpha_n = 0.028;
const float alpha_m = 0.147;

#elif PARAM_SET == 2
const float ro = 12;
const float ri = ro/1.5;
const float b1 = 0.3;
const float b2 = 0.360;
const float d1 = 0.420;
const float d2 = 0.69;
const float alpha_n = 0.049;
const float alpha_m = 0.121;

#endif

#define PI 3.14159265359
// Area = πr^2
const float ai = PI*ri*ri;
const float ao = PI*ro*ro - ai; // actually the area inbetween the ro and ri

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
    return texelFetch(texture0, ivec2(pos), 0).r;
}

mat2x2 grid4(vec2 pos)
{
    // textureGather -> vec4( x0y1, x1y1, x1y0, x0y0 )
    // mat2x2( x0y0, x0y1, x1y0, x1y1 ) for [x][y]
    return mat2x2(textureGather(texture0, (pos + 0.5) / resolution, 0).wxzy);
}

void main()
{
    vec2 center = gl_FragCoord.xy;
    const float ro2 = ro*ro;
    const float ri2 = ri*ri;

    float so = 0;
    float si = 0;
    for (float dx = -ro; dx <= ro; dx += 2)
    for (float dy = -ro; dy <= ro; dy += 2) {
        vec2 offset = vec2(dx, dy);

/* This strategy does not reduce time, even increases it sometimes */
//        vec2 center_of2x2 = offset + 1;
//        vec2 corner_closer_to_origin = center_of2x2 - sign(center_of2x2);
//        if (dot(corner_closer_to_origin, corner_closer_to_origin) > ro2) continue;

        vec2 pos = center + offset;
        mat2x2 texels2x2 = grid4(pos);

        for (int subx = 0; subx <= 1; subx += 1)
        for (int suby = 0; suby <= 1; suby += 1) {
            float val = texels2x2[subx][suby];
            vec2 suboffset = offset + vec2(subx, suby);
            float len2 = dot(suboffset, suboffset);
            if(len2 <= ro2) so += val;
            if(len2 <= ri2) si += val;
        }
    }
    so -= si; // it is actually the sum between ro and ri
    so /= ao;
    si /= ai;
    float q = s(so, si);
    float diff = q * 2 - 1;
    float v = grid(center) + dt * diff;

    finalColor = vec4(v, v, v, 1);
}
