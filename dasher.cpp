#include "raylib.h"

struct AnimData
{
    Rectangle rec;
    Vector2 pos;
    int frame;
    float updateTime;
    float runningTime;
    bool passed; // used for scoring when a nebula passes the player
   
    //Per-anim tint and golden flag for colored nebula variants
    Color tint;
    bool isGolden;
};

enum GameState {
    MENU,
    INSTRUCTIONS,
    PLAYING
};

struct MenuParticle
{
    Vector2 pos;
    Vector2 velocity;
    float size;
    float alpha;
};

// Easy tuning variables for forgiving collision
const float PLAYER_HITBOX_PADDING = 25.0f;
const float NEBULA_HITBOX_PADDING = 30.0f;
bool showHitboxes = false;

static float moveTowards(float current, float target, float delta)
{
    if (current < target)
        return (current + delta > target) ? target : current + delta;
    else
        return (current > target) ? ((current - delta < target) ? target : current - delta) : target;
}

static MenuParticle createMenuParticle(int screenWidth, int screenHeight)
{
    MenuParticle particle;
    particle.pos = {(float)GetRandomValue(0, screenWidth), (float)GetRandomValue(20, screenHeight - 60)};
    particle.velocity = {(float)GetRandomValue(-30, -10), 0.0f};
    particle.size = (float)GetRandomValue(1, 3);
    particle.alpha = (float)GetRandomValue(50, 160);
    return particle;
}

static void updateMenuParticle(MenuParticle &particle, float deltaTime, int screenWidth, int screenHeight)
{
    particle.pos.x += particle.velocity.x * deltaTime;
    if (particle.pos.x < -20.0f)
    {
        particle = createMenuParticle(screenWidth, screenHeight);
        particle.pos.x = (float)screenWidth + 20.0f;
    }
}

bool isOnGround(AnimData data, int windowHeight)
{
    return data.pos.y >= windowHeight - data.rec.height;
}

AnimData updateAnimData(AnimData data, float deltaTime, int maxFrame)
{
    // update running time
    data.runningTime += deltaTime;
    if (data.runningTime >= data.updateTime)
    {
        data.runningTime = 0.0;
        // update animation frame
        data.rec.x = data.frame * data.rec.width;
        data.frame++;
        if (data.frame > maxFrame)
        {
            data.frame = 0;
        }
    }

    return data;
}

static Rectangle getPlayerHitbox(const AnimData &player)
{
    return Rectangle{
        player.pos.x + PLAYER_HITBOX_PADDING,
        player.pos.y + PLAYER_HITBOX_PADDING,
        player.rec.width - 2 * PLAYER_HITBOX_PADDING,
        player.rec.height - 2 * PLAYER_HITBOX_PADDING
    };
}

static Rectangle getNebulaHitbox(const AnimData &nebula)
{
    return Rectangle{
        nebula.pos.x + NEBULA_HITBOX_PADDING,
        nebula.pos.y + NEBULA_HITBOX_PADDING,
        nebula.rec.width - 2 * NEBULA_HITBOX_PADDING,
        nebula.rec.height - 2 * NEBULA_HITBOX_PADDING
    };
}

// new helper functions for endless nebula recycling
float getRandomNebulaSpacing()
{
    return (float)GetRandomValue(250, 800);
}

// Color palette and helper for nebula tinting
static const Color NEBULA_PALETTE[] = {
    /* Purple (default) */ Color{160,  64, 220, 255},
    /* Blue */            Color{80,  150, 255, 255},
    /* Red */             Color{255, 80,  100, 255},
    /* Green */           Color{120, 255, 140, 255},
    /* Yellow */          Color{255, 220, 80,  255},
    /* Cyan */            Color{80,  240, 230, 255},
    /* Orange */          Color{255, 150, 60,  255}
};
static const int NEBULA_PALETTE_COUNT = sizeof(NEBULA_PALETTE) / sizeof(NEBULA_PALETTE[0]);

// Returns a color from the palette and sets isGolden when golden roll occurs.
// Golden chance: 7% 
static Color randomNebulaColor(bool &isGolden)
{
    const int goldenChancePercent = 7; // 7% chance for golden nebula
    int roll = GetRandomValue(1, 100);
    if (roll <= goldenChancePercent)
    {
        isGolden = true;
        return Color{255, 215, 64, 255}; // bright gold tint
    }
    isGolden = false;
    int idx = GetRandomValue(0, NEBULA_PALETTE_COUNT - 1);
    return NEBULA_PALETTE[idx];
}

float getFurthestNebulaX(AnimData nebulae[], int size)
{
    float furthestX = 0.0f;
    for (int i = 0; i < size; i++)
    {
        if (nebulae[i].pos.x > furthestX)
        {
            furthestX = nebulae[i].pos.x;
        }
    }
    return furthestX;
}

// forward declaration
void resetGame(AnimData& scarfyData, AnimData Nebulae[], float& bgX, float& mgX, float& fgX, 
               int& velocity, bool& isInAir, bool& collision, bool& gameEnded,
               int windowDimensions[], const int sizeOfNebulae);

void resetGame(AnimData& scarfyData, AnimData Nebulae[], float& bgX, float& mgX, float& fgX, 
               int& velocity, bool& isInAir, bool& collision, bool& gameEnded,
               int windowDimensions[], const int sizeOfNebulae)
{
    // reset player position and state
    scarfyData.pos.x = windowDimensions[0]/2 - scarfyData.rec.width/2;
    scarfyData.pos.y = windowDimensions[1] - scarfyData.rec.height;
    scarfyData.frame = 0;
    scarfyData.runningTime = 0.0;
    
    // reset physics
    velocity = 0;
    isInAir = false;
    
    // reset collision
    collision = false;
    
    // reset background positions
    bgX = 0.0;
    mgX = 0.0;
    fgX = 0.0;
    
    // reset obstacles with random spacing
    float spawnX = windowDimensions[0];
    for (int i = 0; i < sizeOfNebulae; i++)
    {
        Nebulae[i].pos.x = spawnX;
        Nebulae[i].frame = 0;
        Nebulae[i].runningTime = 0.0;
        Nebulae[i].passed = false;
        // randomize tint and golden flag on reset
        Nebulae[i].tint = randomNebulaColor(Nebulae[i].isGolden);
        spawnX += getRandomNebulaSpacing();
    }
    
    // exit game over state
    gameEnded = false;
}


int main(){

    // window dimensions 
    int windowDimensions[2];
    windowDimensions[0] = 800;
    windowDimensions[1] = 500;
  
    //window initialization
   InitWindow(windowDimensions[0], windowDimensions[1], "Nebula Dash");
   SetTargetFPS(60);
   InitAudioDevice(); // initialize audio system
   bool collision{};
   GameState gameState = MENU; // start in menu
   bool gameEnded = false; // game over state flag
   int score = 0; // current score
   int nebulaPassedCount = 0; // number of obstacles passed successfully
   int highScore = 0; // displayed on menu
   
   // audio system variables
   Music bgMusic = LoadMusicStream("audio/background_music.mp3");
   float musicVolume = 0.5f; // track current volume
   SetMusicVolume(bgMusic, musicVolume);
   bool isMusicMuted = false;
   PlayMusicStream(bgMusic); // start music on menu

    // int posY{windowHeight-scarfyRec.height};
    int velocity{0};
    
    //acceleration
    const int gravity{1'000};

    // scarfy variables
    Texture2D scarfy = LoadTexture("textures/scarfy.png");
    AnimData scarfyData;
    scarfyData.rec.width = scarfy.width/6;
    scarfyData.rec.height = scarfy.height;
    scarfyData.rec.x = 0;
    scarfyData.rec.y = 0;
    scarfyData.pos.x = windowDimensions[0]/2 - scarfyData.rec.width/2;
    scarfyData.pos.y = windowDimensions[1] - scarfyData.rec.height;
    scarfyData.frame = 0;
    scarfyData.updateTime = 1.0/12.0;
    scarfyData.runningTime = 0.0;

    // nebula variables
    Texture2D nebula = LoadTexture("textures/12_nebula_spritesheet.png");
    // building texture
    Texture2D background = LoadTexture("textures/far-buildings.png");
    float bgX{};
    Texture2D midground = LoadTexture("textures/back-buildings.png");
    float mgX{};
    Texture2D foreground = LoadTexture("textures/foreground.png");
    float fgX{};

    const int sizeOfNebulae{6};
    AnimData Nebulae[6]{};
    float spawnX = windowDimensions[0];
    for (int i = 0; i < 6; i++)
{
    Nebulae[i].rec.x = 0.0;
    Nebulae[i].rec.y = 0.0;
    Nebulae[i].rec.width = nebula.width/8;
    Nebulae[i].rec.height = nebula.height/8;
    Nebulae[i].pos.y = windowDimensions[1] - nebula.height/8;
    Nebulae[i].frame = 0;
    Nebulae[i].runningTime = 0.0;
    Nebulae[i].updateTime = 1.0/16.0;
    Nebulae[i].pos.x = spawnX; // Position each nebula with random spacing
    Nebulae[i].passed = false;
    //Initialization change: assign a random vibrant tint and golden flag
    Nebulae[i].tint = randomNebulaColor(Nebulae[i].isGolden);
    spawnX += getRandomNebulaSpacing();
}

    const int menuNebulaCount{6};
    AnimData menuNebula[menuNebulaCount]{};
    for (int i = 0; i < menuNebulaCount; i++)
    {
        menuNebula[i].rec.x = 0.0;
        menuNebula[i].rec.y = 0.0;
        menuNebula[i].rec.width = nebula.width/8;
        menuNebula[i].rec.height = nebula.height/8;
        menuNebula[i].pos = {(float)GetRandomValue(100, 700), (float)GetRandomValue(50, 220)};
        menuNebula[i].frame = 0;
        menuNebula[i].runningTime = 0.0;
        menuNebula[i].updateTime = 1.0/14.0;
        // give menu nebula a safe default tint (does not affect menu drawing that uses Fade())
        menuNebula[i].tint = Color{100, 220, 255, 255};
        menuNebula[i].isGolden = false;
    }
    
    const int menuParticleCount = 28;
    MenuParticle menuParticles[menuParticleCount]{};
    for (int i = 0; i < menuParticleCount; i++)
    {
        menuParticles[i] = createMenuParticle(windowDimensions[0], windowDimensions[1]);
    }

    Rectangle startButton = {(float)windowDimensions[0] / 2 - 130, 280, 260, 65}; // slightly larger START button
    Rectangle exitButton = {(float)windowDimensions[0] / 2 - 110, 370, 220, 55};
    int hoveredButton = -1;
    float buttonScale[2] = {1.0f, 1.0f};
    const float buttonHoverScale = 1.08f;
    const float buttonScaleSpeed = 8.0f;
    float titleGlow = 0.0f;
    float titleGlowDir = 1.0f;
    float instructionPulse = 0.0f;
    float instructionPulseDir = 1.0f;
    float instructionAlpha = 0.0f;

    // nebula
    int nebVel{-200};

    // prevent air jumping
    bool isInAir{};
    // jump velocity(pixels/frame)
    const int jumpVel{-700};

    while (!WindowShouldClose()) {

        // delta time (time since last frame)
        const float dT = GetFrameTime();
        
        // update music stream every frame
        UpdateMusicStream(bgMusic);
        
        // keyboard controls: M = mute/unmute, + = increase volume, - = decrease volume
        if (IsKeyPressed(KEY_M))
        {
            isMusicMuted = !isMusicMuted;
            if (isMusicMuted)
                PauseMusicStream(bgMusic);
            else
                ResumeMusicStream(bgMusic);
        }
        if (IsKeyPressed(KEY_EQUAL))
        {
            if (musicVolume < 1.0f)
            {
                musicVolume += 0.1f;
                if (musicVolume > 1.0f) musicVolume = 1.0f;
                SetMusicVolume(bgMusic, musicVolume);
            }
        }
        if (IsKeyPressed(KEY_MINUS))
        {
            if (musicVolume > 0.0f)
            {
                musicVolume -= 0.1f;
                if (musicVolume < 0.0f) musicVolume = 0.0f;
                SetMusicVolume(bgMusic, musicVolume);
            }
        }
        
        //drawing

        BeginDrawing();
        ClearBackground(RAYWHITE);

        // scroll the background
        bgX -= 20 * dT;
        if (bgX <= -background.width * 3.5f) {
            bgX = 0.0f;
        }

        // scroll the midground
        mgX -= 40 * dT;
        if (mgX <= -midground.width * 3.5f) {
            mgX = 0.0f;
        }
        // scroll the foreground
        fgX -= 80 * dT;
        if (fgX <= -foreground.width * 3.5f) {
            fgX = 0.0f;
        }
        //Draw the background
        Vector2 bgPos = { bgX, 0.0f };
        DrawTextureEx(background, bgPos, 0.0f, 3.5f, RAYWHITE);
        float bgPos2X = bgX + background.width * 3.5f;
        Vector2 bgPos2 = { bgPos2X, 0.0f };
        DrawTextureEx(background, bgPos2, 0.0f, 3.5f, RAYWHITE);
        //Draw the midground
        Vector2 mgPos = { mgX, 0.0f };
        DrawTextureEx(midground, mgPos, 0.0f, 3.5f, RAYWHITE);
        float mgPos2X = mgX + midground.width * 3.5f;
        Vector2 mgPos2 = { mgPos2X, 0.0f };
        DrawTextureEx(midground, mgPos2, 0.0f, 3.5f, RAYWHITE);
        //Draw the foreground
        Vector2 fgPos = { fgX, 0.0f };
        DrawTextureEx(foreground, fgPos, 0.0f, 3.5f, RAYWHITE);
        float fgPos2X = fgX + foreground.width * 3.5f;
        Vector2 fgPos2 = { fgPos2X, 0.0f };
        DrawTextureEx(foreground, fgPos2, 0.0f, 3.5f, RAYWHITE);

        // MENU state: show title, buttons, controls, and floating decorations
        if (gameState == MENU)
        {
            // animate title glow
            titleGlow += titleGlowDir * 100.0f * dT;
            if (titleGlow >= 120.0f)
            {
                titleGlow = 120.0f;
                titleGlowDir = -1.0f;
            }
            else if (titleGlow <= 0.0f)
            {
                titleGlow = 0.0f;
                titleGlowDir = 1.0f;
            }

            const Vector2 mousePoint = GetMousePosition();
            hoveredButton = -1;
            if (CheckCollisionPointRec(mousePoint, startButton)) hoveredButton = 0;
            else if (CheckCollisionPointRec(mousePoint, exitButton)) hoveredButton = 1;

            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT))
            {
                if (hoveredButton == 0)
                {
                    gameState = INSTRUCTIONS;
                }
                else if (hoveredButton == 1)
                {
                    CloseWindow();
                    return 0;
                }
            }
            if (IsKeyPressed(KEY_ENTER))
            {
                gameState = INSTRUCTIONS;
            }

            // draw cyberpunk overlay and particles
            DrawRectangle(0, 0, windowDimensions[0], windowDimensions[1], Color{8, 16, 30, 120});
            for (int i = 0; i < menuParticleCount; i++)
            {
                updateMenuParticle(menuParticles[i], dT, windowDimensions[0], windowDimensions[1]);
                const Color particleColor = Fade(Color{80, 220, 255, 255}, menuParticles[i].alpha / 255.0f);
                DrawCircleV(menuParticles[i].pos, menuParticles[i].size, particleColor);
            }

            // draw floating nebula decorations
            for (int i = 0; i < menuNebulaCount; i++)
            {
                menuNebula[i] = updateAnimData(menuNebula[i], dT, 7);
                menuNebula[i].pos.x += 18 * dT;
                if (menuNebula[i].pos.x > windowDimensions[0] + 120)
                {
                    menuNebula[i].pos.x = -120;
                    menuNebula[i].pos.y = (float)GetRandomValue(50, 220);
                }
                DrawTextureRec(nebula, menuNebula[i].rec, menuNebula[i].pos, Fade(Color{100, 220, 255, 255}, 0.55f));
            }

            // title glow
            titleGlow += titleGlowDir * 120.0f * dT;
            if (titleGlow >= 140.0f)
            {
                titleGlow = 140.0f;
                titleGlowDir = -1.0f;
            }
            else if (titleGlow <= 20.0f)
            {
                titleGlow = 20.0f;
                titleGlowDir = 1.0f;
            }

            const int titleSize = 74;
            const int titleWidth = MeasureText("NEBULA DASH", titleSize);
            const int titleX = (windowDimensions[0] - titleWidth) / 2;
            DrawText("NEBULA DASH", titleX + 4, 80 + 4, titleSize, BLACK);
            DrawText("NEBULA DASH", titleX, 76, titleSize, Color{120, 230, 255, (unsigned char)(180 + titleGlow * 0.5f)});
            DrawText("INFINITE LOOP", (windowDimensions[0] - MeasureText("INFINITE LOOP", 28)) / 2, 160, 28, Color{170, 235, 255, 190});

            // menu buttons
            buttonScale[0] = moveTowards(buttonScale[0], hoveredButton == 0 ? buttonHoverScale : 1.0f, buttonScaleSpeed * dT);
            buttonScale[1] = moveTowards(buttonScale[1], hoveredButton == 1 ? buttonHoverScale : 1.0f, buttonScaleSpeed * dT);

            auto drawNeonButton = [&](Rectangle rect, const char *text, bool hovered, float scale)
            {
                Vector2 center = {rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f};
                Rectangle scaledRect = {center.x - rect.width * scale / 2.0f, center.y - rect.height * scale / 2.0f, rect.width * scale, rect.height * scale};
                Color panel = Fade(Color{12, 28, 50, 220}, hovered ? 0.92f : 0.78f);
                DrawRectangleRounded(scaledRect, 0.18f, 30, panel);
                DrawRectangleRoundedLines(scaledRect, 0.18f, 30, Fade(Color{70, 220, 255, 255}, hovered ? 0.95f : 0.55f));
                DrawText(text, scaledRect.x + scaledRect.width / 2 - MeasureText(text, 28) / 2, scaledRect.y + scaledRect.height / 2 - 16, 28, WHITE);
            };

            drawNeonButton(startButton, "START", hoveredButton == 0, buttonScale[0]);
            drawNeonButton(exitButton, "EXIT", hoveredButton == 1, buttonScale[1]);

            // menu info
            DrawText(TextFormat("HIGH SCORE: %04i", highScore), 50, 34, 18, Fade(Color{150, 240, 255, 255}, 0.95f));
            DrawText("SPACE = JUMP", 50, 430, 18, Fade(Color{150, 240, 255, 255}, 0.90f));
            DrawText("R = RESTART", 50, 458, 18, Fade(Color{150, 240, 255, 255}, 0.90f));
            DrawText("M = MUTE", 50, 486, 18, Fade(Color{150, 240, 255, 255}, 0.90f));

            EndDrawing();
            continue;
        }

        if (gameState == INSTRUCTIONS)
        {
            instructionAlpha += dT * 1.6f;
            if (instructionAlpha > 1.0f) instructionAlpha = 1.0f;
            instructionPulse += instructionPulseDir * 1.2f * dT;
            if (instructionPulse >= 1.0f)
            {
                instructionPulse = 1.0f;
                instructionPulseDir = -1.0f;
            }
            else if (instructionPulse <= 0.0f)
            {
                instructionPulse = 0.0f;
                instructionPulseDir = 1.0f;
            }

            // stronger panel alpha for better visibility
            const float panelAlpha = 0.38f * instructionAlpha;
            const Rectangle instructionPanel = { 120.0f, 120.0f, 560.0f, 260.0f };
            DrawRectangleRounded(instructionPanel, 0.22f, 30, Fade(Color{10, 18, 30, 220}, panelAlpha * 0.98f));
            DrawRectangleRoundedLines(instructionPanel, 0.22f, 30, Fade(Color{90, 240, 255, 255}, 0.95f));
            DrawRectangleRounded({ instructionPanel.x + 8, instructionPanel.y + 8, instructionPanel.width - 16, instructionPanel.height - 16 }, 0.22f, 30, Fade(Color{64, 220, 255, 255}, panelAlpha * 0.18f));

            const int titleSize = 52;
            const int titleWidth = MeasureText("HOW TO PLAY", titleSize);
            const int titleX = (windowDimensions[0] - titleWidth) / 2;
            const int titleY = 150;
            DrawText("HOW TO PLAY", titleX + 3, titleY + 3, titleSize, Fade(BLACK, instructionAlpha));
            DrawText("HOW TO PLAY", titleX, titleY, titleSize, Fade(Color{120, 230, 255, 255}, 0.9f * instructionAlpha));

            const int itemSize = 26;
            const char *line1 = "Press SPACEBAR to Jump";
            const char *line2 = "Pass Nebulae to Score Points";
            const int line1X = (windowDimensions[0] - MeasureText(line1, itemSize)) / 2;
            const int line2X = (windowDimensions[0] - MeasureText(line2, itemSize)) / 2;
            // draw subtle shadow then brighter text for contrast
            DrawText(line1, line1X + 2, titleY + 80 + 2, itemSize, Fade(BLACK, instructionAlpha));
            DrawText(line1, line1X, titleY + 80, itemSize, Fade(Color{150, 250, 255, 255}, instructionAlpha));
            DrawText(line2, line2X + 2, titleY + 120 + 2, itemSize, Fade(BLACK, instructionAlpha));
            DrawText(line2, line2X, titleY + 120, itemSize, Fade(Color{140, 240, 200, 255}, instructionAlpha));

            const char *beginText = "Press ENTER to Begin";
            const int beginSize = 26;
            const int beginWidth = MeasureText(beginText, beginSize);
            const int beginX = (windowDimensions[0] - beginWidth) / 2;
            const int beginY = titleY + 180;
            DrawText(beginText, beginX + 2, beginY + 2, beginSize, Fade(RED, instructionAlpha));
            DrawText(beginText, beginX, beginY, beginSize, Fade(Color{180, 235, 255, 255}, 0.88f * instructionAlpha));

            if (IsKeyPressed(KEY_ENTER))
            {
                gameState = PLAYING;
            }

            EndDrawing();
            continue;
        }

        // gameplay HUD: current score in top-right
        if (!gameEnded)
        {
            DrawText(TextFormat("Score: %i", score), windowDimensions[0] - 170, 30, 20, SKYBLUE);
        }

        // perfrorm ground check
        if (isOnGround(scarfyData, windowDimensions[1])) {
           
            // rect is on the ground
            velocity=0;
            isInAir=false;
        } else {
            
            // rect is in the air
            velocity+=gravity*dT;
            isInAir=true;
        }

        // jump check
        if (IsKeyPressed(KEY_SPACE) && !isInAir) {
            velocity+=jumpVel;
        }
        
        // update scarfy animation frame
        if(!isInAir) {
            // update running time
        scarfyData=updateAnimData(scarfyData, dT, 5);
            
         }
     
         for (int i = 0; i < sizeOfNebulae; i++)
        {
         // update nebula animation frame
        Nebulae[i]=updateAnimData(Nebulae[i], dT, 7);
        }
        
        // nebula pass detection and scoring
        if (!gameEnded)
        {
            for (int i = 0; i < sizeOfNebulae; i++)
            {
                if (!Nebulae[i].passed && (Nebulae[i].pos.x + Nebulae[i].rec.width) < scarfyData.pos.x)
                {
                    Nebulae[i].passed = true;
                    // award 200 for golden nebula, otherwise 100
                    int award = Nebulae[i].isGolden ? 200 : 100;
                    score += award;
                    nebulaPassedCount += 1;
                    if (score > highScore)
                    {
                        highScore = score;
                    }
                }
            }
        }

        // update nebula 2 animation frame
        Nebulae[1].runningTime += dT;
        if (Nebulae[1].runningTime >= Nebulae[1].updateTime)
    {
        Nebulae[1].runningTime = 0.0;
        Nebulae[1].rec.x = Nebulae[1].frame * Nebulae[1].rec.width;
        Nebulae[1].frame++;
        if (Nebulae[1].frame > 7)
        {
            Nebulae[1].frame = 0;
        }
    }
    
    Rectangle playerHitbox = getPlayerHitbox(scarfyData);

    for (int i = 0; i < sizeOfNebulae; i++)
    {
        Rectangle nebulaHitbox = getNebulaHitbox(Nebulae[i]);

        if (CheckCollisionRecs(nebulaHitbox, playerHitbox))
        {
            collision = true;
        }

        if (showHitboxes)
        {
            DrawRectangleLinesEx(nebulaHitbox, 2.0f, RED);
        }
    }

    if (showHitboxes)
    {
        DrawRectangleLinesEx(playerHitbox, 2.0f, GREEN);
    }

    if(collision)
    {
        // lose game
        gameEnded = true;
        StopMusicStream(bgMusic); // stop music on collision
        // draw a dim overlay and the centered game over panel
        DrawRectangle(0, 0, windowDimensions[0], windowDimensions[1], Fade(BLACK, 0.55f));
        const Rectangle panelRect = { windowDimensions[0] / 2.0f - 220.0f, 120.0f, 440.0f, 260.0f };
        DrawRectangleRounded(panelRect, 0.22f, 30, Fade(Color{12, 20, 36, 220}, 0.94f));
        DrawRectangleRoundedLines(panelRect, 0.22f, 30, Fade(Color{120, 220, 255, 255}, 0.78f));

        const int gameOverSize = 64;
        const int gameOverWidth = MeasureText("GAME OVER", gameOverSize);
        const int gameOverX = (windowDimensions[0] - gameOverWidth) / 2;
        DrawText("GAME OVER", gameOverX + 3, panelRect.y + 22, gameOverSize, BLACK);
        DrawText("GAME OVER", gameOverX, panelRect.y + 20, gameOverSize, RED);

        const int infoSize = 28;
        const int infoLeft = panelRect.x + 30;
        const int infoStartY = panelRect.y + 110;
        DrawText(TextFormat("Score: %i", score), infoLeft, infoStartY, infoSize, SKYBLUE);
        DrawText(TextFormat("Nebula Passed: %i", nebulaPassedCount), infoLeft, infoStartY + 38, infoSize, Color{130, 255, 160, 255});
        DrawText(TextFormat("High Score: %i", highScore), infoLeft, infoStartY + 76, infoSize, Color{255, 220, 120, 255});

        // separator removed to match previous version

        // restart prompt text - centered below the panel
        const int restartSize = 26;
        const char *restartText = "Press R to Restart";
        const int restartWidth = MeasureText(restartText, restartSize);
        const int restartX = (windowDimensions[0] - restartWidth) / 2;
        const int restartY = (int)(panelRect.y + panelRect.height + 24);
        DrawText(restartText, restartX + 2, restartY + 2, restartSize, BLACK);
        DrawText(restartText, restartX, restartY, restartSize, Color{230, 240, 255, 255});
    }
    else
    { 
         for(int i = 0; i < sizeOfNebulae; i++)
        {
            // >>> Draw with per-nebula tint; golden nebula use their gold tint
            Color drawTint = Nebulae[i].tint;
            DrawTextureRec(nebula, Nebulae[i].rec, Nebulae[i].pos, drawTint);
        }
        //draw scarfy
        DrawTextureRec(scarfy, scarfyData.rec, scarfyData.pos, RAYWHITE);
    }

    // update scarfy position
        scarfyData.pos.y += velocity*dT;

        for (int i = 0; i < sizeOfNebulae; i++)
        {
            // update nebula position
            Nebulae[i].pos.x += nebVel*dT;

            if (Nebulae[i].pos.x + Nebulae[i].rec.width < 0)
            {
                float furthestX = getFurthestNebulaX(Nebulae, sizeOfNebulae);
                Nebulae[i].pos.x = furthestX + getRandomNebulaSpacing();
                Nebulae[i].frame = 0;
                Nebulae[i].runningTime = 0.0;
                Nebulae[i].rec.x = 0;
                Nebulae[i].passed = false;
                // >>> Respawn change: pick a new random tint and golden status on respawn
                Nebulae[i].tint = randomNebulaColor(Nebulae[i].isGolden);
            }
        }

        // restart game when R is pressed after game ends
        if (gameEnded && IsKeyPressed(KEY_R))
        {
            resetGame(scarfyData, Nebulae, bgX, mgX, fgX, velocity, isInAir, collision, gameEnded, windowDimensions, sizeOfNebulae);
            score = 0;
            nebulaPassedCount = 0;
            PlayMusicStream(bgMusic); // resume music on restart
        }

        // end drawing
        EndDrawing();
    }
    UnloadMusicStream(bgMusic); // unload music
    CloseAudioDevice(); // close audio device
    UnloadTexture(scarfy);
    UnloadTexture(nebula);
    UnloadTexture(background);
    UnloadTexture(midground);
    UnloadTexture(foreground);
    CloseWindow();
    
}
