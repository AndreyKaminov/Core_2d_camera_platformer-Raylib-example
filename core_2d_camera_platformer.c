#include <stdio.h> // Для функций форматирования строк
#include <stdlib.h>
#include <math.h>
#include <string.h>
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

// --- Типы платформ ---
typedef enum { PLATFORM_NONE = 0, PLATFORM_SOLID = 1, PLATFORM_JUMPTHRU = 2 } PlatformType; // Типы платформ

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

// --- Структура платформы ---
typedef struct EnvItem {
    Rectangle rect;     // Прямоугольник платформы
    PlatformType type;  // Тип платформы
    Color color;        // Цвет платформы
} EnvItem;

 // --- Уровень: добавлен JumpThru справа от оранжевой платформы ---
    EnvItem envItems[] = {
        {{ 0, 0,  1000,   400 },      PLATFORM_NONE,    LIGHTGRAY }, // Фон
        {{ 0, 400, 5000, 200 }, PLATFORM_SOLID, GRAY },             // Земля
        {{ 0, -10, 50, 2000 }, PLATFORM_SOLID, GRAY },              // Левая стена
        {{ 300, 200, 400, 10 }, PLATFORM_SOLID, GRAY },             // Платформа
        {{ 250, 300, 100, 10 }, PLATFORM_SOLID, GRAY },             // Платформа
        {{ 650, 300, 100, 10 }, PLATFORM_SOLID, GRAY },             // Платформа
        {{ 800, 300, 100, 20 }, PLATFORM_SOLID, ORANGE },           // Оранжевая платформа
        {{ 950, 320, 120, 10 }, PLATFORM_JUMPTHRU, VIOLET }         // JumpThru-платформа
    };
    int envItemsLength = sizeof(envItems)/sizeof(envItems[0]); // Количество платформ

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
        particles[slot].life = 1.2f;
        particles[slot].maxLife = 1.2f;
        particles[slot].size = GetRandomValue(10, 20);
        particles[slot].active = true;
        // Случайный оттенок серого/коричневого
        particles[slot].color = (Color){ 255, 255, 255, 180 }; // Белый с альфой 180
    }
}

// --- Обновление частиц ---
void UpdateParticles(float dt)
{
    // Высота платформы (земли) — ищем самую верхнюю SOLID платформу под частицей
    extern EnvItem envItems[];
    extern int envItemsLength;
    for (int i = 0; i < MAX_PARTICLES; i++) {
        if (particles[i].active) {
            particles[i].pos.x += particles[i].vel.x * 80.0f * dt;
            particles[i].pos.y += particles[i].vel.y * 80.0f * dt;
            // Слабая гравитация
            particles[i].vel.y += 0.18f * dt;
            // Проверяем, не ниже ли частица платформы
            float groundY = 1e9f;
            for (int j = 0; j < envItemsLength; j++) {
                if (envItems[j].type == PLATFORM_SOLID) {
                    Rectangle r = envItems[j].rect;
                    if (particles[i].pos.x >= r.x && particles[i].pos.x <= r.x + r.width) {
                        if (r.y < groundY && particles[i].pos.y <= r.y + 2.0f) groundY = r.y;
                    }
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
void UpdatePlayer(Player *player, EnvItem *envItems, int envItemsLength, float delta, bool *justLanded, bool *justLandedSuperJump, Vector2 *landPos); // Обновление состояния игрока
void UpdateCameraCenter(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height); // Камера по центру игрока
void UpdateCameraCenterInsideMap(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height); // Камера по центру, но в пределах карты
void UpdateCameraCenterSmoothFollow(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height); // Плавное следование камеры
void UpdateCameraEvenOutOnLanding(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height); // Камера выравнивается после приземления
void UpdateCameraPlayerBoundsPush(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height); // Камера сдвигается, если игрок у края

int main(void)
{
    const int screenWidth = 1600; // Ширина окна
    const int screenHeight = 800; // Высота окна

    InitWindow(screenWidth, screenHeight, "PlatformerTest + Dust + JumpThru"); // Инициализация окна

    playerTexture = LoadTexture(PLAYER_SPRITE_PATH); // Загружаем спрайт игрока

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

    void (*cameraUpdaters[])(Camera2D*, Player*, EnvItem*, int, float, int, int) = {
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
            player.dropDown = true;
            player.isJumping = false; // Отключаем прыжок!
            player.jumpTime = 0.0f;
            player.canJump = false;
            player.speed = 200.0f; // Даем значительную скорость вниз для проваливания
        }

        bool justLanded = false;
        bool justLandedSuperJump = false;
        Vector2 landPos = {0,0};
        UpdatePlayer(&player, envItems, envItemsLength, deltaTime, &justLanded, &justLandedSuperJump, &landPos);

        if (justLanded) {
            int dustCount = justLandedSuperJump ? 100 : 24;
            SpawnDustParticles((Vector2){player.position.x, player.position.y+1}, dustCount);
        }

        UpdateParticles(deltaTime);

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

        if (IsKeyPressed(KEY_R))
        {
            camera.zoom = 2.0f;
            player.position = (Vector2){ 400, 280 };
        }

        if (IsKeyPressed(KEY_C)) cameraOption = (cameraOption + 1)%cameraUpdatersLength;

        cameraUpdaters[cameraOption](&camera, &player, envItems, envItemsLength, deltaTime, screenWidth, screenHeight);

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

            BeginMode2D(camera);

                // Применяем скриншейк к камере
                if (screenShakeTime > 0.0f) {
                    float shakeX = GetRandomValue(-screenShakeIntensity, screenShakeIntensity);
                    float shakeY = GetRandomValue(-screenShakeIntensity, screenShakeIntensity);
                    camera.offset.x = originalCameraOffset.x + shakeX;
                    camera.offset.y = originalCameraOffset.y + shakeY;
                }

                for (int i = 0; i < envItemsLength; i++) DrawRectangleRec(envItems[i].rect, envItems[i].color);

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

                DrawParticles();

            EndMode2D();

            DrawText("Controls:", 20, 20, 10, BLACK);
            DrawText("- Right/Left to move", 40, 40, 10, DARKGRAY);
            DrawText("- Space to jump", 40, 60, 10, DARKGRAY);
            DrawText("- Down+Space to drop through JumpThru", 40, 80, 10, DARKGRAY);
            DrawText("- Mouse Wheel to Zoom in-out, R to reset zoom", 40, 100, 10, DARKGRAY);
            DrawText("- C to change camera mode", 40, 120, 10, DARKGRAY);
            DrawText("Current camera mode:", 20, 140, 10, BLACK);
            DrawText(cameraDescriptions[cameraOption], 40, 160, 10, DARKGRAY);

            // Отображение счетчика прыжков
            char jumpCountText[64];
            snprintf(jumpCountText, sizeof(jumpCountText), "Jump count: %d (superjump on 3)", player.jumpCount+1);
            DrawText(jumpCountText, 40, 180, 10, DARKGRAY);

            // Отображение количества активных частиц
            int activeParticles = 0;
            for (int i = 0; i < MAX_PARTICLES; i++) if (particles[i].active) activeParticles++;
            char particleCountText[64];
            snprintf(particleCountText, sizeof(particleCountText), "Active particles: %d", activeParticles);
            DrawText(particleCountText, 40, 200, 10, DARKGRAY);

            {
                const int fpsFontSize = 20;
                const int padding = 10;
                const char *fpsText = TextFormat("FPS: %d", GetFPS());
                int textWidth = MeasureText(fpsText, fpsFontSize);
                int xPos = GetScreenWidth() - textWidth - padding;
                int yPos = padding;
                DrawText(fpsText, xPos, yPos, fpsFontSize, DARKBLUE);
            }

        EndDrawing();
    }

    CloseWindow();

    UnloadTexture(playerTexture); // Освобождаем текстуру игрока

    return 0;
}

// --- Игрок с поддержкой JumpThru платформ и drop-down ---
void UpdatePlayer(Player *player, EnvItem *envItems, int envItemsLength, float delta, bool *justLanded, bool *justLandedSuperJump, Vector2 *landPos)
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
    Rectangle newPlayerRectX = { newX - playerWidth/2, player->position.y - playerHeight, playerWidth, playerHeight };

    for (int i = 0; i < envItemsLength; i++)
    {
        if (envItems[i].type == PLATFORM_SOLID)
        {
            Rectangle envRect = envItems[i].rect;
            if (CheckCollisionRecs(newPlayerRectX, envRect))
            {
                if (player->velocityX > 0)
                    newX = envRect.x - playerWidth/2;
                else if (player->velocityX < 0)
                    newX = envRect.x + envRect.width + playerWidth/2;
                player->velocityX = 0;
                break;
            }
        }
    }
    player->position.x = newX;

    // 2. Вертикальное перемещение и коллизии (SOLID и JumpThru)
    float newY = player->position.y + player->speed * delta;
    Rectangle newPlayerRectY = { player->position.x - playerWidth/2, newY - playerHeight, playerWidth, playerHeight };

    bool onGround = false;

    for (int i = 0; i < envItemsLength; i++)
    {
        Rectangle envRect = envItems[i].rect;

        // --- SOLID платформы ---
        if (envItems[i].type == PLATFORM_SOLID)
        {
            if (CheckCollisionRecs(newPlayerRectY, envRect))
            {
                if (player->speed > 0)
                {
                    newY = envRect.y;
                    onGround = true;
                }
                else if (player->speed < 0)
                {
                    newY = envRect.y + envRect.height + playerHeight;
                }
                player->speed = 0;
                break;
            }
        }
        // --- JumpThru платформы ---
        else if (envItems[i].type == PLATFORM_JUMPTHRU)
        {
            float prevBottom = player->position.y;
            float platTop = envRect.y;
            float platLeft = envRect.x;
            float platRight = envRect.x + envRect.width;
            float playerLeft = player->position.x - playerWidth/2;
            float playerRight = player->position.x + playerWidth/2;

            // Если dropDown активен — полностью игнорируем платформу
            if (player->dropDown) {
                if (prevBottom > platTop + 10.0f) { // Увеличиваем расстояние для сброса dropDown
                    player->dropDown = false; // Сбросить dropDown после выхода вниз
                }
                continue; // Пропускаем все проверки коллизий с этой платформой
            }

            // Если падаем сверху и НЕ dropDown — обычная посадка на платформу
            if (player->speed >= 0 &&
                prevBottom <= platTop + 8.0f && // увеличен допуск по высоте
                playerRight > platLeft + 2.0f && playerLeft < platRight - 2.0f)
            {
                if (CheckCollisionRecs(newPlayerRectY, envRect))
                {
                    newY = envRect.y;
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
void UpdateCameraCenter(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{ camera->target = player->position; }
void UpdateCameraCenterInsideMap(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{ camera->target = player->position; }
void UpdateCameraCenterSmoothFollow(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{ camera->target = Vector2Lerp(camera->target, player->position, 0.1f); }
void UpdateCameraEvenOutOnLanding(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{ camera->target = player->position; }
void UpdateCameraPlayerBoundsPush(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{ camera->target = player->position; }
