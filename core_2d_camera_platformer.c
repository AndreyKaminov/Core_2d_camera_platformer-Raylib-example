#include <stdio.h> // Для функций форматирования строк
/*******************************************************************************************
*
*   raylib [core] example - 2D Camera platformer + Dust Particles + JumpThru Platforms
*
********************************************************************************************/

#include "raylib.h" // Подключение библиотеки raylib
#include "raymath.h" // Подключение библиотеки raymath

// --- Константы игрока ---
#define G 900 // Гравитация
#define PLAYER_JUMP_SPD 350.0f // Скорость прыжка игрока
#define PLAYER_MAX_SPEED 500.0f // Максимальная горизонтальная скорость игрока
#define PLAYER_ACCELERATION 400.0f // Ускорение игрока
#define PLAYER_DECELERATION 280.0f // Замедление игрока
#define PLAYER_MAX_JUMP_TIME 0.30f // Максимальное время удержания прыжка
#define PLAYER_JUMP_HOLD_FORCE 350.0f // Сила удержания прыжка
#define PLAYER_DASH_SPEED 900.0f // Скорость рывка
#define PLAYER_DASH_TIME 0.18f   // Длительность рывка (сек)

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
<<<<<<< HEAD
    float dashCooldown; // Кулдаун между дэшами
=======
>>>>>>> 14b060b5641763622f26fa33fcea2aa8b3c63fb6
} Player;

// --- Структура платформы ---
typedef struct EnvItem {
    Rectangle rect;     // Прямоугольник платформы
    PlatformType type;  // Тип платформы
    Color color;        // Цвет платформы
} EnvItem;

// --- Частицы пыли ---
#define MAX_PARTICLES 500 // Максимальное количество частиц
typedef struct Particle {
    Vector2 pos;  // Позиция частицы
    Vector2 vel; // Скорость частицы
    float life; // Оставшееся время жизни
    float maxLife; // Максимальное время жизни
    float size; // Размер частицы
    bool active; // Активна ли частица
} Particle;
Particle particles[MAX_PARTICLES] = {0}; // Массив частиц

// --- Функция спавна пыли ---
void SpawnDustParticles(Vector2 pos, int count)
{
    for (int i = 0; i < MAX_PARTICLES && count > 0; i++) // Перебор всех частиц, пока не заспавнили нужное количество
    {
        if (!particles[i].active) // Если частица неактивна
        {
            float angle = DEG2RAD * (GetRandomValue(200, 340)); // Случайный угол разлета
            float speed = GetRandomValue(50, 120) / 100.0f; // Случайная скорость
            particles[i].pos = pos; // Устанавливаем позицию частицы
            particles[i].vel = (Vector2){ cosf(angle) * speed, -fabsf(sinf(angle) * speed) }; // Устанавливаем скорость
            particles[i].life = 1.5f; // Время жизни частицы
            particles[i].maxLife = 1.5f; // Максимальное время жизни
            particles[i].size = GetRandomValue(6, 12); // Размер частицы
            particles[i].active = true; // Активируем частицу
            count--; // Уменьшаем счетчик требуемых частиц
        }
    }
}

// --- Обновление частиц ---
void UpdateParticles(float dt)
{
    for (int i = 0; i < MAX_PARTICLES; i++) // Перебор всех частиц
    {
        if (particles[i].active) // Если частица активна
        {
            particles[i].pos.x += particles[i].vel.x * 80.0f * dt; // Обновляем позицию по X
            particles[i].pos.y += particles[i].vel.y * 80.0f * dt; // Обновляем позицию по Y
            particles[i].vel.y += 0.5f * dt; // Добавляем гравитацию по Y
            particles[i].life -= dt; // Уменьшаем время жизни
            if (particles[i].life <= 0.0f)
                particles[i].active = false; // Деактивируем частицу, если жизнь закончилась
        }
    }
}

// --- Рисование частиц ---
void DrawParticles(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++) // Перебор всех частиц
    {
        if (particles[i].active) // Если частица активна
        {
            float alpha = particles[i].life / particles[i].maxLife; // Вычисляем прозрачность
            Color c = Fade(WHITE, alpha); // Получаем цвет с нужной прозрачностью
            float currentSize = particles[i].size * alpha; // Размер частицы зависит от жизни
            DrawCircleV(particles[i].pos, currentSize, c); // Рисуем частицу
        }
    }
}

// --- Прототипы функций управления игроком и камерой ---
void UpdatePlayer(Player *player, EnvItem *envItems, int envItemsLength, float delta, bool *justLanded, Vector2 *landPos); // Обновление состояния игрока
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
<<<<<<< HEAD
    player.dashCooldown = 0.0f; // Кулдаун дэша
=======
>>>>>>> 14b060b5641763622f26fa33fcea2aa8b3c63fb6

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

    Camera2D camera = {0}; // Структура камеры
    camera.target = player.position; // Камера смотрит на игрока
    camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f }; // Центр экрана
    camera.rotation = 0.0f; // Без поворота
<<<<<<< HEAD
    camera.zoom = 2.0f; // Масштаб 1:1
    float cameraTargetZoom = 2.0f; // Желаемый zoom камеры
=======
    camera.zoom = 1.0f; // Масштаб 1:1
>>>>>>> 14b060b5641763622f26fa33fcea2aa8b3c63fb6

    void (*cameraUpdaters[])(Camera2D*, Player*, EnvItem*, int, float, int, int) = {
        UpdateCameraCenter,
        UpdateCameraCenterInsideMap,
        UpdateCameraCenterSmoothFollow,
        UpdateCameraEvenOutOnLanding,
        UpdateCameraPlayerBoundsPush
    }; // Массив указателей на функции обновления камеры
<<<<<<< HEAD
    int cameraOption = 2; // Текущий режим камеры
=======
    int cameraOption = 0; // Текущий режим камеры
>>>>>>> 14b060b5641763622f26fa33fcea2aa8b3c63fb6
    int cameraUpdatersLength = sizeof(cameraUpdaters)/sizeof(cameraUpdaters[0]); // Количество режимов камеры
    char *cameraDescriptions[] = {
        "Follow player center",
        "Follow player center, but clamp to map edges",
        "Follow player center; smoothed",
        "Follow player center horizontally; update player center vertically after landing",
        "Player push camera on getting too close to screen edge"
    }; // Описания режимов камеры

<<<<<<< HEAD
    SetTargetFPS(144); // 144 кадров в секунду
=======
    SetTargetFPS(60); // 60 кадров в секунду
>>>>>>> 14b060b5641763622f26fa33fcea2aa8b3c63fb6

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
        Vector2 landPos = {0,0};
        UpdatePlayer(&player, envItems, envItemsLength, deltaTime, &justLanded, &landPos);

        if (justLanded)
            SpawnDustParticles((Vector2){player.position.x, player.position.y+1}, 12);

        UpdateParticles(deltaTime);

        camera.zoom += ((float)GetMouseWheelMove()*0.05f);
        if (camera.zoom > 3.0f) camera.zoom = 3.0f;
        else if (camera.zoom < 0.25f) camera.zoom = 0.25f;

        // --- Эффект отдаления камеры при прыжке ---
        if (!player.canJump || fabs(player.velocityX) >= PLAYER_MAX_SPEED) {
            cameraTargetZoom = 1.7f; // Отдалить камеру при прыжке или максимальной скорости
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

        BeginDrawing();

            ClearBackground(LIGHTGRAY);

            BeginMode2D(camera);

                for (int i = 0; i < envItemsLength; i++) DrawRectangleRec(envItems[i].rect, envItems[i].color);

                Rectangle playerRect = { player.position.x - 20, player.position.y - 40, 40.0f, 40.0f };
                DrawRectangleRec(playerRect, BLUE);

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

    return 0;
}

// --- Игрок с поддержкой JumpThru платформ и drop-down ---
void UpdatePlayer(Player *player, EnvItem *envItems, int envItemsLength, float delta, bool *justLanded, Vector2 *landPos)
{
    static bool wasOnGround = false;

    // --- DASH (рывок) ---
<<<<<<< HEAD
    if (player->dashCooldown > 0.0f) player->dashCooldown -= delta;

    if (!player->dashing && (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT))) {
        // Рывок только если есть движение влево или вправо и кулдаун закончился
        if (player->dashCooldown <= 0.0f) {
            if (IsKeyDown(KEY_LEFT)) {
                player->dashing = true;
                player->dashTime = PLAYER_DASH_TIME;
                player->velocityX = -PLAYER_DASH_SPEED;
                player->dashCooldown = 1.5f; // Устанавливаем кулдаун
            } else if (IsKeyDown(KEY_RIGHT)) {
                player->dashing = true;
                player->dashTime = PLAYER_DASH_TIME;
                player->velocityX = PLAYER_DASH_SPEED;
                player->dashCooldown = 1.5f; // Устанавливаем кулдаун
            }
=======
    if (!player->dashing && (IsKeyPressed(KEY_LEFT_SHIFT) || IsKeyPressed(KEY_RIGHT_SHIFT))) {
        // Рывок только если есть движение влево или вправо
        if (IsKeyDown(KEY_LEFT)) {
            player->dashing = true;
            player->dashTime = PLAYER_DASH_TIME;
            player->velocityX = -PLAYER_DASH_SPEED;
        } else if (IsKeyDown(KEY_RIGHT)) {
            player->dashing = true;
            player->dashTime = PLAYER_DASH_TIME;
            player->velocityX = PLAYER_DASH_SPEED;
>>>>>>> 14b060b5641763622f26fa33fcea2aa8b3c63fb6
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
            }
        } else {
            player->jumpCount = 0; // Если прыжок не на максимальной скорости, сбрасываем счетчик
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
    if (*justLanded) *landPos = player->position;
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
