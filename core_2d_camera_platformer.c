/*******************************************************************************************
*
*   raylib [core] example - 2D Camera platformer + Dust Particles on Landing
*
********************************************************************************************/

#include "raylib.h"
#include "raymath.h"

// Константы для игрока
#define G 750 // гравитация
#define PLAYER_JUMP_SPD 350.0f
#define PLAYER_MAX_SPEED 500.0f
#define PLAYER_ACCELERATION 400.0f
#define PLAYER_DECELERATION 280.0f
#define PLAYER_MAX_JUMP_TIME 0.30f
#define PLAYER_JUMP_HOLD_FORCE 350.0f

typedef struct Player {
    Vector2 position;
    float speed;
    float velocityX;
    bool canJump;
    float jumpTime;
    bool isJumping;
} Player;

typedef struct EnvItem {
    Rectangle rect;
    int blocking;
    Color color;
} EnvItem;

// ======= Пыль (частицы) =======
#define MAX_PARTICLES 128

typedef struct Particle {
    Vector2 pos;
    Vector2 vel;
    float life;
    float maxLife;
    float size;
    bool active;
} Particle;

Particle particles[MAX_PARTICLES] = {0};

void SpawnDustParticles(Vector2 pos, int count)
{
    for (int i = 0; i < MAX_PARTICLES && count > 0; i++)
    {
        if (!particles[i].active)
        {
            float angle = DEG2RAD * (GetRandomValue(200, 340)); // В стороны
            float speed = GetRandomValue(50, 120) / 100.0f;
            particles[i].pos = pos;
            particles[i].vel = (Vector2){ cosf(angle) * speed, -fabsf(sinf(angle) * speed) };
            particles[i].life = 1.5f;
            particles[i].maxLife = 1.5f;
            particles[i].size = GetRandomValue(6, 12); // УВЕЛИЧЕНО В 10 РАЗ (было /10.0f)
            particles[i].active = true;
            count--;
        }
    }
}

void UpdateParticles(float dt)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (particles[i].active)
        {
            particles[i].pos.x += particles[i].vel.x * 80.0f * dt;
            particles[i].pos.y += particles[i].vel.y * 80.0f * dt;
            particles[i].vel.y += 0.5f * dt; // легкая гравитация
            particles[i].life -= dt;
            if (particles[i].life <= 0.0f)
                particles[i].active = false;
        }
    }
}

void DrawParticles(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (particles[i].active)
        {
            float alpha = particles[i].life / particles[i].maxLife;
            Color c = Fade(WHITE, alpha);
            DrawCircleV(particles[i].pos, particles[i].size, c);
        }
    }
}

// ======= Прототипы =======
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

    InitWindow(screenWidth, screenHeight, "PlatformerTest + Dust");

    Player player = { 0 };
    player.position = (Vector2){ 400, 280 };
    player.speed = 0;
    player.velocityX = 0;
    player.canJump = false;
    player.jumpTime = 0.0f;
    player.isJumping = false;

    EnvItem envItems[] = {
        {{ 0, 0, 1000, 400 }, 0, LIGHTGRAY },
        {{ 0, 400, 1000, 200 }, 1, GRAY },
        {{ 300, 200, 400, 10 }, 1, GRAY },
        {{ 250, 300, 100, 10 }, 1, GRAY },
        {{ 650, 300, 100, 10 }, 1, GRAY }
    };

    int envItemsLength = sizeof(envItems)/sizeof(envItems[0]);

    Camera2D camera = { 0 };
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

    // Для детекции приземления:
    bool wasOnGround = false;

    while (!WindowShouldClose())
    {
        float deltaTime = GetFrameTime();

        // --- Детекция приземления ---
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
            DrawText("- Mouse Wheel to Zoom in-out, R to reset zoom", 40, 80, 10, DARKGRAY);
            DrawText("- C to change camera mode", 40, 100, 10, DARKGRAY);
            DrawText("Current camera mode:", 20, 120, 10, BLACK);
            DrawText(cameraDescriptions[cameraOption], 40, 140, 10, DARKGRAY);

            // FPS в правом верхнем углу
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

// ======= Игрок с детекцией приземления =======
void UpdatePlayer(Player *player, EnvItem *envItems, int envItemsLength, float delta, bool *justLanded, Vector2 *landPos)
{
    static bool wasOnGround = false;

    // Horizontal Movement with Acceleration and Deceleration
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
    player->position.x += player->velocityX * delta;

    // Jumping and gravity
    if (IsKeyPressed(KEY_SPACE) && player->canJump)
    {
        player->speed = -PLAYER_JUMP_SPD;
        player->canJump = false;
        player->isJumping = true;
        player->jumpTime = 0.0f;
    }

    if (IsKeyDown(KEY_SPACE) && player->isJumping && player->jumpTime < PLAYER_MAX_JUMP_TIME)
    {
        player->speed = -PLAYER_JUMP_HOLD_FORCE;
        player->jumpTime += delta;
    }
    else
    {
        player->isJumping = false;
    }

    bool hitObstacle = false;
    for (int i = 0; i < envItemsLength; i++)
    {
        EnvItem *ei = envItems + i;
        Vector2 *p = &(player->position);
        if (ei->blocking &&
            ei->rect.x <= p->x &&
            ei->rect.x + ei->rect.width >= p->x &&
            ei->rect.y >= p->y &&
            ei->rect.y <= p->y + player->speed*delta)
        {
            hitObstacle = true;
            player->speed = 0.0f;
            p->y = ei->rect.y;
            break;
        }
    }

    bool onGround = hitObstacle;

    if (!hitObstacle)
    {
        player->position.y += player->speed*delta;
        player->speed += G*delta;
        player->canJump = false;
    }
    else
    {
        player->canJump = true;
        player->isJumping = false;
        player->jumpTime = 0.0f;
    }

    // Детект приземления
    *justLanded = false;
    if (!wasOnGround && onGround)
    {
        *justLanded = true;
        *landPos = player->position;
    }
    wasOnGround = onGround;
}

// ======= Функции камеры =======
void UpdateCameraCenter(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{
    camera->offset = (Vector2){ width/2.0f, height/2.0f };
    camera->target = player->position;
}

void UpdateCameraCenterInsideMap(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{
    camera->target = player->position;
    camera->offset = (Vector2){ width/2.0f, height/2.0f };
    float minX = 1000, minY = 1000, maxX = -1000, maxY = -1000;

    for (int i = 0; i < envItemsLength; i++)
    {
        EnvItem *ei = envItems + i;
        minX = fminf(ei->rect.x, minX);
        maxX = fmaxf(ei->rect.x + ei->rect.width, maxX);
        minY = fminf(ei->rect.y, minY);
        maxY = fmaxf(ei->rect.y + ei->rect.height, maxY);
    }

    Vector2 max = GetWorldToScreen2D((Vector2){ maxX, maxY }, *camera);
    Vector2 min = GetWorldToScreen2D((Vector2){ minX, minY }, *camera);

    if (max.x < width) camera->offset.x = width - (max.x - width/2);
    if (max.y < height) camera->offset.y = height - (max.y - height/2);
    if (min.x > 0) camera->offset.x = width/2 - min.x;
    if (min.y > 0) camera->offset.y = height/2 - min.y;
}

void UpdateCameraCenterSmoothFollow(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{
    static float minSpeed = 30;
    static float minEffectLength = 10;
    static float fractionSpeed = 0.8f;

    camera->offset = (Vector2){ width/2.0f, height/2.0f };
    Vector2 diff = Vector2Subtract(player->position, camera->target);
    float length = Vector2Length(diff);

    if (length > minEffectLength)
    {
        float speed = fmaxf(fractionSpeed*length, minSpeed);
        camera->target = Vector2Add(camera->target, Vector2Scale(diff, speed*delta/length));
    }
}

void UpdateCameraEvenOutOnLanding(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{
    static float evenOutSpeed = 700;
    static int eveningOut = false;
    static float evenOutTarget;

    camera->offset = (Vector2){ width/2.0f, height/2.0f };
    camera->target.x = player->position.x;

    if (eveningOut)
    {
        if (evenOutTarget > camera->target.y)
        {
            camera->target.y += evenOutSpeed*delta;

            if (camera->target.y > evenOutTarget)
            {
                camera->target.y = evenOutTarget;
                eveningOut = 0;
            }
        }
        else
        {
            camera->target.y -= evenOutSpeed*delta;

            if (camera->target.y < evenOutTarget)
            {
                camera->target.y = evenOutTarget;
                eveningOut = 0;
            }
        }
    }
    else
    {
        if (player->canJump && (player->speed == 0) && (player->position.y != camera->target.y))
        {
            eveningOut = 1;
            evenOutTarget = player->position.y;
        }
    }
}

void UpdateCameraPlayerBoundsPush(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{
    static Vector2 bbox = { 0.2f, 0.2f };

    Vector2 bboxWorldMin = GetScreenToWorld2D((Vector2){ (1 - bbox.x)*0.5f*width, (1 - bbox.y)*0.5f*height }, *camera);
    Vector2 bboxWorldMax = GetScreenToWorld2D((Vector2){ (1 + bbox.x)*0.5f*width, (1 + bbox.y)*0.5f*height }, *camera);
    camera->offset = (Vector2){ (1 - bbox.x)*0.5f * width, (1 - bbox.y)*0.5f*height };

    if (player->position.x < bboxWorldMin.x) camera->target.x = player->position.x;
    if (player->position.y < bboxWorldMin.y) camera->target.y = player->position.y;
    if (player->position.x > bboxWorldMax.x) camera->target.x = bboxWorldMin.x + (player->position.x - bboxWorldMax.x);
    if (player->position.y > bboxWorldMax.y) camera->target.y = bboxWorldMin.y + (player->position.y - bboxWorldMax.y);
}
