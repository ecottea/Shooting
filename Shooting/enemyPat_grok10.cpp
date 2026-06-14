// enemyPat_Tmp_Dragon.cpp
// 竜をモチーフにした弾幕パターン（1ファイル完結版）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================
//  竜の息（Fire Breath） - 扇状に広がる炎の弾幕
//  中央は太い火柱、周囲は鱗のような小玉が蛇行しながら飛ぶ
// =============================================
static void DragonFireBreath(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 発射時処理
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);  // 重めの炎音

        const int total = 12;                    // 合計24発
        const double centerAngle = pEnemyShotSet->muki;
        const double spread = DX_PI / 3.0;       // ±60度くらいの扇

        for (int i = 0; i < total; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            double offset = (i - total / 2.0) / (total / 2.0) * spread * 1.1;
            pEnemyShot->muki = centerAngle + offset;

            // 中央ほど太く速い炎、端は小さい鱗弾
            if (abs(offset) < DX_PI / 12.0) {  // 中央部
                pEnemyShot->speed = 4.2 + GetRand(80) / 100.0;
                pEnemyShot->kind = img_enemyShotLargeBall[0];   // 赤 大玉（炎）
            }
            else {
                pEnemyShot->speed = 2.8 + GetRand(120) / 100.0;
                pEnemyShot->kind = img_enemyShotScale[0];       // 赤 鱗弾
            }

            // リンク挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム移動＋蛇行処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 基本進行
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 竜のうねり（時間でサイン波）
        int t = pEnemyShotSet->count;
        double wave = sin(t * 0.25 + pEnemyShot->x * 0.03) * 0.8;
        pEnemyShot->x += wave * sin(pEnemyShot->muki + DX_PI / 2.0);
        pEnemyShot->y += wave * cos(pEnemyShot->muki + DX_PI / 2.0);

        // 徐々に加速（息が勢いづくイメージ）
        if (pEnemyShotSet->count > 20) {
            pEnemyShot->speed += 0.015;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// =============================================
//  竜の尾撃（Tail Lash） - 左右から巻きつくような弾
// =============================================
static void DragonTailLash(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const int arms = 2;
        for (int arm = 0; arm < arms; arm++) {
            double baseAngle = pEnemyShotSet->muki + (arm == 0 ? DX_PI / 2 : -DX_PI / 2);
            for (int i = 0; i < 11; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;

                double offset = (i - 5.0) * 0.18;
                pEnemyShot->muki = baseAngle + offset;
                pEnemyShot->speed = (1.8 + i * 0.22) * 1.2;

                pEnemyShot->kind = img_enemyShotDiamond[4];  // 青 菱形弾（爪・尾のイメージ）

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 徐々にカーブして巻きつく
        pEnemyShot->muki += 0.018 * (pEnemyShotSet->count > 15 ? 1.0 : -1.0);

        if (pEnemyShot->count >= 360) {
            pEnemyShot->x = 9999;
            pEnemyShot->y = 9999;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// =============================================
//  敵本体パターン：Dragon
// =============================================
void EnemyPat_Dragon_Grok()
{
    static int phase = 0;
    static int mukiX = 1;
    static double hover = 0.0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        mukiX = 1;
        hover = 0.0;
    }

    // 竜らしいゆったりとしたホバリング＋左右移動
    hover += 0.045;
    enemy.y = 60.0 + sin(hover) * 18.0;
    enemy.x += 1.15 * mukiX;

    if (enemy.x < 120.0) mukiX = 1;
    if (enemy.x > 360.0) mukiX = -1;

    // 位相で攻撃パターンを切り替え
    if (count % 110 == 0) {
        phase = (phase + 1) % 3;
    }

    if (count % 8 == 0 && count >= 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 18.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        if (phase == 0) {
            // 正面への強力な火炎放射
            pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            pEnemyShotSet->patternFunc = DragonFireBreath;
        }
        else if (phase == 1) {
            // 少し予測を入れて火炎
            double predict = atan2(player.y + player.y * 0.1 - pEnemyShotSet->y,
                                  player.x - pEnemyShotSet->x);
            pEnemyShotSet->muki = predict;
            pEnemyShotSet->patternFunc = DragonFireBreath;
        }
        else {
            // 尾撃
            pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            pEnemyShotSet->patternFunc = DragonTailLash;
        }

        // ShotSetをリンク
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}