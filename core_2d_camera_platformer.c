/*******************************************************************************************
*
*   raylib [core] example - 2D Camera platformer + Dust Particles on Landing
*
********************************************************************************************/

#include "raylib.h"  // Подключение основной библиотеки raylib для графики, ввода и т.д.
#include "raymath.h" // Подключение библиотеки raymath для работы с векторами и мат.функциями

// Константы для игрока
#define G 900                          // Константа гравитации (ускорение вниз)
#define PLAYER_JUMP_SPD 350.0f          // Начальная скорость прыжка игрока вверх
#define PLAYER_MAX_SPEED 500.0f         // Максимальная горизонтальная скорость игрока
#define PLAYER_ACCELERATION 400.0f      // Ускорение при движении влево/вправо
#define PLAYER_DECELERATION 280.0f      // Замедление, когда игрок отпускает клавишу движения
#define PLAYER_MAX_JUMP_TIME 0.30f      // Максимальное время удержания прыжка (сек)
#define PLAYER_JUMP_HOLD_FORCE 350.0f   // Сила, с которой игрок может "удерживать" прыжок
#define PLAYER_DASH_SPEED 900.0f    // Скорость при дэш-ускорении



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
    Rectangle rect;        // Прямоугольник платформы (x, y, ширина, высота)
    int blocking;
    Color color;
} EnvItem;

// ======= Пыль (частицы) =======
#define MAX_PARTICLES 500   // Максимальное количество одновременных частиц

typedef struct Particle {
    Vector2 pos;            // Позиция частицы
    Vector2 vel;            // Скорость частицы
    float life;             // Оставшееся время жизни частицы (сек)
    float maxLife;          // Максимальное время жизни частицы (сек)
    float size;             // Размер частицы (радиус)
    bool active;            // Активна ли частица (true - отрисовывается, false - свободна)
} Particle;

Particle particles[MAX_PARTICLES] = {0};    // Массив всех частиц, изначально все неактивны

 // --- Функция спавна пыли при приземлении ---
void SpawnDustParticles(Vector2 pos, int count)    
{
    // Перебираем массив частиц и активируем свободные
    for (int i = 0; i < MAX_PARTICLES && count > 0; i++)
    {
        if (!particles[i].active)   // Если частица неактивна, можно использовать
        {
            float angle = DEG2RAD * (GetRandomValue(200, 340));                                 // Случайный угол разлёта (в стороны)
            float speed = GetRandomValue(50, 120) / 100.0f;                                     // Случайная скорость (0.5 - 1.2)
            particles[i].pos = pos;                                                             // Позиция появления (под игроком)
            particles[i].vel = (Vector2){ cosf(angle) * speed, -fabsf(sinf(angle) * speed) };   // Скорость (разлёт в стороны и чуть вверх)
            particles[i].life = 1.5f;                                                           // Время жизни частицы (сек)
            particles[i].maxLife = 1.5f;                                                        // Максимальное время жизни
            particles[i].size = GetRandomValue(6, 12); // УВЕЛИЧЕНО В 10 РАЗ (было /10.0f)      // Размер частицы (0.6 - 1.2 пикселя)
            particles[i].active = true;                                                         // Активируем частицу
            count--;                                                                            // Уменьшаем счётчик новых частиц
        }
    }
}

// --- Обновление всех частиц ---
void UpdateParticles(float dt)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (particles[i].active)    // Только для активных частиц
        {
            particles[i].pos.x += particles[i].vel.x * 80.0f * dt;      // Движение по X
            particles[i].pos.y += particles[i].vel.y * 80.0f * dt;      // Движение по Y
            particles[i].vel.y += 0.5f * dt;                            // Лёгкая гравитация на частицу
            particles[i].life -= dt;                                    // Уменьшаем время жизни
            if (particles[i].life <= 0.0f)
                particles[i].active = false;                            // Деактивируем, если "умерла"
        }
    }
}
// --- Отрисовка всех частиц ---
void DrawParticles(void)
{
    for (int i = 0; i < MAX_PARTICLES; i++)
    {
        if (particles[i].active)
        {
            float alpha = particles[i].life / particles[i].maxLife;
            Color c = Fade(WHITE, alpha);
            float currentSize = particles[i].size * alpha; // Добавляем уменьшение размера
            DrawCircleV(particles[i].pos, currentSize, c); // Используем текущий размер
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
    const int screenWidth = 1600;     // Ширина окна
    const int screenHeight = 800;    // Высота окна

    InitWindow(screenWidth, screenHeight, "PlatformerTest + Dust"); // Создаём окно

    Player player = { 0 };                               // Обнуляем все поля структуры игрока
    player.position = (Vector2){ 400, 280 };             // Начальная позиция игрока
    player.speed = 0;                                    // Вертикальная скорость = 0
    player.velocityX = 0;                                // Горизонтальная скорость = 0
    player.canJump = false;                              // По умолчанию прыгать нельзя
    player.jumpTime = 0.0f;                              // Время прыжка = 0
    player.isJumping = false;                            // Не прыгает
    //LEVELDESIGN
    EnvItem envItems[] = {                          // Массив платформ (препятствий)
        // X  Y  ширина  высота   коллизия       цвет
        {{ 0, 0,  1000,   400 },      0,       LIGHTGRAY },  // Фон (не препятствие)
        {{ 0, 400, 5000, 200 }, 1, GRAY },          // Земля (препятствие)
        {{ 0, -10, 50, 2000 }, 1, GRAY },
        {{ 300, 200, 400, 10 }, 1, GRAY },          // Платформа (препятствие)
        {{ 250, 300, 100, 10 }, 1, GRAY },          // Платформа (препятствие)
        {{ 650, 300, 100, 10 }, 1, GRAY },          // Платформа (препятствие)
        {{ 800, 300, 100, 20 }, 1, ORANGE } 
    };

    int envItemsLength = sizeof(envItems)/sizeof(envItems[0]);  // Количество платформ (автоматически)

    Camera2D camera = { 0 };                                    // Обнуляем все поля структуры камеры
    camera.target = player.position;                            // Центр камеры на игроке
    camera.offset = (Vector2){ screenWidth/2.0f, screenHeight/2.0f };   // Смещение камеры (центр экрана)
    camera.rotation = 0.0f;                                             // Без поворота
    camera.zoom = 1.0f;                                                 // Масштаб 1:1

    void (*cameraUpdaters[])(Camera2D*, Player*, EnvItem*, int, float, int, int) = {    // Массив функций для разных режимов камеры
        UpdateCameraCenter,                                                            
        UpdateCameraCenterInsideMap,                                                    
        UpdateCameraCenterSmoothFollow,
        UpdateCameraEvenOutOnLanding,
        UpdateCameraPlayerBoundsPush
    };

    int cameraOption = 0;                                                               // Текущий режим камеры (индекс)
    int cameraUpdatersLength = sizeof(cameraUpdaters)/sizeof(cameraUpdaters[0]);        // Количество режимов камеры

    char *cameraDescriptions[] = {                                                          // Описания режимов камеры
        "Follow player center",
        "Follow player center, but clamp to map edges",
        "Follow player center; smoothed",
        "Follow player center horizontally; update player center vertically after landing",
        "Player push camera on getting too close to screen edge"
    };

    SetTargetFPS(60);                                                                   // Устанавливаем частоту кадров 60 FPS

    // Для детекции приземления:
    //bool wasOnGround = false;                                                           // Был ли игрок на земле на предыдущем кадре

    while (!WindowShouldClose())                                                        // Главный игровой цикл, пока окно не закрыто
    {
        float deltaTime = GetFrameTime();                                               // Получаем время между кадрами (секунды)

        // --- Детекция приземления ---
        bool justLanded = false;                                                            // Флаг: только что приземлились
        Vector2 landPos = {0,0};                                                            // Координаты приземления
        UpdatePlayer(&player, envItems, envItemsLength, deltaTime, &justLanded, &landPos);  // Обновляем состояние игрока

        if (justLanded)
            SpawnDustParticles((Vector2){player.position.x, player.position.y+1}, 12);      // Спавним пыль при приземлении

        UpdateParticles(deltaTime);                                                         // Обновляем частицы

        camera.zoom += ((float)GetMouseWheelMove()*0.05f);                                  // Масштаб камеры меняется колесиком мыши

        if (camera.zoom > 3.0f) camera.zoom = 3.0f;                                         // Ограничение максимального масштаба
        else if (camera.zoom < 0.25f) camera.zoom = 0.25f;                                  // Ограничение минимального масштаба

        if (IsKeyPressed(KEY_R))                                                            // Если нажата клавиша R
        {
            camera.zoom = 1.0f;                                                             // Сбросить масштаб камеры
            player.position = (Vector2){ 400, 280 };                                        // Сбросить позицию игрока
        }

        if (IsKeyPressed(KEY_C)) cameraOption = (cameraOption + 1)%cameraUpdatersLength;     // Если нажата клавиша C переключить режим камеры

        cameraUpdaters[cameraOption](&camera, &player, envItems, envItemsLength, deltaTime, screenWidth, screenHeight); // Обновить камеру

        BeginDrawing();                                                                                                 // Начинаем рисовать кадр

            ClearBackground(LIGHTGRAY);          // Очищаем фон

            BeginMode2D(camera);                // Включаем режим 2D-камеры

                for (int i = 0; i < envItemsLength; i++) DrawRectangleRec(envItems[i].rect, envItems[i].color);     // Рисуем все платформы

                Rectangle playerRect = { player.position.x - 20, player.position.y - 40, 40.0f, 40.0f };            // Прямоугольник игрока
                DrawRectangleRec(playerRect, BLUE);                              // Рисуем игрока

                DrawCircleV(player.position, 5.0f, GOLD);                       // Рисуем точку в центре игрока

                DrawParticles();                                                // Рисуем частицы пыли

            EndMode2D();                                                         // Выключаем режим 2D-камеры

            DrawText("Controls:", 20, 20, 10, BLACK);                             // Подсказки управления
            DrawText("- Right/Left to move", 40, 40, 10, DARKGRAY);
            DrawText("- Space to jump", 40, 60, 10, DARKGRAY);
            DrawText("- Mouse Wheel to Zoom in-out, R to reset zoom", 40, 80, 10, DARKGRAY);
            DrawText("- C to change camera mode", 40, 100, 10, DARKGRAY);
            DrawText("Current camera mode:", 20, 120, 10, BLACK);
            DrawText(cameraDescriptions[cameraOption], 40, 140, 10, DARKGRAY);

            // FPS в правом верхнем углу
            {
                const int fpsFontSize = 20;                                     // Размер шрифта FPS
                const int padding = 10;                                         // Отступ от края
                const char *fpsText = TextFormat("FPS: %d", GetFPS());          // Текст с FPS
                int textWidth = MeasureText(fpsText, fpsFontSize);              // Ширина текста
                int xPos = GetScreenWidth() - textWidth - padding;              // X-координата
                int yPos = padding;                                             // Y-координата
                DrawText(fpsText, xPos, yPos, fpsFontSize, DARKBLUE);           // Рисуем FPS
            }

        EndDrawing();                                                           // Заканчиваем рисовать кадр
    }

    CloseWindow();                                                              // Закрываем окно

    return 0;                                                                   // Завершаем программу
}

// ======= Игрок с детекцией приземления =======
void UpdatePlayer(Player *player, EnvItem *envItems, int envItemsLength, float delta, bool *justLanded, Vector2 *landPos)
{
    static bool wasOnGround = false;                                    // Был ли игрок на земле на прошлом кадре

    // --- Горизонтальное движение с ускорением и замедлением ---
    float targetSpeed = 0.0f;                                           // Целевая скорость (куда хотим двигаться)
    if (IsKeyDown(KEY_LEFT)) targetSpeed -= PLAYER_MAX_SPEED;           // Если нажата влево, уменьшаем скорость
    if (IsKeyDown(KEY_RIGHT)) targetSpeed += PLAYER_MAX_SPEED;          // Если нажата вправо, увеличиваем скорость

    if (targetSpeed != 0)
    {
        if (player->velocityX < targetSpeed)
        {
            player->velocityX += PLAYER_ACCELERATION * delta;           // Ускоряемся вправо
            if (player->velocityX > targetSpeed) player->velocityX = targetSpeed;   // Не превышаем целевую скорость
        }
        else if (player->velocityX > targetSpeed)
        {
            player->velocityX -= PLAYER_ACCELERATION * delta;
            if (player->velocityX < targetSpeed) player->velocityX = targetSpeed;   // Не превышаем целевую скорость
        }
    }
    else
    {
        if (player->velocityX > 0)
        {
            player->velocityX -= PLAYER_DECELERATION * delta; // Замедляемся вправо
            if (player->velocityX < 0) player->velocityX = 0;
        }
        else if (player->velocityX < 0)
        {
            player->velocityX += PLAYER_DECELERATION * delta; // Замедляемся влево
            if (player->velocityX > 0) player->velocityX = 0;
        }
    }
    player->position.x += player->velocityX * delta;     // Обновляем позицию игрока по X

      // --- Прыжок и гравитация ---
    if (IsKeyPressed(KEY_SPACE) && player->canJump)     // Если нажата клавиша прыжка и можно прыгать
    {
        player->speed = -PLAYER_JUMP_SPD;               // Задаем вертикальную скорость вверх
        player->canJump = false;                        // Нельзя прыгать, пока не приземлимся
        player->isJumping = true;                       // Начинаем прыжок
        player->jumpTime = 0.0f;                        // Сбрасываем таймер прыжка
    }

    if (IsKeyDown(KEY_SPACE) && player->isJumping && player->jumpTime < PLAYER_MAX_JUMP_TIME)
    {
        player->speed = -PLAYER_JUMP_HOLD_FORCE;        // Удерживаем прыжок (меньше гравитация)
        player->jumpTime += delta;                      // Увеличиваем таймер прыжка
    }
    else
    {
        player->isJumping = false;                      // Если не удерживаем, прыжок завершён
    }

    bool hitObstacle = false;                           // Столкнулся ли игрок с препятствием снизу
    for (int i = 0; i < envItemsLength; i++)    
    {
        EnvItem *ei = envItems + i;                     // Текущий элемент окружения
        Vector2 *p = &(player->position);               // Позиция игрока
        // Проверяем, есть ли препятствие под игроком
        if (ei->blocking &&
            ei->rect.x <= p->x &&
            ei->rect.x + ei->rect.width >= p->x &&
            ei->rect.y >= p->y &&
            ei->rect.y <= p->y + player->speed*delta)
        {
            hitObstacle = true;                         // Нашли препятствие под игроком
            player->speed = 0.0f;                       // Обнуляем вертикальную скорость
            p->y = ei->rect.y;                          // Ставим игрока на платформу
            break;                                      // Выходим из цикла
        }
    }

    bool onGround = hitObstacle;                        // Стоит ли игрок на земле

    if (!hitObstacle)                                   
    {
        player->position.y += player->speed*delta;      // Обновляем позицию по Y
        player->speed += G*delta;                       // Применяем гравитацию
        player->canJump = false;                        // Прыгать нельзя, пока не на земле
    }
    else
    {
        player->canJump = true;                         // Можно прыгать (стоим на земле)
        player->isJumping = false;                      // Не прыгаем
        player->jumpTime = 0.0f;                        // Сбрасываем таймер прыжка
    }

    // Детект приземления
    *justLanded = false;                                // Сброс флага приземления
    if (!wasOnGround && onGround)                       // Если только что приземлились
    {
        *justLanded = true;                             // Ставим флаг приземления
        *landPos = player->position;                    // Запоминаем позицию приземления
    }
    wasOnGround = onGround;                             // Запоминаем состояние "на земле"
}

// ======= Функции камеры =======
void UpdateCameraCenter(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{
    camera->offset = (Vector2){ width/2.0f, height/2.0f };      // Центр экрана
    camera->target = player->position;                          // Центр камеры на игроке
}

void UpdateCameraCenterInsideMap(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{
    camera->target = player->position;                          // Центр камеры на игроке
    camera->offset = (Vector2){ width/2.0f, height/2.0f };      // Центр экрана
    float minX = 1000, minY = 1000, maxX = -1000, maxY = -1000; // Инициализация границ уровня

    for (int i = 0; i < envItemsLength; i++)
    {
        EnvItem *ei = envItems + i;                             
        minX = fminf(ei->rect.x, minX);                         // Минимальный X
        maxX = fmaxf(ei->rect.x + ei->rect.width, maxX);        // Максимальный X
        minY = fminf(ei->rect.y, minY);                         // Минимальный Y
        maxY = fmaxf(ei->rect.y + ei->rect.height, maxY);       // Максимальный Y
    }

    Vector2 max = GetWorldToScreen2D((Vector2){ maxX, maxY }, *camera);     // Координаты края карты на экране
    Vector2 min = GetWorldToScreen2D((Vector2){ minX, minY }, *camera);

    if (max.x < width) camera->offset.x = width - (max.x - width/2);        // Не даём выйти за правый край
    if (max.y < height) camera->offset.y = height - (max.y - height/2);     // Не даём выйти за нижний край
    if (min.x > 0) camera->offset.x = width/2 - min.x;                      // Не даём выйти за левый край
    if (min.y > 0) camera->offset.y = height/2 - min.y;                     // Не даём выйти за верхний край
}

void UpdateCameraCenterSmoothFollow(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{
    static float minSpeed = 30;                                             // Минимальная скорость следования
    static float minEffectLength = 10;                                      // Минимальная дистанция для движения камеры
    static float fractionSpeed = 0.8f;                                      // Доля расстояния, которую камера проходит за кадр


    camera->offset = (Vector2){ width/2.0f, height/2.0f };                  // Центр экрана
    Vector2 diff = Vector2Subtract(player->position, camera->target);       // Разница между позицией игрока и камеры
    float length = Vector2Length(diff);                                     // Расстояние между игроком и камерой

    if (length > minEffectLength)
    {
        float speed = fmaxf(fractionSpeed*length, minSpeed);                                 // Скорость движения камеры
        camera->target = Vector2Add(camera->target, Vector2Scale(diff, speed*delta/length)); // Плавное движение камеры к игроку
    }
}

void UpdateCameraEvenOutOnLanding(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{
    static float evenOutSpeed = 700;                            // Скорость выравнивания камеры
    static int eveningOut = false;                              // Флаг, идет ли выравнивание
    static float evenOutTarget;                                 // Целевая позиция по Y

    camera->offset = (Vector2){ width/2.0f, height/2.0f };      // Центр экрана
    camera->target.x = player->position.x;                      // Камера всегда по X за игроком

    if (eveningOut)
    {
        if (evenOutTarget > camera->target.y)
        {
            camera->target.y += evenOutSpeed*delta;             // Двигаем камеру вниз

            if (camera->target.y > evenOutTarget)              
            {
                camera->target.y = evenOutTarget;                // Останавливаем на целевой позиции 
                eveningOut = 0;
            }
        }
        else
        {
            camera->target.y -= evenOutSpeed*delta;             // Двигаем камеру вверх

            if (camera->target.y < evenOutTarget)
            {
                camera->target.y = evenOutTarget;                // Останавливаем на целевой позиции
                eveningOut = 0;
            }
        }
    }
    else
    {
        if (player->canJump && (player->speed == 0) && (player->position.y != camera->target.y))    // Если игрок на земле и камера не выровнена
        {
            eveningOut = 1;                         // Начинаем выравнивание
            evenOutTarget = player->position.y;     // Запоминаем целевую позицию
        }
    }
}

void UpdateCameraPlayerBoundsPush(Camera2D *camera, Player *player, EnvItem *envItems, int envItemsLength, float delta, int width, int height)
{
    static Vector2 bbox = { 0.2f, 0.2f };         // Размер области вокруг игрока (проценты от экрана)

    Vector2 bboxWorldMin = GetScreenToWorld2D((Vector2){ (1 - bbox.x)*0.5f*width, (1 - bbox.y)*0.5f*height }, *camera); // Левая верхняя точка области
    Vector2 bboxWorldMax = GetScreenToWorld2D((Vector2){ (1 + bbox.x)*0.5f*width, (1 + bbox.y)*0.5f*height }, *camera); // Правая нижняя точка области
    camera->offset = (Vector2){ (1 - bbox.x)*0.5f * width, (1 - bbox.y)*0.5f*height }; // Смещение камеры

    if (player->position.x < bboxWorldMin.x) camera->target.x = player->position.x; // Если игрок вышел за левую границу области
    if (player->position.y < bboxWorldMin.y) camera->target.y = player->position.y; // Если игрок вышел за верхнюю границу области
    if (player->position.x > bboxWorldMax.x) camera->target.x = bboxWorldMin.x + (player->position.x - bboxWorldMax.x); // За правую
    if (player->position.y > bboxWorldMax.y) camera->target.y = bboxWorldMin.y + (player->position.y - bboxWorldMax.y); // За нижнюю./////
}
