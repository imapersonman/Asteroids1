//
//  main.cpp
//  Asteroids1
//
//  Created by Koissi Adjorlolo on 2/24/16.
//  Copyright © 2016 centuryapps. All rights reserved.
//

#include <stdlib.h>
#include <iostream>
#include <vector>
#include <SDL2/SDL.h>
#include <SDL2_ttf/SDL_ttf.h>

typedef struct
{
    float x, y;
} Vector2f;

typedef struct
{
    Vector2f p1, p2;
} Line;

typedef struct
{
    // Ax + By = C
    float a, b, c;
} LineEquation;

static const int N_LINES = 5;

typedef struct
{
    int nLines;
    Line lines[N_LINES];
} Polygon;

typedef int AsteroidSize;

typedef struct
{
    AsteroidSize size;
    Vector2f position;
    Vector2f velocity;
    float angle;
    float angularVelocity;
    Polygon shape;
} Asteroid;

static const AsteroidSize ASTEROIDSIZE_SMALL = 10;
static const AsteroidSize ASTEROIDSIZE_MEDIUM = 30;
static const AsteroidSize ASTEROIDSIZE_LARGE = 50;
static const int ASTEROIDVEL_SMALL = 3;
static const int ASTEROIDVEL_MEDIUM = 2;
static const int ASTEROIDVEL_LARGE = 1;
static const int N_INIT_ASTEROIDS = 10;

static const int N_SHIP_LINES = 3;

typedef struct
{
    Vector2f position;
    Vector2f velocity;
    float speed;
    float thrust;
    float angle;
    Line lines[N_SHIP_LINES];
    bool turnLeft;
    bool turnRight;
    bool thrusting;
    bool shooting;
    int cooldown;
} Ship;

typedef struct
{
    Vector2f position;
    Vector2f velocity;
    int lifeCounter;
} Projectile;

static const int PROJECTILE_SIZE = 2;
static const int PROJECTILE_LIFETIME = 1.5 * 1000; // Milliseconds
static const int PROJECTILE_COOLDOWN = 0.05 * 1000; // Milliseconds
static const float PROJECTILE_SPEED = 8.0f;

static const float SHIP_MAXSPEED = 4.0f;
static const float SHIP_THRUST = 0.05f;
static const float SHIP_ANGULARSPEED = 0.05f;

static const int WRAPBUFFER_X = 10;
static const int WRAPBUFFER_Y = 10;

static const char *TITLE = "Asteroids";
static const int WINDOW_POSX = SDL_WINDOWPOS_UNDEFINED;
static const int WINDOW_POSY = SDL_WINDOWPOS_UNDEFINED;
static const int WINDOW_WIDTH = 1024;
static const int WINDOW_HEIGHT = 1024;
static const Uint32 WINDOW_FLAGS = 0;
static const Uint32 RENDERER_FLAGS = SDL_RENDERER_ACCELERATED |
                                     SDL_RENDERER_PRESENTVSYNC;

typedef enum
{
    GameState_Game,
    GameState_Lost,
    GameState_Won
} GameState;

static const double MS_PER_UPDATE = 1000 / 60;

static void init();
static void quit();
static Asteroid createAsteroid(AsteroidSize size);
static Ship createShip();
static Projectile createProjectile(Vector2f position, float angle, float speed);
static void update();
static void updateAsteroids(std::vector<Asteroid> &asteroids);
static void updateAsteroid(Asteroid &asteroid);
static void updateShip(Ship &ship);
static void updateProjectiles(std::vector<Projectile> &projectiles, int lifeTime);
static void updateProjectile(Projectile &projectile);
static void render();
static void renderAsteroids(std::vector<Asteroid> asteroids);
static void renderAsteroid(Asteroid asteroid);
static void renderShip(Ship ship);
static void renderProjectiles(std::vector<Projectile> projectiles);
static void renderProjectile(Projectile &projectile);
static void renderText(const char *text, Vector2f position);
static int randomDirection();
static int random(int min, int max);
static float randomNormal();
static bool linesIntersect(Vector2f origin1, Vector2f origin2, Line l1, Line l2);
static bool counterClockwise(Vector2f a, Vector2f b, Vector2f c);
static float distance(Vector2f p1, Vector2f p2);
static void wrapPosition(Vector2f &position, int bufferX, int bufferY);
static void checkCollisions(Ship ship, std::vector<Asteroid> asteroids);
static void checkCollision(Ship ship, Asteroid asteroids);
static void fireProjectileFromPoint(Vector2f point, float angle);
static void destroyProjectile(int projectileIndex, std::vector<Projectile> &projectiles);
static void destroyAsteroid(int asteroidIndex);
static void splitAsteroid(int asteroidIndex);
static void checkProjectileCollisions(std::vector<Projectile> projectiles,
                                      std::vector<Asteroid> asteroids);
static void explode(Vector2f position);
static TTF_Font *loadFont(const char *path);
static void checkWin();

static bool gRunning = false;
static SDL_Window *gWindow = nullptr;
static SDL_Renderer *gRenderer = nullptr;

static std::vector<Asteroid> gAsteroids;
static Ship gShip;
static std::vector<Projectile> gProjectiles;
static std::vector<Projectile> gParticles;

static TTF_Font *gDefaultFont;
static GameState gState;

int main(int argc, const char * argv[])
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        std::cout << "Unable to init SDL" << std::endl;
        exit(1);
    }
    
    gWindow = SDL_CreateWindow(TITLE,
                               WINDOW_POSX,
                               WINDOW_POSY,
                               WINDOW_WIDTH,
                               WINDOW_HEIGHT,
                               WINDOW_FLAGS);
    
    if (gWindow == nullptr)
    {
        std::cout << "Unable to create window" << std::endl;
        SDL_Quit();
        exit(1);
    }
    
    gRenderer = SDL_CreateRenderer(gWindow, -1, RENDERER_FLAGS);
    
    if (gRenderer == nullptr)
    {
        std::cout << "Unable to create renderer" << std::endl;
        SDL_Quit();
        exit(1);
    }
    
    if (TTF_Init() == -1)
    {
        std::cout << "Unable to init SDL_ttf" << std::endl;
        quit();
        exit(1);
    }
    
    init();
    
    gDefaultFont = loadFont("Resources/Fonts/alterebro-pixel-font.ttf");
    gRunning = true;
    SDL_Event event;
    
    double previous = (double)SDL_GetTicks();
    double lag = 0.0;
    
    while (gRunning)
    {
        while (SDL_PollEvent(&event))
        {
            switch (event.type)
            {
                case SDL_QUIT:
                    gRunning = false;
                    break;
                    
                case SDL_KEYDOWN:
                    switch (event.key.keysym.sym)
                    {
                        case SDLK_LEFT:
                            gShip.turnLeft = true;
                            break;
                            
                        case SDLK_RIGHT:
                            gShip.turnRight = true;
                            break;
                            
                        case SDLK_UP:
                            gShip.thrusting = true;
                            break;
                            
                        case SDLK_SPACE:
                            gShip.shooting = true;
                            break;
                            
                        case SDLK_RETURN:
                            if (gState == GameState_Lost ||
                                gState == GameState_Won)
                            {
                                gAsteroids.clear();
                                gParticles.clear();
                                gProjectiles.clear();
                                init();
                            }
                            break;
                            
                        default:
                            break;
                    }
                    break;
                    
                case SDL_KEYUP:
                    switch (event.key.keysym.sym)
                    {
                        case SDLK_LEFT:
                            gShip.turnLeft = false;
                            break;
                            
                        case SDLK_RIGHT:
                                gShip.turnRight = false;
                            break;
                            
                        case SDLK_UP:
                            gShip.thrusting = false;
                            break;
                                
                        case SDLK_SPACE:
                            gShip.shooting = false;
                            
                        default:
                            break;
                    }
                    break;
                    
                default:
                    break;
            }
        }
        
        double current = (double)SDL_GetTicks();
        double elapsed = current - previous;
        lag += elapsed;
        previous = current;
        
        while (lag >= MS_PER_UPDATE)
        {
            update();
            lag -= MS_PER_UPDATE;
        }
        
        render();
    }
    
    quit();
    
    return 0;
}

static void init()
{
    srand((int)time(nullptr));
    gState = GameState_Game;
    
    for (int asteroidIndex = 0;
         asteroidIndex < N_INIT_ASTEROIDS;
         asteroidIndex++)
    {
        gAsteroids.push_back(createAsteroid(ASTEROIDSIZE_LARGE));
    }
    
    gShip = createShip();
}

static void quit()
{
    SDL_DestroyRenderer(gRenderer);
    SDL_DestroyWindow(gWindow);
    
    gRenderer = nullptr;
    gWindow = nullptr;
    
    SDL_Quit();
}

static Asteroid createAsteroid(AsteroidSize size)
{
    Asteroid asteroid;
    asteroid.size = size;
    asteroid.angle = 0.0f;
    asteroid.angularVelocity = 0.02 * randomDirection();
    int maxVel = 0;
    
    switch (size)
    {
        case ASTEROIDSIZE_SMALL:
            maxVel = ASTEROIDVEL_SMALL;
            break;
            
        case ASTEROIDSIZE_MEDIUM:
            maxVel = ASTEROIDVEL_MEDIUM;
            break;
            
        case ASTEROIDSIZE_LARGE:
            maxVel = ASTEROIDVEL_LARGE;
            break;
            
        default:
            break;
    }
    
    asteroid.velocity.x = maxVel * randomNormal();
    asteroid.velocity.y = sqrtf(powf(maxVel, 2.0f) -
                                powf(asteroid.velocity.x, 2.0f));
    
    asteroid.velocity.x *= randomDirection();
    asteroid.velocity.y *= randomDirection();
    
    Polygon pentagon;
    // The distance each vertex is from center is equal to its size id.
    int r = size;
    float theta = (2 * M_PI) / N_LINES;
    
    for (int i = 0; i < N_LINES; i++)
    {
        pentagon.lines[i] = {
            {
                (r * cosf(theta * (i - 1) + asteroid.angle)),
                (r * sinf(theta * (i - 1) + asteroid.angle))
            },
            {
                (r * cosf(theta * i)),
                (r * sinf(theta * i))
            }
        };
    }
    
    asteroid.shape = pentagon;
    
    asteroid.position = {
        (float)random(0, WINDOW_WIDTH),
        (float)random(0, WINDOW_HEIGHT)
    };
    
    return asteroid;
}

static Ship createShip()
{
    Ship ship;
    
    ship.cooldown = 0;
    ship.turnLeft = false;
    ship.turnRight = false;
    ship.thrusting = false;
    ship.speed = 0.0f;
    ship.thrust = 0.5f;
    ship.angle = 0.0f;
    ship.position = {
        WINDOW_WIDTH / 2,
        WINDOW_HEIGHT / 2
    };
    
    ship.velocity = { 0.0f, 0.0f };
    
    ship.lines[0] = {
        { 10.0f, 00.0f },
        { -8.0f, -5.0f }
    };
    ship.lines[1] = {
        { -8.0f, -5.0f },
        { -8.0f, 5.0f }
    };
    ship.lines[2] = {
        { -8.0f, 5.0f },
        { 10.0f, 0.0f }
    };
    
    return ship;
}

static Projectile createProjectile(Vector2f position, float angle, float speed)
{
    Projectile projectile;
    
    projectile.lifeCounter = 0;
    projectile.position = position;
    projectile.velocity = {
        cosf(angle) * speed,
        sinf(angle) * speed
    };
    
    return projectile;
}

static void update()
{
    updateProjectiles(gProjectiles, PROJECTILE_LIFETIME);
    updateProjectiles(gParticles, 0.5 * 1000);
    updateAsteroids(gAsteroids);
    
    switch (gState)
    {
        case GameState_Game:
            updateShip(gShip);
            checkCollisions(gShip, gAsteroids);
            checkWin();
            break;
        case GameState_Lost:
            break;
        case GameState_Won:
            updateShip(gShip);
            break;
            
        default:
            break;
    }
    
    checkProjectileCollisions(gProjectiles, gAsteroids);
}

static void updateAsteroids(std::vector<Asteroid> &asteroids)
{
    for (int asteroidIndex = 0;
         asteroidIndex < asteroids.size();
         asteroidIndex++)
    {
        updateAsteroid(asteroids[asteroidIndex]);
    }
}

static void updateAsteroid(Asteroid &asteroid)
{
    asteroid.position.x += asteroid.velocity.x;
    asteroid.position.y += asteroid.velocity.y;
    asteroid.angle += asteroid.angularVelocity;
    
    wrapPosition(asteroid.position, WRAPBUFFER_X, WRAPBUFFER_Y);
    
    float r = distance({ 0, 0 },
                       asteroid.shape.lines[0].p1);
    float theta = (2 * M_PI) / N_LINES;
    
    for (int i = 0; i < N_LINES; i++)
    {
        asteroid.shape.lines[i] = {
            {
                (r * cosf(theta * (i - 1) + asteroid.angle)),
                (r * sinf(theta * (i - 1) + asteroid.angle))
            },
            {
                (r * cosf(theta * i + asteroid.angle)),
                (r * sinf(theta * i + asteroid.angle))
            }
        };
    }
}

static void updateShip(Ship &ship)
{
    ship.position.x += ship.velocity.x;
    ship.position.y += ship.velocity.y;
    
    wrapPosition(ship.position, WRAPBUFFER_X, WRAPBUFFER_Y);
    
    float angle = 0.0f;
    
    if (ship.turnLeft)
    {
        angle -= SHIP_ANGULARSPEED;
    }
    
    if (ship.turnRight)
    {
        angle += SHIP_ANGULARSPEED;
    }
    
    if (ship.thrusting)
    {
        if (ship.speed < SHIP_MAXSPEED)
        {
            ship.speed += SHIP_THRUST;
        }
        
        if (ship.speed > SHIP_MAXSPEED)
        {
            ship.speed = SHIP_MAXSPEED;
        }
        
        ship.velocity = {
            cosf(ship.angle) * ship.speed,
            sinf(ship.angle) * ship.speed
        };
    }
    else
    {
        ship.velocity = {
            ship.velocity.x * 0.99f,
            ship.velocity.y * 0.99f
        };
        
        ship.speed *= 0.9f;
        
        if (fabs(ship.velocity.x) < 0.005f)
        {
            ship.velocity.x = 0;
        }
        
        if (fabs(ship.velocity.y) < 0.005f)
        {
            ship.velocity.y = 0;
        }
    }
    
    ship.angle += angle;
    
    if (ship.shooting)
    {
        if (ship.cooldown == 0)
        {
            // A bit messy but I think the point has come across.
            fireProjectileFromPoint(ship.position, ship.angle);
        }
        
        ship.cooldown += MS_PER_UPDATE;
    }
    else
    {
        ship.cooldown = 0;
    }
    
    if (ship.cooldown >= PROJECTILE_COOLDOWN)
    {
        ship.cooldown = 0;
    }
    
    for (int i = 0;
         i < N_SHIP_LINES;
         i++)
    {
        Vector2f p1 = ship.lines[i].p1;
        Vector2f p2 = ship.lines[i].p2;
        
        ship.lines[i] = {
            {
                p1.x * cosf(angle) - p1.y * sinf(angle),
                p1.x * sinf(angle) + p1.y * cosf(angle)
            },
            {
                p2.x * cosf(angle) - p2.y * sinf(angle),
                p2.x * sinf(angle) + p2.y * cosf(angle)
            }
        };
    }
}

static void updateProjectiles(std::vector<Projectile> &projectiles, int lifeTime)
{
    for (int projectileIndex = 0;
         projectileIndex < projectiles.size();
         projectileIndex++)
    {
        Projectile &projectile = projectiles[projectileIndex];
        
        updateProjectile(projectile);
        
        if (projectile.lifeCounter >= lifeTime)
        {
            destroyProjectile(projectileIndex, projectiles);
        }
    }
}

static void updateProjectile(Projectile &projectile)
{
    projectile.position.x += projectile.velocity.x;
    projectile.position.y += projectile.velocity.y;
    projectile.lifeCounter += MS_PER_UPDATE;
    
    wrapPosition(projectile.position, WRAPBUFFER_X, WRAPBUFFER_Y);
}

static void render()
{
    SDL_SetRenderDrawColor(gRenderer, 0, 0, 0, 255);
    SDL_RenderClear(gRenderer);
    
    renderProjectiles(gProjectiles);
    renderProjectiles(gParticles);
    renderAsteroids(gAsteroids);
    
    switch (gState)
    {
        case GameState_Game:
            renderShip(gShip);
            break;
        case GameState_Lost:
            renderText("You Lost.", { WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 - 30 });
            renderText("Press RETURN to play again.", { WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 30 });
            break;
        case GameState_Won:
            renderShip(gShip);
            renderText("You Won.", { WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 - 30 });
            renderText("Press RETURN to play again.", { WINDOW_WIDTH / 2, WINDOW_HEIGHT / 2 + 30 });
            break;
            
        default:
            break;
    }
    
    SDL_RenderPresent(gRenderer);
}

static void renderAsteroids(std::vector<Asteroid> asteroids)
{
    for (int asteroidIndex = 0;
         asteroidIndex < asteroids.size();
         asteroidIndex++)
    {
        renderAsteroid(asteroids[asteroidIndex]);
    }
}

static void renderAsteroid(Asteroid asteroid)
{
    SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);
    
    for (int lineIndex = 0;
         lineIndex < N_LINES;
         lineIndex++)
    {
        Line current = asteroid.shape.lines[lineIndex];
        
        SDL_RenderDrawLine(gRenderer,
                           current.p1.x + asteroid.position.x,
                           current.p1.y + asteroid.position.y,
                           current.p2.x + asteroid.position.x,
                           current.p2.y + asteroid.position.y);
    }
}

static void renderShip(Ship ship)
{
    for (int lineIndex = 0;
         lineIndex < N_SHIP_LINES;
         lineIndex++)
    {
        Line current = ship.lines[lineIndex];
        
        SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);
        SDL_RenderDrawLine(gRenderer,
                           current.p1.x + ship.position.x,
                           current.p1.y + ship.position.y,
                           current.p2.x + ship.position.x,
                           current.p2.y + ship.position.y);
    }
}

static void renderProjectiles(std::vector<Projectile> projectiles)
{
    for (int projectileIndex = 0;
         projectileIndex < projectiles.size();
         projectileIndex++)
    {
        renderProjectile(projectiles[projectileIndex]);
    }
}

static void renderProjectile(Projectile &projectile)
{
    SDL_Rect rect = {
        (int)projectile.position.x - PROJECTILE_SIZE / 2,
        (int)projectile.position.y - PROJECTILE_SIZE / 2,
        PROJECTILE_SIZE,
        PROJECTILE_SIZE
    };
    
    SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, 255);
    SDL_RenderFillRect(gRenderer, &rect);
}

static void renderText(const char *text, Vector2f position)
{
    SDL_Color color = { 200, 200, 200 };
    SDL_Surface *fontSurface = TTF_RenderText_Solid(gDefaultFont, text, color);
    
    if (fontSurface == nullptr)
    {
        std::cout << "Unable to render font" << std::endl;
        std::cout << TTF_GetError() << std::endl;
        quit();
        exit(1);
    }
    
    SDL_Texture *fontTexture = SDL_CreateTextureFromSurface(gRenderer, fontSurface);
    
    if (fontTexture == nullptr)
    {
        std::cout << "Unable to render font" << std::endl;
        std::cout << SDL_GetError() << std::endl;
        quit();
        exit(1);
    }
    
    // I need to do this a better way.  I know how.  Just need to do it.
    SDL_Rect srcRect = { 0, 0, 0, 0 };
    SDL_QueryTexture(fontTexture, nullptr, nullptr, &srcRect.w, &srcRect.h);
    
    SDL_Rect dstRect = {
        (int)position.x - srcRect.w / 2,
        (int)position.y - srcRect.h / 2,
        srcRect.w,
        srcRect.h
    };
    
    SDL_RenderCopy(gRenderer, fontTexture, &srcRect, &dstRect);
}

static int randomDirection()
{
    int random = rand() % 2;
    return (random == 0) ? -1 : 1;
}

static int random(int min, int max)
{
    return rand() % (max - min + 1) + min;
}

static float randomNormal()
{
    return random(0, 100000) / 100000.0f;
}

static bool linesIntersect(Vector2f origin1, Vector2f origin2, Line l1, Line l2)
{
    Line modl1 = {
        { l1.p1.x + origin1.x, l1.p1.y + origin1.y },
        { l1.p2.x + origin1.x, l1.p2.y + origin1.y }
    };
    
    Line modl2 = {
        { l2.p1.x + origin2.x, l2.p1.y + origin2.y },
        { l2.p2.x + origin2.x, l2.p2.y + origin2.y }
    };
    
    return (counterClockwise(modl1.p1, modl2.p1, modl2.p2) !=
            counterClockwise(modl1.p2, modl2.p1, modl2.p2)) &&
           (counterClockwise(modl1.p1, modl1.p2, modl2.p1) !=
            counterClockwise(modl1.p1, modl1.p2, modl2.p2));
}

// http://bryceboe.com/2006/10/23/line-segment-intersection-algorithm/
static bool counterClockwise(Vector2f a, Vector2f b, Vector2f c)
{
    return (c.y - a.y) * (b.x - a.x) > (b.y - a.y) * (c.x - a.x);
}

static float distance(Vector2f p1, Vector2f p2)
{
    return sqrt(powf(p1.x - p2.x, 2.0f) +
                powf(p1.y - p2.y, 2.0f));
}

static void wrapPosition(Vector2f &position, int bufferX, int bufferY)
{
    Vector2f wrapMin = {
        -WRAPBUFFER_X,
        -WRAPBUFFER_Y
    };
    
    Vector2f wrapMax = {
        WINDOW_WIDTH + WRAPBUFFER_X,
        WINDOW_HEIGHT + WRAPBUFFER_Y
    };
    
    if (position.x < wrapMin.x)
    {
        position.x = wrapMax.x - 1;
    }
    
    if (position.x >= wrapMax.x)
    {
        position.x = wrapMin.x;
    }
    
    if (position.y < wrapMin.y)
    {
        position.y = wrapMax.y - 1;
    }
    
    if (position.y >= wrapMax.y)
    {
        position.y = wrapMin.y;
    }
}

static void checkCollisions(Ship ship, std::vector<Asteroid> asteroids)
{
    for (int asteroidIndex = 0;
         asteroidIndex < asteroids.size();
         asteroidIndex++)
    {
        Asteroid currentAsteroid = asteroids[asteroidIndex];
        checkCollision(ship, currentAsteroid);
    }
}

static void checkCollision(Ship ship, Asteroid asteroid)
{
    for (int aLineIndex = 0;
         aLineIndex < N_LINES;
         aLineIndex++)
    {
        Line currentAsteroidLine = asteroid.shape.lines[aLineIndex];
        
        for (int sLineIndex = 0;
             sLineIndex < N_SHIP_LINES;
             sLineIndex++)
        {
            Line currentShipLine = ship.lines[sLineIndex];
            
            if (linesIntersect(ship.position,
                               asteroid.position,
                               currentShipLine,
                               currentAsteroidLine))
            {
                explode(ship.position);
                gState = GameState_Lost;
            }
        }
    }
}

static void fireProjectileFromPoint(Vector2f point, float angle)
{
    gProjectiles.push_back(createProjectile(point, angle, PROJECTILE_SPEED));
}

static void destroyProjectile(int projectileIndex, std::vector<Projectile> &projectiles)
{
    if (projectileIndex != projectiles.size() - 1)
    {
        projectiles[projectileIndex] = projectiles[projectiles.size() - 1];
    }
    
    projectiles.pop_back();
}

static void destroyAsteroid(int asteroidIndex)
{
    if (asteroidIndex != gAsteroids.size() - 1)
    {
        gAsteroids[asteroidIndex] = gAsteroids[gAsteroids.size() - 1];
    }
    
    gAsteroids.pop_back();
}

static void splitAsteroid(int asteroidIndex)
{
    Asteroid asteroid = gAsteroids[asteroidIndex];
    AsteroidSize newSize = asteroid.size;
    
    switch (asteroid.size)
    {
        case ASTEROIDSIZE_SMALL:
            destroyAsteroid(asteroidIndex);
            return;
            break;
            
        case ASTEROIDSIZE_MEDIUM:
            newSize = ASTEROIDSIZE_SMALL;
            break;
            
        case ASTEROIDSIZE_LARGE:
            newSize = ASTEROIDSIZE_MEDIUM;
            break;
            
        default:
            break;
    }
    
    Asteroid newAsteroid1 = createAsteroid(newSize);
    Asteroid newAsteroid2 = createAsteroid(newSize);
    
    newAsteroid1.position = asteroid.position;
    newAsteroid2.position = asteroid.position;
    
    gAsteroids.push_back(newAsteroid1);
    gAsteroids.push_back(newAsteroid2);
    
    destroyAsteroid(asteroidIndex);
}

static void checkProjectileCollisions(std::vector<Projectile> projectiles,
                                      std::vector<Asteroid> asteroids)
{
    for (int asteroidIndex = 0;
         asteroidIndex < asteroids.size();
         asteroidIndex++)
    {
        Asteroid asteroid = asteroids[asteroidIndex];
        bool breakFlag = false;
        
        for (int aLineIndex = 0;
             aLineIndex < N_LINES;
             aLineIndex++)
        {
            Line asteroidLine = asteroid.shape.lines[aLineIndex];
            breakFlag = false;
            
            for (int projectileIndex = 0;
                 projectileIndex < projectiles.size();
                 projectileIndex++)
            {
                Projectile projectile = projectiles[projectileIndex];
                
                Vector2f lastPoint = {
                    projectile.position.x - projectile.velocity.x,
                    projectile.position.y - projectile.velocity.y
                };
                
                Line collisionLine = { projectile.position, lastPoint };
                
                if (linesIntersect({ 0, 0 }, asteroid.position, collisionLine, asteroidLine))
                {
                    explode(asteroid.position);
                    splitAsteroid(asteroidIndex);
                    destroyProjectile(projectileIndex, gProjectiles);
                    breakFlag = true;
                    break;
                }
            }
            
            if (breakFlag)
            {
                break;
            }
        }
        
        if (breakFlag)
        {
            break;
        }
    }
}

static void explode(Vector2f position)
{
    int nParticles = 10;
    float speed = 2.0f;
    float step = (2 * M_PI) / nParticles;
    
    for (int i = 0; i < nParticles; i++)
    {
        gParticles.push_back(createProjectile(position, step * i, speed));
    }
}

static TTF_Font *loadFont(const char *path)
{
    SDL_RWops *fontRWops = SDL_RWFromFile(path, "rb");
    
    if (fontRWops == nullptr)
    {
        std::cout << "Unable to load font " << path << std::endl;
        std::cout << TTF_GetError() << std::endl;
        quit();
        exit(1);
    }
    
    TTF_Font *font = TTF_OpenFontRW(fontRWops, 1, 72);
    
    if (font == nullptr)
    {
        std::cout << "Unable to load font " << path << std::endl;
        std::cout << TTF_GetError() << std::endl;
        quit();
        exit(1);
    }
    
    return font;
}

static void checkWin()
{
    std::cout << "Asteroids: " << gAsteroids.size() << std::endl;
    
    if (gAsteroids.size() == 0)
    {
        gState = GameState_Won;
    }
}
