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
 *>> First pass on shader (2.9 ms, 345 FPS)
 * Removed extra texture lookups on each branch (gpu did not optimize..) (5.0 ms -> 2.95 ms :pog:)
 * Refactored & reformatted the shader (somehow 2.95 ms -> 2.90 ms)
 *>> Using glsl 400 (1.4 ms, 714 FPS)
 * Switched to textureGather, requires glsl 400 (should reduce texture bandwidth by 4) (2.9 ms -> 1.4 ms :pog:)
 * The good news is when I disabled the simulation it was still 1.4 ms :D Maybe the workload is too small to make an
 *  impact somehow, not sure. But I tried to run it on full scale (1600x900) and it took 11.8 ms, success :)
 *>> User interaction
 * Simulation: replay: Left Arrow, next: Right Arrow, simulation per frame +/-: Up/Down Arrow
 * Brush: add/remove: Left/Right Click, size -/+: Q/W, change shape A/S, toggle preview: Middle Click
 *
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

// Draw a rectangle with teCoords from (0,0) to (1,1)
void DrawRectangleUV01(Rectangle rec, Color color)
{
    rlBegin(RL_QUADS);

        rlColor4ub(color.r, color.g, color.b, color.a);

        rlTexCoord2f(0, 0), rlVertex2f(rec.x, rec.y);
        rlTexCoord2f(0, 1), rlVertex2f(rec.x, rec.y + rec.height);
        rlTexCoord2f(1, 1), rlVertex2f(rec.x + rec.width, rec.y + rec.height);
        rlTexCoord2f(1, 0), rlVertex2f(rec.x + rec.width, rec.y);

    rlEnd();
}

Image initial_image;
void init_image(int texture_width, int texture_height) {
//    initial_image = GenImagePerlinNoise(texture_width, texture_height, 0, 0, 5.0f);
//    initial_image = GenImageWhiteNoise(texture_width, texture_height, 0.9f);
//    initial_image = GenImageCellular(texture_width, texture_height, texture_height/6);
    initial_image = GenImageColor(texture_width, texture_height, BLACK);
    for (int y = 0; y < texture_height * 3/4; ++y)
    for (int x = 0; x < texture_width  * 3/4; ++x) {
        uint8_t v = rand_uint8() > 180 ? 255 : 0;
        Color color = { v, v, v, 255 };
        ImageDrawPixel(&initial_image, x, y, color);
    }
	ImageFormat(&initial_image, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);
}

int main(void)
{
    float factor = 100;
    float screen_width = 16 * factor;
    float screen_height = 9 * factor;
    float scale = 1;
    float texture_width = screen_width * scale;
    float texture_height = screen_height * scale;

    InitWindow(screen_width, screen_height, "SmoothLife");
    SetTargetFPS(60);

    Shader simulation_shader = LoadShader(NULL, "./smoothlife.fs");
    if (simulation_shader.id == rlGetShaderIdDefault()) return 1;
    int resolution_loc = GetShaderLocation(simulation_shader, "resolution");
    Vector2 resolution = {texture_width, texture_height};
    SetShaderValue(simulation_shader, resolution_loc, &resolution, SHADER_UNIFORM_VEC2);

    Shader brush_shader = LoadShader(NULL, "./brush.fs");
    if (brush_shader.id == rlGetShaderIdDefault()) return 1;
    int brush_shape_loc = GetShaderLocation(brush_shader, "brush_shape");


    init_image(texture_width, texture_height);

    RenderTexture2D state[2];

    state[0] = LoadRenderTextureWithFormat(texture_width, texture_height, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);
    SetTextureWrap(state[0].texture, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(state[0].texture, TEXTURE_FILTER_BILINEAR);
    UpdateTexture(state[0].texture, initial_image.data);

    state[1] = LoadRenderTextureWithFormat(texture_width, texture_height, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE);
    SetTextureWrap(state[1].texture, TEXTURE_WRAP_REPEAT);
    SetTextureFilter(state[1].texture, TEXTURE_FILTER_BILINEAR);

    char str[256];
    size_t i = 0;
    while (!WindowShouldClose()) {
        // simulation controls
        if (IsKeyPressed(KEY_LEFT))
            UpdateTexture(state[i].texture, initial_image.data);
        if (IsKeyPressed(KEY_RIGHT))
            init_image(texture_width, texture_height),
            UpdateTexture(state[i].texture, initial_image.data);

        static int simulation_per_frame = 1;
        if (IsKeyPressed(KEY_UP))   simulation_per_frame += 1;
        if (IsKeyPressed(KEY_DOWN)) simulation_per_frame = max(1, simulation_per_frame - 1);

        // brush controls
        static Color brush_color = {0, 0, 0, 0};
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))  brush_color = CLITERAL(Color){255, 0, 0, 255};
        if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) brush_color = CLITERAL(Color){  0, 0, 0, 255};

        static float brush_size = 128;
        if (IsKeyDown('Q')) brush_size = max(1, brush_size / 1.02);
        if (IsKeyDown('W')) brush_size = brush_size * 1.02;

        static float brush_shape = 2;
        if (IsKeyDown('A')) brush_shape = max(.01, brush_shape / 1.02);
        if (IsKeyDown('S')) brush_shape = min(100, brush_shape * 1.02);

        static bool show_brush_preview = true;
        if (IsMouseButtonPressed(MOUSE_BUTTON_MIDDLE)) show_brush_preview = !show_brush_preview;


		// brush
        if (brush_color.a != 0) {
            BeginTextureMode(state[i]);
                BeginShaderMode(brush_shader);
                    SetShaderValue(brush_shader, brush_shape_loc, &brush_shape, SHADER_UNIFORM_FLOAT);
                    DrawRectangleUV01(
						CLITERAL(Rectangle) {
                            scale * (GetMouseX() - 0.5f * brush_size),
                            scale * (screen_height - GetMouseY() - 0.5f * brush_size), // invert y because render texture is not inverted
                        	scale * brush_size, scale * brush_size,
						},
                        brush_color
                    );
                EndShaderMode();
            EndTextureMode();
            brush_color.a = 0;
        }


        // simulation
        BeginTextureMode(state[1 - i]);
            BeginShaderMode(simulation_shader);
                DrawTexture(state[i].texture, 0, 0, WHITE);
            EndShaderMode();
        EndTextureMode();
        for (int _ = 0; _ < simulation_per_frame - 1; ++_)
        {
            i = 1 - i;
            BeginTextureMode(state[1 - i]);
                BeginShaderMode(simulation_shader);
                    DrawTexture(state[i].texture, 0, 0, WHITE);
                EndShaderMode();
            EndTextureMode();
        }


        // presentation
        BeginDrawing();
#if 1
            DrawTextureEx(state[1 - i].texture, CLITERAL(Vector2){0}, 0, 1 / scale, WHITE);
#else // debug view
            DrawTextureEx(state[0].texture, CLITERAL(Vector2){0.0f * screen_width}, 0, 0.5f / scalar, WHITE);
            DrawTextureEx(state[1].texture, CLITERAL(Vector2){0.5f * screen_width}, 0, 0.5f / scalar, WHITE);
			DrawText("was src", (i == 0 ? 1 : 3) * screen_width / 4, 0.5f * screen_height, 24, RED);
			DrawText("was dst", (i == 0 ? 3 : 1) * screen_width / 4, 0.5f * screen_height, 24, RED);
#endif

            if (show_brush_preview) {
                BeginShaderMode(brush_shader);
                    BeginBlendMode(BLEND_SUBTRACT_COLORS);
                        SetShaderValue(brush_shader, brush_shape_loc, &brush_shape, SHADER_UNIFORM_FLOAT);
                        DrawRectangleUV01(
                            CLITERAL(Rectangle) {
                                GetMouseX() - 0.5f * brush_size,
                                GetMouseY() - 0.5f * brush_size,
                                brush_size, brush_size,
                            },
                            CLITERAL(Color){255, 255, 255, 255}
                        );
                    EndBlendMode();
                EndShaderMode();
            }

            sprintf_s(
                str, sizeof(str), "%i FPS\n%.3f ms\n%i Per Frame",
                GetFPS(), 1000.0f / (float)GetFPS(), simulation_per_frame
            );
			DrawText(str, 10, 10, 24, LIME);
        EndDrawing();

        i = 1 - i;
    }

    CloseWindow();

    return 0;
}
