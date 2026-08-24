#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "shader.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <irrKlang.h>

#include <cmath>
#include <iostream>
#include <vector>
#include <algorithm>
#include <map>
#include <string>
#include <thread>
#include <chrono>

//struct per il testo
struct Character {
    unsigned int TextureID;
    glm::ivec2   Size;
    glm::ivec2   Bearing;
    unsigned int Advance;
};

std::map<char, Character> Characters;
unsigned int textVAO, textVBO;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void renderText(Shader& s, std::string text, float x, float y, float scale, glm::vec3 color);

//impostazioni
const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
float aspect = (float)SCR_WIDTH / SCR_HEIGHT; //Da utilizzare su texture per uniformare alla dimensione 1280:720
glm::vec2 pos(0.0f, 0.0f);
float lastTime;
float delta;                                //= 0.00095f;   valore vecchio
float scale = 0.25f;

//contatore uccisioni e variabile per il gameOver
int scoreEnemies = 0;
bool gameOver = false;

//Vita del porto
float harborHp = 120.0f;

//Contatore monete momentanee e totali
int coinCount = 0;

int totalCoinsCollected = 0;

bool victorySoundPlayed = false;

//gameOver animazione variabili
float gameOverScale = 0.1f;
bool gameOverAnimFinished = false;

//animazione vittoria variabili
float victoryScale = 0.1f;
float victoryPosY = -1.0f;
bool victoryAnimFinished = false;

//Variabili delle ondate
int currentLevel = 0;
int currentWave = 0;
bool waveActive = false;
float pauseTimer = 0.0f;

//Stati del gioco
enum GameState {
    STATE_MENU,
    STATE_PLAYING,
    STATE_OPTIONS,
    STATE_GAMEOVER,
    STATE_VICTORY,
    STATE_STATS
};

//statistiche ritorna a pagina vittoria se va indietro, stesso ragionamento sconfitta
GameState previousState = STATE_MENU;

//Menù come stato iniziale
GameState gameState = STATE_MENU;

// mezzo lato del quadrato scalato
float halfSize = 0.5f * scale;

// limiti in coordinate NDC (-1 a +1)
float minX = aspect * (-0.55f) + halfSize;
float maxX = aspect * 1.0f - halfSize;
float minY = -1.0f + halfSize;
float maxY = 0.75f - halfSize;

struct wave {
    int enemyCount;
    float pauseAfter;
};

struct level {
    std::vector<wave> waves;
};

struct Entity {
    glm::vec2 posE;
    float speed;
    float hp;
    float radius;
    bool alive;

    //timer per gestire animazione esplosione
    float explTimer = 0.0f;  // 0 == off
};

enum DropType {
    DROP_HEART,
    DROP_COIN
};

struct Drop : public Entity {
    DropType type;
};

struct proiettile {
    glm::vec2 posP;
    glm::vec2 velocity;
    bool alive;
    float dmg;
    float radius;
};

//VolumeDelProiettile e SpeedDelPlayer
float Volume = 0.25f;
float playerSpeed = 0.5f;

struct naveNemica : public Entity {
    float shootCooldown = 0.0f;
};

struct navePlayer : public Entity {
    float invulnerabilityTime = 0.0f;
};

//struct del boss
struct Boss : public naveNemica {
    int phase = 0;
    float phaseHp = 0.0f;
    float attackTimer = 0.0f;
    float moveTimer = 0.0f;
    float defeatTimer = 0.0f;
};

//struct animazione monete
struct VictoryCoin {
    glm::vec2 pos;
    glm::vec2 velocity;
};

//variabili globali del boss
Boss boss;
bool bossActive = false;
bool bossJustDefeated = false;

//Bottoni del menù
struct Bottone {
    float x, y;
    float w, h;
};
Bottone bottonePlay = { 530, 335, 150, 50 };  // x, y, larghezza, altezza
Bottone bottoneOption = { 505, 210, 220, 50 };
Bottone bottoneExit = { 545, 85, 130, 50 };
Bottone bottoneBack = { 500, 200, 220, 40 };
Bottone bottoneVolMinus = { 710, 390, 30, 30 };
Bottone bottoneVolPlus = { 740, 390, 30, 30 };
Bottone bottoneSpdMinus = { 710, 320, 30, 30 };
Bottone bottoneSpdPlus = { 740, 320, 30, 30 };

//bottoni per gameOver e vittoria
Bottone bottoneGoMenu = { 250, 120, 140, 50 };
Bottone bottoneGoStats = { 500, 120, 280, 50 };
Bottone bottoneGoExit = { 890, 120, 140, 50 };

//bottone indietro in statistiche
Bottone bottoneStatsBack = { 540, 50, 200, 50 };

bool mousePressedLastFrame = false;

bool spacePressedLastFrame = false;

//vettore dei livelli
std::vector<level> levels = {
    {
        //level 1
        {
            {3, 3.0f},            //wave 1
            {5, 3.0f}             //wave 2
        }
    },
    {
        //level 2
        {
            {4, 3.0f},
            {6, 3.0f}
        }
    },
    /*{
        //level 3
        {
            {4, 2.0f },
            {5, 2.0f}
        }
    },
    {
        //level 4
        {
            {5, 4.0f },
            {6, 3.0f},
        }
    }*/
};

//Arrays
std::vector<proiettile> proiettiliPlayer;
std::vector<proiettile> proiettiliEnemies;
std::vector<naveNemica> navi;
std::vector<Drop> drops;
std::vector<VictoryCoin> victoryCoins;

//update movimento boss
void updateBoss(Boss& b)
{
    b.moveTimer += delta;
    b.posE.y = sin(b.moveTimer) * 0.5f;

    if (b.posE.x > 0.5f)
        b.posE.x -= b.speed * delta;
}

//colpo a ventaglio boss
void bossShootFan(Boss& b)
{
    for (int i = -2; i <= 2; i++) {
        proiettile p;
        p.posP = b.posE;
        float angle = i * 0.25f;
        p.velocity = glm::vec2(cos(angle), sin(angle)) * -0.6f;
        p.alive = true;
        p.radius = 0.025f;
        p.dmg = 20;
        proiettiliEnemies.push_back(p);
    }
}

//mira boss a player
void bossShootAim(Boss& b, const navePlayer& player)
{
    glm::vec2 dir = glm::normalize(player.posE - b.posE);
    proiettile p;
    p.posP = b.posE;
    p.velocity = dir * 0.9f;
    p.alive = true;
    p.radius = 0.03f;
    p.dmg = 25;
    proiettiliEnemies.push_back(p);
}

bool collides(const proiettile& p, const Entity& n) {
    float dist = glm::length(n.posE - p.posP);
    return dist < (n.radius + p.radius);
}

navePlayer generaNaveP(float posX, float posY) {
    navePlayer n;
    n.alive = true;
    n.hp = 100;
    n.posE.x = posX;
    n.posE.y = posY;
    n.radius = 0.13f;
    n.speed = 0.2f;
    return n;
}

naveNemica generaNaveStd(float posX, float posY, int currentLevel) {
    naveNemica n;
    n.alive = true;
    n.hp = 100;
    n.posE.x = posX;
    n.posE.y = posY;
    n.radius = 0.15f;

    // Rallentato un pelo la velocità base (era 0.2f)
    n.speed = 0.15f;
    // Abbassato un pelino il cooldown (sparano più frequenti) (era 1.5f)
    n.shootCooldown = 1.3f;

    n.speed += currentLevel * 0.05f;
    n.shootCooldown = std::max(0.5f, 1.3f - (currentLevel - 1) * 0.2f);

    //Inizializza il timer a 0
    n.explTimer = 0.0f;

    navi.push_back(n);

    return n;
}

void spawnEnemy(int currentLevel) {
    float x = aspect + static_cast<float>(rand()) / RAND_MAX * 1.0f;
    float y = -0.75f + static_cast<float>(rand()) / RAND_MAX * 1.4f;

    generaNaveStd(x, y, currentLevel);
}

void spawnWave(const wave& w, int currentLevel) {
    for (int i = 0; i < w.enemyCount; i++) {
        spawnEnemy(currentLevel);
    }
}

//navi si spostano verso sinistra
void updateNavePos(naveNemica& n) {
    n.posE.x -= n.speed * delta;
}

//se hp < 0 la nave muore
void updateNaveStatus(naveNemica& n) {
    // in render loop la nuova versione per esplosione
    // if (n.hp <= 0) n.alive = false;  
}

//controlla se c'e' collisione
bool checkNaveCollide(navePlayer& np, naveNemica& n) {
    float dist = glm::length(np.posE - n.posE);
    return dist < (np.radius + n.radius);
}

//applica danni collisione alle 2 barche
void naviCollision(navePlayer& np, naveNemica& n) {
    np.hp -= 50;
    n.hp -= 80;
    np.invulnerabilityTime = 1.0f;
}

//aggiunge un drop nel vettore drops che ha posizione "position" (in game)
void spawnDrop(const glm::vec2& position, DropType type) {
    Drop d;
    d.posE = position;
    d.radius = 0.05f;
    d.alive = true;
    d.type = type;
    d.speed = 0.4f;
    drops.push_back(d);
}

//applica l'effetto del drop e fa "morire" il drop
void applyDropEffect(Drop& d, navePlayer& player) {
    switch (d.type) {

    case DROP_HEART:
        player.hp = std::min(100.0f, player.hp + 30.0f);
        break;

    case DROP_COIN:
        coinCount++;
        totalCoinsCollected++;
        break;
    }

    d.alive = false;
}

//disegna la barra degli hp del player
void drawHealthBar(const Entity& n, Shader& shader, unsigned int VAO) {
    float hpBarMaxWidth = 0.2f;
    float hpBarHeight = 0.02f;

    float hpRatio = glm::clamp(n.hp / 100.0f, 0.0f, 1.0f);
    float currentHealth = hpBarMaxWidth * hpRatio;

    glm::mat4 trans = glm::mat4(1.0f);
    trans = glm::translate(trans, glm::vec3(n.posE.x - hpBarMaxWidth * 0.08f, n.posE.y + 0.125f, 0.0f));        // sopra la nave
    trans = glm::scale(trans, glm::vec3(currentHealth, hpBarHeight, 1.0f));

    shader.use();
    shader.setMat4("transform", trans);
    glm::vec3 color = glm::mix(
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        hpRatio
    );

    shader.setBool("useColor", true);
    shader.setVec3("solidColor", color);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    shader.setBool("useColor", false);
}

//barra vita porto
void drawHarborHealthBar(Shader& shader, unsigned int VAO) {
    float hpBarMaxWidth = 0.4f;
    float hpBarHeight = 0.05f;

    float hpRatio = glm::clamp(harborHp / 120.0f, 0.0f, 1.0f);
    float currentHealth = hpBarMaxWidth * hpRatio;

    glm::mat4 trans = glm::mat4(1.0f);

    trans = glm::translate(trans, glm::vec3(-1.30f, /*-0.90f*/-0.10, 0.0f));
    trans = glm::scale(trans, glm::vec3(currentHealth, hpBarHeight, 1.0f));

    shader.use();
    shader.setMat4("transform", trans);

    glm::vec3 color = glm::mix(
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        hpRatio
    );

    shader.setBool("useColor", true);
    shader.setVec3("solidColor", color);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    shader.setBool("useColor", false);
}

//Hp bar del boss piu grossa e lunga di quella del player
void drawHpBarBoss(const Boss& b, Shader& shader, unsigned int VAO) {
    float hpBarMaxWidth = 0.4f;
    float hpBarHeight = 0.03f;

    float hpRatio = glm::clamp(b.hp / 300.0f, 0.0f, 1.0f);
    float currentHealth = hpBarMaxWidth * hpRatio;

    glm::mat4 trans = glm::mat4(1.0f);
    trans = glm::translate(trans, glm::vec3(b.posE.x - hpBarMaxWidth * 0.08f, b.posE.y + 0.125f, 0.0f));        // sopra la nave
    trans = glm::scale(trans, glm::vec3(currentHealth, hpBarHeight, 1.0f));

    shader.use();
    shader.setMat4("transform", trans);
    glm::vec3 color = glm::mix(
        glm::vec3(1.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        hpRatio
    );

    shader.setBool("useColor", true);
    shader.setVec3("solidColor", color);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    shader.setBool("useColor", false);
}

void enemyShoot(naveNemica& n, const navePlayer& player) {
    proiettile p;
    p.posP = n.posE;

    glm::vec2 dir = player.posE - n.posE;
    dir = glm::normalize(dir);

    float bulletSpeed = 0.7f;
    p.velocity = dir * bulletSpeed;

    p.alive = true;
    p.radius = 0.02f;
    p.dmg = 15;

    proiettiliEnemies.push_back(p);
}

//da barra a un valore su scala 0 10
int mapTo0_10(float value, float minVal, float maxVal) {
    float normalized = (value - minVal) / (maxVal - minVal);
    return static_cast<int>(round(normalized * 10.0f));
}

//Controllo se il muose si trova sul bottone
bool isMouseInsideButton(GLFWwindow* window, const Bottone& b)
{
    double mx, my;
    glfwGetCursorPos(window, &mx, &my);

    int windowWidth, windowHeight;
    glfwGetWindowSize(window, &windowWidth, &windowHeight);

    //rapporto tra risoluzione della finestra attualmente e quella del gioco
    float scaleX = (float)SCR_WIDTH / windowWidth;
    float scaleY = (float)SCR_HEIGHT / windowHeight;

    mx *= scaleX;
    my *= scaleY;

    my = SCR_HEIGHT - my;

    return mx >= b.x && mx <= b.x + b.w && my >= b.y && my <= b.y + b.h;
}

void drawMenuButton(Shader& shader, unsigned int texture, float x, float y, float w, float h, unsigned int VAO)
{

    shader.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture);

    //conversione pixel in NDC
    float ndcX = (x + w * 0.5f) / SCR_WIDTH * 2.0f - 1.0f;
    float ndcY = (y + h * 0.5f) / SCR_HEIGHT * 2.0f - 1.0f;

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(ndcX, ndcY, 0.0f));
    model = glm::scale(model, glm::vec3(w / SCR_WIDTH, h / SCR_HEIGHT, 1.0f));

    shader.setMat4("transform", model);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

//Reset dopo GameOver
void resetGame(navePlayer& player)
{
    pos = glm::vec2(0.0f, 0.0f);
    player.hp = 100;
    player.invulnerabilityTime = 0.0f;

    //reset timer esplosione
    player.explTimer = 0.0f;

    //reset condizioni gioco
    coinCount = 0;
    totalCoinsCollected = 0;
    harborHp = 120.0f;

    scoreEnemies = 0;
    gameOver = false;

    currentLevel = 0;
    currentWave = 0;
    waveActive = false;
    pauseTimer = 0.0f;
    bossActive = false;
    bossJustDefeated = false;
    boss = Boss();
    boss.alive = false;

    navi.clear();
    proiettiliPlayer.clear();
    proiettiliEnemies.clear();
    drops.clear();
    victoryCoins.clear();
}

//Funzione per caricare le texture
unsigned int loadTexture(const std::string& path, bool alpha = true, GLint wrapS = GL_CLAMP_TO_EDGE, GLint wrapT = GL_CLAMP_TO_EDGE, GLint minFilter = GL_LINEAR, GLint magFilter = GL_LINEAR) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrChannels, alpha ? 4 : 3);

    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cout << "Failed to load texture: " << path << std::endl;
    }

    stbi_image_free(data);
    return textureID;
}

irrklang::ISoundEngine* engine = nullptr; //Puntatore a vuoto per usarlo in processInput
irrklang::ISound* menuMusic = nullptr;
irrklang::ISound* gameMusic = nullptr;
irrklang::ISound* gameOverSound = nullptr;

int main()
{
    srand(static_cast<unsigned int>(time(nullptr)));  //serve a far generare ad ogni avvio numeri random diversi

    //Inizializzazione suono
    engine = irrklang::createIrrKlangDevice();
    if (!engine)
        return 0; // errore all'avvio

    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "VideogiocoProgetto", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    //inizializzo variabile lastTime
    lastTime = static_cast<float>(glfwGetTime());

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    //qui inizializzo freeType per il testo
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
    FT_Face face;
    if (FT_New_Face(ft, "PressStart2P-Regular.ttf", 0, &face)) std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
    FT_Set_Pixel_Sizes(face, 0, 48);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (unsigned char c = 0; c < 128; c++)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) continue;
        unsigned int texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, face->glyph->bitmap.width, face->glyph->bitmap.rows, 0, GL_RED, GL_UNSIGNED_BYTE, face->glyph->bitmap.buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        Character character = { texture, glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows), glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top), (unsigned int)face->glyph->advance.x };
        Characters.insert(std::pair<char, Character>(c, character));
    }
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // build and compile our shader zprogram
    // ------------------------------------
    Shader ourShader("shader.vs", "shader.fs");
    Shader textShader("text.vs", "text.fs");
    glm::mat4 projection = glm::ortho(0.0f, (float)SCR_WIDTH, 0.0f, (float)SCR_HEIGHT);
    textShader.use();
    textShader.setMat4("projection", projection);

    glm::mat4 worldProj = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);
    ourShader.use();
    ourShader.setMat4("projection", worldProj);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        // positions          // texture coords
         0.5f,  0.5f, 0.0f,   1.0f, 1.0f, // top right
         0.5f, -0.5f, 0.0f,   1.0f, 0.0f, // bottom right
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, // bottom left
        -0.5f,  0.5f, 0.0f,   0.0f, 1.0f  // top left
    };

    unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3  // second triangle

    };

    glEnable(GL_BLEND);                                                                          // per rendere le immagini con sfondo trasparente
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord attribute
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);


    // load and create a texture 
    // -------------------------
    unsigned int texture1 = loadTexture("resources/images/pixelNavePiBlur.png", true, GL_REPEAT, GL_REPEAT);
    unsigned int texture2 = loadTexture("resources/images/pallaDiCannone.png", true, GL_REPEAT, GL_REPEAT);
    unsigned int textureHeart = loadTexture("resources/images/cuore.png");
    unsigned int textureSkull = loadTexture("resources/images/teschio.png");
    unsigned int textureEnemyShip = loadTexture("resources/images/naviNemiche.png", true, GL_REPEAT, GL_REPEAT);
    unsigned int textureGameOver = loadTexture("resources/images/gameOver.png");
    unsigned int textureVictory = loadTexture("resources/images/Vittoria.png");
    unsigned int textureMenu = loadTexture("resources/images/SfondoDiProva.png");
    unsigned int textureOpzioni = loadTexture("resources/images/SfondoOpzioni1.png");
    unsigned int textureSottoScritte = loadTexture("resources/images/ImmagineSottoScritte.png");
    unsigned int textureLogo = loadTexture("resources/images/LogoGioco.png");
    unsigned int textureGioco = loadTexture("resources/images/SfondoGioco.png", true, GL_REPEAT, GL_REPEAT, GL_NEAREST, GL_NEAREST);
    unsigned int textureCoin = loadTexture("resources/images/moneta.png");

    unsigned int textureExplosion1 = loadTexture("resources/images/esplosione1.png"); //textures esplosione proiettile
    unsigned int textureExplosion2 = loadTexture("resources/images/esplosione2.png");
    unsigned int textureExplosion3 = loadTexture("resources/images/esplosione3.png");
    unsigned int textureExplosion4 = loadTexture("resources/images/esplosione4.png");
    unsigned int textureExplosion5 = loadTexture("resources/images/esplosione5.png");

    //Vettore per accedere alle texture tramite indice nel loop
    std::vector<unsigned int> expTexs;
    expTexs.push_back(textureExplosion1);
    expTexs.push_back(textureExplosion2);
    expTexs.push_back(textureExplosion3);
    expTexs.push_back(textureExplosion4);
    expTexs.push_back(textureExplosion5);

    // tell opengl for each sampler to which texture unit it belongs to (only has to be done once)
    // -------------------------------------------------------------------------------------------
    ourShader.use();
    ourShader.setInt("texture1", 0);
    ourShader.setInt("texture2", 1);

    //NAVE PLAYER
    navePlayer player = generaNaveP(pos.x, pos.y);

    // render loop
    // -----------
    while (!glfwWindowShouldClose(window))
    {
        if (gameState == STATE_PLAYING)
        {
            processInput(window);
        }
        else if (gameState == STATE_MENU)
        {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, true);
        }
        else if (gameState == STATE_GAMEOVER || gameState == STATE_VICTORY)
        {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, true);
        }
        else if (gameState == STATE_STATS)
        {
            if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
                glfwSetWindowShouldClose(window, true);
        }

        //calcolo delta reale
        float currentTime = glfwGetTime();
        delta = currentTime - lastTime;
        lastTime = currentTime;

        //Avvio musica del menù
        if ((gameState == STATE_MENU || gameState == STATE_OPTIONS) && engine)
        {
            if (!menuMusic) {
                menuMusic = engine->play2D("resources/media/MusicaMenu1.mp3", true, false, true);
                menuMusic->setVolume(Volume);
            }
        }

        //Avvio musica di gioco
        if (gameState == STATE_PLAYING && engine)
        {
            if (!gameMusic) {
                gameMusic = engine->play2D("resources/media/MusicaGameplay.mp3", true, false, true);
                if (gameMusic) {
                    gameMusic->setVolume(Volume);
                }
            }
        }

        // render
        // ------
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        unsigned int transformLoc2 = glGetUniformLocation(ourShader.ID, "transform");
        //Gestione del menù
        if (gameState == STATE_MENU)
        {
            //rendering sfondo
            ourShader.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureMenu);
            glm::mat4 bg = glm::mat4(1.0f);
            bg = glm::scale(bg, glm::vec3(aspect * 2.0f, 2.0f, 1.0f));
            glUniformMatrix4fv(transformLoc2, 1, GL_FALSE, glm::value_ptr(bg));
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            //rendering logo
            ourShader.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureLogo);
            glm::mat4 bg1 = glm::mat4(1.0f);
            bg1 = glm::translate(bg1, glm::vec3(0.0f, 0.4f, 0.0f));
            bg1 = glm::scale(bg1, glm::vec3(aspect * 1.4f, 1.4f, 1.4f));
            glUniformMatrix4fv(transformLoc2, 1, GL_FALSE, glm::value_ptr(bg1));
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            drawMenuButton(ourShader, textureSottoScritte, 0, 100, 1200, 500, VAO);
            renderText(textShader, "GIOCA", 530, 330, 0.65f, glm::vec3(1, 1, 0));
            drawMenuButton(ourShader, textureSottoScritte, 0, -25, 1200, 500, VAO);
            renderText(textShader, "OPZIONI", 505, 205, 0.65f, glm::vec3(1, 1, 0));
            drawMenuButton(ourShader, textureSottoScritte, 0, -150, 1200, 500, VAO);
            renderText(textShader, "ESCI", 545, 80, 0.65f, glm::vec3(1, 1, 0));

            //Controllo della pressione dei bottoni
            bool mousePressedNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            if (mousePressedNow && !mousePressedLastFrame)
            {
                if (isMouseInsideButton(window, bottonePlay))
                {
                    if (menuMusic) {
                        menuMusic->stop();
                        menuMusic->drop();
                        menuMusic = nullptr;
                    }
                    pos = glm::vec2(0.0f, 0.0f);
                    scoreEnemies = 0;
                    gameOver = false;
                    gameState = STATE_PLAYING;
                }
                else if (isMouseInsideButton(window, bottoneOption))
                {
                    gameState = STATE_OPTIONS;
                }
                else if (isMouseInsideButton(window, bottoneExit))
                {
                    glfwSetWindowShouldClose(window, true);
                }
            }
            mousePressedLastFrame = mousePressedNow;
        }

        //opzioni
        else if (gameState == STATE_OPTIONS)
        {
            // sfondo
            ourShader.use();
            glBindTexture(GL_TEXTURE_2D, textureOpzioni);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::scale(model, glm::vec3(aspect * 2.0f, 2.0f, 1.0f));
            glUniformMatrix4fv(transformLoc2, 1, GL_FALSE, glm::value_ptr(model));

            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            int volDisplay = mapTo0_10(Volume, 0.0f, 0.5f);
            int spdDisplay = mapTo0_10(playerSpeed, 0.25f, 0.75f);

            // testi
            drawMenuButton(ourShader, textureSottoScritte, 0, 270, 1200, 500, VAO);
            renderText(textShader, "OPZIONI", 515, 505, 0.60f, glm::vec3(1, 1, 0));

            renderText(textShader, "Volume generale", 410, 400, 0.4f, glm::vec3(1, 1, 0));
            renderText(textShader, "-", 710, 390, 0.65f, glm::vec3(1, 1, 0));
            renderText(textShader, "+", 740, 390, 0.65f, glm::vec3(1, 1, 0));
            renderText(textShader, std::to_string(volDisplay), 780, 390, 0.7f, glm::vec3(1, 1, 0));

            renderText(textShader, "Velocita' nave", 410, 330, 0.4f, glm::vec3(1, 1, 0));
            renderText(textShader, "-", 710, 320, 0.60f, glm::vec3(1, 1, 0));
            renderText(textShader, "+", 740, 320, 0.60f, glm::vec3(1, 1, 0));
            renderText(textShader, std::to_string(spdDisplay), 780, 320, 0.7f, glm::vec3(1, 1, 0));

            drawMenuButton(ourShader, textureSottoScritte, 0, -40, 1200, 500, VAO);
            renderText(textShader, "INDIETRO", 500, 195, 0.60f, glm::vec3(1, 1, 0));

            bool mousePressedNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            if (mousePressedNow && !mousePressedLastFrame)
            {
                if (isMouseInsideButton(window, bottoneBack))
                {
                    gameState = STATE_MENU;
                }
                else if (isMouseInsideButton(window, bottoneVolMinus))
                {
                    Volume = std::max(0.0f, Volume - 0.05f);
                    if (menuMusic)
                        menuMusic->setVolume(Volume);
                }
                else if (isMouseInsideButton(window, bottoneVolPlus))
                {
                    Volume = std::min(0.5f, Volume + 0.05f);
                    if (menuMusic)
                        menuMusic->setVolume(Volume);
                }
                else if (isMouseInsideButton(window, bottoneSpdMinus))
                {
                    playerSpeed = std::max(0.25f, playerSpeed - 0.05f);
                }
                else if (isMouseInsideButton(window, bottoneSpdPlus))
                {
                    playerSpeed = std::min(0.75f, playerSpeed + 0.05f);
                }
            }

            mousePressedLastFrame = mousePressedNow;
        }

        //Gioco
        else if (gameState == STATE_PLAYING) {

            //rendering sfondo
            ourShader.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureGioco);
            glm::mat4 gioco = glm::mat4(1.0f);
            gioco = glm::scale(gioco, glm::vec3(aspect * 2.0f, 2.0f, 1.0f));
            glUniformMatrix4fv(transformLoc2, 1, GL_FALSE, glm::value_ptr(gioco));
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            //aggiornamento boss timer
            if (bossJustDefeated) {
                boss.defeatTimer -= delta;

                if (boss.defeatTimer <= 0.0f) {
                    bossJustDefeated = false;
                    currentWave = 0;
                    currentLevel++;

                    if (currentLevel >= levels.size()) {
                        gameState = STATE_VICTORY;
                        //animazione vittoria
                        victoryScale = 0.1f;
                        victoryPosY = -1.0f;
                        victoryAnimFinished = false;
                        continue;
                    }
                }
            }

            //aggiornamento posizione nave player 
            player.posE.x = pos.x;
            player.posE.y = pos.y;

            //aggiornamento inv cooldown nave player
            if (player.invulnerabilityTime > 0.0f)
                player.invulnerabilityTime -= delta;

            //aggiorno timer player e fine loop
            if (player.explTimer > 0.0f) {
                player.explTimer += delta;
                if (player.explTimer > 0.50f) player.explTimer = 0.0f; // Reset
            }

            //in caso di collisione col drop applico effetto
            for (auto& d : drops) {
                if (!d.alive) continue;

                float dist = glm::length(d.posE - player.posE);
                if (dist < (d.radius + player.radius)) {
                    applyDropEffect(d, player);
                }
            }

            if (player.hp <= 0) {
                gameState = STATE_GAMEOVER;
                gameOverScale = 0.1f;
                gameOverAnimFinished = false;
            }

            level& lvl = levels[currentLevel];

            //WAVE LOGIC
            if (!bossActive && !bossJustDefeated)
            {
                // se non c'è una wave attiva e non ci sono nemici
                if (!waveActive && navi.empty())
                {
                    //controllo se ci sono ancora wave
                    if (currentWave < lvl.waves.size())
                    {
                        spawnWave(lvl.waves[currentWave], currentLevel);
                        pauseTimer = lvl.waves[currentWave].pauseAfter;
                        waveActive = true;
                    }
                    else
                    {
                        //finite le wave, spawn boss
                        boss.posE = glm::vec2(aspect * 1.2f, 0.0f);
                        boss.hp = 300;
                        boss.radius = 0.25f;
                        boss.speed = 0.10f;
                        boss.alive = true;
                        boss.phase = 1;
                        boss.phaseHp = boss.hp;
                        boss.defeatTimer = 3.0f;
                        bossActive = true;
                    }
                }

                //se wave finita attendo la prossima
                if (waveActive && navi.empty())
                {
                    pauseTimer -= delta;
                    if (pauseTimer <= 0.0f)
                    {
                        waveActive = false;
                        currentWave++;
                    }
                }
            }

            //bind textures on corresponding texture units
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture1);

            // create transformations
            glm::mat4 trans = glm::mat4(1.0f);
            trans = glm::translate(trans, glm::vec3(player.posE.x, player.posE.y, 0.0f));

            //trans = glm::rotate(trans, glm::radians(90.0f), glm::vec3(0.0, 0.0, 1.0));
            trans = glm::scale(trans, glm::vec3(0.30, 0.30, 0.25));

            //get matrix's uniform location and set matrix
            ourShader.use();
            unsigned int transformLoc = glGetUniformLocation(ourShader.ID, "transform");
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans));

            //variabile per fare flashare il player quando invulnerabile
            bool flashing = player.invulnerabilityTime > 0.0f && fmod(glfwGetTime(), 0.2f) < 0.1f;

            //render player
            if (!flashing) {
                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }

            drawHealthBar(player, ourShader, VAO);
            drawHarborHealthBar(ourShader, VAO);

            ourShader.use();

            //rendering esplosione proiettili sul Player
            if (player.explTimer > 0.0f) {
                int frame = (int)(player.explTimer / 0.10f);
                if (frame >= 0 && frame < 5) {
                    glBindTexture(GL_TEXTURE_2D, expTexs[frame]);
                    glm::mat4 transEx = glm::mat4(1.0f);
                    transEx = glm::translate(transEx, glm::vec3(player.posE.x, player.posE.y, 0.0f));
                    transEx = glm::scale(transEx, glm::vec3(0.25f));
                    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transEx));
                    glBindVertexArray(VAO);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                }
            }


            //RENDER BARCHE NEMICHE (MODIFICATO COMPLETO)
            glActiveTexture(GL_TEXTURE0);

            for (auto& n : navi) {

                // Gestione danno al porto se la nave esce a sinistra
                if (n.posE.x <= minX) {
                    n.alive = false;
                    harborHp -= 20.0f;
                    //gameOver per porto distrutto
                    if (harborHp <= 0.0f) {
                        gameState = STATE_GAMEOVER;
                        gameOverScale = 0.1f;
                        gameOverAnimFinished = false;
                    }
                }

                updateNavePos(n);

                //Update Timer Esplosione
                if (n.explTimer > 0.0f) {
                    n.explTimer += delta;
                    if (n.explTimer > 0.50f) n.explTimer = 0.0f; //loop per eplosione
                }

                if (player.invulnerabilityTime <= 0.0f && checkNaveCollide(player, n))
                    naviCollision(player, n);

                updateNaveStatus(n);

                //morte ritardata per esplosione
                //La nave muore solo se hp sotto 0 e l'esplosione termina
                if (n.hp <= 0 && n.explTimer == 0.0f) {
                    n.alive = false;
                    scoreEnemies++;

                    float r = static_cast<float>(rand()) / RAND_MAX;
                    if (r < 0.25f) spawnDrop(n.posE, DROP_HEART);
                    else if (r < 0.10f) spawnDrop(n.posE, DROP_COIN);

                    continue;
                }

                //sparo
                n.shootCooldown -= delta;
                if (n.shootCooldown <= 0.0f && n.hp > 0) { // Spara solo se viva
                    enemyShoot(n, player);
                    n.shootCooldown = std::max(0.5f, 1.3f - ((currentLevel - 1) * 0.2f));
                }

                //disegno nave se hp > 0
                if (n.hp > 0) {
                    glBindTexture(GL_TEXTURE_2D, textureEnemyShip);
                    glm::mat4 trans1 = glm::mat4(1.0f);
                    trans1 = glm::translate(trans1, glm::vec3(n.posE, 0.0f));
                    trans1 = glm::scale(trans1, glm::vec3(0.35, 0.35, 0.25));
                    glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(trans1));
                    glBindVertexArray(VAO);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                }

                //disegno esplosione
                if (n.explTimer > 0.0f) {
                    int frame = (int)(n.explTimer / 0.10f);
                    if (frame >= 0 && frame < 5) {
                        glBindTexture(GL_TEXTURE_2D, expTexs[frame]);
                        glm::mat4 transEx = glm::mat4(1.0f);
                        transEx = glm::translate(transEx, glm::vec3(n.posE, 0.0f));
                        transEx = glm::scale(transEx, glm::vec3(0.25f));
                        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transEx));
                        glBindVertexArray(VAO);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                    }
                }
            }

            //render and update Boss
            if (bossActive && boss.alive)
            {
                //timer esplosione del boss
                if (boss.explTimer > 0.0f) {
                    boss.explTimer += delta;
                    if (boss.explTimer > 0.50f) boss.explTimer = 0.0f;
                }

                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, textureEnemyShip);

                glm::mat4 t = glm::mat4(1.0f);
                t = glm::translate(t, glm::vec3(boss.posE, 0.0f));
                t = glm::scale(t, glm::vec3(0.45f));
                glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(t));
                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

                drawHpBarBoss(boss, ourShader, VAO);

                //render esplosione boss
                if (boss.explTimer > 0.0f) {
                    int frame = (int)(boss.explTimer / 0.10f);

                    if (frame >= 0 && frame < 5) {
                        glBindTexture(GL_TEXTURE_2D, expTexs[frame]);

                        glm::mat4 transEx = glm::mat4(1.0f);
                        //posizione esplosione per ora in centro immagine
                        transEx = glm::translate(transEx, glm::vec3(boss.posE.x, boss.posE.y - 0.05f, 0.0f));

                        transEx = glm::scale(transEx, glm::vec3(0.45f));

                        glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transEx));
                        glBindVertexArray(VAO);
                        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                    }
                }

                //update
                if (boss.hp < boss.phaseHp * 0.50f && boss.phase == 1)
                    boss.phase = 2;
                updateBoss(boss);

                boss.attackTimer -= delta;
                if (boss.attackTimer <= 0.0f)
                {
                    if (boss.phase == 1) bossShootAim(boss, player);
                    if (boss.phase == 2) bossShootFan(boss);

                    boss.attackTimer = std::max(0.7f, 1.5f - (0.15f * (currentLevel - 1)));
                }

                if (boss.hp <= 0 && boss.alive) {
                    boss.alive = false;
                    bossActive = false;
                    bossJustDefeated = true;
                    //fix: aggiungo vettore per spostare la posizione (non sommo float al vec2)
                    spawnDrop(boss.posE + glm::vec2(0.15f, 0.0f), DROP_HEART);
                    spawnDrop(boss.posE, DROP_COIN);

                }
            }

            //update proiettili Player
            for (auto& p : proiettiliPlayer)
            {
                if (!p.alive) continue;

                p.posP.x += p.velocity.x * delta;
                p.posP.y += p.velocity.y * delta;

                if (bossActive) {
                    if (collides(p, boss)) {
                        boss.hp -= p.dmg;
                        p.alive = false;

                        // Avvio animazione e SUONO esplosione boss
                        if (boss.explTimer <= 0.0f) {
                            boss.explTimer = 0.001f;
                            if (engine) {
                                irrklang::ISound* s = engine->play2D("resources/media/esplosioneAudio.mp3", false, false, true);
                                if (s) s->setVolume(Volume);
                            }
                        }
                    }
                }
                else {
                    for (naveNemica& n : navi) {
                        if (collides(p, n)) {
                            n.hp -= p.dmg;
                            p.alive = false;

                            // Avvio animazione e SUONO esplosione nave nemica
                            if (n.explTimer <= 0.0f) {
                                n.explTimer = 0.001f;
                                if (engine) {
                                    irrklang::ISound* s = engine->play2D("resources/media/esplosioneAudio.mp3", false, false, true);
                                    if (s) s->setVolume(Volume);
                                }
                            }
                            break;
                        }
                    }
                }

                if (p.posP.x > aspect * 1.1f || p.posP.x < aspect * (-1.1f) || p.posP.y > 1.1f || p.posP.y < -1.1f)
                    p.alive = false;
            }

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture2);

            //render proiettili player
            for (auto& p : proiettiliPlayer) {
                if (!p.alive) continue;
                glm::mat4 transP = glm::mat4(1.0f);
                transP = glm::translate(transP, glm::vec3(p.posP.x, p.posP.y, 0.0f));
                transP = glm::scale(transP, glm::vec3(0.05f, 0.05f, 0.05f));
                glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transP));
                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }

            //update proiettili enemies
            for (auto& p : proiettiliEnemies)
            {
                if (!p.alive) continue;

                p.posP += p.velocity * delta;

                if (collides(p, player) && player.invulnerabilityTime <= 0.0f) {
                    player.hp -= p.dmg;
                    p.alive = false;

                    // Timer esplosione player e SUONO colpo ricevuto
                    player.explTimer = 0.001f;
                    if (engine) {
                        irrklang::ISound* s = engine->play2D("resources/media/esplosioneAudio.mp3", false, false, true);
                        if (s) s->setVolume(Volume);
                    }
                    break;
                }

                if (p.posP.x > aspect * 1.1f || p.posP.x < aspect * -1.1f || p.posP.y > 1.1f || p.posP.y < -1.1f)
                    p.alive = false;
            }


            //render proiettili enemies
            for (auto& p : proiettiliEnemies) {
                if (!p.alive) continue;
                glm::mat4 transP = glm::mat4(1.0f);
                transP = glm::translate(transP, glm::vec3(p.posP.x, p.posP.y, 0.0f));
                transP = glm::scale(transP, glm::vec3(0.05f, 0.05f, 0.05f));
                glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(transP));
                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }

            //update drops
            for (auto& d : drops) {
                if (!d.alive) continue;
                d.posE.x -= d.speed * delta;
            }

            //render drops
            for (auto& d : drops) {
                if (!d.alive) continue;

                unsigned int tex =
                    (d.type == DROP_HEART) ? textureHeart : textureCoin;

                glBindTexture(GL_TEXTURE_2D, tex);

                glm::mat4 t = glm::mat4(1.0f);
                t = glm::translate(t, glm::vec3(d.posE, 0.0f));
                t = glm::scale(t, glm::vec3(0.06f));

                glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(t));
                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }

            // rendering HUD vita e nemici sconfitti
            int vite = (int)ceil(player.hp / 10.0f);
            if (vite < 0) vite = 0;

            float topHUD = SCR_HEIGHT - 40.0f;

            //HUD Cuori
            renderText(textShader, std::to_string(vite) + " x", 130.0f, topHUD, 0.3f, glm::vec3(0.0f, 0.0f, 1.0f));
            ourShader.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureHeart);
            glm::mat4 heartModel = glm::mat4(1.0f);
            heartModel = glm::translate(heartModel, glm::vec3(-1.20f, 0.91f, 0.0f));
            heartModel = glm::scale(heartModel, glm::vec3(0.055f, 0.055f, 1.0f));
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(heartModel));
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            //HUD Score (Teschi)
            renderText(textShader, std::to_string(scoreEnemies) + " x", 240.0f, topHUD, 0.3f, glm::vec3(0.0f, 0.0f, 1.0f));
            ourShader.use();
            glBindTexture(GL_TEXTURE_2D, textureSkull);
            glm::mat4 skullModel = glm::mat4(1.0f);
            skullModel = glm::translate(skullModel, glm::vec3(-0.90f, 0.91f, 0.0f));
            skullModel = glm::scale(skullModel, glm::vec3(0.055f, 0.055f, 1.0f));
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(skullModel));
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            //HUD Monete
            renderText(textShader, std::to_string(coinCount) + " x", 350.0f, topHUD, 0.3f, glm::vec3(0.0f, 0.0f, 1.0f));
            ourShader.use();
            glBindTexture(GL_TEXTURE_2D, textureCoin);
            glm::mat4 coinModel = glm::mat4(1.0f);
            // La posizione x è calibrata
            coinModel = glm::translate(coinModel, glm::vec3(-0.60f, 0.91f, 0.0f));
            coinModel = glm::scale(coinModel, glm::vec3(0.055f, 0.055f, 1.0f));
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(coinModel));
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            //contatore livelli e ondate
            std::string statusText = "Livello : " + std::to_string(currentLevel + 1) + "  Ondata : " + std::to_string(currentWave + 1);
            renderText(textShader, statusText, 500.0f, topHUD, 0.3f, glm::vec3(0.0f, 0.0f, 1.0f));

            proiettiliPlayer.erase(std::remove_if(proiettiliPlayer.begin(), proiettiliPlayer.end(), [](const proiettile& p) { return !p.alive; }), proiettiliPlayer.end());
            proiettiliEnemies.erase(std::remove_if(proiettiliEnemies.begin(), proiettiliEnemies.end(), [](const proiettile& p) { return !p.alive; }), proiettiliEnemies.end());
            navi.erase(std::remove_if(navi.begin(), navi.end(), [](const naveNemica& n) { return !n.alive; }), navi.end());
            drops.erase(std::remove_if(drops.begin(), drops.end(), [](const Drop& d) { return !d.alive; }), drops.end());
        }

        //gestione del GameOver
        else if (gameState == STATE_GAMEOVER) {
            if (menuMusic) {
                menuMusic->stop();
                menuMusic->drop();
                menuMusic = nullptr;
            }
            if (gameMusic) {
                gameMusic->stop();
                gameMusic->drop();
                gameMusic = nullptr;
            }

            if (!gameOverSound && engine) {
                gameOverSound = engine->play2D("resources/media/GameOver.mp3", false, false, true);
                if (gameOverSound) {
                    gameOverSound->setVolume(Volume);
                }
            }

            //animazione
            if (!gameOverAnimFinished) {
                gameOverScale += delta * 2.0f;
                if (gameOverScale >= 2.0f) {
                    gameOverScale = 2.0f;
                    gameOverAnimFinished = true;
                }
            }

            //rendering del gameOver
            ourShader.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureGameOver);
            glm::mat4 goModel = glm::mat4(1.0f);

            goModel = glm::scale(goModel, glm::vec3(gameOverScale, gameOverScale, 1.0f));

            unsigned int transformLoc = glGetUniformLocation(ourShader.ID, "transform");
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(goModel));
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            //testo e buttons solo dopo animazione
            if (gameOverAnimFinished) {
                renderText(textShader, "MENU", 250, 120, 0.8f, glm::vec3(1, 0, 0));
                renderText(textShader, "STATISTICHE", 500, 120, 0.6f, glm::vec3(1, 1, 0));
                renderText(textShader, "ESCI", 890, 120, 0.8f, glm::vec3(1, 0, 0));

                bool mousePressedNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
                if (mousePressedNow && !mousePressedLastFrame)
                {
                    if (isMouseInsideButton(window, bottoneGoMenu))
                    {
                        resetGame(player);
                        gameState = STATE_MENU;

                        if (gameOverSound) {
                            gameOverSound->stop();
                            gameOverSound->drop();
                            gameOverSound = nullptr;
                        }
                    }
                    else if (isMouseInsideButton(window, bottoneGoStats))
                    {
                        previousState = STATE_GAMEOVER;
                        gameState = STATE_STATS;
                    }
                    else if (isMouseInsideButton(window, bottoneGoExit))
                    {
                        glfwSetWindowShouldClose(window, true);
                    }
                }
                mousePressedLastFrame = mousePressedNow;
            }
        }

        //gestione vittoria
        else if (gameState == STATE_VICTORY) {
            //stop musica
            if (gameMusic) {
                gameMusic->stop();
                gameMusic->drop();
                gameMusic = nullptr;
            }
            if (!victorySoundPlayed && engine) {
                engine->play2D("resources/media/suonoVittoria.mp3", false);
                victorySoundPlayed = true;
            }

            //creazione animazione
            if (victoryCoins.empty()) {
                for (int i = 0; i < 30; i++) {
                    VictoryCoin vc;
                    //posizione casuale
                    vc.pos = glm::vec2(
                        (static_cast<float>(rand()) / RAND_MAX * 2.0f * aspect) - aspect,
                        (static_cast<float>(rand()) / RAND_MAX * 2.0f) - 1.0f
                    );
                    vc.velocity = glm::vec2(
                        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.8f,
                        (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.8f
                    );
                    victoryCoins.push_back(vc);
                }
            }

            //logica movimento monete
            for (auto& vc : victoryCoins) {
                vc.pos += vc.velocity * delta;

                //rimbalzo x
                if (vc.pos.x < -aspect + 0.1f || vc.pos.x > aspect - 0.1f) {
                    vc.velocity.x = -vc.velocity.x;
                }
                //rimbalzo y
                if (vc.pos.y < -1.0f + 0.1f || vc.pos.y > 1.0f - 0.1f) {
                    vc.velocity.y = -vc.velocity.y;
                }
            }

            //Rendering monete vittoria
            ourShader.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureCoin);

            unsigned int transformLoc = glGetUniformLocation(ourShader.ID, "transform");

            for (auto& vc : victoryCoins) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, glm::vec3(vc.pos, 0.0f));
                model = glm::scale(model, glm::vec3(0.15f));
                glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(model));
                glBindVertexArray(VAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
            }

            if (!victoryAnimFinished) {
                victoryPosY += delta * 0.8f;
                victoryScale += delta * 0.8f;

                if (victoryPosY >= 0.0f) victoryPosY = 0.0f;
                if (victoryScale >= 1.0f) {
                    victoryScale = 1.0f;
                    victoryAnimFinished = true;
                }
            }

            //rendewring png vittoria
            ourShader.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureVictory);
            glm::mat4 vicModel = glm::mat4(1.0f);
            vicModel = glm::translate(vicModel, glm::vec3(0.0f, victoryPosY, 0.0f));
            vicModel = glm::scale(vicModel, glm::vec3(victoryScale * aspect, victoryScale, 1.0f));

            transformLoc = glGetUniformLocation(ourShader.ID, "transform");
            glUniformMatrix4fv(transformLoc, 1, GL_FALSE, glm::value_ptr(vicModel));
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            if (victoryAnimFinished) {
                renderText(textShader, "MENU", 250, 120, 0.8f, glm::vec3(0, 1, 0));
                renderText(textShader, "STATISTICHE", 500, 120, 0.6f, glm::vec3(1, 1, 0));
                renderText(textShader, "ESCI", 890, 120, 0.8f, glm::vec3(0, 1, 0));

                bool mousePressedNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
                if (mousePressedNow && !mousePressedLastFrame)
                {
                    if (isMouseInsideButton(window, bottoneGoMenu))
                    {
                        resetGame(player);
                        gameState = STATE_MENU;
                    }
                    else if (isMouseInsideButton(window, bottoneGoStats))
                    {
                        previousState = STATE_VICTORY;
                        gameState = STATE_STATS;
                    }
                    else if (isMouseInsideButton(window, bottoneGoExit))
                    {
                        glfwSetWindowShouldClose(window, true);
                    }
                }
                mousePressedLastFrame = mousePressedNow;
            }
        }

        //rendering e logica statistiche
        else if (gameState == STATE_STATS) {
            ourShader.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, textureOpzioni);
            glm::mat4 bg = glm::mat4(1.0f);
            bg = glm::scale(bg, glm::vec3(aspect * 2.0f, 2.0f, 1.0f));
            glUniformMatrix4fv(transformLoc2, 1, GL_FALSE, glm::value_ptr(bg));
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            renderText(textShader, "STATISTICHE PARTITA", 350, 600, 0.8f, glm::vec3(1, 1, 0));

            renderText(textShader, "Nemici Sconfitti: " + std::to_string(scoreEnemies), 300, 450, 0.5f, glm::vec3(1, 1, 1));
            renderText(textShader, "Monete Raccolte Totali: " + std::to_string(totalCoinsCollected), 300, 380, 0.5f, glm::vec3(1, 1, 1));
            renderText(textShader, "Livello Raggiunto: " + std::to_string(currentLevel + 1), 300, 310, 0.5f, glm::vec3(1, 1, 1));
            renderText(textShader, "Ondata Raggiunta: " + std::to_string(currentWave + 1), 300, 240, 0.5f, glm::vec3(1, 1, 1));

            drawMenuButton(ourShader, textureSottoScritte, 0, -185, 1300, 500, VAO);
            renderText(textShader, "INDIETRO", 540, 50, 0.6f, glm::vec3(1, 1, 0));

            bool mousePressedNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
            if (mousePressedNow && !mousePressedLastFrame)
            {
                if (isMouseInsideButton(window, bottoneStatsBack))
                {
                    gameState = previousState;
                }
            }
            mousePressedLastFrame = mousePressedNow;
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    //Terminazione dei suoni
    if (menuMusic) {
        menuMusic->stop();
        menuMusic->drop();
    }

    if (gameOverSound) {
        gameOverSound->stop();
        gameOverSound->drop();
    }

    if (engine)
        engine->drop();

    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        pos.y += playerSpeed * delta;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        pos.x += playerSpeed * delta;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        pos.x -= playerSpeed * delta;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        pos.y -= playerSpeed * delta;

    if (pos.x < minX) pos.x = minX;
    if (pos.x > maxX) pos.x = maxX;
    if (pos.y < minY) pos.y = minY;
    if (pos.y > maxY) pos.y = maxY;

    bool mousePressedNow = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    if (mousePressedNow && !mousePressedLastFrame)
    {
        proiettile p;
        p.posP.x = pos.x + 0.5 * halfSize;
        p.posP.y = pos.y - 0.3 * halfSize;
        p.velocity = glm::vec2(1.0f, 0.0f);
        p.alive = true;
        p.radius = 0.01f;
        p.dmg = 19;
        proiettiliPlayer.push_back(p);
        //genero un suono ogni volta che parte un proiettile
        if (engine) {
            irrklang::ISound* s = engine->play2D("resources/media/cannone2.wav", false, false, true);
            if (s) s->setVolume(Volume);
        }
    }

    mousePressedLastFrame = mousePressedNow;

    //Gestione attacco speciale con barra spaziatrice
    bool spacePressedNow = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (spacePressedNow && !spacePressedLastFrame) {
        if (coinCount > 0) {
            for (int i = -2; i <= 2; i++) {
                proiettile p;
                p.posP.x = pos.x + 0.5 * halfSize;
                p.posP.y = pos.y - 0.3 * halfSize;

                float angle = i * 0.15f;
                p.velocity = glm::vec2(cos(angle), sin(angle)) * 1.0f;

                p.alive = true;
                p.radius = 0.01f;
                p.dmg = 25;
                proiettiliPlayer.push_back(p);
            }

            coinCount--;

            if (engine) {
                irrklang::ISound* s = engine->play2D("cannone2.wav", false, false, true);
                if (s) s->setVolume(Volume);
            }
        }
    }
    spacePressedLastFrame = spacePressedNow;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

void renderText(Shader& s, std::string text, float x, float y, float scale, glm::vec3 color)
{
    s.use();
    s.setVec3("textColor", color);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);
    for (std::string::const_iterator c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];
        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;
        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f }, { xpos,     ypos,       0.0f, 1.0f }, { xpos + w, ypos,       1.0f, 1.0f },
            { xpos,     ypos + h,   0.0f, 0.0f }, { xpos + w, ypos,       1.0f, 1.0f }, { xpos + w, ypos + h,   1.0f, 0.0f }
        };
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += (ch.Advance >> 6) * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
