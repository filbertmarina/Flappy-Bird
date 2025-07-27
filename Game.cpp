#include "Engine.h"
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <windows.h>
#include <vector>

//
//  You are free to modify this file
//

//  is_key_pressed(int button_vk_code) - check if a key is pressed,
//                                       use keycodes (VK_SPACE, VK_RIGHT, VK_LEFT, VK_UP, VK_DOWN, 'A', 'B')
//
//  get_cursor_x(), get_cursor_y() - get mouse cursor position
//  is_mouse_button_pressed(int button) - check if mouse button is pressed (0 - left button, 1 - right button)
//  clear_buffer() - set all pixels in buffer to 'black'
//  is_window_active() - returns true if window is active
//  schedule_quit_game() - quit game after act()


bool game_started = false;
int score = 0;
bool first_start = true;

HWND hwnd = NULL;

typedef struct {
    float x, y;
    float a; // ускорение
    int size;
    uint32_t color;
} Bird;

Bird bird;

typedef struct {
    float x;
    float gapY; // координата прохода
    int width;
    int gapHeight;
    bool passed;
} Obstacle;

#define MAX_OBSTACLES 4
Obstacle obstacles[MAX_OBSTACLES];
std::vector<int> list(MAX_OBSTACLES);
size_t index = 3;

float gravity = 630.0f;
float flap_strength = -280.0f;
float obstacle_speed = 60.0f;
float obstacle_spacing = 500.0f;

HFONT g_font = NULL;
HBITMAP g_bitmap = NULL;
uint32_t* g_pixels = NULL;

void initialize()
{
    bird.x = SCREEN_WIDTH / 4;
    bird.y = SCREEN_HEIGHT / 2;
    bird.a = 0;
    bird.size = 30; 
    bird.color = 0xFFFF00; // желтый

    for (int i = 0; i < MAX_OBSTACLES; i++)
    {
        obstacles[i].x = SCREEN_WIDTH + i * obstacle_spacing;
        obstacles[i].width = 100; 
        obstacles[i].gapHeight = 150;
        obstacles[i].gapY = rand() % (SCREEN_HEIGHT - obstacles[i].gapHeight);
        obstacles[i].passed = false;
    }
    index = 3;

    g_font = CreateFont(24, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        ANTIALIASED_QUALITY, FF_DONTCARE, TEXT("Arial"));

    int g_textWidth = 220;
    int g_textHeight = 30;
    HDC hdc = GetDC(NULL);
    g_bitmap = CreateCompatibleBitmap(hdc, g_textWidth, g_textHeight);
    ReleaseDC(NULL, hdc);

    g_pixels = (uint32_t*)malloc(g_textWidth * g_textHeight * 4);
}


void check_start_game()
{
    if (!game_started && is_key_pressed(VK_RETURN))
    {
        game_started = true;
        score = 0;
        bird.x = SCREEN_WIDTH / 4;
        bird.y = SCREEN_HEIGHT / 2;
        bird.a = 0;

        for (int i = 0; i < MAX_OBSTACLES; i++)
        {
            obstacles[i].x = SCREEN_WIDTH + i * obstacle_spacing;
            obstacles[i].gapY = rand() % (SCREEN_HEIGHT - obstacles[i].gapHeight);
            obstacles[i].passed = false;
        }
        first_start = false;
    }
    
    if (is_key_pressed(VK_ESCAPE))
    {
        schedule_quit_game();
    }
}


void act(float dt)
{
    check_start_game();

    if (!game_started)
        return;

    if (is_key_pressed(VK_ESCAPE))
        schedule_quit_game();

    if (is_key_pressed(VK_SPACE) || is_mouse_button_pressed(0))
    {
        bird.a = flap_strength; // прыжок
    }

    bird.a += gravity * dt;
    bird.y += bird.a * dt;

    //столкновение с землей и потолком
    if (bird.y < 0)
    {
        bird.y = 0;
        game_started = false;
    }
    if (bird.y > SCREEN_HEIGHT - bird.size)
    {
        bird.y = SCREEN_HEIGHT - bird.size;
        game_started = false;
    }

    //перезапись препятствий
    for (int i = 0; i < MAX_OBSTACLES; i++)
    {
        obstacles[i].x -= obstacle_speed * dt;

        if (obstacles[i].x + obstacles[i].width < 0)
        {
            obstacles[i].x = obstacles[index].x + (int)obstacle_spacing;
            index = (index + 1) % list.size();
            obstacles[i].gapY = rand() % (SCREEN_HEIGHT - obstacles[i].gapHeight);
            obstacles[i].passed = false;
        }

        //столкновение с препятствиями
        if (bird.x + bird.size > obstacles[i].x && bird.x < obstacles[i].x + obstacles[i].width)
        {
            if (bird.y < obstacles[i].gapY || bird.y + bird.size > obstacles[i].gapY + obstacles[i].gapHeight)
                game_started = false;
        }

        //подсчет очков
        if (!obstacles[i].passed && bird.x > obstacles[i].x + obstacles[i].width + 1)
        {
            obstacles[i].passed = true;
            score++;
        }
    }
}


void draw_text(int x, int y, const char* text, uint32_t color, int g_textWidth, int g_textHeight)
{
    if (!g_font || !g_bitmap || !g_pixels) return;

    HDC hdcMem = CreateCompatibleDC(NULL);
    if (!hdcMem) return;

    HDC hdcScreen = GetDC(NULL);
    HBITMAP oldBmp = (HBITMAP)SelectObject(hdcMem, g_bitmap);
    HGDIOBJ oldFont = SelectObject(hdcMem, g_font);


    HBRUSH hbrBk = (HBRUSH)GetStockObject(BLACK_BRUSH);
    RECT rect = { 0, 0, g_textWidth, g_textHeight };
    FillRect(hdcMem, &rect, hbrBk);

  
    COLORREF textColor = RGB((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF);
    SetTextColor(hdcMem, textColor);
    SetBkMode(hdcMem, TRANSPARENT);


    RECT textRect = { 0, 0, g_textWidth, g_textHeight };
    DrawTextA(hdcMem, text, -1, &textRect, DT_LEFT | DT_TOP);

 
    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = g_textWidth;
    bmi.bmiHeader.biHeight = -g_textHeight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    GetDIBits(hdcMem, g_bitmap, 0, g_textHeight, g_pixels, &bmi, DIB_RGB_COLORS);

    for (int row = 0; row < g_textHeight; row++)
    {
        int bufY = y + row;
        if (bufY < 0 || bufY >= SCREEN_HEIGHT) continue;

        for (int col = 0; col < g_textWidth; col++)
        {
            int bufX = x + col;
            if (bufX < 0 || bufX >= SCREEN_WIDTH) continue;

            uint32_t pixel = g_pixels[row * g_textWidth + col];
            buffer[bufY][bufX] = pixel;
        }
    }

    SelectObject(hdcMem, oldBmp);
    SelectObject(hdcMem, oldFont);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

void draw()
{
    //меню
    if (!game_started) {
        const char* msg;
        if (first_start)
            msg = "Press Enter to Play";
        else
            msg = "Press Enter to Restart";
        index = 3;
        int msg_x = SCREEN_WIDTH / 2 - 210 / 2;
        int msg_y = SCREEN_HEIGHT / 2;
        draw_text(msg_x, msg_y, msg, 0xFFFFFFFF, 210, 25);
        return;
    }

    uint32_t backgroundColor = 0xFF87CEFA; // голубой
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            buffer[y][x] = backgroundColor;
        }
    }
    // земля
    int ground_height = 50; 
    uint32_t ground_color = 0xFF8B4513; // коричневый
    for (int y = SCREEN_HEIGHT - ground_height; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            buffer[y][x] = ground_color;
        }
    }

    //облако
    static bool clouds_initialized = false;
    static std::vector<std::pair<float, float>> clouds;
    const float cloudSpeed = 30.0f; 

    if (!clouds_initialized) {
        int cloudCount = 5;
        for (int i = 0; i < cloudCount; i++) {
            float cx = rand() % (SCREEN_WIDTH - 100);
            float cy = rand() % 300; 
            clouds.push_back({ cx, cy });
        }
        clouds_initialized = true;
    }

  
    for (auto& c : clouds) {
        c.first -= cloudSpeed * 0.016f; 
        if (c.first + 50 < 0) {
            c.first = SCREEN_WIDTH + rand() % 50;
            c.second = rand() % 50;
        }
    }

 
    for (auto& c : clouds) {
        int cx = (int)c.first;
        int cy = (int)c.second;
        int radius = 30;
        for (int angle = 0; angle < 360; angle += 30) {
            int ox = (int)(cx + cosf(angle * 3.14159f / 180.0f) * radius);
            int oy = (int)(cy + sinf(angle * 3.14159f / 180.0f) * radius);
            for (int y_off = -radius; y_off <= radius; y_off++) {
                for (int x_off = -radius; x_off <= radius; x_off++) {
                    int px = ox + x_off;
                    int py = oy + y_off;
                    if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                        if (x_off * x_off + y_off * y_off <= radius * radius) {
                            buffer[py][px] = 0xFFFFFFFF; // белый
                        }
                    }
                }
            }
        }
    }


    for (int i = 0; i < MAX_OBSTACLES; i++)
    {
        Obstacle* obs = &obstacles[i];

  
        obs->x -= obstacle_speed * 0.016f;

        if (obs->x + obs->width < 0)
        {
            obs->x = obstacles[index].x + (int)obstacle_spacing;
            index = (index + 1) % list.size();
            obs->gapY = rand() % (SCREEN_HEIGHT - obs->gapHeight);
            obs->passed = false;
        }

   
        for (int y = 0; y < (int)obs->gapY; y++)
        {
            for (int x = 0; x < obs->width; x++)
            {
                int drawX = (int)obs->x + x;
                if (drawX < 0 || drawX >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT)
                    continue;
                buffer[y][drawX] = 0xFF00FF00; // зеленый
            }
        }
        for (int y = (int)(obs->gapY + obs->gapHeight); y < SCREEN_HEIGHT; y++)
        {
            for (int x = 0; x < obs->width; x++)
            {
                int drawX = (int)obs->x + x;
                if (drawX < 0 || drawX >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT)
                    continue;
                buffer[y][drawX] = 0xFF00FF00; // зеленый
            }
        }

      
        if (bird.x + bird.size > obs->x && bird.x < obs->x + obs->width)
        {
            if (bird.y < obs->gapY || bird.y + bird.size > obs->gapY + obs->gapHeight)
                game_started = false;
        }
        if (!obs->passed && bird.x > obs->x + obs->width + 1)
        {
            obs->passed = true;
            score++;
        }
        int startX = (int)obs->x;
        int endX = (int)obs->x + obs->width - 1;

        int topY = 0;
        int bottomY = (int)obs->gapY;
        int bottomGapY = (int)obs->gapY + obs->gapHeight;
        int bottomYLimit = SCREEN_HEIGHT - 50; 

        uint32_t outline_color = 0xFF000000;
 
        for (int x = startX; x <= endX; x++) {
            if (x >= 0 && x < SCREEN_WIDTH) {
                // верхняя граница
                if (topY >= 0 && topY < SCREEN_HEIGHT)
                    buffer[topY][x] = outline_color;
                // нижняя граница верхнего прямоугольника
                if (bottomY >= 0 && bottomY < SCREEN_HEIGHT)
                    buffer[bottomY - 1][x] = outline_color;
            }
        }

     
        for (int x = startX; x <= endX; x++) {
            if (x >= 0 && x < SCREEN_WIDTH) {
                // верхняя граница нижнего 
                if (bottomGapY >= 0 && bottomGapY < SCREEN_HEIGHT)
                    buffer[bottomGapY][x] = outline_color;
                // нижняя граница
                if (bottomYLimit >= 0 && bottomYLimit < SCREEN_HEIGHT)
                    buffer[bottomYLimit][x] = outline_color;
            }
        }

       
        for (int y = 0; y <= bottomY; y++) {
            if (startX >= 0 && startX < SCREEN_WIDTH)
                buffer[y][startX] = outline_color;
            if (endX >= 0 && endX < SCREEN_WIDTH)
                buffer[y][endX] = outline_color;
        }
        for (int y = bottomGapY; y <= bottomYLimit; y++) {
            if (startX >= 0 && startX < SCREEN_WIDTH)
                buffer[y][startX] = outline_color;
            if (endX >= 0 && endX < SCREEN_WIDTH)
                buffer[y][endX] = outline_color;
        }
    }

   
    int bird_x = (int)(bird.x);
    int bird_y = (int)(bird.y);

    float max_tilt_deg = 30.0f;
    float tilt_deg = (bird.a / gravity) * max_tilt_deg;
    if (tilt_deg > max_tilt_deg) tilt_deg = max_tilt_deg;
    if (tilt_deg < -max_tilt_deg) tilt_deg = -max_tilt_deg;
    float tilt_rad = tilt_deg * 3.14159f / 180.0f;

    auto shift_point = [&](int px, int py) {
        int cx = bird_x + bird.size / 2;
        int cy = bird_y + bird.size / 2;
        int dx = px - cx;
        int dy = py - cy;
        int new_x = (int)(cx + dx * cosf(tilt_rad) - dy * sinf(tilt_rad));
        int new_y = (int)(cy + dx * sinf(tilt_rad) + dy * cosf(tilt_rad));
        return std::pair<int, int>(new_x, new_y);
        };

   
    for (int y = 0; y < bird.size; y++)
    {
        int drawY = bird_y + y;
        if (drawY < 0 || drawY >= SCREEN_HEIGHT) continue;
        for (int x = 0; x < bird.size; x++)
        {
            int drawX = bird_x + x;
            if (drawX < 0 || drawX >= SCREEN_WIDTH) continue;
            auto p = shift_point(drawX, drawY);
            if (p.first >= 0 && p.first < SCREEN_WIDTH && p.second >= 0 && p.second < SCREEN_HEIGHT)
                buffer[p.second][p.first] = bird.color;
        }
    }

    //глаз
    int eye_x = bird_x + bird.size / 2; 
    int eye_y = bird_y + bird.size / 2; 

    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            buffer[eye_y + i][eye_x + j] = 0xFF000000;
        }
    }
    
    float offset = 0;
    if (bird.a > 0.1f) offset = -1; // падает - вниз
    else if (bird.a < -0.1f) offset = 1; // летит вверх - вверх
    else offset = 0;

   
    int beak_length = 7;  
    int beak_height = 4;   
    int tail_length = 11;  
    int tail_height = 6;   


    int beak_x = bird_x + bird.size;
    int beak_y_top = bird_y + bird.size / 3 + (int)offset;
    int beak_y_bottom = bird_y + bird.size / 2 + (int)offset;

    
    for (int i = 0; i < beak_length; i++)
    {
        int x = beak_x + i;
        int y_top = beak_y_top - (i * beak_height) / beak_length;
        int y_bottom = beak_y_bottom + (i * beak_height) / beak_length;

        auto p_top = shift_point(x, y_top);
        auto p_bottom = shift_point(x, y_bottom);

        if (p_top.first >= 0 && p_top.first < SCREEN_WIDTH && p_top.second >= 0 && p_top.second < SCREEN_HEIGHT)
            buffer[p_top.second][p_top.first] = 0xFFFFFF00;
        if (p_bottom.first >= 0 && p_bottom.first < SCREEN_WIDTH && p_bottom.second >= 0 && p_bottom.second < SCREEN_HEIGHT)
            buffer[p_bottom.second][p_bottom.first] = 0xFFFFFF00;
    }

    //хвост
    int tail_x = bird_x; 
    int tail_y_top = bird_y + bird.size / 4 + (int)offset;
    int tail_y_bottom = bird_y + 3 * bird.size / 4 + (int)offset;

    for (int i = 0; i < tail_length; i++)
    {
        int x = tail_x - i;
        int y_top = tail_y_top + (i * tail_height) / tail_length;
        int y_bottom = tail_y_bottom - (i * tail_height) / tail_length;

        auto p_top = shift_point(x, y_top);
        auto p_bottom = shift_point(x, y_bottom);

        if (p_top.first >= 0 && p_top.first < SCREEN_WIDTH && p_top.second >= 0 && p_top.second < SCREEN_HEIGHT)
            buffer[p_top.second][p_top.first] = 0xFFFFFF00;
        if (p_bottom.first >= 0 && p_bottom.first < SCREEN_WIDTH && p_bottom.second >= 0 && p_bottom.second < SCREEN_HEIGHT)
            buffer[p_bottom.second][p_bottom.first] = 0xFFFFFF00;
    }

    // отображение счета
    char score_text[50];
    sprintf_s(score_text, "Score: %d", score);
    draw_text(10, 10, score_text, 0xFFFFFFFF, 100, 25);
}

void finalize()
{
    if (g_font) DeleteObject(g_font);
    if (g_bitmap) DeleteObject(g_bitmap);
    if (g_pixels) free(g_pixels);
    clear_buffer();
}

int main()
{
    initialize();

    hwnd = FindWindow("GameTemplateWndClass", "Game");
    while (is_window_active())
    {
        float dt = 0.016f;
       // check_start_game();

        act(dt);
        draw();
    }

    finalize();
    return 0;
}