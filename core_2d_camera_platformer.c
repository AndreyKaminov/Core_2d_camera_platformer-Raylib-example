#include <stdio.h> // Для функций форматирования строк
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h> // Для типа bool
/*******************************************************************************************
*
*   raylib [core] example - 2D Camera platformer + Dust Particles + JumpThru Platforms
*
********************************************************************************************/

#include "raylib.h" // Подключение библиотеки raylib
#include "raymath.h" // Подключение библиотеки raymath
#define PLAYER_SPRITE_PATH "resources/player.png"
Texture2D playerTexture;

// --- Константы игрока ---
#define G 950 // Гравитация
#define PLAYER_JUMP_SPD 350.0f // Скорость прыжка игрока
#define PLAYER_MAX_SPEED 500.0f // Максимальная горизонтальная скорость игрока
#define PLAYER_ACCELERATION 400.0f // Ускорение игрока
#define PLAYER_DECELERATION 450.0f // Замедление игрока
#define PLAYER_MAX_JUMP_TIME 0.40f // Максимальное время удержания прыжка
#define PLAYER_JUMP_HOLD_FORCE 500.0f // Сила удержания прыжка
#define PLAYER_DASH_SPEED 900.0f // Скорость рывка
#define PLAYER_DASH_TIME 0.18f   // Длительность рывка (сек)

// --- Константы скриншейка ---
#define SCREEN_SHAKE_DURATION 0.3f // Длительность скриншейка
#define SCREEN_SHAKE_INTENSITY 20.0f // Интенсивность скриншейка

// --- Константы режима редактирования ---
#define EDIT_MODE_MENU_WIDTH 250
#define EDIT_MODE_MENU_HEIGHT 200  // Увеличим высоту для нового инструмента
#define EDIT_MODE_TILE_SIZE 24
#define LEVEL_FILE_PATH "level.json"

// --- Константы инструментов редактирования ---
#define TOOL_SELECT 0
#define TOOL_RECTANGLE 1

// --- Структура игрока ---
typedef struct Player {
    Vector2 position;   // Центр ног игрока
    float speed;        // Вертикальная скорость
    float velocityX;    // Горизонтальная скорость
    bool canJump;       // Может прыгать
    float jumpTime;     // Время удержания прыжка
    bool isJumping;     // Сейчас прыгает
    bool dropDown;      // Флаг: инициировано спрыгивание с JumpThru
    int jumpCount;      // Счетчик прыжков для распрыжки
    bool dashing;       // Сейчас выполняется рывок
    float dashTime;     // Оставшееся время рывка
    bool isSuperJump;   // Флаг супер-прыжка
    bool wasSuperJump;  // Флаг: был ли последний прыжок супер-прыжком
    int lastDirection;  // Последнее направление движения: 1 — вправо, -1 — влево
    bool superJumpWasInAir; // Был ли в воздухе после супер-прыжка
} Player;

#define TILE_SIZE 32 // Размер тайла в пикселях
#define TILEMAP_WIDTH  600 // Ширина карты в тайлах (можно менять)
#define TILEMAP_HEIGHT 800 // Высота карты в тайлах (можно менять)

// --- Типы тайлов ---
typedef enum {
    TILE_EMPTY = 0,
    TILE_SOLID = 1,
    TILE_JUMPTHRU = 2
} TileType;

// Карта тайлов (можно заполнить позже)
unsigned char tilemap[TILEMAP_HEIGHT][TILEMAP_WIDTH] = {0};

// --- Функции для работы с тайловой системой ---

// Получить тип тайла по координатам мира
TileType GetTileAtWorldPos(float worldX, float worldY) {
    int tileX = (int)(worldX / TILE_SIZE);
    int tileY = (int)(worldY / TILE_SIZE);
    
    if (tileX < 0 || tileX >= TILEMAP_WIDTH || tileY < 0 || tileY >= TILEMAP_HEIGHT) {
        return TILE_EMPTY; // За пределами карты
    }
    
    return (TileType)tilemap[tileY][tileX];
}

// Установить тайл по координатам мира
void SetTileAtWorldPos(float worldX, float worldY, TileType type) {
    int tileX = (int)(worldX / TILE_SIZE);
    int tileY = (int)(worldY / TILE_SIZE);
    
    if (tileX >= 0 && tileX < TILEMAP_WIDTH && tileY >= 0 && tileY < TILEMAP_HEIGHT) {
        tilemap[tileY][tileX] = (unsigned char)type;
    }
}

// Заполнить прямоугольную область тайлами
void FillTileArea(int startX, int startY, int width, int height, TileType type) {
    for (int y = startY; y < startY + height && y < TILEMAP_HEIGHT; y++) {
        for (int x = startX; x < startX + width && x < TILEMAP_WIDTH; x++) {
            if (x >= 0 && y >= 0) {
                tilemap[y][x] = (unsigned char)type;
            }
        }
    }
}

// Инициализация тайловой карты (создаем уровень)
void InitializeTilemap() {
    // Очищаем карту
    memset(tilemap, 0, sizeof(tilemap));
    
    // Земля внизу (сплошная линия тайлов)
    FillTileArea(0, TILEMAP_HEIGHT - 6, TILEMAP_WIDTH, 6, TILE_SOLID);
    
    // Левая стена
    FillTileArea(0, 0, 2, TILEMAP_HEIGHT, TILE_SOLID);
    
    // Платформы (конвертируем из старой системы)
    // Платформа на позиции (300, 200) размером 400x10
    FillTileArea(300/TILE_SIZE, 200/TILE_SIZE, 400/TILE_SIZE, 10/TILE_SIZE, TILE_SOLID);
    
    // Платформа на позиции (250, 300) размером 100x10
    FillTileArea(250/TILE_SIZE, 300/TILE_SIZE, 100/TILE_SIZE, 10/TILE_SIZE, TILE_SOLID);
    
    // Платформа на позиции (650, 300) размером 100x10
    FillTileArea(650/TILE_SIZE, 300/TILE_SIZE, 100/TILE_SIZE, 10/TILE_SIZE, TILE_SOLID);
    
    // Оранжевая платформа на позиции (800, 300) размером 100x20
    FillTileArea(800/TILE_SIZE, 300/TILE_SIZE, 100/TILE_SIZE, 20/TILE_SIZE, TILE_SOLID);
    
    // JumpThru платформа на позиции (950, 320) размером 120x10
    FillTileArea(950/TILE_SIZE, 320/TILE_SIZE, 120/TILE_SIZE, 10/TILE_SIZE, TILE_JUMPTHRU);
}

// --- Оптимизированная отрисовка тайловой карты ---
void DrawTilemapOptimized(Camera2D* cam) {
    // Получаем границы видимой области в мировых координатах
    float left   = GetScreenToWorld2D((Vector2){0, 0}, *cam).x;
    float right  = GetScreenToWorld2D((Vector2){GetScreenWidth(), 0}, *cam).x;
    float top    = GetScreenToWorld2D((Vector2){0, 0}, *cam).y;
    float bottom = GetScreenToWorld2D((Vector2){0, GetScreenHeight()}, *cam).y;

    // Переводим в индексы тайлов
    int startX = (int)(left / TILE_SIZE) - 1;
    int endX   = (int)(right / TILE_SIZE) + 1;
    int startY = (int)(top / TILE_SIZE) - 1;
    int endY   = (int)(bottom / TILE_SIZE) + 1;

    // Ограничиваем границы
    if (startX < 0) startX = 0;
    if (endX >= TILEMAP_WIDTH) endX = TILEMAP_WIDTH - 1;
    if (startY < 0) startY = 0;
    if (endY >= TILEMAP_HEIGHT) endY = TILEMAP_HEIGHT - 1;

    for (int y = startY; y <= endY; y++) {
        for (int x = startX; x <= endX; x++) {
            TileType tileType = (TileType)tilemap[y][x];
            if (tileType != TILE_EMPTY) {
                Rectangle tileRect = {
                    x * TILE_SIZE,
                    y * TILE_SIZE,
                    TILE_SIZE,
                    TILE_SIZE
                };
                Color tileColor;
                switch (tileType) {
                    case TILE_SOLID: tileColor = GRAY; break;
                    case TILE_JUMPTHRU: tileColor = VIOLET; break;
                    default: tileColor = WHITE; break;
                }
                DrawRectangleRec(tileRect, tileColor);
                DrawRectangleLinesEx(tileRect, 1, BLACK);
            }
        }
    }
}

// --- Частицы пыли ---
#define MAX_PARTICLES 2000 // Максимальное количество частиц
typedef struct Particle {
    Vector2 pos;  // Позиция частицы
    Vector2 vel; // Скорость частицы
    float life; // Оставшееся время жизни
    float maxLife; // Максимальное время жизни
    float size; // Размер частицы
    bool active; // Активна ли частица
    Color color; // Цвет частицы
} Particle;
Particle particles[MAX_PARTICLES] = {0}; // Массив частиц

// --- Глобальные переменные для режима редактирования ---
bool editMode = false;
TileType selectedTileType = TILE_SOLID;
bool showTileMenu = false;
int currentTool = TOOL_SELECT;  // Текущий выбранный инструмент

// --- Переменные для инструмента прямоугольника ---
bool isDrawingRectangle = false;
Vector2 rectangleStart = {0, 0};
Vector2 rectangleEnd = {0, 0};

// --- Глобальные переменные для скриншейка ---
float screenShakeTime = 0.0f;
float screenShakeIntensity = 0.0f;
Vector2 originalCameraOffset = {0}; // Сохраняем оригинальное положение камеры

// Вспомогательная структура для оболочки
typedef struct { float x, y; } HullPoint;

// Векторное произведение для определения поворота
float cross(const HullPoint *O, const HullPoint *A, const HullPoint *B) {
    return (A->x - O->x) * (B->y - O->y) - (A->y - O->y) * (B->x - O->x);
}

// Построение выпуклой оболочки Грэхема (возвращает количество точек в hull)
int convex_hull(const HullPoint *points, int n, HullPoint *hull) {
    if (n < 3) { for (int i=0; i<n; i++) hull[i]=points[i]; return n; }
    int k = 0;
    HullPoint *tmp = malloc(sizeof(HullPoint)*n*2);
    // Сортировка по x, затем по y
    memcpy(tmp, points, sizeof(HullPoint)*n);
    for (int i=0; i<n-1; i++) for (int j=i+1; j<n; j++)
        if (tmp[i].x > tmp[j].x || (tmp[i].x==tmp[j].x && tmp[i].y>tmp[j].y)) {
            HullPoint t=tmp[i]; tmp[i]=tmp[j]; tmp[j]=t;
        }
    // Нижняя оболочка
    for (int i=0; i<n; i++) {
        while (k>=2 && cross(&hull[k-2], &hull[k-1], &tmp[i])<=0) k--;
        hull[k++] = tmp[i];
    }
    // Верхняя оболочка
    int t = k+1;
    for (int i=n-2; i>=0; i--) {
        while (k>=t && cross(&hull[k-2], &hull[k-1], &tmp[i])<=0) k--;
        hull[k++] = tmp[i];
    }
    free(tmp);
    return k-1;
}

// --- Функция спавна пыли ---
void SpawnDustParticles(Vector2 pos, int count)
{
    for (int c = 0; c < count; c++)
    {
        int slot = -1;
        float minLife = 9999.0f;
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (!particles[i].active) { slot = i; break; }
            if (particles[i].life < minLife) { minLife = particles[i].life; slot = i; }
        }
        // Больший разброс по X и Y
        float offsetX = GetRandomValue(-30, 30);
        float offsetY = GetRandomValue(-8, 8);
        Vector2 spawnPos = (Vector2){ pos.x + offsetX, pos.y + offsetY };
        // Веерный угол разлёта
        float angle = DEG2RAD * (GetRandomValue(120, 420));
        float speed = GetRandomValue(60, 160) / 100.0f;
        particles[slot].pos = spawnPos;
        particles[slot].vel = (Vector2){ cosf(angle) * speed, -fabsf(sinf(angle) * speed) };
        particles[slot].life = 0.9f;
        particles[slot].maxLife = 0.9f;
        particles[slot].size = GetRandomValue(5, 8);
        particles[slot].active = true;
        // Случайный оттенок серого/коричневого
        particles[slot].color = (Color){ 255, 255, 255, 180 }; // Белый с альфой 180
    }
}

// --- Обновление частиц ---
void UpdateParticles(float dt)
{
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            particles[i].pos.x += particles[i].vel.x * 80.0f * dt;
            particles[i].pos.y += particles[i].vel.y * 80.0f * dt;
            // Слабая гравитация
            particles[i].vel.y += 0.18f * dt;
            
            // Проверяем коллизию с тайлами
            float groundY = 1e9f;
            for (int checkX = -1; checkX <= 1; checkX++) {
                float checkPosX = particles[i].pos.x + checkX * TILE_SIZE/2;
                TileType tileType = GetTileAtWorldPos(checkPosX, particles[i].pos.y + 2.0f);
                if (tileType == TILE_SOLID) {
                    float tileTop = (int)((particles[i].pos.y + 2.0f) / TILE_SIZE) * TILE_SIZE;
                    if (tileTop < groundY) groundY = tileTop;
                }
            }
            
            if (particles[i].pos.y > groundY) {
                particles[i].pos.y = groundY;
                particles[i].vel.y = 0;
            }
            
            particles[i].life -= dt;
            if (particles[i].life <= 0.0f)
                particles[i].active = false;
        }
    }
}

// --- Рисование частиц ---
void DrawParticles(void)
{
    // Сначала рисуем "фейковый" аутлайн: те же частицы, но чёрным цветом и с небольшим смещением
    const int outlineOffsets[8][2] = {
        {1,0}, {-1,0}, {0,1}, {0,-1}, {1,1}, {-1,1}, {1,-1}, {-1,-1}
    };
    for (int o = 0; o < 8; o++) {
        int dx = outlineOffsets[o][0];
        int dy = outlineOffsets[o][1];
        for (int i = 0; i < MAX_PARTICLES; i++) {
            if (particles[i].active) {
                float alpha = particles[i].life / particles[i].maxLife;
                float currentSize = particles[i].size * alpha;
                Vector2 pos = { particles[i].pos.x + dx, particles[i].pos.y + dy };
                DrawCircleV(pos, currentSize, BLACK);
            }
        }
    }
    // Теперь рисуем сами белые частицы
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            Color c = (Color){255,255,255,255};
            float alpha = particles[i].life / particles[i].maxLife;
            float currentSize = particles[i].size * alpha;
            DrawCircleV(particles[i].pos, currentSize, c);
        }
    }
}

// --- Прототипы функций управления игроком и камерой ---
void UpdatePlayer(Player *player, float delta, bool *justLanded, bool *justLandedSuperJump, Vector2 *landPos); // Обновление состояния игрока
void UpdateCameraCenter(Camera2D *camera, Player *player, float delta, int width, int height); // Камера по центру игрока
void UpdateCameraCenterInsideMap(Camera2D *camera, Player *player, float delta, int width, int height); // Камера по центру, но в пределах карты
void UpdateCameraCenterSmoothFollow(Camera2D *camera, Player *player, float delta, int width, int height); // Плавное следование камеры
void UpdateCameraEvenOutOnLanding(Camera2D *camera, Player *player, float delta, int width, int height); // Камера выравнивается после приземления
void UpdateCameraPlayerBoundsPush(Camera2D *camera, Player *player, float delta, int width, int height); // Камера сдвигается, если игрок у края

// --- Прототипы функций для редактирования уровня ---
void SaveLevelToJSON(const char* filename);
bool LoadLevelFromJSON(const char* filename);
void UpdateEditMode(Camera2D *camera, Player *player);
void DrawEditModeUI(void);
void HandleTilePlacement(Camera2D *camera);

int main(void)
{
    const int screenWidth = 1600; // Ширина окна
    const int screenHeight = 800; // Высота окна

    InitWindow(screenWidth, screenHeight, "PlatformerTest + Dust + JumpThru"); // Инициализация окна

    playerTexture = LoadTexture(PLAYER_SPRITE_PATH); // Загружаем спрайт игрока

    // Пытаемся загрузить уровень из JSON файла
    bool levelLoaded = LoadLevelFromJSON(LEVEL_FILE_PATH);

    // Если файл не найден, инициализируем уровень по умолчанию
    if (!levelLoaded) {
        InitializeTilemap();
    }

    Player player = {0}; // Создание структуры игрока и обнуление
    player.position = (Vector2){ 400, 280 }; // Начальная позиция игрока
    player.speed = 0; // Начальная вертикальная скорость
    player.velocityX = 0; // Начальная горизонтальная скорость
    player.canJump = false; // Может ли прыгать
    player.jumpTime = 0.0f; // Время удержания прыжка
    player.isJumping = false; // Прыгает ли сейчас
    player.dropDown = false; // Флаг спрыгивания с JumpThru
    player.jumpCount = 0; // Счетчик прыжков для распрыжки
    player.dashing = false; // Не в рывке
    player.dashTime = 0.0f; // Таймер рывка
    player.isSuperJump = false; // Флаг супер-прыжка
    player.wasSuperJump = false; // Флаг: был ли последний прыжок супер-прыжком
    player.lastDirection = 1; // По умолчанию смотрит вправо
    player.superJumpWasInAir = false;

    Camera2D camera = {0}; // Структура камеры
    camera.target = player.position; // Камера смотрит на игрока
    camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f }; // Центр экрана
    originalCameraOffset = camera.offset; // Сохраняем начальное положение
    camera.rotation = 0.0f; // Без поворота
    camera.zoom = 2.0f; // Масштаб 1:1
    float cameraTargetZoom = 2.0f; // Желаемый zoom камеры

    // Отдельная камера для режима редактирования
    Camera2D editCamera = {0};
    editCamera.target = (Vector2){ 400, 280 }; // Начальная позиция камеры редактирования
    editCamera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
    editCamera.rotation = 0.0f;
    editCamera.zoom = 2.0f;

    void (*cameraUpdaters[])(Camera2D*, Player*, float, int, int) = {
        UpdateCameraCenter,
        UpdateCameraCenterInsideMap,
        UpdateCameraCenterSmoothFollow,
        UpdateCameraEvenOutOnLanding,
        UpdateCameraPlayerBoundsPush
    }; // Массив указателей на функции обновления камеры
    int cameraOption = 2; // Текущий режим камеры
    int cameraUpdatersLength = sizeof(cameraUpdaters)/sizeof(cameraUpdaters[0]); // Количество режимов камеры
    char *cameraDescriptions[] = {
        "Follow player center",
        "Follow player center, but clamp to map edges",
        "Follow player center; smoothed",
        "Follow player center horizontally; update player center vertically after landing",
        "Player push camera on getting too close to screen edge"
    }; // Описания режимов камеры

    SetTargetFPS(144); // 144 кадров в секунду

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        // --- Спрыгивание с JumpThru: если стоим и нажали вниз+пробел, активируем dropDown и НЕ прыгаем! ---
        if (IsKeyDown(KEY_DOWN) && IsKeyPressed(KEY_SPACE))
        {
            float playerWidth = 40.0f;
            float playerHeight = 40.0f;
            // Проверяем, стоим ли на JumpThru-тайле
            bool onJumpThru = false;
            for (int checkX = -1; checkX <= 1; checkX++) {
                float checkPosX = player.position.x + checkX * TILE_SIZE/2;
                TileType tileType = GetTileAtWorldPos(checkPosX, player.position.y);
                if (tileType == TILE_JUMPTHRU) {
                    onJumpThru = true;
                    break;
                }
            }
            
            if (onJumpThru) {
                player.dropDown = true;
                player.isJumping = false;
                player.jumpTime = 0.0f;
                player.canJump = false;
                player.speed = 200.0f;
            }
        }

        // --- Переключение режима редактирования ---
        if (IsKeyPressed(KEY_E) || (editMode && IsKeyPressed(KEY_ESCAPE))) {
            if (editMode) {
                // Выход из режима редактирования
                editMode = false;
                showTileMenu = false;
                // Синхронизируем основную камеру с камерой редактирования
                camera.target = editCamera.target;
                camera.zoom = editCamera.zoom;
                SaveLevelToJSON(LEVEL_FILE_PATH);
            } else {
                // Вход в режим редактирования
                editMode = true;
                showTileMenu = true;
                // Синхронизируем камеру редактирования с основной камерой
                editCamera.target = camera.target;
                editCamera.zoom = camera.zoom;
            }
        }

        // --- Обновление режима редактирования ---
        if (editMode) {
            UpdateEditMode(&editCamera, &player);
        }

        bool justLanded = false;
        bool justLandedSuperJump = false;
        Vector2 landPos = {0,0};
        UpdatePlayer(&player, deltaTime, &justLanded, &justLandedSuperJump, &landPos);

        if (justLanded) {
            int dustCount = justLandedSuperJump ? 100 : 24;
            SpawnDustParticles((Vector2){player.position.x, player.position.y+1}, dustCount);
        }

        UpdateParticles(deltaTime);

        // Управление зумом только в игровом режиме
        if (!editMode) {
            camera.zoom += ((float)GetMouseWheelMove()*0.05f);
            if (camera.zoom > 3.0f) camera.zoom = 3.0f;
            else if (camera.zoom < 0.25f) camera.zoom = 0.25f;

            // --- Эффект отдаления камеры при прыжке ---
            if (!player.canJump) { // Если в воздухе
                if (player.isSuperJump) { // Если это супер-прыжок
                    cameraTargetZoom = 1.0f; // Очень сильное отдаление для супер-прыжка
                } else {
                    cameraTargetZoom = 1.7f; // Обычное отдаление для обычного прыжка
                }
            } else if (fabs(player.velocityX) >= PLAYER_MAX_SPEED) { // Если на земле и на максимальной скорости
                cameraTargetZoom = 1.7f;
            } else {
                cameraTargetZoom = 2.0f; // Вернуть камеру при приземлении и обычной скорости
            }
            // Плавная интерполяция zoom
            float zoomLerpSpeed = 6.0f; // Чем больше, тем быстрее
            camera.zoom += (cameraTargetZoom - camera.zoom) * zoomLerpSpeed * deltaTime;
        }

        if (IsKeyPressed(KEY_R))
        {
            camera.zoom = 2.0f;
            editCamera.zoom = 2.0f;
            player.position = (Vector2){ 400, 280 };
        }

        if (IsKeyPressed(KEY_C)) cameraOption = (cameraOption + 1)%cameraUpdatersLength;

        // Обновление камеры только в игровом режиме
        if (!editMode) {
            cameraUpdaters[cameraOption](&camera, &player, deltaTime, screenWidth, screenHeight);
        }

        // Обновление скриншейка
        if (screenShakeTime > 0.0f) {
            screenShakeTime -= deltaTime;
            if (screenShakeTime <= 0.0f) {
                screenShakeTime = 0.0f;
                screenShakeIntensity = 0.0f;
                camera.offset = originalCameraOffset; // Восстанавливаем оригинальное положение
            }
        }

        BeginDrawing();

            ClearBackground(LIGHTGRAY);

            // Выбираем камеру в зависимости от режима
            Camera2D* currentCamera = editMode ? &editCamera : &camera;

            BeginMode2D(*currentCamera);

                // Применяем скриншейк к камере (только в игровом режиме)
                if (!editMode && screenShakeTime > 0.0f) {
                    float shakeX = GetRandomValue(-screenShakeIntensity, screenShakeIntensity);
                    float shakeY = GetRandomValue(-screenShakeIntensity, screenShakeIntensity);
                    camera.offset.x = originalCameraOffset.x + shakeX;
                    camera.offset.y = originalCameraOffset.y + shakeY;
                }

                DrawTilemapOptimized(currentCamera);

                // Отрисовка предварительного просмотра прямоугольника
                if (editMode && currentTool == TOOL_RECTANGLE && isDrawingRectangle) {
                    float startX = fminf(rectangleStart.x, rectangleEnd.x);
                    float endX = fmaxf(rectangleStart.x, rectangleEnd.x);
                    float startY = fminf(rectangleStart.y, rectangleEnd.y);
                    float endY = fmaxf(rectangleStart.y, rectangleEnd.y);
                    
                    // Прилипаем к сетке тайлов
                    int startTileX = (int)(startX / TILE_SIZE);
                    int endTileX = (int)(endX / TILE_SIZE);
                    int startTileY = (int)(startY / TILE_SIZE);
                    int endTileY = (int)(endY / TILE_SIZE);
                    
                    // Конвертируем обратно в мировые координаты для отрисовки
                    float snappedStartX = startTileX * TILE_SIZE;
                    float snappedEndX = (endTileX + 1) * TILE_SIZE;
                    float snappedStartY = startTileY * TILE_SIZE;
                    float snappedEndY = (endTileY + 1) * TILE_SIZE;
                    
                    Rectangle previewRect = { snappedStartX, snappedStartY, snappedEndX - snappedStartX, snappedEndY - snappedStartY };
                    DrawRectangleLinesEx(previewRect, 2, RED);
                    
                    // Показываем размер в тайлах
                    int tileWidth = endTileX - startTileX + 1;
                    int tileHeight = endTileY - startTileY + 1;
                    char sizeText[64];
                    snprintf(sizeText, sizeof(sizeText), "%dx%d tiles", tileWidth, tileHeight);
                    DrawText(sizeText, snappedEndX + 5, snappedStartY, 20, RED);
                }

                // Отрисовка спрайта игрока только в игровом режиме
                if (!editMode) {
                    // Отрисовка спрайта игрока по центру ног с сохранением пропорций и отражением по направлению
                    int targetW = 40;
                    float aspect = (float)playerTexture.height / (float)playerTexture.width;
                    int targetH = (int)(targetW * aspect);
                    Rectangle srcRect;
                    if (player.lastDirection == -1) {
                        srcRect = (Rectangle){ playerTexture.width, 0, -playerTexture.width, playerTexture.height };
                    } else {
                        srcRect = (Rectangle){ 0, 0, playerTexture.width, playerTexture.height };
                    }
                    Rectangle destRect = { player.position.x - targetW/2, player.position.y - targetH, targetW, targetH };
                    Vector2 origin = { 0, 0 };
                    DrawTexturePro(playerTexture, srcRect, destRect, origin, 0.0f, WHITE);

                    DrawCircleV(player.position, 5.0f, GOLD);
                }

                DrawParticles();

            EndMode2D();

            DrawText("Controls:", 20, 20, 10, BLACK);
            DrawText("- Right/Left to move", 40, 40, 10, DARKGRAY);
            DrawText("- Space to jump", 40, 60, 10, DARKGRAY);
            DrawText("- Down+Space to drop through JumpThru", 40, 80, 10, DARKGRAY);
            DrawText("- Mouse Wheel to Zoom in-out, R to reset zoom", 40, 100, 10, DARKGRAY);
            DrawText("- C to change camera mode", 40, 120, 10, DARKGRAY);
            DrawText("- E to enter/exit edit mode", 40, 140, 10, DARKGRAY);
            DrawText("- In edit mode: LMB=place, RMB=delete, MMB=pan, M=menu", 40, 160, 10, DARKGRAY);
            DrawText("Current camera mode:", 20, 180, 10, BLACK);
            DrawText(cameraDescriptions[cameraOption], 40, 200, 10, DARKGRAY);

            // Отображение счетчика прыжков
            char jumpCountText[64];
            snprintf(jumpCountText, sizeof(jumpCountText), "Jump count: %d (superjump on 3)", player.jumpCount+1);
            DrawText(jumpCountText, 40, 220, 10, DARKGRAY);

            // Отображение количества активных частиц
            int activeParticles = 0;
            for (int i = 0; i < MAX_PARTICLES; i++) if (particles[i].active) activeParticles++;
            char particleCountText[64];
            snprintf(particleCountText, sizeof(particleCountText), "Active particles: %d", activeParticles);
            DrawText(particleCountText, 40, 240, 10, DARKGRAY);

            // Отображение координат игрока для отладки
            char playerPosText[128];
            snprintf(playerPosText, sizeof(playerPosText), "Player pos: (%.1f, %.1f)", player.position.x, player.position.y);
            DrawText(playerPosText, 40, 260, 10, DARKGRAY);

            // Отображение информации о тайле под игроком
            TileType tileUnderPlayer = GetTileAtWorldPos(player.position.x, player.position.y);
            char tileInfoText[64];
            const char* tileTypeName = "EMPTY";
            if (tileUnderPlayer == TILE_SOLID) tileTypeName = "SOLID";
            else if (tileUnderPlayer == TILE_JUMPTHRU) tileTypeName = "JUMPTHRU";
            snprintf(tileInfoText, sizeof(tileInfoText), "Tile under: %s", tileTypeName);
            DrawText(tileInfoText, 40, 280, 10, DARKGRAY);

            // Индикатор режима редактирования
            if (editMode) {
                DrawText("EDIT MODE ACTIVE", 40, 300, 12, RED);
            }

            {
                const int fpsFontSize = 20;
                const int padding = 10;
                const char *fpsText = TextFormat("FPS: %d", GetFPS());
                int textWidth = MeasureText(fpsText, fpsFontSize);
                int xPos = GetScreenWidth() - textWidth - padding;
                int yPos = padding;
                DrawText(fpsText, xPos, yPos, fpsFontSize, DARKBLUE);
            }

            // Отладочная информация (под счетчиком FPS)
            if (editMode) {
                int debugX = GetScreenWidth() - 300;
                int debugY = 50;
                DrawText("DEBUG INFO:", debugX, debugY, 15, RED);
                DrawText(TextFormat("Camera Target: (%.1f, %.1f)", editCamera.target.x, editCamera.target.y), debugX, debugY + 20, 12, WHITE);
                DrawText(TextFormat("Camera Zoom: %.2f", editCamera.zoom), debugX, debugY + 35, 12, WHITE);
                DrawText(TextFormat("Map Size: %dx%d tiles", TILEMAP_WIDTH, TILEMAP_HEIGHT), debugX, debugY + 50, 12, WHITE);
                DrawText(TextFormat("World Size: %dx%d pixels", TILEMAP_WIDTH * TILE_SIZE, TILEMAP_HEIGHT * TILE_SIZE), debugX, debugY + 65, 12, WHITE);
                
                // Информация о текущем инструменте
                const char* toolName = (currentTool == TOOL_SELECT) ? "Select" : "Rectangle";
                DrawText(TextFormat("Current Tool: %s", toolName), debugX, debugY + 80, 12, WHITE);
                
                if (currentTool == TOOL_RECTANGLE) {
                    const char* tileName = "Unknown";
                    switch (selectedTileType) {
                        case TILE_SOLID: tileName = "Solid"; break;
                        case TILE_JUMPTHRU: tileName = "JumpThru"; break;
                        case TILE_EMPTY: tileName = "Empty"; break;
                    }
                    DrawText(TextFormat("Rectangle Fill: %s", tileName), debugX, debugY + 95, 12, WHITE);
                }
            }

            // --- Отрисовка UI режима редактирования ---
            if (editMode) {
                DrawEditModeUI();
            }

        EndDrawing();
    }

    CloseWindow();

    UnloadTexture(playerTexture); // Освобождаем текстуру игрока

    return 0;
}

// --- Игрок с поддержкой тайловой системы ---
void UpdatePlayer(Player *player, float delta, bool *justLanded, bool *justLandedSuperJump, Vector2 *landPos)
{
    static bool wasOnGround = false;

    // Запоминаем последнее направление ВСЕГДА
    if (IsKeyDown(KEY_LEFT) && !IsKeyDown(KEY_RIGHT)) player->lastDirection = -1;
    if (IsKeyDown(KEY_RIGHT) && !IsKeyDown(KEY_LEFT)) player->lastDirection = 1;

    if (!player->dashing && (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT))) {
        // Рывок только если есть движение влево или вправо
        if (IsKeyDown(KEY_LEFT)) {
            player->dashing = true;
            player->dashTime = PLAYER_DASH_TIME;
            player->velocityX = -PLAYER_DASH_SPEED;
            player->lastDirection = -1;
        } else if (IsKeyDown(KEY_RIGHT)) {
            player->dashing = true;
            player->dashTime = PLAYER_DASH_TIME;
            player->velocityX = PLAYER_DASH_SPEED;
            player->lastDirection = 1;
        }
    }
    if (player->dashing) {
        player->dashTime -= delta;
        // Во время рывка игнорируем обычное управление (кроме гравитации и коллизий)
        if (player->dashTime <= 0.0f) {
            player->dashing = false;
            // После рывка скорость сбрасывается к обычной максимальной, если была выше
            if (player->velocityX > PLAYER_MAX_SPEED) player->velocityX = PLAYER_MAX_SPEED;
            if (player->velocityX < -PLAYER_MAX_SPEED) player->velocityX = -PLAYER_MAX_SPEED;
        }
    } else {
        // --- Горизонтальное движение ---
        float targetSpeed = 0.0f;
        if (IsKeyDown(KEY_LEFT)) targetSpeed -= PLAYER_MAX_SPEED;
        if (IsKeyDown(KEY_RIGHT)) targetSpeed += PLAYER_MAX_SPEED;

        if (targetSpeed != 0)
        {
            if (player->velocityX < targetSpeed)
            {
                player->velocityX += PLAYER_ACCELERATION * delta;
                if (player->velocityX > targetSpeed) player->velocityX = targetSpeed;
            }
            else if (player->velocityX > targetSpeed)
            {
                player->velocityX -= PLAYER_ACCELERATION * delta;
                if (player->velocityX < targetSpeed) player->velocityX = targetSpeed;
            }
        }
        else
        {
            if (player->velocityX > 0)
            {
                player->velocityX -= PLAYER_DECELERATION * delta;
                if (player->velocityX < 0) player->velocityX = 0;
            }
            else if (player->velocityX < 0)
            {
                player->velocityX += PLAYER_DECELERATION * delta;
                if (player->velocityX > 0) player->velocityX = 0;
            }
        }
    }

    // Сброс счетчика прыжков, если персонаж остановился
    if (player->velocityX == 0) {
        player->jumpCount = 0;
    }

    // --- Прыжок с контролем по времени удержания ---
    if (IsKeyPressed(KEY_SPACE) && player->canJump && !player->dropDown)
    {
        // Блокируем прыжок, если зажата ВНИЗ и стоим на SOLID тайле
        bool blockJump = false;
        if (IsKeyDown(KEY_DOWN)) {
            float playerWidth = 40.0f;
            float playerHeight = 40.0f;
            
            // Проверяем тайлы под ногами игрока
            for (int checkX = -1; checkX <= 1; checkX++) {
                float checkPosX = player->position.x + checkX * TILE_SIZE/2;
                TileType tileType = GetTileAtWorldPos(checkPosX, player->position.y);
                if (tileType == TILE_SOLID) {
                    blockJump = true;
                    break;
                }
            }
        }
        if (!blockJump) {
            float jumpSpeed = -PLAYER_JUMP_SPD;
            if (fabs(player->velocityX) >= PLAYER_MAX_SPEED) {
                player->jumpCount++;
                if (player->jumpCount == 3) {
                    jumpSpeed *= 2.0f; // В 2 раза выше
                    player->jumpCount = 0; // Сбросить счетчик
                    player->isSuperJump = true; // Устанавливаем флаг супер-прыжка
                    player->superJumpWasInAir = true; // Запоминаем, что был супер-прыжок
                }
            } else {
                player->jumpCount = 0; // Если прыжок не на максимальной скорости, сбрасываем счетчик
                player->isSuperJump = false;
            }
            player->speed = jumpSpeed;
            player->canJump = false;
            player->isJumping = true;
            player->jumpTime = 0.0f;
        }
    }

    if (IsKeyDown(KEY_SPACE) && player->isJumping && player->jumpTime < PLAYER_MAX_JUMP_TIME)
    {
        player->speed -= PLAYER_JUMP_HOLD_FORCE * delta;
        player->jumpTime += delta;
    }
    else
    {
        player->isJumping = false;
    }

    // --- Гравитация ---
    player->speed += G * delta;

    // --- Размеры игрока ---
    float playerWidth = 40.0f;
    float playerHeight = 40.0f;

    // --- Сначала движение по X, потом по Y ---
    // 1. Горизонтальное перемещение и коллизии
    float newX = player->position.x + player->velocityX * delta;
    Rectangle playerRectX = { newX - playerWidth/2, player->position.y - playerHeight, playerWidth, playerHeight };
    bool collisionX = false;
    // Проверяем только тайлы в зоне хитбокса игрока
    int tileStartX = (int)((playerRectX.x) / TILE_SIZE);
    int tileEndX   = (int)((playerRectX.x + playerRectX.width) / TILE_SIZE);
    int tileStartY = (int)((playerRectX.y) / TILE_SIZE);
    int tileEndY   = (int)((playerRectX.y + playerRectX.height) / TILE_SIZE);
    for (int ty = tileStartY; ty <= tileEndY; ty++) {
        for (int tx = tileStartX; tx <= tileEndX; tx++) {
            if (tx < 0 || tx >= TILEMAP_WIDTH || ty < 0 || ty >= TILEMAP_HEIGHT) continue;
            TileType tileType = (TileType)tilemap[ty][tx];
            if (tileType == TILE_SOLID) {
                Rectangle tileRect = { tx * TILE_SIZE, ty * TILE_SIZE, TILE_SIZE, TILE_SIZE };
                if (CheckCollisionRecs(playerRectX, tileRect)) {
                    if (player->velocityX > 0) {
                        newX = tileRect.x - playerWidth/2;
                    } else if (player->velocityX < 0) {
                        newX = tileRect.x + tileRect.width + playerWidth/2;
                    }
                    player->velocityX = 0;
                    collisionX = true;
                    break;
                }
            }
        }
        if (collisionX) break;
    }
    player->position.x = newX;

    // 2. Вертикальное перемещение и коллизии
    float newY = player->position.y + player->speed * delta;
    Rectangle playerRectY = { player->position.x - playerWidth/2, newY - playerHeight, playerWidth, playerHeight };
    bool onGround = false;
    tileStartX = (int)((playerRectY.x) / TILE_SIZE);
    tileEndX   = (int)((playerRectY.x + playerRectY.width) / TILE_SIZE);
    tileStartY = (int)((playerRectY.y) / TILE_SIZE);
    tileEndY   = (int)((playerRectY.y + playerRectY.height) / TILE_SIZE);
    for (int ty = tileStartY; ty <= tileEndY; ty++) {
        for (int tx = tileStartX; tx <= tileEndX; tx++) {
            if (tx < 0 || tx >= TILEMAP_WIDTH || ty < 0 || ty >= TILEMAP_HEIGHT) continue;
            TileType tileType = (TileType)tilemap[ty][tx];
            Rectangle tileRect = { tx * TILE_SIZE, ty * TILE_SIZE, TILE_SIZE, TILE_SIZE };
            if (tileType == TILE_SOLID) {
                if (CheckCollisionRecs(playerRectY, tileRect)) {
                    if (player->speed > 0) {
                        newY = tileRect.y;
                        onGround = true;
                    } else if (player->speed < 0) {
                        newY = tileRect.y + tileRect.height + playerHeight;
                    }
                    player->speed = 0;
                    break;
                }
            } else if (tileType == TILE_JUMPTHRU && !player->dropDown) {
                // JumpThru: только если падаем сверху и только если игрок был выше платформы
                if (player->speed > 0 && player->position.y - playerHeight < tileRect.y &&
                    player->position.y <= tileRect.y + 8.0f &&
                    CheckCollisionRecs(playerRectY, tileRect)) {
                    newY = tileRect.y;
                    onGround = true;
                    player->speed = 0;
                    break;
                }
            }
        }
    }
    player->position.y = newY;

    // --- Проверка приземления для пыли ---
    *justLanded = (!wasOnGround && onGround);
    if (*justLanded) {
        *landPos = player->position;
        *justLandedSuperJump = player->superJumpWasInAir;
        player->superJumpWasInAir = false;
    } else {
        *justLandedSuperJump = false;
    }
    wasOnGround = onGround;
    player->canJump = onGround;
}

// --- Заглушки для функций камеры ---
void UpdateCameraCenter(Camera2D *camera, Player *player, float delta, int width, int height)
{ camera->target = player->position; }
void UpdateCameraCenterInsideMap(Camera2D *camera, Player *player, float delta, int width, int height)
{ camera->target = player->position; }
void UpdateCameraCenterSmoothFollow(Camera2D *camera, Player *player, float delta, int width, int height)
{ camera->target = Vector2Lerp(camera->target, player->position, 0.1f); }
void UpdateCameraEvenOutOnLanding(Camera2D *camera, Player *player, float delta, int width, int height)
{ camera->target = player->position; }
void UpdateCameraPlayerBoundsPush(Camera2D *camera, Player *player, float delta, int width, int height)
{ camera->target = player->position; }

// --- Прототипы функций для редактирования уровня ---
void SaveLevelToJSON(const char* filename) {
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("Ошибка: не удалось открыть файл %s для записи\n", filename);
        return;
    }
    
    fprintf(file, "{\n");
    fprintf(file, "  \"tilemap\": {\n");
    fprintf(file, "    \"width\": %d,\n", TILEMAP_WIDTH);
    fprintf(file, "    \"height\": %d,\n", TILEMAP_HEIGHT);
    fprintf(file, "    \"tileSize\": %d,\n", TILE_SIZE);
    fprintf(file, "    \"data\": [\n");
    
    for (int y = 0; y < TILEMAP_HEIGHT; y++) {
        fprintf(file, "      [");
        for (int x = 0; x < TILEMAP_WIDTH; x++) {
            fprintf(file, "%d", tilemap[y][x]);
            if (x < TILEMAP_WIDTH - 1) fprintf(file, ", ");
        }
        fprintf(file, "]");
        if (y < TILEMAP_HEIGHT - 1) fprintf(file, ",");
        fprintf(file, "\n");
    }
    
    fprintf(file, "    ]\n");
    fprintf(file, "  }\n");
    fprintf(file, "}\n");
    
    fclose(file);
    printf("Уровень сохранен в %s\n", filename);
}
bool LoadLevelFromJSON(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Файл %s не найден, используется уровень по умолчанию\n", filename);
        return false;
    }

    char line[65536]; // Длинная строка для больших массивов
    bool inData = false;
    int y = 0;
    while (fgets(line, sizeof(line), file) && y < TILEMAP_HEIGHT) {
        // Ищем начало массива данных
        if (!inData) {
            if (strstr(line, "\"data\"") && strchr(line, '[')) {
                inData = true;
            }
            continue;
        }
        // Если встретили строку только с ] — конец массива
        char* trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        if (*trimmed == ']' && (trimmed[1] == '\0' || trimmed[1] == '\n')) {
            break;
        }
        // Парсим строку с массивом
        char* start = strchr(line, '[');
        if (start) start++;
        else start = line;
        char* end = strrchr(start, ']');
        if (end) *end = '\0';
        char* token = strtok(start, ",");
        int x = 0;
        while (token && x < TILEMAP_WIDTH) {
            tilemap[y][x] = (unsigned char)atoi(token);
            token = strtok(NULL, ",");
            x++;
        }
        y++;
    }
    fclose(file);
    printf("Уровень загружен из %s\n", filename);
    return true;
}
void UpdateEditMode(Camera2D *camera, Player *player) {
    // Обработка размещения тайлов
    HandleTilePlacement(camera);
    
    // Обработка меню выбора тайлов (теперь не скрывается при выборе)
    if (showTileMenu) {
        Vector2 mousePos = GetMousePosition();
        int menuX = 20;
        int menuY = 20;
        
        // Кнопка "Solid" (серый тайл)
        Rectangle solidButton = { menuX, menuY, EDIT_MODE_MENU_WIDTH, 40 };
        if (CheckCollisionPointRec(mousePos, solidButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            selectedTileType = TILE_SOLID;
        }
        
        // Кнопка "JumpThru" (оранжевый тайл)
        Rectangle jumpThruButton = { menuX, menuY + 50, EDIT_MODE_MENU_WIDTH, 40 };
        if (CheckCollisionPointRec(mousePos, jumpThruButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            selectedTileType = TILE_JUMPTHRU;
        }
        
        // Кнопка "Empty" (удаление тайлов)
        Rectangle emptyButton = { menuX, menuY + 100, EDIT_MODE_MENU_WIDTH, 40 };
        if (CheckCollisionPointRec(mousePos, emptyButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            selectedTileType = TILE_EMPTY;
        }
        
        // Кнопка "Rectangle Tool" (инструмент прямоугольника)
        Rectangle rectangleButton = { menuX, menuY + 150, EDIT_MODE_MENU_WIDTH, 40 };
        if (CheckCollisionPointRec(mousePos, rectangleButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            currentTool = TOOL_RECTANGLE;
        }
        
        // Кнопка "Select Tool" (обычный инструмент)
        Rectangle selectButton = { menuX, menuY + 200, EDIT_MODE_MENU_WIDTH, 40 };
        if (CheckCollisionPointRec(mousePos, selectButton) && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            currentTool = TOOL_SELECT;
        }
    }
    
    // Переключение меню по клавише M
    if (IsKeyPressed(KEY_M)) {
        showTileMenu = !showTileMenu;
    }
}
void DrawEditModeUI(void) {
    // Фон для режима редактирования
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), (Color){0, 0, 0, 100});
    
    // Информация о режиме редактирования
    DrawText("EDIT MODE", 20, GetScreenHeight() - 40, 20, RED);
    DrawText("E or ESC - Exit Edit Mode", 20, GetScreenHeight() - 20, 15, WHITE);
    
    // Меню выбора тайлов
    if (showTileMenu) {
        int menuX = 20;
        int menuY = 20;
        
        // Фон меню
        DrawRectangle(menuX - 10, menuY - 10, EDIT_MODE_MENU_WIDTH + 20, EDIT_MODE_MENU_HEIGHT + 20, DARKGRAY);
        DrawRectangleLinesEx((Rectangle){menuX - 10, menuY - 10, EDIT_MODE_MENU_WIDTH + 20, EDIT_MODE_MENU_HEIGHT + 20}, 2, BLACK);
        
        // Кнопка "Solid" (серый тайл)
        Rectangle solidButton = { menuX, menuY, EDIT_MODE_MENU_WIDTH, 40 };
        Color solidButtonColor = (selectedTileType == TILE_SOLID) ? GRAY : LIGHTGRAY;
        DrawRectangleRec(solidButton, solidButtonColor);
        DrawRectangleLinesEx(solidButton, 2, BLACK);
        DrawText("Solid (Gray)", menuX + 10, menuY + 10, 20, BLACK);
        
        // Кнопка "JumpThru" (оранжевый тайл)
        Rectangle jumpThruButton = { menuX, menuY + 50, EDIT_MODE_MENU_WIDTH, 40 };
        Color jumpThruButtonColor = (selectedTileType == TILE_JUMPTHRU) ? ORANGE : LIGHTGRAY;
        DrawRectangleRec(jumpThruButton, jumpThruButtonColor);
        DrawRectangleLinesEx(jumpThruButton, 2, BLACK);
        DrawText("JumpThru (Purple)", menuX + 10, menuY + 60, 20, BLACK);
        
        // Кнопка "Empty" (удаление тайлов)
        Rectangle emptyButton = { menuX, menuY + 100, EDIT_MODE_MENU_WIDTH, 40 };
        Color emptyButtonColor = (selectedTileType == TILE_EMPTY) ? WHITE : LIGHTGRAY;
        DrawRectangleRec(emptyButton, emptyButtonColor);
        DrawRectangleLinesEx(emptyButton, 2, BLACK);
        DrawText("Empty (Delete)", menuX + 10, menuY + 110, 20, BLACK);
        
        // Кнопка "Rectangle Tool" (инструмент прямоугольника)
        Rectangle rectangleButton = { menuX, menuY + 150, EDIT_MODE_MENU_WIDTH, 40 };
        Color rectangleButtonColor = (currentTool == TOOL_RECTANGLE) ? BLUE : LIGHTGRAY;
        DrawRectangleRec(rectangleButton, rectangleButtonColor);
        DrawRectangleLinesEx(rectangleButton, 2, BLACK);
        DrawText("Rectangle Tool", menuX + 10, menuY + 160, 20, BLACK);
        
        // Кнопка "Select Tool" (обычный инструмент)
        Rectangle selectButton = { menuX, menuY + 200, EDIT_MODE_MENU_WIDTH, 40 };
        Color selectButtonColor = (currentTool == TOOL_SELECT) ? GREEN : LIGHTGRAY;
        DrawRectangleRec(selectButton, selectButtonColor);
        DrawRectangleLinesEx(selectButton, 2, BLACK);
        DrawText("Select Tool", menuX + 10, menuY + 210, 20, BLACK);
    } else {
        // Показываем выбранный тайл
        int infoX = 20;
        int infoY = 20;
        DrawRectangle(infoX - 5, infoY - 5, 150, 30, DARKGRAY);
        DrawRectangleLinesEx((Rectangle){infoX - 5, infoY - 5, 150, 30}, 2, BLACK);
        
        const char* tileName = "Unknown";
        Color tileColor = WHITE;
        switch (selectedTileType) {
            case TILE_SOLID:
                tileName = "Solid";
                tileColor = GRAY;
                break;
            case TILE_JUMPTHRU:
                tileName = "JumpThru";
                tileColor = ORANGE;
                break;
            case TILE_EMPTY:
                tileName = "Empty";
                tileColor = WHITE;
                break;
        }
        
        DrawText(TextFormat("Selected: %s", tileName), infoX, infoY, 20, BLACK);
        DrawRectangle(infoX + 120, infoY + 5, 20, 20, tileColor);
        DrawRectangleLinesEx((Rectangle){infoX + 120, infoY + 5, 20, 20}, 1, BLACK);
        
        // Подсказки управления (смещены правее)
        int controlsX = 250;
        int controlsY = 20;
        DrawText("Controls:", controlsX, controlsY, 20, WHITE);
        DrawText("Left Click - Place tile", controlsX, controlsY + 25, 15, WHITE);
        DrawText("Right Click - Delete tile", controlsX, controlsY + 45, 15, WHITE);
        DrawText("Middle Click - Pan camera", controlsX, controlsY + 65, 15, WHITE);
        DrawText("Mouse Wheel - Zoom", controlsX, controlsY + 85, 15, WHITE);
        DrawText("M - Toggle menu", controlsX, controlsY + 105, 15, WHITE);
        
        // Дополнительные подсказки для инструмента прямоугольника
        if (currentTool == TOOL_RECTANGLE) {
            DrawText("Rectangle Tool:", controlsX, controlsY + 135, 15, BLUE);
            DrawText("LMB - Draw rectangle with selected tile", controlsX, controlsY + 155, 12, WHITE);
            DrawText("RMB - Erase rectangle area", controlsX, controlsY + 170, 12, WHITE);
        }
    }
}
void HandleTilePlacement(Camera2D *camera) {
    Vector2 mousePos = GetMousePosition();
    
    // Конвертируем позицию мыши в мировые координаты
    Vector2 worldPos = GetScreenToWorld2D(mousePos, *camera);
    
    // Конвертируем в координаты тайла
    int tileX = (int)(worldPos.x / TILE_SIZE);
    int tileY = (int)(worldPos.y / TILE_SIZE);
    
    // Обработка инструмента прямоугольника
    if (currentTool == TOOL_RECTANGLE) {
        // Начало рисования прямоугольника (левая кнопка - рисование, правая - удаление)
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
            isDrawingRectangle = true;
            rectangleStart = worldPos;
            rectangleEnd = worldPos;
        }
        
        // Обновление размера прямоугольника при зажатой кнопке
        if (isDrawingRectangle && (IsMouseButtonDown(MOUSE_LEFT_BUTTON) || IsMouseButtonDown(MOUSE_RIGHT_BUTTON))) {
            rectangleEnd = worldPos;
        }
        
        // Завершение рисования прямоугольника
        if (isDrawingRectangle && (IsMouseButtonReleased(MOUSE_LEFT_BUTTON) || IsMouseButtonReleased(MOUSE_RIGHT_BUTTON))) {
            // Определяем границы прямоугольника
            float startX = fminf(rectangleStart.x, rectangleEnd.x);
            float endX = fmaxf(rectangleStart.x, rectangleEnd.x);
            float startY = fminf(rectangleStart.y, rectangleEnd.y);
            float endY = fmaxf(rectangleStart.y, rectangleEnd.y);
            
            // Конвертируем в координаты тайлов (прилипаем к сетке)
            int startTileX = (int)(startX / TILE_SIZE);
            int endTileX = (int)(endX / TILE_SIZE);
            int startTileY = (int)(startY / TILE_SIZE);
            int endTileY = (int)(endY / TILE_SIZE);
            
            // Ограничиваем границы карты
            startTileX = fmaxf(0, startTileX);
            endTileX = fminf(TILEMAP_WIDTH - 1, endTileX);
            startTileY = fmaxf(0, startTileY);
            endTileY = fminf(TILEMAP_HEIGHT - 1, endTileY);
            
            // Заполняем прямоугольную область
            TileType fillType = IsMouseButtonReleased(MOUSE_LEFT_BUTTON) ? selectedTileType : TILE_EMPTY;
            for (int y = startTileY; y <= endTileY; y++) {
                for (int x = startTileX; x <= endTileX; x++) {
                    tilemap[y][x] = (unsigned char)fillType;
                }
            }
            
            isDrawingRectangle = false;
        }
    } else {
        // Обычный инструмент выбора
        // Проверяем границы карты
        if (tileX >= 0 && tileX < TILEMAP_WIDTH && tileY >= 0 && tileY < TILEMAP_HEIGHT) {
            // Левая кнопка мыши - рисование выбранным тайлом
            if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                tilemap[tileY][tileX] = (unsigned char)selectedTileType;
            }
            // Правая кнопка мыши - удаление тайлов (установка TILE_EMPTY)
            else if (IsMouseButtonDown(MOUSE_RIGHT_BUTTON)) {
                tilemap[tileY][tileX] = (unsigned char)TILE_EMPTY;
            }
        }
    }
    
    // Средняя кнопка мыши - панорамирование камеры
    if (IsMouseButtonDown(MOUSE_MIDDLE_BUTTON)) {
        Vector2 mouseDelta = GetMouseDelta();
        camera->target.x -= mouseDelta.x / camera->zoom;
        camera->target.y -= mouseDelta.y / camera->zoom;
    }
    
    // Прокрутка колесика мыши - изменение масштаба
    float wheelMove = GetMouseWheelMove();
    if (wheelMove != 0) {
        camera->zoom += wheelMove * 0.1f;
        if (camera->zoom > 3.0f) camera->zoom = 3.0f;
        else if (camera->zoom < 0.25f) camera->zoom = 0.25f;
    }
}
