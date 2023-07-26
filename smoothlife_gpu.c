#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <raylib.h>
#include <rlgl.h>

#if defined(WIN32) || defined(_WIN32)
// To run the application with the external GPU, https://stackoverflow.com/a/39047129/2073225
__declspec(dllexport) int64_t NvOptimusEnablement = 1;
__declspec(dllexport) int32_t AmdPowerXpressRequestHighPerformance = 1;
#endif


/* Change Log:
 *>> Initial (5.4 ms, 185 FPS)
 * run on an NVIDIA GTX 960M
 *>> First pass (5.0 ms, 200 FPS)
 * Removed unnecessary ClearBackgrounds (each pixel will be set to something anyway) (did not effect the FPS tho :/)
 * Changed RenderTextures' format to Grayscale (should reduce texture bandwidth by 4)
*/


float rand_float(void) { return (float)rand()/(float)RAND_MAX; }
uint8_t rand_uint8(void) { return rand() / (RAND_MAX / (1 << 8)); }

// direct copy with minor modification: respects format
RenderTexture2D LoadRenderTextureWithFormat(int width, int height, int format)
{
    RenderTexture2D target = { 0 };

    target.id = rlLoadFramebuffer(width, height);   // Load an empty framebuffer

    if (target.id > 0)
    {
        rlEnableFramebuffer(target.id);

        // Create color texture (default to RGBA)
        target.texture.id = rlLoadTexture(NULL, width, height, format, 1);
        target.texture.width = width;
        target.texture.height = height;
        target.texture.format = format;
        target.texture.mipmaps = 1;

        // Create depth renderbuffer/texture
        target.depth.id = rlLoadTextureDepth(width, height, true);
        target.depth.width = width;
        target.depth.height = height;
        target.depth.format = 19;       //DEPTH_COMPONENT_24BIT?
        target.depth.mipmaps = 1;

        // Attach color texture and depth renderbuffer/texture to FBO
        rlFramebufferAttach(target.id, target.texture.id, RL_ATTACHMENT_COLOR_CHANNEL0, RL_ATTACHMENT_TEXTURE2D, 0);
        rlFramebufferAttach(target.id, target.depth.id, RL_ATTACHMENT_DEPTH, RL_ATTACHMENT_RENDERBUFFER, 0);

        // Check if fbo is complete with attachments (valid)
        if (rlFramebufferComplete(target.id)) TRACELOG(LOG_INFO, "FBO: [ID %i] Framebuffer object created successfully", target.id);

        rlDisableFramebuffer();
    }
    else TRACELOG(LOG_WARNING, "FBO: Framebuffer object can not be created");

    return target;
}

int main(void)
{
    float factor = 100;
    float screen_width = 16*factor;
    float screen_height = 9*factor;
    float scalar = 0.2;
    float texture_width = screen_width*scalar;
    float texture_height = screen_height*scalar;

    InitWindow(screen_width, screen_height, "SmoothLife");

    // Image image = GenImagePerlinNoise(texture_width, texture_height, 0, 0, 5.0f);
    // Image image = GenImageWhiteNoise(texture_width, texture_height, 0.9f);
    // Image image = GenImageCellular(texture_width, texture_height, texture_height/6);
    Image image = GenImageColor(texture_width, texture_height, BLACK);
    for (int y = 0; y < texture_height*3/4; ++y) {
        for (int x = 0; x < texture_width*3/4; ++x) {
            uint8_t v = rand_uint8();
            Color color = { v, v, v, v };
            ImageDrawPixel(&image, x, y, color);
        }
    }
	ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);

    RenderTexture2D state[2];

    state[0] = LoadRenderTextureWithFormat(texture_width, texture_height, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);
    SetTextureWrap(state[0].texture, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(state[0].texture, TEXTURE_FILTER_BILINEAR);
    UpdateTexture(state[0].texture, image.data);

    state[1] = LoadRenderTextureWithFormat(texture_width, texture_height, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);
    SetTextureWrap(state[1].texture, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(state[1].texture, TEXTURE_FILTER_BILINEAR);

    Shader shader = LoadShader(NULL, "./smoothlife.fs");
    Vector2 resolution = {texture_width, texture_height};
    int resolution_loc = GetShaderLocation(shader, "resolution");
    SetShaderValue(shader, resolution_loc, &resolution, SHADER_UNIFORM_VEC2);

    char str[256];
    size_t i = 0;
    while (!WindowShouldClose()) {
        BeginTextureMode(state[1 - i]);
            BeginShaderMode(shader);
                DrawTexture(state[i].texture, 0, 0, WHITE);
            EndShaderMode();
        EndTextureMode();

        BeginDrawing();
            DrawTextureEx(state[1 - i].texture, CLITERAL(Vector2){0}, 0, 1/scalar, WHITE);
            // DrawTexture(state[i].texture, 0, 0, WHITE);

            sprintf_s(str, sizeof(str), "%4i FPS | %.3f ms", GetFPS(), 1000.0f / (float)GetFPS());
			DrawText(str, 10, 10, 24, RED);
        EndDrawing();

        i = 1 - i;
    }

    CloseWindow();

    return 0;
}
