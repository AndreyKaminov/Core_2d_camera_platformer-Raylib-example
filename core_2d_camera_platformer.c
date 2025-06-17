/*******************************************************************************************
*
*   raylib [core] example - 2D Camera platformer + Dust Particles on Landing
*
********************************************************************************************/

#include "raylib.h"  // Подключаем основную библиотеку raylib для графики, ввода и т.д.
#include "raymath.h" // Подключаем библиотеку raymath для работы с векторами и мат.функциями

// Константы для игрока
#define G 950                      // Константа гравитации (ускорение вниз)
#define PLAYER_JUMP_SPD 400.0f         // Начальная скорость прыжка игрока вверх
#define PLAYER_MAX_SPEED 500.0f        // Максимальная горизонтальная скорость игрока
#define PLAYER_ACCELERATION 720.0f     // Ускорение при движении влево/вправо
#define PLAYER_DECELERATION 450.0f     // Замедление, когда игрок отпускает клавишу движения
#define PLAYER_MAX_JUMP_TIME 0.50f     // Максимальное время удержания прыжка (сек)
#define PLAYER_JUMP_HOLD_FORCE 500.0f  // Сила, с которой игрок может "удерживать" прыжок

// --- Структура игрока ---
typedef struct Player {
    Vector2 position;   // Позиция игрока (x, y) в игровом мире
    float speed;        // Вертикальная скорость (для прыжка и падения)
    float velocityX;    // Горизонтальная скорость (влево/вправо)
    bool canJump;       // Может ли игрок прыгать (стоит ли он на земле)
    float jumpTime;     // Время, сколько игрок уже прыгает (для удержания прыжка)
    bool isJumping;     // Находится ли игрок сейчас в прыжке
} Player;

// --- Структура платформы/препятствия ---
typedef struct EnvItem {
    Rectangle rect;     // Прямоугольник платформы (x, y, ширина, высота)
    int blocking;       // Является ли препятствием (1 - да, 0 - нет)
    Color color;        // Цвет платформы
} EnvItem;

// ======= Пыль (частицы) =======
#define MAX_PARTICLES 500   // Максимальное количество одновременных частиц

typedef struct Particle {
    Vector2 pos;        // Позиция частицы
    Vector2 vel;        // Скорость частицы
    float life;         // Оставшееся время жизни частицы (сек)
    float maxLife;      // Максимальное время жизни частицы (сек)
    float size;         // Размер частицы (радиус)
    bool active;        // Активна ли частица (true - отрисовывается, false - свободна)
} Particle;

Particle particles[MAX_PARTICLES] = {0};    // Массив всех частиц, изначально все неактивны

// --- Функция спавна пыли при приземлении ---
void SpawnDustParticles(Vector2 pos, int count)
{
    for (int i = 0; i < MAX_PARTICLES && count > 0; i++) // Перебираем массив частиц и активируем свободные
    {
        if (!particles[i].active) // Если частица неактивна, можно использовать
        {
            float angle = DEG2RAD * (GetRandomValue(200, 340)); // Случайный угол разлёта (в стороны)
            float speed = GetRandomValue(50, 120) / 100.0f; // Случайная скорость (0.5 - 1.2)
            particles[i].pos = pos; // Позиция появления (под игроком)
            particles[i].vel = (Vector2){ cosf(angle) * speed, -fabsf(sinf(angle) * speed) }; // Скорость (разлёт в стороны и чуть вверх)
            particles[i].life = 1.5f; // Время жизни частицы (сек)
            particles[i].maxLife = 1.5f; // Максимальное время жизни
            particles[i].size = GetRandomValue(6, 12); // Размер частицы (6 - 12 пикселей)
            particles[i].active = true; // Активируем частицу
            count--; // Уменьшаем счётчик новых частиц
        }
    }
}

// --- Обновление всех частиц ---
void UpdateParticles(float dt)
{
    for (int i = 0; i < MAX_PARTICLES; i++) // Перебираем все частицы
    {
        if (particles[i].active) // Только для активных частиц
        {
            particles[i].pos.x += particles[i].vel.x * 80.0f * dt; // Движение по X
            particles[i].pos.y += particles[i].vel.y * 80.0f * dt; // Движение по Y
            particles[i].vel.y += 0.5f * dt; // Лёгкая гравитация на частицу
            particles[i].life -= dt; // Уменьшаем время жизни
            if (particles[i].life <= 0.0f) // Если жизнь закончилась
                particles[i].active = false; // Деактивируем частицу
        }
    }
}

// --- Отрисовка всех частиц ---
void DrawParticles(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++) // Перебираем все частицы
    {
        if (particles[i].active) // Только для активных
        {
            float alpha = particles[i].life / particles[i].maxLife; // Прозрачность по времени жизни
            Color c = Fade(WHITE, alpha); // Цвет с прозрачностью
            float currentSize = particles[i].size * alpha; // Размер уменьшается с течением времени
            DrawCircleV(particles[i].pos, currentSize, c); // Рисуем круг
        }
    }
}

// ======= Прототипы функций управления игроком и камерой =======
void UpdatePlayer(Player *player, EnvItem *envItems, int envItemsLength, float delta, bool *justLanded, Vector2 *landPos); // Обновление игрока и детекция приземления
void UpdateCameraCenter(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height); // Камера: всегда по центру игрока
void UpdateCameraCenterInsideMap(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height); // Камера: центр игрока, но не выходит за пределы карты
void UpdateCameraCenterSmoothFollow(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height); // Камера: плавно следует за игроком
void UpdateCameraEvenOutOnLanding(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height); // Камера: выравнивание после приземления
void UpdateCameraPlayerBoundsPush(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height); // Камера: игрок "толкает" границы экрана

int main(void)
{
    const int screenWidth = 1600; // Ширина окна
    const int screenHeight = 800; // Высота окна

    InitWindow(screenWidth, screenHeight, "PlatformerTest + Dust"); // Создаём окно

    Player player = { 0 }; // Обнуляем все поля структуры игрока
    player.position = (Vector2){ 400, 280 }; // Начальная позиция игрока
    player.speed = 0; // Вертикальная скорость = 0
    player.velocityX = 0; // Горизонтальная скорость = 0
    player.canJump = false; // По умолчанию прыгать нельзя
    player.jumpTime = 0.0f; // Время прыжка = 0
    player.isJumping = false; // Не прыгает

    //LEVELDESIGN
    EnvItem envItems[] = { // Массив платформ (препятствий)
        {{ 0, 0,  1000,   400 },      0,       LIGHTGRAY }, // Фон (не препятствие)
        {{ 0, 400, 5000, 200 }, 1, GRAY }, // Земля (препятствие)
        {{ 0, -10, 50, 2000 }, 1, GRAY }, // Левая стена
        {{ 300, 200, 400, 10 }, 1, GRAY }, // Платформа (препятствие)
        {{ 250, 300, 100, 10 }, 1, GRAY }, // Платформа (препятствие)
        {{ 650, 300, 100, 10 }, 1, GRAY }, // Платформа (препятствие)
        {{ 800, 300, 100, 20 }, 1, ORANGE } // Платформа (препятствие)
    };

    int envItemsLength = sizeof(envItems)/sizeof(envItems[0]); // Количество платформ (автоматически)

    Camera2D camera = { 0 }; // Обнуляем все поля структуры камеры
    camera.target = player.position; // Центр камеры на игроке
    camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f }; // Смещение камеры (центр экрана)
    camera.rotation = 0.0f; // Без поворота
    camera.zoom = 1.0f; // Масштаб 1:1

    void (*cameraUpdaters[])(Camera2D*, Player*, EnvItem*, int, float, int, int) = { // Массив функций для разных режимов камеры
        UpdateCameraCenter,
        UpdateCameraCenterInsideMap,
        UpdateCameraCenterSmoothFollow,
        UpdateCameraEvenOutOnLanding,
        UpdateCameraPlayerBoundsPush
    };

    int cameraOption = 0; // Текущий режим камеры (индекс)
    int cameraUpdatersLength = sizeof(cameraUpdaters)/sizeof(cameraUpdaters[0]); // Количество режимов камеры

    char *cameraDescriptions[] = { // Описания режимов камеры
        "Follow player center",
        "Follow player center, but clamp to map edges",
        "Follow player center; smoothed",
        "Follow player center horizontally; update player center vertically after landing",
        "Player push camera on getting too close to screen edge"
    };

    SetTargetFPS(60); // Устанавливаем частоту кадров 60 FPS

    while (!WindowShouldClose()) // Главный игровой цикл, пока окно не закрыто
    {
        float deltaTime = GetFrameTime(); // Получаем время между кадрами (секунды)

        bool justLanded = false; // Флаг: только что приземлились
        Vector2 landPos = {0,0}; // Координаты приземления
        UpdatePlayer(&player, envItems, envItemsLength, deltaTime, &justLanded, &landPos); // Обновляем состояние игрока

        if (justLanded) // Если только что приземлились
            SpawnDustParticles((Vector2){player.position.x, player.position.y+1}, 12); // Спавним пыль при приземлении

        UpdateParticles(deltaTime); // Обновляем частицы

        camera.zoom += ((float)GetMouseWheelMove()*0.05f); // Масштаб камеры меняется колесиком мыши
        if (camera.zoom > 3.0f) camera.zoom = 3.0f; // Ограничение максимального масштаба
        else if (camera.zoom < 0.25f) camera.zoom = 0.25f; // Ограничение минимального масштаба

        if (IsKeyPressed(KEY_R)) // Если нажата клавиша R
        {
            camera.zoom = 1.0f; // Сбросить масштаб камеры
            player.position = (Vector2){ 400, 280 }; // Сбросить позицию игрока
        }

        if (IsKeyPressed(KEY_C)) cameraOption = (cameraOption + 1)%cameraUpdatersLength; // Если нажата клавиша C переключить режим камеры

        cameraUpdaters[cameraOption](&camera, &player, envItems, envItemsLength, deltaTime, screenWidth, screenHeight); // Обновить камеру

        BeginDrawing(); // Начинаем рисовать кадр

            ClearBackground(LIGHTGRAY); // Очищаем фон

            BeginMode2D(camera); // Включаем режим 2D-камеры

                for (int i = 0; i < envItemsLength; i++) DrawRectangleRec(envItems[i].rect, envItems[i].color); // Рисуем все платформы

                Rectangle playerRect = { player.position.x - 20, player.position.y - 40, 40.0f, 40.0f }; // Прямоугольник игрока
                DrawRectangleRec(playerRect, BLUE); // Рисуем игрока

                DrawCircleV(player.position, 5.0f, GOLD); // Рисуем точку в центре игрока

                DrawParticles(); // Рисуем частицы пыли

            EndMode2D(); // Выключаем режим 2D-камеры

            DrawText("Controls:", 20, 20, 10, BLACK); // Подсказки управления
            DrawText("- Right/Left to move", 40, 40, 10, DARKGRAY);
            DrawText("- Space to jump", 40, 60, 10, DARKGRAY);
            DrawText("- Mouse Wheel to Zoom in-out, R to reset zoom", 40, 80, 10, DARKGRAY);
            DrawText("- C to change camera mode", 40, 100, 10, DARKGRAY);
            DrawText("Current camera mode:", 20, 120, 10, BLACK);
            DrawText(cameraDescriptions[cameraOption], 40, 140, 10, DARKGRAY);

            { // FPS в правом верхнем углу
                const int fpsFontSize = 20; // Размер шрифта FPS
                const int padding = 10; // Отступ от края
                const char *fpsText = TextFormat("FPS: %d", GetFPS()); // Текст с FPS
                int textWidth = MeasureText(fpsText, fpsFontSize); // Ширина текста
                int xPos = GetScreenWidth() - textWidth - padding; // X-координата
                int yPos = padding; // Y-координата
                DrawText(fpsText, xPos, yPos, fpsFontSize, DARKBLUE); // Рисуем FPS
            }

        EndDrawing(); // Заканчиваем рисовать кадр
    }

    CloseWindow(); // Закрываем окно

    return 0; // Завершаем программу
}

// ======= Игрок с контролем высоты прыжка и коллизиями =======
void UpdatePlayer(Player *player, EnvItem *envItems, int envItemsLength, float delta, bool *justLanded, Vector2 *landPos)
{
    static bool wasOnGround = false; // Был ли игрок на земле на прошлом кадре

    // --- Горизонтальное движение с ускорением и замедлением ---
    float targetSpeed = 0.0f; // Целевая скорость (куда хотим двигаться)
    if (IsKeyDown(KEY_LEFT)) targetSpeed -= PLAYER_MAX_SPEED; // Если нажата влево, уменьшаем скорость
    if (IsKeyDown(KEY_RIGHT)) targetSpeed += PLAYER_MAX_SPEED; // Если нажата вправо, увеличиваем скорость

    if (targetSpeed != 0)
    {
        if (player->velocityX < targetSpeed)
        {
            player->velocityX += PLAYER_ACCELERATION * delta; // Ускоряемся вправо
            if (player->velocityX > targetSpeed) player->velocityX = targetSpeed; // Не превышаем целевую скорость
        }
        else if (player->velocityX > targetSpeed)
        {
            player->velocityX -= PLAYER_ACCELERATION * delta; // Ускоряемся влево
            if (player->velocityX < targetSpeed) player->velocityX = targetSpeed; // Не превышаем целевую скорость
        }
    }
    else
    {
        if (player->velocityX > 0)
        {
            player->velocityX -= PLAYER_DECELERATION * delta; // Замедляемся вправо
            if (player->velocityX < 0) player->velocityX = 0; // Не уходим в отрицательную скорость
        }
        else if (player->velocityX < 0)
        {
            player->velocityX += PLAYER_DECELERATION * delta; // Замедляемся влево
            if (player->velocityX > 0) player->velocityX = 0; // Не уходим в положительную скорость
        }
    }

    // --- Прыжок (контроль по времени удержания) ---
    if (IsKeyPressed(KEY_SPACE) && player->canJump)
    {
        player->speed = -PLAYER_JUMP_SPD; // Начальная скорость прыжка вверх
        player->canJump = false; // Пока прыгаем, прыгать нельзя
        player->isJumping = true; // Флаг: сейчас прыгаем
        player->jumpTime = 0.0f; // Сбросить таймер прыжка
    }

    // Если прыжок начат и пробел удерживается, а время удержания не превышено
    if (IsKeyDown(KEY_SPACE) && player->isJumping && player->jumpTime < PLAYER_MAX_JUMP_TIME)
    {
        player->speed -= PLAYER_JUMP_HOLD_FORCE * delta; // Добавляем силу прыжка пока держим пробел
        player->jumpTime += delta; // Увеличиваем таймер прыжка
    }
    else
    {
        player->isJumping = false; // Если пробел отпущен или время вышло — больше не прыгаем
    }

    // --- Гравитация ---
    player->speed += G * delta; // Всегда действует гравитация

    // Размеры игрока
    float playerWidth = 40.0f; // Ширина игрока
    float playerHeight = 40.0f; // Высота игрока

    // --- Сначала движение по X, потом по Y ---
    // 1. Горизонтальное перемещение и коллизии
    float newX = player->position.x + player->velocityX * delta; // Новая позиция по X
    Rectangle newPlayerRectX = { newX - playerWidth/2, player->position.y - playerHeight, playerWidth, playerHeight }; // Прямоугольник игрока после перемещения по X

    for (int i = 0; i < envItemsLength; i++) // Перебираем все платформы
    {
        if (envItems[i].blocking) // Только препятствия
        {
            Rectangle envRect = envItems[i].rect; // Прямоугольник платформы
            if (CheckCollisionRecs(newPlayerRectX, envRect)) // Если есть пересечение
            {
                if (player->velocityX > 0) // Движение вправо
                {
                    newX = envRect.x - playerWidth/2; // Ставим игрока вплотную к левой стороне платформы
                }
                else if (player->velocityX < 0) // Движение влево
                {
                    newX = envRect.x + envRect.width + playerWidth/2; // Ставим игрока вплотную к правой стороне платформы
                }
                player->velocityX = 0; // Останавливаем по X
                break; // Выходим из цикла, чтобы не было дребезга между несколькими платформами
            }
        }
    }
    player->position.x = newX; // Обновляем позицию по X

    // 2. Вертикальное перемещение и коллизии
    float newY = player->position.y + player->speed * delta; // Новая позиция по Y
    Rectangle newPlayerRectY = { player->position.x - playerWidth/2, newY - playerHeight, playerWidth, playerHeight }; // Прямоугольник игрока после перемещения по Y

    bool onGround = false; // Флаг: стоим ли на земле
    for (int i = 0; i < envItemsLength; i++) // Перебираем все платформы
    {
        if (envItems[i].blocking) // Только препятствия
        {
            Rectangle envRect = envItems[i].rect; // Прямоугольник платформы
            if (CheckCollisionRecs(newPlayerRectY, envRect)) // Если есть пересечение
            {
                if (player->speed > 0) // Падение вниз
                {
                    newY = envRect.y; // Ставим игрока на платформу
                    onGround = true; // Стоим на земле
                }
                else if (player->speed < 0) // Прыжок вверх
                {
                    newY = envRect.y + envRect.height + playerHeight; // Ставим игрока под потолок
                }
                player->speed = 0; // Останавливаем по Y
                break; // Выходим из цикла, чтобы не было дребезга между несколькими платформами
            }
        }
    }
    player->position.y = newY; // Обновляем позицию по Y

    // --- Проверка приземления для пыли ---
    *justLanded = (!wasOnGround && onGround); // Если только что приземлились
    if (*justLanded) *landPos = player->position; // Сохраняем позицию приземления
    wasOnGround = onGround; // Запоминаем состояние на следующий кадр
    player->canJump = onGround; // Можно прыгать только если стоим на земле
}

// --- Заглушки для функций камеры (оставьте свои реализации, если нужно) ---
void UpdateCameraCenter(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{
    camera->target = player->position; // Камера всегда по центру игрока
}
void UpdateCameraCenterInsideMap(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{
    camera->target = player->position; // Камера по центру игрока, но не выходит за карту
}
void UpdateCameraCenterSmoothFollow(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{
    camera->target = Vector2Lerp(camera->target, player->position, 0.1f); // Плавное следование камеры за игроком
}
void UpdateCameraEvenOutOnLanding(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{
    camera->target = player->position; // Камера выравнивается после приземления
}
void UpdateCameraPlayerBoundsPush(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{
    camera->target = player->position; // Камера: игрок "толкает" границы экрана
}
