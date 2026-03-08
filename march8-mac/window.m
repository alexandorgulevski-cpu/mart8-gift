#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreText/CoreText.h>
#import <ImageIO/ImageIO.h>
#import <sys/time.h>
#import <stdlib.h>
#import <stdio.h>
#import <unistd.h>
#import <time.h>

// Константы
#define MAX_PARTICLES 150
#define PROGRAM_LIFETIME 39

// Структура частицы салюта
typedef struct {
    int x, y;
    int startY;
    int vx, vy;
    int active;
    float r, g, b;
    int life;
} Particle;

// Глобальные переменные
static Particle particles[MAX_PARTICLES];
static int screenWidth, screenHeight;
static float messageOffset = 0;
static int programEnded = 0;
static CGImageRef backgroundImage = NULL;
static time_t startTime;

// Текст с эмодзи
static const char *message[] = {
    "С 8 марта, наш главный разработчик счастья! 🎉",
    "",
    "С праздником, наша красотка! Хочется пожелать тебе, чтобы твоя операционная система",
    "всегда работала на «отлично», чтобы все жизненные задачи решались парой строчек кода,",
    "а любой деплой проходил успешно.",
    "",
    "Желаю тебе никогда не зависать, всегда быстро рендерить улыбку и иметь бесконечный",
    "запас прочности. Пусть твой внутренний firewall пропускает только хорошие эмоции,",
    "а все неприятности получают Permission denied.",
    "",
    "Ты — эксклюзивный экземпляр, который прошел все тесты на «отлично». Пусть твое",
    "настроение всегда имеет версию Premium, а жизнь будет самым красивым фронтендом,",
    "который мы когда-либо видели.",
    "",
    "Ты просто великолепна, помни об этом. Твой код лучший! И просто знай:",
    "ты самая лучшая девушка на свете. ❤️",
    NULL
};

// Подсчет строк текста
int messageLines(void) {
    int count = 0;
    while (message[count] != NULL) count++;
    return count;
}

// Проверка файла
int FileExists(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

// Загрузка фонового изображения
void LoadBackgroundImage(void) {
    if (!FileExists("assets/background.jpg")) {
        return;
    }
    
    CFStringRef path = CFStringCreateWithCString(NULL, "assets/background.jpg", kCFStringEncodingUTF8);
    CFURLRef url = CFURLCreateWithFileSystemPath(NULL, path, kCFURLPOSIXPathStyle, false);
    
    CGImageSourceRef source = CGImageSourceCreateWithURL(url, NULL);
    if (source) {
        backgroundImage = CGImageSourceCreateImageAtIndex(source, 0, NULL);
        CFRelease(source);
    }
    
    CFRelease(url);
    CFRelease(path);
}

// Запуск видео с принудительным разрешением
void StartVideoAndImage(void) {
    char cmd[512];
    
    if (FileExists("assets/video.mp4")) {
        char path[512];
        realpath("assets/video.mp4", path);
        
        // Способ 1: Снимаем карантин с файла (чтобы не спрашивал)
        char quarantineCmd[512];
        sprintf(quarantineCmd, "xattr -d com.apple.quarantine \"%s\"", path);
        system(quarantineCmd);
        
        // Способ 2: Запускаем Chrome с флагом --allow-file-access-from-files
        sprintf(cmd, 
            "osascript -e 'tell application \"Google Chrome\" to activate' "
            "-e 'tell application \"Google Chrome\" to open location \"file://%s\"' "
            "-e 'tell application \"Google Chrome\" to set fullscreen of first window to true' "
            "-e 'delay 1' "
            "-e 'tell application \"System Events\" to keystroke return'",  // Имитируем нажатие Enter
            path);
        system(cmd);
    }
    
    sleep(2);
    
    if (FileExists("assets/card.jpg")) {
        sprintf(cmd, "open assets/card.jpg");
        system(cmd);
    }
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
        case 0: p->r = 1; p->g = 0; p->b = 0; break;
        case 1: p->r = 1; p->g = 0.65; p->b = 0; break;
        case 2: p->r = 1; p->g = 1; p->b = 0; break;
        case 3: p->r = 0; p->g = 1; p->b = 0; break;
        case 4: p->r = 0; p->g = 1; p->b = 1; break;
        case 5: p->r = 1; p->g = 0; p->b = 1; break;
        case 6: p->r = 1; p->g = 1; p->b = 1; break;
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
                particles[j].r = (float)(rand() % 256) / 255.0;
                particles[j].g = (float)(rand() % 256) / 255.0;
                particles[j].b = (float)(rand() % 256) / 255.0;
                break;
            }
        }
    }
}

void UpdateParticles(void) {
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
    
    // Двигаем текст ВВЕРХ
    messageOffset -= 0.8;
    
    // Сброс когда весь текст ушел
    if (messageOffset < -2000) {
        messageOffset = 0;
    }
}

// View для рисования
@interface RenderView : NSView
@end

@implementation RenderView

- (BOOL)canBecomeKeyView {
    return YES;
}

- (BOOL)acceptsFirstResponder {
    return YES;
}

- (void)keyDown:(NSEvent *)event {
    if ([event keyCode] == 53) { // ESC
        system("killall afplay");
        [NSApp terminate:nil];
    }
}

- (void)drawRect:(NSRect)dirtyRect {
    CGContextRef ctx = [[NSGraphicsContext currentContext] CGContext];
    
    // Рисуем фон
    if (backgroundImage) {
        CGRect rect = CGRectMake(0, 0, screenWidth, screenHeight);
        CGContextDrawImage(ctx, rect, backgroundImage);
        
        // Затемняем фон на 15%
        CGContextSetRGBFillColor(ctx, 0, 0, 0, 0.15);
        CGContextFillRect(ctx, rect);
    } else {
        CGContextSetRGBFillColor(ctx, 0, 0, 0, 1);
        CGContextFillRect(ctx, CGRectMake(0, 0, screenWidth, screenHeight));
    }
    
    // Рисуем салют
    for(int i = 0; i < MAX_PARTICLES; i++) {
        if(particles[i].active) {
            CGContextSetRGBFillColor(ctx, particles[i].r, particles[i].g, particles[i].b, 1);
            
            int size = particles[i].life > 50 ? 5 : (particles[i].life > 20 ? 3 : 2);
            CGContextFillEllipseInRect(ctx, CGRectMake(
                particles[i].x - size,
                screenHeight - particles[i].y - size,
                size * 2, size * 2
            ));
        }
    }
    
    // Рисуем текст снизу вверх
    int lineHeight = 42;
    int startY = -500;
    int totalLines = messageLines();
    
    for(int i = 0; i < totalLines; i++) {
        int messageIndex = totalLines - 1 - i;
        float y = startY + i * lineHeight - messageOffset;
        
        if (y > -200 && y < screenHeight + 200) {
            NSString *text = [NSString stringWithUTF8String:message[messageIndex]];
            
            NSDictionary *attrs = @{
                NSFontAttributeName: [NSFont fontWithName:@"Helvetica" size:36],
                NSForegroundColorAttributeName: [NSColor whiteColor],
                NSStrokeColorAttributeName: [NSColor blackColor],
                NSStrokeWidthAttributeName: @-3.0
            };
            
            [text drawAtPoint:NSMakePoint(50, y) withAttributes:attrs];
        }
    }
}

@end

// App Delegate
@interface AppDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate>
@property (strong) NSWindow *window;
@property (strong) NSTimer *animationTimer;
@property (strong) NSTimer *exitTimer;
@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    srand(time(NULL));
    
    NSRect screenRect = [[NSScreen mainScreen] frame];
    screenWidth = screenRect.size.width;
    screenHeight = screenRect.size.height;
    
    // Загружаем фон
    LoadBackgroundImage();
    
    // Инициализация частиц
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
    
    // Запускаем музыку
    if (FileExists("assets/music.wav")) {
        system("afplay assets/music.wav &");
    }
    
    // Создаем окно
    self.window = [[NSWindow alloc] initWithContentRect:screenRect
                                              styleMask:NSWindowStyleMaskBorderless
                                                backing:NSBackingStoreBuffered
                                                  defer:NO];
    
    [self.window setLevel:NSMainMenuWindowLevel + 1];
    [self.window setOpaque:YES];
    [self.window setBackgroundColor:[NSColor blackColor]];
    
    RenderView *view = [[RenderView alloc] initWithFrame:screenRect];
    [self.window setContentView:view];
    
    [self.window makeFirstResponder:view];
    [self.window setAcceptsMouseMovedEvents:YES];
    [self.window makeKeyAndOrderFront:nil];
    [self.window setDelegate:self];
    
    // Таймер анимации (60 FPS)
    self.animationTimer = [NSTimer scheduledTimerWithTimeInterval:1.0/60.0
                                                           target:self
                                                         selector:@selector(updateAnimation)
                                                         userInfo:nil
                                                          repeats:YES];
    
    // Таймер выхода (39 секунд)
    self.exitTimer = [NSTimer scheduledTimerWithTimeInterval:PROGRAM_LIFETIME
                                                       target:self
                                                     selector:@selector(exitProgram)
                                                     userInfo:nil
                                                      repeats:NO];
    
    startTime = time(NULL);
}

- (void)updateAnimation {
    UpdateParticles();
    [self.window.contentView setNeedsDisplay:YES];
}

- (void)exitProgram {
    [self.animationTimer invalidate];
    [self.exitTimer invalidate];
    system("killall afplay");
    StartVideoAndImage();
    [NSApp terminate:nil];
}

- (void)windowWillClose:(NSNotification *)notification {
    system("killall afplay");
    [NSApp terminate:nil];
}

@end

int main(void) {
    [NSApplication sharedApplication];
    AppDelegate *delegate = [[AppDelegate alloc] init];
    [NSApp setDelegate:delegate];
    [NSApp run];
    return 0;
}