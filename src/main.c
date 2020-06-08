#include <stdio.h>
#include <raylib.h>

#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>

#define WIDTH 800
#define HEIGHT 600
#define UPDATE_PORT 2066

// Some colors for the cubes, be carefull to not go beyond the limit of this array.
Color colors[] = {
    LIGHTGRAY,
    GRAY,
    DARKGRAY,
    YELLOW,
    GOLD,
    ORANGE,
    PINK,
    RED,
    MAROON,
    GREEN,
    LIME,
    DARKGREEN,
    SKYBLUE,
    BLUE,
    DARKBLUE,
    PURPLE,
    VIOLET,
    DARKPURPLE,
    BEIGE,
    BROWN,
    DARKBROWN
};

enum Commands {
    CMD_STATUS,
    CMD_DISCONNECT,
    CMD_COUNT
};

typedef struct packet_t
{
    char text[64];
    float x;
    float z;
    int command;
} packet_t;

#define TSIZE 50

int sock;
int client_len, yes = 1;
struct sockaddr_in client;

static packet_t table[TSIZE] = {0};

static int entries = 0;


void *update_slave_cube(void *ptr)
{
    bool found;
    packet_t pkt = {0};
    int packet_count = 0, i;

    while (!WindowShouldClose())
    {
        found = false;
        client_len = sizeof client;
        recvfrom(sock, (char *)&pkt, sizeof pkt, 0,
                 (const struct sockaddr *)&client, &client_len);

        for(i = 0; i < entries; i++) {
            if(strcmp(table[i].text, pkt.text) == 0) {
                found = true;
                break;
            }
        }

        if(found) 
        {
            // You can send any command you want. Extend this switch to handle it.
            switch(pkt.command) {
                case CMD_DISCONNECT:
                    for(int j = i; i < entries; i++){
                        table[j] = table[j + 1];
                    }
                    entries--;
                    break;
                case CMD_STATUS:
                    table[i].x = pkt.x;
                    table[i].z = pkt.z;
                    break;
            }
        } else {
            assert(entries < TSIZE && "Table is full");
            strcpy(table[entries].text, pkt.text);
            entries++;
        }
    }
    return NULL;
}

static char *rand_string(char *str, size_t size)
{
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJK...";
    if (size) {
        --size;
        for (size_t n = 0; n < size; n++) {
            int key = rand() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }
        str[size] = '\0';
    }
    return str;
}

int main(int argc, char *argv[])
{
#ifdef OS_Windows_NT
    printf("Windows dettected\n");
#elif defined OS_Linux
    printf("LINUS dettected\n");
#elif defined OS_Darwin
    printf("MacOS dettected\n");
#endif

    // Network vars
    struct sockaddr_in server;
    packet_t pkt = {0};

    // Datagram socket
    sock = socket(AF_INET, SOCK_DGRAM, 0);

    Vector3 cube_position = {0};

    // We configure the socekts options to broadcast and to reuse the address.
    // If we don't reuse the address, only one client can connect.
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof yes);
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, (char *)&yes, sizeof yes);

    /* Bind our well-known port number */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY); // inet_addr("127.0.0.1"); // this is hardcoded to localhost
    server.sin_port = htons(UPDATE_PORT);
    if (bind(sock, (struct sockaddr *)&server, sizeof server) < 0)
    {
        perror("bind error");
        exit(1);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = 0xffffffff;
    server.sin_port = htons(UPDATE_PORT);

    InitWindow(WIDTH, HEIGHT, "This is a network test");
    SetTargetFPS(30);

    Camera3D camera = {0};

    camera.fovy = 45.0f;
    camera.target = (Vector3){.0f, .0f, .0f};
    camera.position = (Vector3){0.0f, 10.0f, 10.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.type = CAMERA_PERSPECTIVE;

    pthread_t client_thread;

    pthread_create(&client_thread, NULL, update_slave_cube, NULL);

    // We need an unique identifier. You can create a more meaningfull use for this.
    rand_string(pkt.text, 10);
    pkt.command = CMD_STATUS;

    while (!WindowShouldClose())
    {
        BeginDrawing();
        {
            ClearBackground(WHITE);
            DrawFPS(10, 10);

            // Draww all the cubes in the cube array
            BeginMode3D(camera);
            {
                for(int i = 0; i < entries; i++){
                    DrawCube((Vector3){table[i].x, 0, table[i].z}, 1, 1, 1, colors[i]);
                    DrawCubeWires((Vector3){table[i].x, 0, table[i].z}, 1, 1, 1, BLUE);
                }
                DrawGrid(10, 1);
            }
            EndMode3D();
            // You can manipualte the field of view with + and -
            if (IsKeyDown(KEY_KP_ADD))
                camera.fovy += 1.0f;
            if (IsKeyDown(KEY_KP_SUBTRACT))
                camera.fovy -= 1.0f;

            // Control your cube
            if (IsKeyPressed(KEY_LEFT))
                cube_position.x -= 1.0f;
            if (IsKeyPressed(KEY_RIGHT))
                cube_position.x += 1.0f;
            if (IsKeyPressed(KEY_UP))
                cube_position.z -= 1.0f;
            if (IsKeyPressed(KEY_DOWN))
                cube_position.z += 1.0f;
        }
        EndDrawing();

        pkt.x = cube_position.x;
        pkt.z = cube_position.z;

        // Broadcast update packet to servers
        sendto(sock, (char *)&pkt, sizeof pkt, 0,
               (struct sockadddr *)&server, sizeof server);
    }

    CloseWindow();
    pkt.command = CMD_DISCONNECT;

    // broadcast disconnection
    sendto(sock, (char *)&pkt, sizeof pkt, 0,
               (struct sockadddr *)&server, sizeof server);

    return 0;
}