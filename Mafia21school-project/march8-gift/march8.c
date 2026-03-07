#include <windows.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

// Константы
#define MAX_PARTICLES 150
#define PROGRAM_LIFETIME 39 // 39 секунд

// Структура частицы салюта
typedef struct {
    int x, y;
    int startY;
    int vx, vy;
    int active;
    int color;
    int life;
} Particle;

// Глобальные переменные
Particle particles[MAX_PARTICLES];
HWND hwnd;
int messageY;
int screenWidth, screenHeight;
HBITMAP hBackgroundBitmap = NULL;
HBITMAP hBackBufferBitmap = NULL;
HDC hBackBufferDC = NULL;
time_t startTime;
int programEnded = 0;

// Текст с эмодзи
wchar_t *message[] = {
    L"С 8 марта, наш главный разработчик счастья! 🎉",
    L"",
    L"С праздником, наша красотка! Хочется пожелать тебе, чтобы твоя операционная система",
    L"всегда работала на «отлично», чтобы все жизненные задачи решались парой строчек кода,",
    L"а любой деплой проходил успешно.",
    L"",
    L"Желаю тебе никогда не зависать, всегда быстро рендерить улыбку и иметь бесконечный",
    L"запас прочности. Пусть твой внутренний firewall пропускает только хорошие эмоции,",
    L"а все неприятности получают Permission denied.",
    L"",
    L"Ты — эксклюзивный экземпляр, который прошел все тесты на «отлично». Пусть твое",
    L"настроение всегда имеет версию Premium, а жизнь будет самым красивым фронтендом,",
    L"который мы когда-либо видели.",
    L"",
    L"Ты просто великолепна, помни об этом. Твой код лучший! И просто знай:",
    L"ты самая лучшая девушка на свете. ❤️",
    NULL
};

// Проверка существования файла
int FileExists(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

// Запуск видео и картинки
void StartVideoAndImage() {
    // Сначала запускаем видео
    char videoPath[MAX_PATH];
    int hasVideo = 0;
    
    if (FileExists("video.wmv")) {
        strcpy(videoPath, "video.wmv");
        hasVideo = 1;
    } else if (FileExists("video.avi")) {
        strcpy(videoPath, "video.avi");
        hasVideo = 1;
    } else if (FileExists("video.mp4")) {
        strcpy(videoPath, "video.mp4");
        hasVideo = 1;
    } else if (FileExists("video.mov")) {
        strcpy(videoPath, "video.mov");
        hasVideo = 1;
    }
    
    if (hasVideo) {
        char fullPath[MAX_PATH];
        GetFullPathNameA(videoPath, MAX_PATH, fullPath, NULL);
        ShellExecuteA(NULL, "open", fullPath, NULL, NULL, SW_SHOWMAXIMIZED);
        Sleep(1000); // Ждем чтобы видео открылось
    }
    
    // Запускаем картинку после видео
    if (FileExists("card.jpg")) {
        // Ждем 2 секунды чтобы видео успело начаться
        Sleep(2000);
        
        char cardPath[MAX_PATH];
        GetFullPathNameA("card.jpg", MAX_PATH, cardPath, NULL);
        ShellExecuteA(NULL, "open", cardPath, NULL, NULL, SW_SHOWMAXIMIZED);
        
        // Даем время картинке открыться
        Sleep(500);
        
        // Делаем картинку поверх всех окон
        HWND hForeground = GetForegroundWindow();
        if (hForeground && hForeground != hwnd) {
            SetWindowPos(hForeground, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
            SetForegroundWindow(hForeground);
        }
    }
}

// Загрузка фонового изображения
void LoadBackgroundImage() {
    if (!FileExists("background.bmp")) {
        return;
    }
    
    hBackgroundBitmap = (HBITMAP)LoadImageA(NULL, "background.bmp", IMAGE_BITMAP, 
                                            screenWidth, screenHeight, 
                                            LR_LOADFROMFILE | LR_CREATEDIBSECTION);
}

// Создание буфера для двойной буферизации
void CreateBackBuffer() {
    HDC hdc = GetDC(hwnd);
    hBackBufferDC = CreateCompatibleDC(hdc);
    hBackBufferBitmap = CreateCompatibleBitmap(hdc, screenWidth, screenHeight);
    SelectObject(hBackBufferDC, hBackBufferBitmap);
    ReleaseDC(hwnd, hdc);
}

void InitParticle(Particle *p) {
    p->active = 1;
    p->x = rand() % screenWidth;
    p->startY = (rand() % (screenHeight / 2)) + screenHeight / 4;
    p->y = screenHeight;
    p->vy = -(rand() % 10 + 8);
    p->vx = (rand() % 8) - 4;
    p->life = 100 + rand() % 50;
    
    switch(rand() % 7) {
        case 0: p->color = RGB(255, 0, 0); break;
        case 1: p->color = RGB(255, 165, 0); break;
        case 2: p->color = RGB(255, 255, 0); break;
        case 3: p->color = RGB(0, 255, 0); break;
        case 4: p->color = RGB(0, 255, 255); break;
        case 5: p->color = RGB(255, 0, 255); break;
        case 6: p->color = RGB(255, 255, 255); break;
    }
}

void CreateExplosion(int x, int y) {
    for(int i = 0; i < 20; i++) {
        for(int j = 0; j < MAX_PARTICLES; j++) {
            if(!particles[j].active) {
                particles[j].active = 1;
                particles[j].x = x;
                particles[j].y = y;
                particles[j].vx = (rand() % 14) - 7;
                particles[j].vy = (rand() % 14) - 7;
                particles[j].life = 40 + rand() % 60;
                particles[j].color = RGB(rand() % 256, rand() % 256, rand() % 256);
                break;
            }
        }
    }
}

// Функция для рисования текста с обводкой
void DrawTextWithOutline(HDC hdc, int x, int y, wchar_t *text, int length) {
    COLORREF oldColor = SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);
    
    for(int dx = -2; dx <= 2; dx++) {
        for(int dy = -2; dy <= 2; dy++) {
            if(dx != 0 || dy != 0) {
                TextOutW(hdc, x + dx, y + dy, text, length);
            }
        }
    }
    
    SetTextColor(hdc, RGB(255, 255, 255));
    TextOutW(hdc, x, y, text, length);
    SetTextColor(hdc, oldColor);
}

// Проверка, установлен ли шрифт в системе
BOOL IsFontInstalled(const wchar_t* fontName) {
    HDC hdc = GetDC(NULL);
    LOGFONTW lf = {0};
    wcscpy_s(lf.lfFaceName, LF_FACESIZE, fontName);
    HFONT hFont = CreateFontIndirectW(&lf);
    
    BOOL exists = (hFont != NULL);
    
    if (hFont) DeleteObject(hFont);
    ReleaseDC(NULL, hdc);
    
    return exists;
}

// Создание шрифта с умным выбором для эмодзи
HFONT CreateSmartFont(int height, int weight) {
    const wchar_t* fontNames[] = {
        L"Noto Color Emoji",
        L"Apple Color Emoji",
        L"Segoe UI Emoji",
        L"Noto Serif JP"
    };
    
    for (int i = 0; i < 4; i++) {
        if (IsFontInstalled(fontNames[i])) {
            return CreateFontW(height, 0, 0, 0, weight, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                              CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                              DEFAULT_PITCH | FF_DONTCARE, fontNames[i]);
        }
    }
    
    return CreateFontW(height, 0, 0, 0, weight, FALSE, FALSE, FALSE,
                      DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                      CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                      DEFAULT_PITCH | FF_DONTCARE, L"Arial");
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
            if(wParam == VK_ESCAPE) {
                PlaySoundA(NULL, NULL, 0);
                PostQuitMessage(0);
                return 0;
            }
            break;
            
        case WM_SIZE:
            if (hBackBufferBitmap) {
                DeleteObject(hBackBufferBitmap);
                DeleteDC(hBackBufferDC);
            }
            CreateBackBuffer();
            break;
            
        case WM_CREATE:
            if (FileExists("music.wav")) {
                PlaySoundA("music.wav", NULL, SND_FILENAME | SND_LOOP | SND_ASYNC);
            }
            break;
            
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            if (hBackBufferDC) {
                RECT rect = {0, 0, screenWidth, screenHeight};
                HBRUSH blackBrush = CreateSolidBrush(RGB(0, 0, 0));
                FillRect(hBackBufferDC, &rect, blackBrush);
                DeleteObject(blackBrush);
                
                if (hBackgroundBitmap) {
                    HDC memDC = CreateCompatibleDC(hBackBufferDC);
                    SelectObject(memDC, hBackgroundBitmap);
                    BitBlt(hBackBufferDC, 0, 0, screenWidth, screenHeight, memDC, 0, 0, SRCCOPY);
                    DeleteDC(memDC);
                } else {
                    HBRUSH pinkBrush = CreateSolidBrush(RGB(255, 200, 220));
                    FillRect(hBackBufferDC, &rect, pinkBrush);
                    DeleteObject(pinkBrush);
                }
                
                for(int i = 0; i < MAX_PARTICLES; i++) {
                    if(particles[i].active) {
                        HBRUSH particleBrush = CreateSolidBrush(particles[i].color);
                        HPEN particlePen = CreatePen(PS_NULL, 0, RGB(0,0,0));
                        
                        SelectObject(hBackBufferDC, particleBrush);
                        SelectObject(hBackBufferDC, particlePen);
                        
                        int size = particles[i].life > 50 ? 5 : (particles[i].life > 20 ? 3 : 2);
                        
                        Ellipse(hBackBufferDC, particles[i].x - size, particles[i].y - size, 
                                particles[i].x + size, particles[i].y + size);
                        
                        DeleteObject(particleBrush);
                        DeleteObject(particlePen);
                    }
                }
                
                HFONT hFont = CreateSmartFont(30, FW_BOLD);
                SelectObject(hBackBufferDC, hFont);
                
                int lineHeight = 42;
                int y = messageY;
                
                for(int i = 0; message[i] != NULL; i++) {
                    if(y > -50 && y < screenHeight + 50) {
                        DrawTextWithOutline(hBackBufferDC, 50, y, message[i], wcslen(message[i]));
                    }
                    y += lineHeight;
                }
                
                DeleteObject(hFont);
                BitBlt(hdc, 0, 0, screenWidth, screenHeight, hBackBufferDC, 0, 0, SRCCOPY);
            }
            
            EndPaint(hwnd, &ps);
            break;
        }
        
        case WM_DESTROY:
            PlaySoundA(NULL, NULL, 0);
            if (hBackgroundBitmap) {
                DeleteObject(hBackgroundBitmap);
            }
            if (hBackBufferBitmap) {
                DeleteObject(hBackBufferBitmap);
                DeleteDC(hBackBufferDC);
            }
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
    if (!programEnded) {
        time_t currentTime = time(NULL);
        if (currentTime - startTime >= PROGRAM_LIFETIME) {
            programEnded = 1;
            PlaySoundA(NULL, NULL, 0);
            StartVideoAndImage();  // Запускаем видео и картинку
            PostQuitMessage(0);
            return;
        }
    }
    
    for(int i = 0; i < MAX_PARTICLES; i++) {
        if(particles[i].active) {
            particles[i].x += particles[i].vx;
            particles[i].y += particles[i].vy;
            particles[i].vy += 1;
            particles[i].life--;
            
            if(particles[i].y <= particles[i].startY && particles[i].vy < 0 && particles[i].life > 30) {
                CreateExplosion(particles[i].x, particles[i].y);
                particles[i].active = 0;
            }
            
            if(particles[i].life <= 0 || particles[i].y > screenHeight + 200) {
                particles[i].active = 0;
            }
        }
    }
    
    static int counter = 0;
    counter++;
    if(counter % 3 == 0) {
        for(int i = 0; i < 2; i++) {
            for(int j = 0; j < MAX_PARTICLES; j++) {
                if(!particles[j].active) {
                    InitParticle(&particles[j]);
                    break;
                }
            }
        }
    }
    
    messageY -= 1;
    if(messageY < -1500) {
        messageY = screenHeight + 200;
    }
    
    InvalidateRect(hwnd, NULL, FALSE);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    screenWidth = GetSystemMetrics(SM_CXSCREEN);
    screenHeight = GetSystemMetrics(SM_CYSCREEN);
    messageY = screenHeight - 100;
    
    srand(time(NULL));
    startTime = time(NULL);
    
    WNDCLASSW wc = {0};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = L"FireworksClass";
    
    if(!RegisterClassW(&wc)) {
        return 1;
    }
    
    hwnd = CreateWindowExW(
        WS_EX_TOPMOST,
        L"FireworksClass",
        L"Поздравление с 8 марта!",
        WS_POPUP,
        0, 0, screenWidth, screenHeight,
        NULL, NULL, hInstance, NULL
    );
    
    if(!hwnd) {
        return 1;
    }
    
    LoadBackgroundImage();
    CreateBackBuffer();
    
    ShowWindow(hwnd, SW_SHOW);
    SetForegroundWindow(hwnd);
    SetFocus(hwnd);
    
    for(int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].active = 0;
    }
    
    for(int i = 0; i < 30; i++) {
        for(int j = 0; j < MAX_PARTICLES; j++) {
            if(!particles[j].active) {
                InitParticle(&particles[j]);
                break;
            }
        }
    }
    
    SetTimer(hwnd, 1, 33, TimerProc);
    
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return msg.wParam;
}