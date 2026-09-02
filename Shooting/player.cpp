#include "DxLib.h"
#include "gv.h"
#include "player.h"
#include "imgSoundLoad.h"
#include "effectRand.h"
#include "tasController.h"
#include "gameScreen.h"
#include <math.h>


constexpr double Sqrt2 = 1.41421356237309504880;

static bool isSlowMode = false;
bool isMuteki = false;



// 力場パーティクル用の定義
#define MAX_FORCE_PARTICLES 300

struct ForceParticle {
    bool active;
    double x, y;
    double vx, vy;
    int life;
    unsigned int color;
};

static ForceParticle forceParts[MAX_FORCE_PARTICLES];

// パーティクル生成（力の方向へ飛ばす）
void spawnForceParticles(double x, double y, double forceX, double forceY, double radius)
{
    if (g_isTasMode) return;
    
    double magnitude = sqrt(forceX * forceX + forceY * forceY);
    int count = 1 + (int)(magnitude * 2.0);
    if (count > 6) count = 6;

    double nx = (magnitude > 0.0) ? forceX / magnitude : 0.0;
    double ny = (magnitude > 0.0) ? forceY / magnitude : 0.0;

    for (int i = 0; i < count; ++i) {
        int idx = -1;
        for (int j = 0; j < MAX_FORCE_PARTICLES; ++j) {
            if (!forceParts[j].active) {
                idx = j;
                break;
            }
        }
        if (idx == -1) continue;

        ForceParticle* p = &forceParts[idx];
        p->active = true;

        // ----- 半径 radius の円内からランダムに発生 -----
        double angle = 2.0 * 3.14159265 * effectRandDouble();
        double dist = radius * sqrt(effectRandDouble());   // sqrt(0～1) で一様分布
        p->x = x + cos(angle) * dist;
        p->y = y + sin(angle) * dist;

        // 速度は力の方向へ（大きさは magnitude 依存）
        double speed = magnitude * (1.0 + effectRandDouble()); // 1.0～2.0倍

        // 力の方向の単位ベクトル
        p->vx = nx * speed;
        p->vy = ny * speed;

        // 接線方向の渦成分（角度±90度）
        double tangentAngle = angle + 3.14159265 / 2.0;   // +90度

        // 力の強さに応じて渦の強さを変える
        double swirlStrength = magnitude * 0.5;
        p->vx += cos(tangentAngle) * swirlStrength;
        p->vy += sin(tangentAngle) * swirlStrength;

        // 乱数は小さめに
        p->vx = nx * speed + (effectRandDouble() - 0.5) * 0.2; // ±0.1程度のばらつき
        p->vy = ny * speed + (effectRandDouble() - 0.5) * 0.2;

        p->life = 10 + effectRandInt(10);  // 10～19

        // 色はお好みで（ここでは明るいシアン系）
        int r = 100 + effectRandInt(101);  // 100～200
        int g = 200 + effectRandInt(56);   // 200～255
        int b = 255;
        p->color = GetColor(r, g, b);
    }
}

// 毎フレームの更新（playerControl の最後で呼ぶ想定）
void updateForceParticles()
{
    if (g_isTasMode) return;

    for (int i = 0; i < MAX_FORCE_PARTICLES; ++i) {
        if (!forceParts[i].active) continue;

        ForceParticle* p = &forceParts[i];
        p->x += p->vx;
        p->y += p->vy;
        p->life--;

        if (p->life <= 0) {
            p->active = false;
        }
    }
}

// 描画（playerDisp の最後で呼ぶ想定）
void drawForceParticles()
{
    SetDrawBlendMode(DX_BLENDMODE_ADD, 150);   // 加算で明るく

    for (int i = 0; i < MAX_FORCE_PARTICLES; ++i) {
        if (!forceParts[i].active) continue;

        ForceParticle* p = &forceParts[i];
        double ratio = (double)p->life / (10.0 + 10.0);  // 最大寿命を適当に20と仮定（実際は10~19だが大まかでOK）
        // 色の減衰：寿命が減るほど暗く
        unsigned int baseR = (p->color >> 16) & 0xFF;
        unsigned int baseG = (p->color >> 8) & 0xFF;
        unsigned int baseB = p->color & 0xFF;
        int r = (int)(baseR * ratio);
        int g = (int)(baseG * ratio);
        int b = (int)(baseB * ratio);
        if (r < 0) r = 0; if (g < 0) g = 0; if (b < 0) b = 0;

        // 小さな円（半径2）で描画
        DrawCircleAA(GAME_AREA_X + (float)p->x, (float)p->y, 2, 8, GetColor(r, g, b), TRUE);
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 力場パーティクルを全消去
void clearAllForceParticles() {
    for (int i = 0; i < MAX_FORCE_PARTICLES; ++i) {
        forceParts[i].active = false;
    }
}



#define MAX_PLAYER_ENGINE_FLAMES 400

struct PlayerEngineFlame {
    bool active;
    double x, y;
    double vx, vy;
    int life;
    int maxLife;
    int baseR, baseG, baseB;
};

static PlayerEngineFlame playerFlamePool[MAX_PLAYER_ENGINE_FLAMES];

// 青系の色（より白っぽく明るい青）
static const int FLAME_BASE_R = 20;    // ほぼ黒に近い青を維持
static const int FLAME_BASE_G = 80;    // 200→80 に変更（青みが強くなる）
static const int FLAME_BASE_B = 255;

// 基本噴射方向（下向き）
static const double BASE_DIR_X = 0.0;
static const double BASE_DIR_Y = 1.0;   // 下方向
static const double BASE_SPEED = 3.0;   // 噴射速度（速め）

void spawnPlayerEngineFlame(double x, double y, double vx, double vy)
{
    if (g_isTasMode) return;

    const int count = 12;   // 密集させるために数を増やす

    // 推力方向 = 基本下向き + 移動の反動（0.3倍で傾ける）
    double dirX = BASE_DIR_X - vx * 0.3;
    double dirY = BASE_DIR_Y - vy * 0.3;
    double length = sqrt(dirX * dirX + dirY * dirY);
    if (length > 0.0) {
        dirX /= length;
        dirY /= length;
    }
    else {
        // 万が一ゼロベクトルなら下向き
        dirX = 0.0; dirY = 1.0;
    }

    for (int i = 0; i < count; ++i) {
        int idx = -1;
        for (int j = 0; j < MAX_PLAYER_ENGINE_FLAMES; ++j) {
            if (!playerFlamePool[j].active) {
                idx = j;
                break;
            }
        }
        if (idx == -1) continue;

        PlayerEngineFlame* p = &playerFlamePool[idx];
        p->active = true;
        // 発生位置：ノズル周辺 ±1.5 ドット（狭く）
        p->x = x + (effectRandDouble() - 0.5) * 4.0;
        p->y = y + (effectRandDouble() - 0.5) * 4.0;

        // 速度 = 基本方向 × 基本速度 ＋ ごくわずかな拡散（0.1未満）
        double speed = BASE_SPEED * (0.9 + effectRandDouble() * 0.2); // 2.7～3.3
        double spread = 0.05;   // ほぼ真っ直ぐ
        p->vx = dirX * speed + (effectRandDouble() - 0.5) * spread;
        p->vy = dirY * speed + (effectRandDouble() - 0.5) * spread;

        // 寿命を短く（4～8フレーム）
        p->maxLife = 4 + effectRandInt(5);   // 4～8
        p->life = p->maxLife;

        // 色のばらつきも控えめに
        p->baseR = FLAME_BASE_R + effectRandInt(11) - 5;   // 15～25
        p->baseG = FLAME_BASE_G + effectRandInt(21) - 10;  // 70～90
        p->baseB = FLAME_BASE_B + effectRandInt(11) - 5;   // 250～255
        // クリップ
        if (p->baseR < 0) p->baseR = 0; else if (p->baseR > 255) p->baseR = 255;
        if (p->baseG < 0) p->baseG = 0; else if (p->baseG > 255) p->baseG = 255;
        if (p->baseB < 0) p->baseB = 0; else if (p->baseB > 255) p->baseB = 255;
    }
}

void updatePlayerEngineFlame() {
    if (g_isTasMode) return; 
    
    for (int i = 0; i < MAX_PLAYER_ENGINE_FLAMES; ++i) {
        if (!playerFlamePool[i].active) continue;
        PlayerEngineFlame* p = &playerFlamePool[i];
        p->x += p->vx;
        p->y += p->vy;
        p->life--;
        if (p->life <= 0) p->active = false;
    }
}

void drawPlayerEngineFlame() {
    SetDrawBlendMode(DX_BLENDMODE_ADD, 200);   // 加算強度も高め
    for (int i = 0; i < MAX_PLAYER_ENGINE_FLAMES; ++i) {
        if (!playerFlamePool[i].active) continue;
        PlayerEngineFlame* p = &playerFlamePool[i];
        double ratio = (double)p->life / p->maxLife;
        int r = (int)(p->baseR * ratio);
        int g = (int)(p->baseG * ratio);
        int b = (int)(p->baseB * ratio);
        if (r < 0) r = 0; if (g < 0) g = 0; if (b < 0) b = 0;
        // 半径 1 の小さな点（アンチエイリアス円だと線に見える）
        DrawCircleAA(GAME_AREA_X + (float)p->x, (float)p->y, 1, 8, GetColor(r, g, b), TRUE);
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// プレイヤーのエンジン炎パーティクルを全消去
void clearAllPlayerEngineFlames() {
    for (int i = 0; i < MAX_PLAYER_ENGINE_FLAMES; ++i) {
        playerFlamePool[i].active = false;
    }
}



// 無敵エフェクト用の定義
#define MAX_INVINCIBLE_PARTICLES 200

struct InvincibleParticle {
    bool active;
    double x, y;          // 現在位置（ゲームエリア内座標）
    double angle;         // 公転角度（ラジアン）
    double radius;        // 中心からの距離
    double angularSpeed;  // 公転速度
    double radialSpeed;   // 外側への拡散速度
    int life;             // 残り寿命
    double hue;           // 色相（0～360）
    double hueSpeed;      // 色相の変化速度
};

static InvincibleParticle invincibleParts[MAX_INVINCIBLE_PARTICLES];

// HSVからRGBへの変換（H: 0～360, S: 0～1, V: 0～1）
static void HSVtoRGB(double h, double s, double v, int& r, int& g, int& b) {
    double c = v * s;
    double hp = h / 60.0;
    double x = c * (1.0 - fabs(fmod(hp, 2.0) - 1.0));
    double r1, g1, b1;
    if (hp < 1) { r1 = c; g1 = x; b1 = 0; }
    else if (hp < 2) { r1 = x; g1 = c; b1 = 0; }
    else if (hp < 3) { r1 = 0; g1 = c; b1 = x; }
    else if (hp < 4) { r1 = 0; g1 = x; b1 = c; }
    else if (hp < 5) { r1 = x; g1 = 0; b1 = c; }
    else { r1 = c; g1 = 0; b1 = x; }
    double m = v - c;
    r = (int)((r1 + m) * 255.0 + 0.5);
    g = (int)((g1 + m) * 255.0 + 0.5);
    b = (int)((b1 + m) * 255.0 + 0.5);
    if (r > 255) r = 255; if (r < 0) r = 0;
    if (g > 255) g = 255; if (g < 0) g = 0;
    if (b > 255) b = 255; if (b < 0) b = 0;
}

// 無敵エフェクト用パーティクルを生成（毎フレーム呼ぶ）
void spawnInvincibleParticles(double playerX, double playerY) {
    if (g_isTasMode) return;

    // 1フレームに発生させる数（必要に応じて調整）
    int count = 5;
    for (int i = 0; i < count; ++i) {
        int idx = -1;
        for (int j = 0; j < MAX_INVINCIBLE_PARTICLES; ++j) {
            if (!invincibleParts[j].active) {
                idx = j;
                break;
            }
        }
        if (idx == -1) continue;  // 空きなし

        InvincibleParticle* p = &invincibleParts[idx];
        p->active = true;

        // 発生位置は自機中心からわずかに離れた位置（初期radius=0でも良いが、少し広げると見栄えが良い）
        p->angle = 2.0 * 3.14159265 * effectRandDouble();  // ランダムな角度
        p->radius = 10.0 + effectRandDouble() * 10.0;     // 初期距離 10～20
        p->x = playerX + cos(p->angle) * p->radius;
        p->y = playerY + sin(p->angle) * p->radius;

        // 公転速度：時計回り or 反時計回りをランダムに（同じ方向でも虹色はきれいなので、揃えても良い）
        p->angularSpeed = (0.05 + effectRandDouble() * 0.08);
        if (effectRandInt(2) == 0) p->angularSpeed = -p->angularSpeed;

        // 外側への拡散速度
        p->radialSpeed = 1.0 + effectRandDouble() * 1.5;

        // 寿命
        p->life = 10 + effectRandInt(15);

        // 色相は発生時の角度に対応させて虹色に分布させる
        p->hue = p->angle * (180.0 / 3.14159265); // ラジアン→度
        if (p->hue < 0) p->hue += 360.0;

        // 色相の回転速度（毎フレーム数度変化）
        p->hueSpeed = 2.0 + effectRandDouble() * 2.0; // 2～4度/フレーム
    }
}

// 無敵エフェクト用パーティクルの更新
void updateInvincibleParticles(double playerX, double playerY) {
    if (g_isTasMode || !isMuteki) return;

    // 毎フレーム生成
    spawnInvincibleParticles(playerX, playerY);

    // 既存パーティクルの更新
    for (int i = 0; i < MAX_INVINCIBLE_PARTICLES; ++i) {
        if (!invincibleParts[i].active) continue;

        InvincibleParticle* p = &invincibleParts[i];

        // 位置更新（公転＋拡散）
        p->angle += p->angularSpeed;
        p->radius += p->radialSpeed;
        p->x = playerX + cos(p->angle) * p->radius;
        p->y = playerY + sin(p->angle) * p->radius;

        // 色相更新
        p->hue += p->hueSpeed;
        if (p->hue >= 360.0) p->hue -= 360.0;
        if (p->hue < 0.0) p->hue += 360.0;

        // 寿命減少
        p->life--;
        if (p->life <= 0) {
            p->active = false;
        }
    }
}

// 無敵エフェクト用パーティクルの描画
void drawInvincibleParticles() {
    SetDrawBlendMode(DX_BLENDMODE_ADD, 255); // 加算ブレンドでより明るく

    for (int i = 0; i < MAX_INVINCIBLE_PARTICLES; ++i) {
        if (!invincibleParts[i].active) continue;

        InvincibleParticle* p = &invincibleParts[i];

        // 寿命による減光（寿命が長いほど明るい）
        double lifeRatio = (double)p->life / 10.0; // 最大寿命10としておおよその割合
        if (lifeRatio > 1.0) lifeRatio = 1.0;
        if (lifeRatio < 0.0) lifeRatio = 0.0;

        // HSVからRGBへ変換（S=1, V=lifeRatio）
        int r, g, b;
        double brightness = lifeRatio * 1.2;
        if (brightness > 1.0) brightness = 1.0;
        HSVtoRGB(p->hue, 1.0, brightness, r, g, b);

        // 粒子サイズ（寿命が短いと小さくなる）
        float size = 2.0f * (float)lifeRatio + 1.0f;

        // 描画座標（既存の力場パーティクルと同様に GAME_AREA_X を加算）
        DrawCircleAA(GAME_AREA_X + (float)p->x, (float)p->y, size, 16, GetColor(r, g, b), TRUE);
    }

    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

// 無敵エフェクト用パーティクルを全て消去
void clearAllInvincibleParticles() {
    for (int i = 0; i < MAX_INVINCIBLE_PARTICLES; ++i) {
        invincibleParts[i].active = false;
    }
}



void playerControl() {
    double playerSpeed;
    if (key[KEY_INPUT_C] != 0) { playerSpeed = 1.5; isSlowMode = true; }
    else                      { playerSpeed = 4.5; isSlowMode = false; }

    if (key[KEY_INPUT_NUMPAD6] != 0) {
        if      (key[KEY_INPUT_NUMPAD5] != 0) { player.x += playerSpeed / Sqrt2; player.y += playerSpeed / Sqrt2; }
        else if (key[KEY_INPUT_NUMPAD8] != 0) { player.x += playerSpeed / Sqrt2; player.y -= playerSpeed / Sqrt2; }
        else                                   { player.x += playerSpeed; }
    }
    else if (key[KEY_INPUT_NUMPAD4] != 0) {
        if      (key[KEY_INPUT_NUMPAD5] != 0) { player.x -= playerSpeed / Sqrt2; player.y += playerSpeed / Sqrt2; }
        else if (key[KEY_INPUT_NUMPAD8] != 0) { player.x -= playerSpeed / Sqrt2; player.y -= playerSpeed / Sqrt2; }
        else                                   { player.x -= playerSpeed; }
    }
    else if (key[KEY_INPUT_NUMPAD5] != 0) { player.y += playerSpeed; }
    else if (key[KEY_INPUT_NUMPAD8] != 0) { player.y -= playerSpeed; }

    if      (player.x < 12.5)  player.x = 12.5;
    else if (player.x > 467.5) player.x = 467.5;
    if      (player.y < 17.5)  player.y = 17.5;
    else if (player.y > 462.5) player.y = 462.5;

    // ノズル位置（画像中心が (x,y)、下端付近 オフセット調整）
    double nozzleX = player.x - 0.5;
    double nozzleY = player.y + 12.0;   // 画像下端に合わせる

    // 速度の逆方向に炎を噴射（推力の反対）
    spawnPlayerEngineFlame(nozzleX, nozzleY, 0.0, 1.0);

    // パーティクル更新
    updatePlayerEngineFlame();

    updateForceParticles();

    if (isMuteki) {
        updateInvincibleParticles(player.x, player.y);
    }
    else {
        clearAllInvincibleParticles();
    }
}

void playerDisp() {
    DrawGraph(GAME_AREA_X + (int)(player.x - 18.0 + 0.5), (int)(player.y - 26.0 + 0.5), imageData[img_player].handle, TRUE);

    if (!isSlowMode && !g_isTasMode) SetDrawBlendMode(DX_BLENDMODE_ALPHA, 128);
    DrawCircle(GAME_AREA_X + (int)(player.x + 0.5), (int)(player.y + 0.5), 4, GetColor(255,0,0), TRUE);
    DrawCircle(GAME_AREA_X + (int)(player.x + 0.5), (int)(player.y + 0.5), 2, GetColor(255,255,255), TRUE);
    if (!isSlowMode && !g_isTasMode) SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);

    drawPlayerEngineFlame();

    drawForceParticles();

    if (isMuteki) {
        //drawInvincibleParticles();
    }
}