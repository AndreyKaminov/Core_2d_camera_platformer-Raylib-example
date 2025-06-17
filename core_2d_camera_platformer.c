/*******************************************************************************************
*
*   raylib [core] example - 2D Camera platformer + Dust Particles + JumpThru Platforms
*
********************************************************************************************/

#include "raylib.h"
#include "raymath.h"

// --- Константы игрока ---
#define G 900
#define PLAYER_JUMP_SPD 350.0f
#define PLAYER_MAX_SPEED 500.0f
#define PLAYER_ACCELERATION 400.0f
#define PLAYER_DECELERATION 280.0f
#define PLAYER_MAX_JUMP_TIME 0.30f
#define PLAYER_JUMP_HOLD_FORCE 350.0f

// --- Типы платформ ---
typedef enum { PLATFORM_NONE = 0, PLATFORM_SOLID = 1, PLATFORM_JUMPTHRU = 2 } PlatformType;

// --- Структура игрока ---
typedef struct Player {
    Vector2 position;   // Центр ног игрока
    float speed;        // Вертикальная скорость
    float velocityX;    // Горизонтальная скорость
    bool canJump;       // Может прыгать
    float jumpTime;     // Время удержания прыжка
    bool isJumping;     // Сейчас прыгает
    bool dropDown;      // Флаг: инициировано спрыгивание с JumpThru
} Player;

// --- Структура платформы ---
typedef struct EnvItem {
    Rectangle rect;     // Прямоугольник платформы
    PlatformType type;  // Тип платформы
    Color color;        // Цвет платформы
} EnvItem;

// --- Частицы пыли ---
#define MAX_PARTICLES 500
typedef struct Particle {
    Vector2 pos;
    Vector2 vel;
    float life;
    float maxLife;
    float size;
    bool active;
} Particle;
Particle particles[MAX_PARTICLES] = {0};

// --- Функция спавна пыли ---
void SpawnDustParticles(Vector2 pos, int count)
{
    for (int i = 0; i < MAX_PARTICLES && count > 0; i++)
    {
        if (!particles[i].active)
        {
            float angle = DEG2RAD * (GetRandomValue(200, 340));
            float speed = GetRandomValue(50, 120) / 100.0f;
            particles[i].pos = pos;
            particles[i].vel = (Vector2){ cosf(angle) * speed, -fabsf(sinf(angle) * speed) };
            particles[i].life = 1.5f;
            particles[i].maxLife = 1.5f;
            particles[i].size = GetRandomValue(6, 12);
            particles[i].active = true;
            count--;
        }
    }
}

// --- Обновление частиц ---
void UpdateParticles(float dt)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (particles[i].active)
        {
            particles[i].pos.x += particles[i].vel.x * 80.0f * dt;
            particles[i].pos.y += particles[i].vel.y * 80.0f * dt;
            particles[i].vel.y += 0.5f * dt;
            particles[i].life -= dt;
            if (particles[i].life <= 0.0f)
                particles[i].active = false;
        }
    }
}

// --- Рисование частиц ---
void DrawParticles(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (particles[i].active)
        {
            float alpha = particles[i].life / particles[i].maxLife;
            Color c = Fade(WHITE, alpha);
            float currentSize = particles[i].size * alpha;
            DrawCircleV(particles[i].pos, currentSize, c);
        }
    }
}

// --- Прототипы функций управления игроком и камерой ---
void UpdatePlayer(Player *player, EnvItem *envItems, int envItemsLength, float delta, bool *justLanded, Vector2 *landPos);
void UpdateCameraCenter(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height);
void UpdateCameraCenterInsideMap(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height);
void UpdateCameraCenterSmoothFollow(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height);
void UpdateCameraEvenOutOnLanding(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height);
void UpdateCameraPlayerBoundsPush(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height);

int main(void)
{
    const int screenWidth = 1600;
    const int screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "PlatformerTest + Dust + JumpThru");

    Player player = {0};
    player.position = (Vector2){ 400, 280 };
    player.speed = 0;
    player.velocityX = 0;
    player.canJump = false;
    player.jumpTime = 0.0f;
    player.isJumping = false;
    player.dropDown = false;

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
    int envItemsLength = sizeof(envItems)/sizeof(envItems[0]);

    Camera2D camera = {0};
    camera.target = player.position;
    camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };
    camera.rotation = 0.0f;
    camera.zoom = 1.0f;

    void (*cameraUpdaters[])(Camera2D*, Player*, EnvItem*, int, float, int, int) = {
        UpdateCameraCenter,
        UpdateCameraCenterInsideMap,
        UpdateCameraCenterSmoothFollow,
        UpdateCameraEvenOutOnLanding,
        UpdateCameraPlayerBoundsPush
    };
    int cameraOption = 0;
    int cameraUpdatersLength = sizeof(cameraUpdaters)/sizeof(cameraUpdaters[0]);
    char *cameraDescriptions[] = {
        "Follow player center",
        "Follow player center, but clamp to map edges",
        "Follow player center; smoothed",
        "Follow player center horizontally; update player center vertically after landing",
        "Player push camera on getting too close to screen edge"
    };

    SetTargetFPS(60);

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
            player.speed = 1.0f; // Дать минимальную скорость вниз, чтобы сразу провалиться
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

        if (IsKeyPressed(KEY_R))
        {
            camera.zoom = 1.0f;
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

    // --- Прыжок с контролем по времени удержания ---
    if (IsKeyPressed(KEY_SPACE) && player->canJump && !player->dropDown)
    {
        player->speed = -PLAYER_JUMP_SPD;
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

            // Если dropDown активен — игнорируем платформу, пока игрок не ушёл ниже её верхней грани
            if (player->dropDown) {
                if (prevBottom > platTop + 1.0f) {
                    player->dropDown = false; // Сбросить dropDown после выхода вниз
                }
                continue;
            }

            // Если падаем сверху и НЕ dropDown — обычная посадка на платформу
            if (player->speed > 0 &&
                prevBottom <= platTop + 2.0f &&
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
