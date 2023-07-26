#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <raylib.h>

#if defined(WIN32) || defined(_WIN32)
// To run the application with the external GPU, https://stackoverflow.com/a/39047129/2073225
__declspec(dllexport) int64_t NvOptimusEnablement = 1;
__declspec(dllexport) int32_t AmdPowerXpressRequestHighPerformance = 1;
#endif


/* Change Log:
 * >> Initial
 *  185 FPS w/ NVIDIA GTX 960M
 *
*/


float rand_float(void)
{
    return (float)rand()/(float)RAND_MAX;
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
    // Image image = GenImageCellular(texture_width, texture_height, texture_height/6); //
    Image image = GenImageColor(texture_width, texture_height, BLACK);
    for (int y = 0; y < texture_height*3/4; ++y) {
        for (int x = 0; x < texture_width*3/4; ++x) {
            uint8_t v = rand_float()*255.0f;
            Color color = { v, v, v, 255 };
            ImageDrawPixel(&image, x, y, color);
        }
    }
    RenderTexture2D state[2];

    state[0] = LoadRenderTexture(texture_width, texture_height);
    SetTextureWrap(state[0].texture, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(state[0].texture, TEXTURE_FILTER_BILINEAR);
    UpdateTexture(state[0].texture, image.data);

    state[1] = LoadRenderTexture(texture_width, texture_height);
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
            ClearBackground(BLACK);
            BeginShaderMode(shader);
                DrawTexture(state[i].texture, 0, 0, WHITE);
            EndShaderMode();
        EndTextureMode();

        BeginDrawing();
            ClearBackground(BLACK);
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
