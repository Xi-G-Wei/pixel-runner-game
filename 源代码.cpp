#include<stdio.h>
#include<easyx.h>
#include<time.h>
#include"tool.h"
#include<math.h>
#include <mmsystem.h>                // 用于多媒体播放
#include<Windows.h>   
#pragma comment(lib,"winmm.lib")   // 链接Windows多媒体库


ExMessage msg = { 0 };

//  添加的全局图片资源
IMAGE img_gamebk;
IMAGE img_run_left, img_run_right, img_jump_left, img_jump_right;
IMAGE img_stand_left, img_stand_right;
IMAGE img_attack_left, img_attack_right;
IMAGE img_start, img_starts[3], img_enemy[2];
IMAGE img_enemydeath_1, img_enemydeath_2;
IMAGE img_name;
int imagesLoaded = 0;


enum GameState {
    MAIN_MENU,
    GAME_PLAYING,
    GAME_OVER
};
//角色
struct Character {
    float x, y;        // 位置
    int width, height; // 尺寸
    float velocityX, velocityY; // 速度
    float cspeed;      // 移动速度
    int direction;     // 移动方向
    int isJumping;    // 是否跳跃
    int isOnGround;   // 是否在地面
    int isAlive;      // 是否存活
    float score;         // 得分
    float runTime;     // 奔跑时间（用于速度递增）
    int jumpCount;     // 跳跃次数（用于二段跳）
    int maxJumps;      // 最大跳跃次数
    int isAttacking;   // 是否正在攻击
    int attackTimer;   // 攻击计时器
};
struct Character player = { 100, 400, 0, 0, 0, 0, 10, 1, 0, 0, 1, 0, 0, 0, 2 };

// 敌人状态枚举
enum EnemyState {
    ENEMY_MOVING,    // 移动状态
    ENEMY_DYING,     // 死亡状态
};

#define ENEMY_TYPES 2           // 敌人类型数量
#define MAX_ENEMIES 40         // 最大敌人数
#define MAX_ENEMY_FRAMES 20      // 每种状态的最大动画帧数

int enemyDeathAnimTimer[MAX_ENEMIES] = { 0 }; // 死亡动画计时器
int enemyDeathAnimIndex[MAX_ENEMIES] = { 0 }; // 死亡动画帧索引
int enemyIsDying[MAX_ENEMIES] = { 0 };        // 是否正在死亡

// 贴图数组 [类型][帧]
IMAGE img_enemies[ENEMY_TYPES][MAX_ENEMY_FRAMES];

// 每种敌人的动画帧数
int enemyFrameCount[ENEMY_TYPES] = { 13, 5 };

// 每种敌人的动画速度
int enemyAnimSpeed[ENEMY_TYPES] = { 1 , 2 };

// 敌人结构体
struct Enemy {
    float x, y;
    float width, height;
    float speed;
    int isActive;
    int type;
    int animIndex;
    int animTimer;
};

Enemy enemies[MAX_ENEMIES];
int enemyCount = 0;

//剑气
struct SwordBeam {
    float x, y;
    float width, height;
    float speed;
    int direction; // 1=向右, -1=向左
    int isActive;
    int lifetime;  // 剑气存在时间
    int currentFrame; // 动画帧
};
#define MAX_SWORD_BEAMS 10
SwordBeam swordBeams[MAX_SWORD_BEAMS];
int swordBeamCount = 0;

IMAGE img_swordbeam_left, img_swordbeam_right;

bool keys[256] = { false };

// 平台结构
typedef struct {
    float x, y;
    float width;
    int hasObstacle;
    float obstacleX;
    int isActive;
} Platform;

#define MAX_PLATFORMS 60
Platform platforms[MAX_PLATFORMS];
int platformCount = 0;
float platformSpeed = 6.0f;
float platformSpawnTimer = 0.0f;
float platformSpawnInterval = 0.5f;

// 中央白线的Y坐标
const int CENTRAL_LINE_Y = 400;

// 检查鼠标位置
int inArea(int mx, int my, int x, int y, int w, int h) {
    if (mx > x && mx < x + w && my > y && my < y + h) {
        return 1;
    }
    return 0;
}

// 工具函数
int mini(int a, int b) { return a < b ? a : b; }
int maxi(int a, int b) { return a > b ? a : b; }
float abs(float x) { return x < 0 ? -x : x; }

// 生成随机平台
void spawnPlatform() {
    // 更严格的平台数量检查
    if (platformCount >= MAX_PLATFORMS) {
        // 如果平台数量已达上限，移除最旧的平台
        if (platformCount > 0) {
            for (int i = 0; i < platformCount - 1; i++) {
                platforms[i] = platforms[i + 1];
            }
            platformCount--;
        }
        else {
            return; // 没有平台可移除，直接返回
        }
    }

    Platform newPlatform;
    newPlatform.width = rand() % 250 + 100;
    newPlatform.x = 1400;

    int heightVariation;
    do {
        heightVariation = rand() % 10;
    } while (heightVariation == 4);

    switch (heightVariation) {
    case 0: newPlatform.y = CENTRAL_LINE_Y - 200; break;
    case 1: newPlatform.y = CENTRAL_LINE_Y - 150; break;
    case 2: newPlatform.y = CENTRAL_LINE_Y - 100; break;
    case 3: newPlatform.y = CENTRAL_LINE_Y - 50; break;
    case 5: newPlatform.y = CENTRAL_LINE_Y + 50; break;
    case 6: newPlatform.y = CENTRAL_LINE_Y + 100; break;
    case 7: newPlatform.y = CENTRAL_LINE_Y + 150; break;
    case 8: newPlatform.y = CENTRAL_LINE_Y + 200; break;
    default: newPlatform.y = CENTRAL_LINE_Y - 50; break;
    }

    newPlatform.hasObstacle = (rand() % 5 == 0);
    newPlatform.obstacleX = newPlatform.x + newPlatform.width / 2 - 15;
    newPlatform.isActive = 1;

    platforms[platformCount] = newPlatform;
    platformCount++;
}

// 移除平台
void removeInactivePlatforms() {
    int i, j;
    for (i = 0; i < platformCount; i++) {
        if (!platforms[i].isActive) {
            // 将后面的平台前移
            for (j = i; j < platformCount - 1; j++) {
                platforms[j] = platforms[j + 1];
            }
            platformCount--;
            i--;
        }
    }
}

// 初始化游戏物理
void initGamePhysics() {
    player.x = 100;
    player.y = CENTRAL_LINE_Y - player.height;
    player.velocityX = 0;
    player.velocityY = 0;
    player.isJumping = 0;
    player.isOnGround = 0;
    player.isAlive = 1;
    player.direction = 1;
    player.score = 0;
    player.runTime = 0;
    player.cspeed = 5;
    player.jumpCount = 0;
    player.maxJumps = 2;
    player.isAttacking = 0;
    player.attackTimer = 0;

    platformCount = 0;
    platformSpawnTimer = 0.0f;

    //clock()增加随机性
    srand((unsigned int)time(NULL) + clock());

    for (int i = 0; i < 5; i++) {
        spawnPlatform();
        if (i == 0) {
            platforms[i].x = 300;
            platforms[i].y = CENTRAL_LINE_Y - 50;
        }
        else if (i == 1) {
            platforms[i].x = 600;
            platforms[i].y = CENTRAL_LINE_Y - 100;
        }
        else if (i == 2) {
            platforms[i].x = 900;
            platforms[i].y = CENTRAL_LINE_Y + 50;
        }
    }
    // 剑气数组初始化
    for (int i = 0; i < MAX_SWORD_BEAMS; i++) {
        swordBeams[i].isActive = 0;
    }
    swordBeamCount = 0;

    //重新播放
    mciSendString("close assets/end.wav", NULL, 0, NULL);
}

// 释放剑气函数
void shootSwordBeam() {
    // 优先检查剑气数量限制
    if (swordBeamCount >= MAX_SWORD_BEAMS) {
        return;  // 超过最大数量，不攻击
    }

    // 设置攻击状态
    player.isAttacking = 1;
    player.attackTimer = 0;

    for (int i = 0; i < MAX_SWORD_BEAMS; i++) {
        if (!swordBeams[i].isActive) {
            if (player.direction == 1) {
                swordBeams[i].x = player.x + 60;
                swordBeams[i].y = player.y;
            }
            else {
                swordBeams[i].x = player.x + 10;
                swordBeams[i].y = player.y;
            }

            swordBeams[i].width = 60;
            swordBeams[i].height = 80;
            swordBeams[i].speed = 20.0f;
            swordBeams[i].direction = player.direction;
            swordBeams[i].isActive = 1;
            swordBeams[i].lifetime = 100;
            swordBeams[i].currentFrame = 0;

            swordBeamCount++;
            break;
        }
    }
}

// 更新攻击状态
void updateAttack(int* attack_rightindex, int* attack_leftindex) {
    if (player.isAttacking) {
        player.attackTimer++;

        // 攻击动画持续15帧
        if (player.attackTimer >= 15) {
            player.isAttacking = 0;
            player.attackTimer = 0;
        }
        // 每3帧更新一次动画帧
        else if (player.attackTimer % 3 == 0) {
            // 根据方向更新对应的动画索引
            if (player.direction == 1) { // 朝右
                *attack_rightindex = (*attack_rightindex + 1) % 4;
            }
            else { // 朝左
                (*attack_leftindex)--;
                if (*attack_leftindex < 0) {
                    *attack_leftindex = 3;
                }
            }
        }
    }
}

// 更新剑气位置
void updateSwordBeams() {
    for (int i = 0; i < MAX_SWORD_BEAMS; i++) {
        if (swordBeams[i].isActive) {
            swordBeams[i].x += swordBeams[i].speed * swordBeams[i].direction;
            swordBeams[i].currentFrame++;
            swordBeams[i].lifetime--;

            if (swordBeams[i].lifetime <= 0 ||
                swordBeams[i].x < -200 ||
                swordBeams[i].x > 1600) {
                swordBeams[i].isActive = 0;
                swordBeamCount--;
            }
        }
    }
}

// 剑气与敌人碰撞检测
void checkBeamEnemyCollisions() {
    for (int i = 0; i < MAX_SWORD_BEAMS; i++) {
        if (!swordBeams[i].isActive) continue;

        for (int j = 0; j < MAX_ENEMIES; j++) {
            if (!enemies[j].isActive) continue;

            float beamLeft = swordBeams[i].x + 10;
            float beamTop = swordBeams[i].y + 5;
            float beamRight = swordBeams[i].x + swordBeams[i].width - 10;
            float beamBottom = swordBeams[i].y + swordBeams[i].height - 5;

            float enemyLeft = enemies[j].x + 20;
            float enemyTop = enemies[j].y + 20;
            float enemyRight = enemies[j].x + enemies[j].width - 20;
            float enemyBottom = enemies[j].y + enemies[j].height - 20;

            if (beamRight > enemyLeft &&
                beamLeft < enemyRight &&
                beamBottom > enemyTop &&
                beamTop < enemyBottom) {

                // 触发死亡动画
                enemyIsDying[j] = 1;
                enemyDeathAnimIndex[j] = 0;
                enemyDeathAnimTimer[j] = 0;

                player.score += 5;
                break;
            }
        }
    }
}

// 剑气与障碍物碰撞检测
void checkBeamObstacleCollisions() {
    for (int i = 0; i < MAX_SWORD_BEAMS; i++) {
        if (!swordBeams[i].isActive) continue;

        for (int j = 0; j < platformCount; j++) {
            if (!platforms[j].isActive || !platforms[j].hasObstacle) continue;

            float beamLeft = swordBeams[i].x + 10;
            float beamTop = swordBeams[i].y + 5;
            float beamRight = swordBeams[i].x + swordBeams[i].width - 10;
            float beamBottom = swordBeams[i].y + swordBeams[i].height - 5;

            float obstacleLeft = platforms[j].obstacleX + 10;
            float obstacleTop = platforms[j].y - 30 + 10;
            float obstacleRight = platforms[j].obstacleX + 30 - 10;
            float obstacleBottom = platforms[j].y - 10;

            if (beamRight > obstacleLeft &&
                beamLeft < obstacleRight &&
                beamBottom > obstacleTop &&
                beamTop < obstacleBottom) {

                swordBeams[i].isActive = 0;
                swordBeamCount--;
                platforms[j].hasObstacle = 0;
                player.score += 2;
                break;
            }
        }
    }
}

// 检测矩形碰撞
int checkRectCollision(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2) {
    return (x1 < x2 + w2 && x1 + w1 > x2 && y1 < y2 + h2 && y1 + h1 > y2);
}

// 地面碰撞检测
int checkGroundCollision() {
    float bottomY = player.y + player.height;
    float leftX = player.x;
    float rightX = player.x + player.width;

    // 中央白线碰撞检测
    if (player.velocityY >= 0) {
        // 只有下落时才检测地面
        // 检查中央白线
        if (bottomY >= CENTRAL_LINE_Y - 8 && bottomY <= CENTRAL_LINE_Y + 12) {
            // 检查是否在屏幕范围内
            if (leftX < 1400 && rightX > 0) {
                // 计算与白线的垂直距离
                float distanceToGround = bottomY - CENTRAL_LINE_Y;
                if (distanceToGround >= -8 && distanceToGround <= 12) {
                    return 1;
                }
            }
        }

        // 检查平台
        for (int i = 0; i < platformCount; i++) {
            if (!platforms[i].isActive) continue;

            // 水平重叠检测
            if (rightX - 40 > platforms[i].x && leftX + 40 < platforms[i].x + platforms[i].width) {
                // 垂直距离检测
                float distanceToPlatform = bottomY - platforms[i].y;
                if (distanceToPlatform >= -8 && distanceToPlatform <= 12) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

// 获取角色脚下的地面高度
float getGroundHeight() {
    float minGroundY = 2000.0f;
    float bottomY = player.y + player.height;
    float leftX = player.x;
    float rightX = player.x + player.width;

    // 检查中央白线
    if (bottomY >= CENTRAL_LINE_Y - 15 && bottomY <= CENTRAL_LINE_Y + 25) {
        if (leftX < 1400 && rightX > 0) {
            minGroundY = CENTRAL_LINE_Y;
        }
    }

    // 检查平台
    for (int i = 0; i < platformCount; i++) {
        if (!platforms[i].isActive) continue;

        if (rightX > platforms[i].x && leftX < platforms[i].x + platforms[i].width) {
            float distanceToPlatform = bottomY - platforms[i].y;
            if (distanceToPlatform >= -15 && distanceToPlatform <= 25) {
                if (platforms[i].y < minGroundY) {
                    minGroundY = platforms[i].y;
                }
            }
        }
    }

    return minGroundY;
}

// 检测障碍物碰撞
int checkObstacleCollision() {
    int i;
    for (i = 0; i < platformCount; i++) {
        if (!platforms[i].isActive || !platforms[i].hasObstacle) continue;

        float collisionWidth = player.width - 100;
        float collisionHeight = player.height - 10;

        float collisionX, collisionY;

        // 使用调试好的朝向偏移参数
        if (player.direction == 1) { // 朝右
            collisionX = player.x + 45;  // 使用调试好的朝右偏移
            collisionY = player.y + 5;
        }
        else { // 朝左
            collisionX = player.x + 52;  // 使用调试好的朝左偏移
            collisionY = player.y + 5;
        }

        // 计算碰撞盒边界
        float playerLeft = collisionX;
        float playerTop = collisionY;
        float playerRight = collisionX + collisionWidth;
        float playerBottom = collisionY + collisionHeight;

        // 障碍物碰撞盒（使用调试好的参数）
        float obstacleLeft = platforms[i].obstacleX + 10;
        float obstacleTop = platforms[i].y - 30 + 10;
        float obstacleRight = platforms[i].obstacleX + 30 - 10;
        float obstacleBottom = platforms[i].y - 10;


        // 精确碰撞检测
        if (playerRight > obstacleLeft &&
            playerLeft < obstacleRight &&
            playerBottom > obstacleTop &&
            playerTop < obstacleBottom) {
            
            //死亡音效
            mciSendString("stop assets/end.wav", NULL, 0, NULL);  // 先停止
            mciSendString("close assets/end.wav", NULL, 0, NULL); // 再关闭
            mciSendString("open assets/end.wav", NULL, 0, NULL); // 重新打开
            mciSendString("play assets/end.wav", NULL, 0, NULL);  // 播放
            
            //关闭背景音乐
            mciSendString("close assets/game_bgm.mp3", NULL, 0, NULL);

            return 1;
        }
    }
    return 0;
}

// 更新平台位置
void updatePlatforms() {
    int i;
    for (i = 0; i < platformCount; i++) {
        if (platforms[i].isActive) {
            //随时间加成
            //platforms[i].x -= platformSpeed + player.runTime * 0.5f;
            //随分数加成
            float currentSpeed = platformSpeed + (player.score * 0.3f);
            if (currentSpeed > 8.0f) currentSpeed = 8.0f;   // 设置最大速度
            platforms[i].x -= currentSpeed;

            platforms[i].obstacleX = platforms[i].x + platforms[i].width / 2 - 15;

            if (platforms[i].x + platforms[i].width < 0) {
                platforms[i].isActive = 0;
            }
        }
    }

    removeInactivePlatforms();

    platformSpawnTimer += 0.05f;
    if (platformSpawnTimer >= platformSpawnInterval) {
        spawnPlatform();
        platformSpawnTimer = 0.0f;
    }
}

//初始化敌方单位
void initEnemies() {
    enemyCount = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        enemies[i].isActive = 0;
    }
}

//生成敌人
void spawnEnemy() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].isActive) {
            enemies[i].type = rand() % ENEMY_TYPES;

            enemies[i].speed = 6.0f + (rand() % 100) / 10.0f;

            enemies[i].animIndex = 0;
            enemies[i].animTimer = 0;

            int totalFrames = enemyFrameCount[enemies[i].type];
            int spriteWidth = img_enemies[enemies[i].type][0].getwidth();
            int spriteHeight = img_enemies[enemies[i].type][0].getheight();

            enemies[i].width = spriteWidth / totalFrames;
            enemies[i].height = spriteHeight;

            // 在窗口最右侧生成
            enemies[i].x = 1400;

            // 完全随机高度生成
            // 最小高度：距离顶部50像素，避免贴边
            // 最大高度：距离底部100像素，确保敌人完全在屏幕内
            int minY = 50;
            int maxY = 800 - enemies[i].height - 100;

            // 确保最大高度不小于最小高度
            if (maxY < minY) {
                maxY = minY;
            }

            enemies[i].y = minY + rand() % (maxY - minY + 1);

            /*调试信息
            printf("生成敌人 %d: 类型=%d, 位置=(%.1f, %.1f), 尺寸=%dx%d\n",
               i, enemies[i].type, enemies[i].x, enemies[i].y,
                enemies[i].width, enemies[i].height);*/

            enemies[i].isActive = 1;
            enemyCount++;
            break;
        }
    }
}

//更新敌人
void updateEnemies() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].isActive) {
            if (enemyIsDying[i]) {
                // 更新死亡动画
                enemyDeathAnimTimer[i]++;
                if (enemyDeathAnimTimer[i] >= 3) {  // 每3帧更新一次
                    enemyDeathAnimIndex[i]++;
                    enemyDeathAnimTimer[i] = 0;

                    // 动画播放完毕
                    if (enemyDeathAnimIndex[i] >= 5) {
                        enemies[i].isActive = 0;
                        enemyIsDying[i] = 0;
                        enemyCount--;
                    }
                }
            }
            else {
                // 向左移动
                enemies[i].x -= enemies[i].speed;

                // 更新动画
                enemies[i].animTimer++;
                int animSpeed = enemyAnimSpeed[enemies[i].type];
                if (enemies[i].animTimer >= animSpeed) {
                    int totalFrames = enemyFrameCount[enemies[i].type];
                    enemies[i].animIndex = (enemies[i].animIndex + 1) % totalFrames;
                    enemies[i].animTimer = 0;
                }

                // 移出屏幕检测
                if (enemies[i].x + enemies[i].width < 0) {
                    enemies[i].isActive = 0;
                    enemyCount--;
                }

            }
        }
    }

    // 生成新敌人
    static int spawnTimer = 0;
    spawnTimer++;
    if (spawnTimer >= 120 && enemyCount < MAX_ENEMIES && rand() % 100 < 3) {
        spawnEnemy();
        spawnTimer = 0;
    }
}

//敌人碰撞检测
void checkEnemyCollisions() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].isActive && player.isAlive) {
            // 敌人碰撞盒
            float enemyLeft = enemies[i].x + 30;
            float enemyTop = enemies[i].y + 10;
            float enemyRight = enemies[i].x + enemies[i].width - 30;
            float enemyBottom = enemies[i].y + enemies[i].height - 10;

            // 玩家碰撞盒
            float collisionWidth = player.width - 100;
            float collisionHeight = player.height - 10;
            float collisionX, collisionY;

            if (player.direction == 1) { // 朝右
                collisionX = player.x + 45;
                collisionY = player.y + 5;
            }
            else { // 朝左
                collisionX = player.x + 52;
                collisionY = player.y + 5;
            }

            float playerLeft = collisionX;
            float playerTop = collisionY;
            float playerRight = collisionX + collisionWidth;
            float playerBottom = collisionY + collisionHeight;

            // 精确碰撞检测
            if (playerRight > enemyLeft &&
                playerLeft < enemyRight &&
                playerBottom > enemyTop &&
                playerTop < enemyBottom) {
                
                //死亡音效
                mciSendString("stop assets/end.wav", NULL, 0, NULL);  // 先停止
                mciSendString("close assets/end.wav", NULL, 0, NULL); // 再关闭
                mciSendString("open assets/end.wav", NULL, 0, NULL); // 重新打开
                mciSendString("play assets/end.wav", NULL, 0, NULL);  // 播放

                //暂停背景音乐
                mciSendString("close assets/game_bgm.mp3", NULL, 0, NULL);

                player.isAlive = 0;
                enemies[i].isActive = 0;
                enemyCount--;
                break;
            }
        }
    }
}

//边界检查
void checkCollisions() {
    player.isOnGround = 0;

    if (checkGroundCollision()) {
        player.isOnGround = 1;
        player.isJumping = 0;
        player.velocityY = 0;
        player.jumpCount = 0;

        float groundHeight = getGroundHeight();
        if (groundHeight != 2000.0f) {
            player.y = groundHeight - player.height;
        }
    }

    if (checkObstacleCollision()) {
        player.isAlive = 0;
    }

    // 屏幕边界检测 
    if (player.x < 0) {
        player.x = 0;
        player.velocityX = 0;
    }

    if (player.x > 1400 - player.width) {
        player.x = 1400 - player.width;
        player.velocityX = 0;
    }

    if (player.y <= 0) {
        player.y = 0;
        if (player.velocityY < 0) {
            player.velocityY = 0;
        }
    }//角色透明边框被算进去，所以看着像未到达顶部

    // 检测是否掉落失败
    if (player.y + player.height > 800) {
        
        //死亡音效
        mciSendString("stop assets/end.wav", NULL, 0, NULL);  // 先停止
        mciSendString("close assets/end.wav", NULL, 0, NULL); // 再关闭
        mciSendString("open assets/end.wav", NULL, 0, NULL); // 重新打开
        mciSendString("play assets/end.wav", NULL, 0, NULL);  // 播放

        //暂停背景音乐
        mciSendString("close assets/game_bgm.mp3", NULL, 0, NULL);

        player.isAlive = 0;
    }
}

// 处理输入
void handleInput() {
    if (keys['A'] || keys[VK_LEFT]) {
        player.velocityX = -player.cspeed;
        player.direction = -1;
    }
    else if (keys['D'] || keys[VK_RIGHT]) {
        player.velocityX = player.cspeed;
        player.direction = 1;
    }
    else {
        player.velocityX *= 0.8;
        if (fabs(player.velocityX) < 0.5) player.velocityX = 0;
    }
}

// 更新得分
void updatePhysics() {
    if (!player.isOnGround && player.isAlive) {
        player.velocityY += 0.8;
    }

    if (player.velocityY > 20) {
        player.velocityY = 20;
    }

    player.x += player.velocityX;
    player.y += player.velocityY;

    if (player.isAlive) {
        player.runTime += 0.016f;
        player.cspeed = 10 + player.runTime * 0.2f;
        if (player.cspeed > 18) player.cspeed = 18;

        player.score += player.cspeed * 0.001f;
    }
}

// 显示游戏结束画面
void showGameOver() {
    setbkmode(TRANSPARENT);
    settextcolor(RED);
    settextstyle(48, 0, "宋体");
    outtextxy(500, 300, "游戏结束");

    settextcolor(WHITE);
    settextstyle(24, 0, "宋体");
    outtextxy(520, 380, "最终得分:");
    char scoreText[50];
    sprintf(scoreText, "%d", int(player.score));
    outtextxy(650, 380, scoreText);

    outtextxy(520, 420, "按R键重新开始");
    outtextxy(520, 460, "按ESC返回菜单");
}

// 添加的资源管理函数
// 加载游戏图片资源
int loadGameImages() {
    if (imagesLoaded) return 1; // 避免重复加载

    // 直接尝试加载图片，如果失败就返回错误
    if (loadimage(&img_gamebk, "assets/bk2.png", 1400, 800) != 0) {
        printf("错误: 加载背景图片 assets/bk2.png 失败\n");
        return 0;
    }

    if (loadimage(&img_run_right, "assets/Run_right.png") != 0) {
        printf("错误: 加载角色图片 assets/Run_right.png 失败\n");
        return 0;
    }

    if (loadimage(&img_run_left, "assets/Run_left.png") != 0) {
        printf("错误: 加载角色图片 assets/Run_left.png 失败\n");
        return 0;
    }
    if (loadimage(&img_stand_right, "assets/stand_right.png") != 0) {
        printf("错误: 加载角色图片 assets/stand_right.png 失败\n");
        return 0;
    }
    if (loadimage(&img_stand_left, "assets/stand_left.png") != 0) {
        printf("错误: 加载角色图片 assets/stand_left.png 失败\n");
        return 0;
    }
    if (loadimage(&img_jump_left, "assets/jump_left.png") != 0) {
        printf("错误: 加载角色图片 assets/jump_left.png 失败\n");
        return 0;
    }
    if (loadimage(&img_jump_right, "assets/jump_right.png") != 0) {
        printf("错误: 加载角色图片 assets/jump_right.png 失败\n");
        return 0;
    }
    if (loadimage(&img_swordbeam_left, "assets/swordbeam_left.png", 60, 80) != 0) {
        printf("错误: 加载角色图片 assets/jswordbeam_left.png 失败\n");
        return 0;
    }
    if (loadimage(&img_swordbeam_right, "assets/swordbeam_right.png", 60, 80) != 0) {
        printf("错误: 加载角色图片 assets/swordbeam_right.png 失败\n");
        return 0;
    }
    if (loadimage(&img_attack_right, "assets/attack_right.png") != 0) {
        printf("错误: 加载角色图片 assets/attack_right.png 失败\n");
        return 0;
    }
    if (loadimage(&img_attack_left, "assets/attack_left.png") != 0) {
        printf("错误: 加载角色图片 assets/attack_left.png 失败\n");
        return 0;
    }
    if (loadimage(&img_enemydeath_1, "assets/enemy_die1.png") != 0) {
        printf("错误: 加载角色图片 assets/enemy_die1.png 失败\n");
    }
    if (loadimage(&img_enemydeath_2, "assets/enemy_die2.png") != 0) {
        printf("错误: 加载角色图片 assets/enemy_die2.png 失败\n");
    }
    if (loadimage(&img_name, "assets/name.png") != 0) {
        printf("错误: 加载角色图片 assets/name.png 失败\n");
    }
    // 敌人移动精灵图文件
    char enemySpriteFiles[ENEMY_TYPES][30] = {
        "assets/enemy1.png", "assets/enemy2.png" };

    // 加载所有敌人贴图
    for (int type = 0; type < ENEMY_TYPES; type++) {
        if (loadimage(&img_enemies[type][0], enemySpriteFiles[type]) != 0) {
            printf("敌人贴图 %s 加载失败\n", enemySpriteFiles[type]);
            return 0;
        }
    }

    imagesLoaded = 1;
    printf("游戏图片资源加载成功\n");
    return 1;
}

// 游戏界面
void gameplaying() {
    cleardevice();

    mciSendString("open assets/game_bgm.mp3", NULL, 0, NULL);
    mciSendString("play assets/game_bgm.mp3 repeat", NULL, 0, NULL);

    // 检查图片资源是否已加载
    if (!imagesLoaded) {
        if (!loadGameImages()) {
            printf("游戏资源加载失败，返回主菜单\n");
            return;
        }
    }

    setbkcolor(LIGHTGRAY);
    cleardevice();

    setbkmode(TRANSPARENT);
    settextcolor(BLACK);
    settextstyle(25, 0, "宋体");
    int speed = 65;
    //run
    int run_totalwidth = img_run_left.getwidth();
    int run_totalheight = img_run_left.getheight();

    int runframes = 8;
    int runimgw = run_totalwidth / runframes;
    int runimgh = run_totalheight;

    int run_rightindex = 0;
    int run_leftindex = 7;
    //stand
    int standframes = 6;
    int stand_rightindex = 0;
    int stand_leftindex = 5;

    int stand_totalwidth = img_stand_right.getwidth();
    int stand_totalheight = img_stand_right.getheight();

    int standimgw = stand_totalwidth / standframes;
    int standimgh = stand_totalheight;
    //jump
    int jumpframes = 12;
    int jump_rightindex = 0;
    int jump_leftindex = 11;

    int jump_totalwidth = img_jump_right.getwidth();
    int jump_totalheight = img_jump_right.getheight();

    int jumpimgw = jump_totalwidth / jumpframes;
    int jumpimgh = jump_totalheight;

    //attack
    int attackframes = 4;
    int attack_rightindex = 0;
    int attack_leftindex = 3;

    int attack_toltalwidth = img_attack_left.getwidth();
    int attack_toltalheight = img_attack_left.getheight();

    int attackimgw = attack_toltalwidth / attackframes;
    int attackimgh = attack_toltalheight;

    player.width = standimgw;
    player.height = standimgh;

    initGamePhysics();
    initEnemies();

    int facingRight = 1;

    memset(keys, 0, sizeof(keys));

    int lasttime = clock();

    // 添加退出标志
    int shouldExit = 0;

    while (!shouldExit)
    {
        // 清空消息队列，防止堆积
        while (peekmessage(&msg, EX_MOUSE | EX_KEY, true)) {
            if (msg.message == WM_KEYDOWN) {
                keys[msg.vkcode] = true;
                if (msg.vkcode == VK_ESCAPE) {
                    mciSendString("close assets/game_bgm.mp3", NULL, 0, NULL);
                    mciSendString("close assets/end.wav", NULL, 0, NULL);
                    shouldExit = 1;  // 设置退出标志
                    break;           // 退出消息处理循环
                }
                if (!player.isAlive && msg.vkcode == 'R') {
                    initGamePhysics();
                    
                    // 播放
                    mciSendString("open assets/game_bgm.mp3", NULL, 0, NULL);
                    mciSendString("play assets/game_bgm.mp3 repeat", NULL, 0, NULL);
                    
                }
                if (msg.vkcode == 'J' && player.isAlive) {
                    if (!player.isAttacking) {
                        // 重置动画索引
                        if (facingRight) {
                            attack_rightindex = 0;
                        }
                        else {
                            attack_leftindex = 3;
                        }
                        // 调用攻击函数
                        shootSwordBeam();
                    }
                }
                if ((msg.vkcode == 'W' || msg.vkcode == VK_UP || msg.vkcode == 'K') && player.isAlive) {
                    if (player.isOnGround) {
                        player.velocityY = -14;
                        player.isJumping = 1;
                        player.isOnGround = 0;
                        player.jumpCount = 1;
                        if (facingRight) {
                            jump_rightindex = 0;
                        }
                        else {
                            jump_rightindex = 11;
                        }
                    }
                    else if (player.jumpCount < player.maxJumps) {
                        player.velocityY = -12;
                        player.isJumping = 1;
                        player.jumpCount++;
                        if (facingRight) {
                            jump_rightindex = 0;
                        }
                        else {
                            jump_leftindex = 11;
                        }
                    }
                }
                if ((msg.vkcode == 'S' || msg.vkcode == VK_DOWN) && player.isOnGround && player.isAlive) {
                    player.y += 10;
                    player.isOnGround = 0;
                    player.velocityY = 5;
                    player.isJumping = 0;
                    player.jumpCount = 0;
                }
            }
            else if (msg.message == WM_KEYUP) {
                keys[msg.vkcode] = false;
            }
        }

        // 如果设置了退出标志，立即退出游戏循环
        if (shouldExit) {
            break;
        }
        if (player.isAlive) {
            handleInput();
            updatePhysics();
            updatePlatforms();
            updateEnemies();
            updateSwordBeams();
            updateAttack(&attack_rightindex, &attack_leftindex);
            checkCollisions();
            checkEnemyCollisions();
            checkBeamEnemyCollisions();
            checkBeamObstacleCollisions();

            if (clock() - lasttime >= speed) {
                if (player.isAlive) {

                    if (!player.isOnGround) {
                        if (facingRight) {
                            jump_rightindex++;
                            jump_rightindex %= 12;
                        }
                        else {
                            jump_leftindex--;
                            if (jump_leftindex < 0) {
                                jump_leftindex = 11;
                            }
                        }
                    }
                    if (keys['D'] || keys[VK_RIGHT] || keys['A'] || keys[VK_LEFT])
                    {
                        if (facingRight) {
                            //runright
                            run_rightindex++;
                            run_rightindex %= runframes;
                        }
                        else {
                            //runleft
                            run_leftindex--;
                            if (run_leftindex < 0) {
                                run_leftindex = 7;
                            }
                        }
                    }
                    else {
                        if (facingRight) {
                            //standright
                            stand_rightindex++;
                            stand_rightindex %= 6;
                        }
                        else {
                            //standright
                            stand_leftindex--;
                            if (stand_leftindex < 0) {
                                stand_leftindex = 5;
                            }
                        }
                    }
                }
                lasttime = clock();
            }

        }

        BeginBatchDraw();

        putimage(0, 0, &img_gamebk);
        setlinecolor(WHITE);
        setlinestyle(PS_SOLID, 4);
        line(0, CENTRAL_LINE_Y, 1400, CENTRAL_LINE_Y);

        setlinestyle(PS_SOLID, 5);
        setfillcolor(BLUE);;
        //绘制平台
        for (int i = 0; i < platformCount; i++) {
            if (platforms[i].isActive) {
                line(platforms[i].x, platforms[i].y, platforms[i].x + platforms[i].width, platforms[i].y);
                setlinestyle(PS_NULL, 1);
                if (platforms[i].hasObstacle) {
                    setfillcolor(RED);
                    fillrectangle(platforms[i].obstacleX, platforms[i].y - 30,
                        platforms[i].obstacleX + 30, platforms[i].y);
                }
                setlinestyle(PS_SOLID, 5);
            }
        }

        // 绘制敌人
        for (int i = 0; i < MAX_ENEMIES; i++) {
            if (enemies[i].isActive) {
                if (enemyIsDying[i]) {
                    // 绘制死亡动画
                    IMAGE* deathImg = (enemies[i].type == 0) ? &img_enemydeath_1 : &img_enemydeath_2;
                    int totalFrames = 5;  // 假设每张死亡图有5帧
                    int frameWidth = deathImg->getwidth() / totalFrames;
                    int frameHeight = deathImg->getheight();

                    drawImg(enemies[i].x, enemies[i].y,
                        frameWidth, frameHeight,
                        deathImg,
                        enemyDeathAnimIndex[i] * frameWidth, 0);
                }
                else {
                    // 绘制移动动画
                    int type = enemies[i].type;
                    int totalFrames = enemyFrameCount[type];
                    int frameWidth = img_enemies[type][0].getwidth() / totalFrames;
                    int frameHeight = img_enemies[type][0].getheight();

                    drawImg(enemies[i].x, enemies[i].y,
                        frameWidth, frameHeight,
                        &img_enemies[type][0],
                        enemies[i].animIndex * frameWidth, 0);
                }
                /*敌人碰撞盒
                if (player.isAlive) {
                    setlinecolor(YELLOW);
                    rectangle(enemies[i].x + 25, enemies[i].y + 10,
                        enemies[i].x + enemies[i].width - 25,
                        enemies[i].y + enemies[i].height - 10);
                }*/
            }
        }
        // 绘制剑气
        for (int i = 0; i < MAX_SWORD_BEAMS; i++) {
            if (swordBeams[i].isActive) {
                if (swordBeams[i].direction == 1) {
                    drawImg(swordBeams[i].x, swordBeams[i].y,
                        swordBeams[i].width, swordBeams[i].height,
                        &img_swordbeam_right, 0, 0);
                }
                else {
                    drawImg(swordBeams[i].x, swordBeams[i].y,
                        swordBeams[i].width, swordBeams[i].height,
                        &img_swordbeam_left, 0, 0);
                }

                /*绘制剑气碰撞盒
                if (player.isAlive) {
                    setlinecolor(CYAN);
                    rectangle(swordBeams[i].x + 10, swordBeams[i].y + 5,
                        swordBeams[i].x + swordBeams[i].width - 10,
                        swordBeams[i].y + swordBeams[i].height - 5);
                }*/
            }
        }
        outtextxy(0, 0, "按ESC键返回菜单");

        char info[100];
        sprintf(info, "控制: A/D-移动  W/K-跳跃(二段)  J-剑气 S-下跳");
        outtextxy(0, 30, info);

        sprintf(info, "得分: %d", int(player.score));
        outtextxy(0, 60, info);

        sprintf(info, "所有图片音频仅为娱乐，绝无恶意");
        outtextxy(0, 90, info);
        

        // 添加碰撞盒
        if (player.isAlive) {
            float collisionWidth = player.width - 100;
            float collisionHeight = player.height - 10;

            float collisionX, collisionY;

            // 分别调试朝左和朝右的最佳偏移
            if (player.direction == 1) {
                // 朝右
                // 朝右时角色在图片左侧，碰撞盒需要靠右
                collisionX = player.x + 45;
                collisionY = player.y + 5;
            }
            else {
                // 朝左
                // 朝左时角色在图片右侧，碰撞盒需要靠左
                collisionX = player.x + 52;
                collisionY = player.y + 5;
            }

            /*绘制角色碰撞盒
            setlinecolor(RED);
            rectangle(collisionX, collisionY,
                collisionX + collisionWidth, collisionY + collisionHeight);*/

            /*绘制障碍物碰撞盒
            setlinecolor(GREEN);
            for (int i = 0; i < platformCount; i++) {
                if (platforms[i].isActive && platforms[i].hasObstacle) {
                    rectangle(platforms[i].obstacleX + 10, platforms[i].y - 30 + 10,
                        platforms[i].obstacleX + 30 - 10, platforms[i].y - 10);
                }
            }*/

        }

        if (player.isAlive) {
            facingRight = (player.direction == 1);

            int ismoving = (keys['A'] || keys['D'] || keys[VK_LEFT] || keys[VK_RIGHT]);
            int isjumping = !player.isOnGround;
            int isattacking = player.isAttacking;
            if (facingRight)
            {
                if (isattacking) {
                    drawImg(player.x, player.y, attackimgw, attackimgh, &img_attack_right,
                        attack_rightindex * attackimgw, 0);
                }
                else if (isjumping) {
                    drawImg(player.x, player.y, jumpimgw, jumpimgh, &img_jump_right, jump_rightindex * jumpimgw, 0);
                }
                else if (ismoving) {
                    drawImg(player.x, player.y, runimgw, runimgh, &img_run_right, run_rightindex * runimgw, 0);
                }
                else {
                    drawImg(player.x, player.y, standimgw, standimgh, &img_stand_right, stand_rightindex * standimgw, 0);
                }
            }
            else {
                if (isattacking) {
                    drawImg(player.x, player.y, attackimgw, attackimgh, &img_attack_left,
                        attack_leftindex * attackimgw, 0);
                }
                else if (isjumping) {
                    drawImg(player.x, player.y, jumpimgw, jumpimgh, &img_jump_left, jump_leftindex * jumpimgw, 0);
                }
                else if (ismoving) {
                    drawImg(player.x, player.y, runimgw, runimgh, &img_run_left, run_leftindex * runimgw, 0);
                }
                else {
                    drawImg(player.x, player.y, standimgw, standimgh, &img_stand_left, stand_leftindex * standimgw, 0);
                }
            }
        }
        else {
            showGameOver();
        }

        EndBatchDraw();

        Sleep(16);
    }

    printf("退出游戏界面\n");
}

// 加载菜单图片资源
int loadMenuImages() {
    // 加载菜单背景
    if (loadimage(&img_start, "assets/bk1.png", 1400, 800) != 0) {
        printf("错误: 加载菜单背景 assets/bk1.png 失败\n");
        return 0;
    }

    // 加载UI按钮图片
    char str[100];
    for (int i = 0; i < 3; i++) {
        sprintf(str, "assets/ui_start_%d.png", i + 1);
        //返回0表示成功
        if (loadimage(img_starts + i, str) != 0) {
            printf("错误: 加载UI图片 %s 失败\n", str);
            return 0;
        }
    }

    printf("菜单图片资源加载成功\n");
    return 1;
}
// 释放图片资源
void freeImages() {

    imagesLoaded = 0;
    printf("资源已释放\n");
}

// 检查鼠标位置
//int inArea(int mx, int my, int x, int y, int w, int h) {
//    if (mx > x && mx < x + w && my > y && my < y + h) {
//        return 1;
//    }
//    return 0;
//}

// UI
// UI
int main()
{
    initgraph(1400, 800, EX_SHOWCONSOLE | EX_DBLCLKS);

    // 程序启动时加载所有图片资源
    printf("正在加载资源...\n");
    if (!loadGameImages() || !loadMenuImages()) {
        printf("程序启动失败：无法加载资源文件\n");
        printf("请确保 assets 文件夹存在且包含所有必要的图片文件\n");
        closegraph();
        return -1;
    }
    printf("所有资源加载完成\n");

    cleardevice();
    enum GameState currenState = MAIN_MENU;
    setbkcolor(LIGHTGRAY);

    int totalwidth = getwidth();
    int totalheight = getheight();

    int img_width = img_starts[0].getwidth();
    int img_height = img_starts[0].getheight();

    // 添加帧率控制变量
    int lastTime = clock();
    int frameDelay = 100; // 每100ms更新一次
    int isHovering = 0;   // 鼠标是否悬停

    // 添加音乐状态控制变量
    int menuMusicPlaying = 0; // 0=未播放，1=正在播放

    while (true)
    {
        // 计算间隔
        int currentTime = clock();
        int timePassed = currentTime - lastTime;

        if (currenState == MAIN_MENU) {
            // 只在菜单状态且音乐未播放时开始播放
            if (!menuMusicPlaying) {
                // 播放菜单音乐
                mciSendString("open assets/menu_bgm.mp3 alias menu_bgm", NULL, 0, NULL);
                mciSendString("play menu_bgm repeat", NULL, 0, NULL);
                menuMusicPlaying = 1;
                printf("菜单音乐开始循环播放\n");
            }

            BeginBatchDraw();
            putimage(0, 0, &img_start);

            // 检查鼠标状态
            int mouseHover = 0;
            if (peekmessage(&msg, EX_MOUSE | EX_KEY))
            {
                if (msg.message == WM_KEYDOWN && msg.vkcode == VK_ESCAPE) {
                    
                    // 退出程序前关闭音乐
                    if (menuMusicPlaying) {
                        mciSendString("stop menu_bgm", NULL, 0, NULL);
                        mciSendString("close menu_bgm", NULL, 0, NULL);
                        menuMusicPlaying = 0;
                    }
                    break; // 退出程序
                }

                // 检查鼠标是否在按钮区域内
                mouseHover = inArea(msg.x, msg.y,
                    (totalwidth - img_width) / 2,
                    (totalheight - img_height) / 2 + 200,
                    img_width, img_height);

                if (mouseHover && (msg.message == WM_LBUTTONDOWN || msg.message == WM_RBUTTONDOWN)) {
                    drawImg((totalwidth - img_width) / 2,
                        (totalheight - img_height) / 2 + 200,
                        img_starts + 2); // 按下状态

                    //点击开始游戏时停止音乐，但不要重置状态变量
                    if (menuMusicPlaying) {
                        mciSendString("stop menu_bgm", NULL, 0, NULL);
                        mciSendString("close menu_bgm", NULL, 0, NULL);
                        // 注意：这里不设置 menuMusicPlaying = 0
                        printf("进入游戏，停止菜单音乐\n");
                    }

                    currenState = GAME_PLAYING;
                    Sleep(100);

                    EndBatchDraw();
                    continue;
                }
            }

            // 更新悬停状态
            if (mouseHover != isHovering) {
                isHovering = mouseHover;
            }

            // 根据悬停状态绘制按钮
            if (timePassed >= frameDelay) {
                if (isHovering) {
                    drawImg((totalwidth - img_width) / 2,
                        (totalheight - img_height) / 2 + 200,
                        img_starts + 1); // 悬停状态
                }
                else {
                    drawImg((totalwidth - img_width) / 2,
                        (totalheight - img_height) / 2 + 200,
                        img_starts);     // 正常状态
                }
                lastTime = currentTime;
            }
            else {
                // 使用上一次绘制的状态
                if (isHovering) {
                    drawImg((totalwidth - img_width) / 2,
                        (totalheight - img_height) / 2 + 200,
                        img_starts + 1);
                }
                else {
                    drawImg((totalwidth - img_width) / 2,
                        (totalheight - img_height) / 2 + 200,
                        img_starts);
                }
            }

            setbkmode(TRANSPARENT);
            settextcolor(BLACK);
            settextstyle(25, 0, "宋体");
            outtextxy(0, 0, "按ESC键退出");
            
            outtextxy(0, 30,"所有图片音频仅为娱乐，绝无恶意");

            drawImg(495, 150, &img_name);           

            EndBatchDraw();

           
        }
        else if (currenState == GAME_PLAYING) {
            // 游戏进行中，确保音乐已停止
            if (menuMusicPlaying) {
                menuMusicPlaying = 0;
            }

            gameplaying();
            currenState = MAIN_MENU;

            //游戏结束返回菜单时，重置音乐状态
            menuMusicPlaying = 0;
            printf("游戏结束，返回菜单，准备重新播放音乐\n");
        }
        Sleep(10);
    }

    // 程序退出前清理资源
    // 程序退出时确保音乐已关闭
    if (menuMusicPlaying) {
        mciSendString("stop menu_bgm", NULL, 0, NULL);
        mciSendString("close menu_bgm", NULL, 0, NULL);
        menuMusicPlaying = 0;
    }

    freeImages();
    closegraph();
    printf("程序正常退出\n");
    return 0;
}