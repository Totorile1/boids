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
    Vector3 position;
    Vector3 direction;
    BOOL leader;
};
const int screenWidth = 1500;
const int screenHeight = 900;

const int CenterX = 750;
const int CenterY = 500;
const int CenterZ = 0;
const Vector3 Center = {750, 500, 0};

const int BirdNum = 1000;


// renders the birds even if they are outside the borser. Usefull if you resize the window or go full screen.
int FullScreen = 1;

// bird movement and size settings
const float delta = 0.5;
const float size = 10;

// movement logic settings
// Separation
int SepRange = 30;
float SepScale = 0.2;
// alignement
int AlignRange = 50;
float AlignScale = 0.1;
// cohesion
int CoheRange = 500;
float CoheScale = 0.000075;
int CoheSepDelta = 10;

// border
int BordRange = 100;
float BordScale = 1;
// as it is more important to separate when close than align or cohere, SepScale >> AliScale >= CoheScale 

// Object (mouse) evading

int ObjRange = 50;
float ObjScale = 0.25;


// Leaders

float LeadScale = 100;
float LeadScale2 = 0.0000005;

// Other options

BOOL Leader = FALSE;
BOOL Mouse = FALSE;
BOOL debug = FALSE;



// inlining gives more fps

inline int rrandom(int a, int b) {
    // random int between a and b
    return (rand() % (b - a)) + a;
}

inline float dist(Vector3 v1, Vector3 v2) {
    float DeltaX = v1.x - v2.x;
    float DeltaY = v1.y - v2.y;
    float DeltaZ = v1.z - v2.z;
    return sqrtf(powf(DeltaX, 2) + powf(DeltaY, 2) + powf(DeltaZ,2));
}

inline Vector3 VScale(Vector3 v, float scale) {
    v.x *= scale;
    v.y *= scale;
    v.z *= scale;
    return v;
}

inline Vector3 VAdd(Vector3 v1, Vector3 v2) {
    v1.x += v2.x;
    v1.y += v2.y;
    v1.z += v2.z;
    return v1;
}

// Normalize the vector and multiplies it by the var size
inline Vector3 normal(Vector3 v) {
    float norm = powf(v.x,2) + powf(v.y,2) + powf(v.z, 2);
    norm = size/sqrtf(norm); // might be more efficient to implement the fast inverse sqrt algorithm
    v = VScale(v, norm);
    return v;

}
// Initialize the bird with a random position and a random direction
struct Birds InitBird() {
    struct Birds TempInit;
    TempInit.position.x = rrandom(100, screenWidth - 100);
    TempInit.position.y = rrandom(100,screenHeight - 100);
    TempInit.position.z = rrandom(10, 30);
    float theta = 2*M_PI*((float)rand()/(float)RAND_MAX);
    TempInit.direction.x = size*cosf(theta);
    TempInit.direction.y = size*sinf(theta);
    TempInit.direction.z = size*sinf(theta);
    TempInit.direction = normal(TempInit.direction);
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
            DrawSphere(bird.position, 9, RED);
            Vector3 v = VAdd(bird.position, bird.direction);
            DrawLine3D(bird.position, v, RED);
        } else if (bird.leader) {
            DrawSphere(bird.position, 9, RED);
            Vector3 v = VAdd(bird.position, bird.direction);
            DrawLine3D(bird.position, v, RED);
        } else {
            DrawSphere(bird.position, 9, BLUE);
            Vector3 v = VAdd(bird.position, bird.direction);
            DrawLine3D(bird.position, v, BLUE);
        }
    }
};

struct Birds UpdateBird(struct Birds bird, struct Birds array[BirdNum], Vector3 MousePos) {
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
    float SepZ = 0;
    for (int i = 0; i < BirdNum; i++) {
        if (Distances[i] <= SepRange && Distances[i] > 0.1) {
            SepX += array[i].position.x;
            SepY += array[i].position.y;
            SepZ += array[i].position.z;
            SepNum++;
            if (debug) {printf("%d", SepNum);}
        }
    }
    if (SepNum) {
        SepX /= SepNum;
        SepY /= SepNum;
        SepZ /= SepNum;
        Vector3 SepV = {SepX, SepY, SepZ};
        if (debug) {printf("%f \n", dist(bird.position, SepV));}
        bird.direction = VAdd(bird.direction, VScale(VAdd(bird.position, VScale(SepV, -1)), SepScale));
    }
    // Alignement takes the average direction of all the birds at a distance closer than AlignRange and steer the direction to it
    int AlignNum = 0;
    float AlignX = 0;
    float AlignY = 0;
    float AlignZ = 0;
    if (!bird.leader) {
    for (int i = 0; i < BirdNum; i++) {
        if (Distances[i] > SepRange && Distances[i] <= AlignRange) {
            if (bird.leader) {
                AlignX += array[i].direction.x*LeadScale;
                AlignY += array[i].direction.y*LeadScale;
                AlignZ += array[i].direction.z*LeadScale;
                AlignNum += LeadScale;
            } else {
                AlignX += array[i].direction.x;
                AlignY += array[i].direction.y;
                AlignZ += array[i].direction.z;
                AlignNum++;
            }
        }
    }
    }
    if (AlignNum) {
        AlignX /= AlignNum;
        AlignY /= AlignNum;
        AlignZ /= AlignNum;
        Vector3 AlignV= {AlignX, AlignY, AlignZ};
        bird.direction = VAdd(bird.direction, VScale(AlignV, AlignScale));
    }
    // Cohesion takes the average position of every bird at a distance between SepRange + CoheSepDelta abd CoheRange and goes to this position
    int CoheNum = 0;
    float CoheX = 0;
    float CoheY = 0;
    float CoheZ = 0;
    if (!bird.leader) {
    for (int i = 0; i < BirdNum; i++) {
        if (Distances[i] > SepRange + CoheSepDelta && Distances[i] <= CoheRange) {
            if (array[i].leader) {
            CoheX += array[i].position.x*LeadScale;
            CoheY += array[i].position.y*LeadScale;
            CoheZ += array[i].position.z*LeadScale;
            CoheNum += LeadScale;
            } else {
            CoheX += array[i].position.x;
            CoheY += array[i].position.y;
            CoheZ += array[i].position.z;
            CoheNum++;}
        }
    }
    if (CoheNum) {
        CoheX /= CoheNum;
        CoheY /= CoheNum;
        CoheZ /= CoheNum;
        Vector3 CoheV = {CoheX, CoheY, CoheZ};
        bird.direction = VAdd(bird.direction, VScale(CoheV, CoheScale));
    }}
    // if near the border of the screen force to the center of the screen. This force is stronger the more you are close to the border
    if (bird.position.x < BordRange || bird.position.x > screenWidth - BordRange || bird.position.y < BordRange || bird.position.y > screenHeight - BordRange) { 
    float d = BordScale/dist(bird.position, Center);
    bird.direction = VAdd(bird.direction, VScale(VAdd(Center, VScale(bird.position, -1)), d));
    }
    // if -30 < z < 30 force to z = 0
    if (bird.position.z > 30 || bird.position.z < -30) {
    Vector3 ZPos = bird.position;
    ZPos.z = 0;
    float dz = BordScale/dist(bird.position, ZPos);
    bird.direction = VAdd(bird.direction, VScale(VAdd(ZPos, VScale(bird.position, -1)), dz));
    }
    // Object (mouse) evading
    if (Mouse) {
    float MouseDist = dist(MousePos, bird.position);
    if (MouseDist <= ObjRange) {
        bird.direction = VAdd(bird.direction, VScale( VAdd(VScale(MousePos, -1), bird.position), ObjScale));
    }
    if (debug) {DrawSphere(MousePos, 10, RED);
    }
    }


    // Attraction to nearest leader
    if (Leader) {
    if (!bird.leader) {
    struct Birds Lead = {{0, 0, 0}, {0, 0, 0}, 0};
    float LeadDist = 0;
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
    Camera3D Cam = {0};
    Cam.position = (Vector3){0, 300, 0};
    Cam.target = (Vector3){0, 0, 0};
    Cam.up = (Vector3){ 0.0f, 0.0f, 0.0f };          // Camera up vector (rotation towards target)
    Cam.projection = CAMERA_PERSPECTIVE;             // Camera projection type
    BeginMode3D(Cam);
    while(!WindowShouldClose()){
        start = time(NULL);
        BeginDrawing();
        ClearBackground(WHITE);
        DrawGrid(100, 100);
        // Renders the birds
        for (int i = 0; i < BirdNum; i++) {
            if (debug) {printf("Rendering the %dth bird. Pos = (%f, %f, %f) Dir = (%f, %f, %f) ||Dir|| = %f\n", i+1, BirdArray[i].position.x, BirdArray[i].position.y, BirdArray[i].position.z, BirdArray[i].direction.x, BirdArray[i].direction.y, BirdArray[i].direction.z, sqrt(powf(BirdArray[i].direction.x,2) + powf(BirdArray[i].direction.y,2) + powf(BirdArray[i].direction.z, 2)) );}
            PrintBird(BirdArray[i]);
        }
        EndDrawing();
        // Updates their direction
        struct Birds NextArray[BirdNum];
        Vector3 Position;
        Position.x = GetMouseX();
        Position.y = GetMouseY();
        Position.z = 0;
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
    EndMode3D();
    CloseWindow();
    return 0;
}
