// gcc boids.c -Ofast -Wall -std=c99 -Wno-missing-braces -I ./include/ -L ./lib/ -lraylib -lm -ldl -lpthread -lGL -lX11
#include "raylib.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <math.h>
#define M_PI           3.14159265358979323846
#define BOOL short
#define TRUE 1
#define FALSE 0
struct Birds {
    /* type Vector2 is define in raylib.h as 
    typdef struct Vector2 {
        float x;
        float y;
    } */
    Vector2 position;
    Vector2 direction;
    BOOL leader;
};
const int screenWidth = 1700;
const int screenHeight = 800;

const int CenterX = 850;
const int CenterY = 400;
const Vector2 Center = {400, 850};

const int BirdNum = 1250;


// renders the birds even if they are outside the borser. Usefull if you resize the window or go full screen only if screenHeight && screenWidth are lowerthan the physical resolution of the display
const int FullScreen = 0;

// bird movement and size settings
const float delta = 0.5;
const float size = 10;

// movement logic settings
// Separation
const int SepRange = 30;
const float SepScale = 0.2;
// alignement
const int AlignRange = 50;
const float AlignScale = 0.3;
// cohesion
const int CoheRange = 500;
const float CoheScale = 0.000075;
const int CoheSepDelta = 10;

// border
const int BordRange = 100;
const float BordScale = 1;
// as it is more important to separate when close than align or cohere, SepScale >> AliScale >= CoheScale 

// Object (mouse) evading

const int ObjRange = 50;
const float ObjScale = 0.25;


// Leaders

const float LeadScale = 100;
const float LeadScale2 = 0.0000005;

// Other options

BOOL Leader = FALSE;
BOOL Mouse = FALSE;
BOOL debug = FALSE;



// inlining gives more fps

inline int rrandom(int a, int b) {
    // random int between a and b
    return (rand() % (b - a)) + a;
}

inline float dist(Vector2 v1, Vector2 v2) {
    float DeltaX = v1.x - v2.x;
    float DeltaY = v1.y - v2.y;
    return sqrtf(powf(DeltaX, 2) + powf(DeltaY, 2));
}

inline Vector2 VScale(Vector2 v, float scale) {
    v.x *= scale;
    v.y *= scale;
    return v;
}

inline Vector2 VAdd(Vector2 v1, Vector2 v2) {
    v1.x += v2.x;
    v1.y += v2.y;
    return v1;
}

// Normalize the vector and multiplies it by the var size
inline Vector2 normal(Vector2 v) {
    float norm = powf(v.x,2) + powf(v.y,2);
    norm = size/sqrtf(norm); // might be more efficient to implement the fast inverse sqrt algorithm
    v = VScale(v, norm);
    return v;

}

// Initialize the bird with a random position and a random direction
struct Birds InitBird() {
    struct Birds TempInit;
    TempInit.position.x = rrandom(100, screenWidth - 100);
    TempInit.position.y = rrandom(100,screenHeight - 100);
    float theta = 2*M_PI*((float)rand()/(float)RAND_MAX);
    TempInit.direction.x = size*cosf(theta);
    TempInit.direction.y = size*sinf(theta);
    if (Leader) {
        if (rrandom(0,100) == 0) {
            TempInit.leader = 1;
            if (debug) {printf("Leader created!!!\n");}
        } else {
            TempInit.leader = 0;
        }
    }
    return TempInit;
}

// Print the bird
void PrintBird(struct Birds bird) {
    if ((bird.position.x < 0 || bird.position.x > screenWidth || bird.position.y < 0 || bird.position.y > screenHeight) && ! FullScreen) {
        // if bird is off screen, doesn't renders it
    }
    else {
        /* Old (ugly triangle)
        Vector2 v1, v2, v3;
        v1.x = bird.position.x + bird.direction.x;
        v1.y = bird.position.y + bird.direction.y;
        v2.x = bird.position.x + bird.direction.x*2;
        v2.y = bird.position.y - bird.direction.y*2;
        v3.x = bird.position.x - bird.direction.x*2;
        v3.y = bird.position.y + bird.direction.y*2;
        DrawCircleV(v1, 5, RED);
        DrawTriangle(v1, v2, v3, WHITE); */
        if (!Leader) {
            DrawCircleV(bird.position, 2, RED);
            Vector2 v = {bird.position.x + bird.direction.x, bird.position.y + bird.direction.y};
            DrawLineEx(bird.position, v, 1, RED);
        } else if (bird.leader) {
            DrawCircleV(bird.position, 2, RED);
            Vector2 v = {bird.position.x + bird.direction.x, bird.position.y + bird.direction.y};
            DrawLineEx(bird.position, v, 1, RED);
        } else {
            DrawCircleV(bird.position, 2, BLUE);
            Vector2 v = {bird.position.x + bird.direction.x, bird.position.y + bird.direction.y};
            DrawLineEx(bird.position, v, 1, BLUE);
        }
    }
};


struct Birds UpdateBird(struct Birds bird, struct Birds array[BirdNum], Vector2 MousePos) {
    /* Updates the bird  */
    /* old test
    test1 
    float temp = bird.direction.y;
    bird.direction.y = bird.direction.x;
    bird.direction.x = temp;
    int t = rand() % 20 - 10;
    int z = rand() % 20 - 10;
    test2 (Random steering)
    bird.direction = normal(ChangeTest(bird.direction, t, z)); */
    // Compute the distance between our bird and every other bird
    float Distances[BirdNum];
    for (int i = 0; i < BirdNum; i++) {
        Distances[i] = dist(bird.position, array[i].position);
    }
    // Separation Takes the average position of all the birds at a distance closer than SepRange and moves to the contrary
    int SepNum = 0;
    float SepX = 0;
    float SepY = 0;
    for (int i = 0; i < BirdNum; i++) {
        if (Distances[i] <= SepRange && Distances[i] > 0.1) {
            SepX += array[i].position.x;
            SepY += array[i].position.y;
            SepNum++;
            if (debug) {printf("%d", SepNum);}
        }
    }
    if (SepNum) {
        SepX /= SepNum;
        SepY /= SepNum;
        Vector2 SepV = {SepX, SepY};
        if (debug) {printf("%f \n", dist(bird.position, SepV));}
        bird.direction = VAdd(bird.direction, VScale(VAdd(bird.position, VScale(SepV, -1)), SepScale));
    }
    // Alignement takes the average direction of all the birds at a distance closer than AlignRange and steer the direction to it
    int AlignNum = 0;
    float AlignX = 0;
    float AlignY = 0;
    if (!bird.leader) {
    for (int i = 0; i < BirdNum; i++) {
        if (Distances[i] > SepRange && Distances[i] <= AlignRange) {
            if (bird.leader) {
                AlignX += array[i].direction.x*LeadScale;
                AlignY += array[i].direction.y*LeadScale;
                AlignNum += LeadScale;
            } else {
                AlignX += array[i].direction.x;
                AlignY += array[i].direction.y;
                AlignNum++;
            }
        }
    }
    }
    if (AlignNum) {
        AlignX /= AlignNum;
        AlignY /= AlignNum;
        Vector2 AlignV= {AlignX, AlignY};
        bird.direction = VAdd(bird.direction, VScale(AlignV, AlignScale));
    }
    // Cohesion takes the average position of every bird at a distance between SepRange + CoheSepDelta abd CoheRange and goes to this position
    int CoheNum = 0;
    float CoheX = 0;
    float CoheY = 0;
    if (!bird.leader) {
    for (int i = 0; i < BirdNum; i++) {
        if (Distances[i] > SepRange + CoheSepDelta && Distances[i] <= CoheRange) {
            if (array[i].leader) {
            CoheX += array[i].position.x*LeadScale;
            CoheY += array[i].position.y*LeadScale;
            CoheNum += LeadScale;
            } else {
            CoheX += array[i].position.x;
            CoheY += array[i].position.y;
            CoheNum++;}
        }
    }
    if (CoheNum) {
        CoheX /= CoheNum;
        CoheY /= CoheNum;
        Vector2 CoheV = {CoheX, CoheY};
        bird.direction = VAdd(bird.direction, VScale(CoheV, CoheScale));
    }}
    // if near the border of the screen force to the center of the screen. This force is stronger the more you are close to the border
    if (bird.position.x < BordRange || bird.position.x > screenWidth - BordRange || bird.position.y < BordRange || bird.position.y > screenHeight - BordRange) { 
    float d = BordScale/dist(bird.position, Center);
    bird.direction = VAdd(bird.direction, VScale(VAdd(Center, VScale(bird.position, -1)), d));
    }
 
    // Object (mouse) evading
    if (Mouse) {
    float MouseDist = dist(MousePos, bird.position);
    if (MouseDist <= ObjRange) {
        bird.direction = VAdd(bird.direction, VScale( VAdd(VScale(MousePos, -1), bird.position), ObjScale));
    }
    if (debug) {DrawCircleV(MousePos, 10, RED);
    }
    }


    // Attraction to nearest leader
    if (Leader) {
    if (!bird.leader) {
    struct Birds Lead = {{0, 0}, {0, 0}, 0};
    float LeadDist = 100000;
    int first = 1;
    for (int i = 0; i < BirdNum; i++) {
        if (array[i].leader && dist(array[i].position, bird.position) < LeadDist) {
            Lead = array[i];
            LeadDist = dist(array[i].position, bird.position);
            first = 0;
        }
    // the condition is there for if there is no leader and Lead has not been attribued a value
    if (!first) {
        bird.direction = VAdd(bird.direction, VScale(VAdd(Lead.position, VScale(bird.position, -1)), LeadScale2));
    }
    }
    }
    }

    // normalize the direction
    bird.direction = normal(bird.direction);
    return bird;
    
}

struct Birds MoveBird(struct Birds bird) {
    bird.position = VAdd(bird.position, VScale(bird.direction, delta));
    return bird;
}


int main() {
    srand(time(NULL)); // seed
    // initialize all birds
    struct Birds BirdArray[BirdNum];
    for (int i = 0; i < BirdNum; i++) {
        BirdArray[i] = InitBird();
        if (debug) {printf("%dth bird initialized\n",i + 1);}
    }

    InitWindow(screenWidth, screenHeight, "raylib basic window test");
    SetTargetFPS(30);

    time_t start;
    time_t end;
    float fps;

    while(!WindowShouldClose()){
        start = time(NULL);
        BeginDrawing();
        ClearBackground(BLANK);
        // Renders the birds
        for (int i = 0; i < BirdNum; i++) {
            if (debug) {printf("Rendering the %dth bird. Pos = (%f, %f) Dir = (%f, %f) ||Dir|| = %f\n", i+1, BirdArray[i].position.x, BirdArray[i].position.y, BirdArray[i].direction.x, BirdArray[i].direction.y, sqrt(pow(BirdArray[i].direction.x,2) + pow(BirdArray[i].direction.y,2)));}
            PrintBird(BirdArray[i]);
        }
        EndDrawing();
        // Updates their direction
        struct Birds NextArray[BirdNum];
        Vector2 Position = GetMousePosition();
        for (int i = 0; i < BirdNum; i++) {
            NextArray[i] = UpdateBird(BirdArray[i], BirdArray, Position);
        }
        // finished updating copying into the array
        for (int i = 0; i < BirdNum; i++) {
            BirdArray[i] = NextArray[i];
        }
        // Moves the bird
        for (int i = 0; i < BirdNum; i++) {
            BirdArray[i] = MoveBird(BirdArray[i]);
        }
            end = time(NULL);
            fps = 1/(difftime(end,start));
        if (debug) {
            printf("%ffps, frame took %fs\n", fps, difftime(end, start));
        }
    }
    CloseWindow();
    return 0;
}
