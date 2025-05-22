#include <raylib.h>

int main()
{

    int ballX = 400;
    int ballY = 400;
    Color green = {20, 160, 133, 255};

    InitWindow(800, 600, "My first raylib game");
    SetTargetFPS(60);
    //Game Loop
    while(WindowShouldClose() == false)
    {
        //1. Event andling
        if(IsKeyDown(KEY_RIGHT))
        {
            ballX += 3;
        }else if (IsKeyDown(KEY_LEFT))
        {
            ballX -=3;
        }else if (IsKeyDown(KEY_UP))
        {
            ballY -= 3;
        }else if(IsKeyDown(KEY_DOWN))
        {
            ballY += 3;
        }
        {
            /* code */
        }
        

        //2. Updating positions
        //ballX += 3;
        //3. Drawing
        BeginDrawing();
        ClearBackground(green);
        //DrawEllipse(ballX, ballY, 20,40,GREEN); 
        DrawCircle(ballX,ballY,20,WHITE);
        EndDrawing();
    }
    


    CloseWindow();
    return 0;
}