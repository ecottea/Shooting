// enemyPat_spiderWeb.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// クモの巣状弾幕パターン（螺旋拡大 + 狙撃バースト）
static void WebPattern(sEnemyShotSet* pEnemyShotSet)
{
    const int    NUM_SPOKES = 8;          // 放射本数（高密度）
    const double ROTATION_SPEED = 0.01;      // 螺旋の回転速度
    const double RADIAL_SPEED_MIN = 1.2;     // 半径方向速度 最小
    const double RADIAL_SPEED_MAX = 2.3;     // 半径方向速度 最大
    const int    SPIDER_SHOT_INTERVAL = 35;  // 狙撃間隔フレーム

    // ---- 初回生成（中心のクモ + 回転方向決定） ----
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 中心のクモ（赤大玉、静止）
        sEnemyShot* pSpider = new sEnemyShot;
        pSpider->x = pEnemyShotSet->x;
        pSpider->y = pEnemyShotSet->y;
        pSpider->muki = 0.0;
        pSpider->speed = 0.0;                  // 速度0で静止
        pSpider->kind = img_enemyShotLargeBall[0]; // 赤大玉

        pSpider->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pSpider->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pSpider;
        pEnemyShotSet->pEnemyShotHead->prev = pSpider;

        // 螺旋の回転方向をランダムに決定（1:反時計, -1:時計）
        pEnemyShotSet->kind = (GetRand(1) == 0) ? -1 : -1;
    }

    // ---- フレーム経過に応じて巣（白小玉）を追加 ----
    if (pEnemyShotSet->count % 5 == 0 && pEnemyShotSet->count < 280) {
        // 時間とともに半径が増加（40→240）
        double radius = 40.0 + (pEnemyShotSet->count / 2) * 1.8;
        if (radius > 240.0) radius = 240.0;

        double angleBase = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        for (int i = 0; i < NUM_SPOKES; ++i) {
            // 各スポークに1個（ランダムな微小オフセット付き）
            double angle = angleBase + i * (2.0 * DX_PI / NUM_SPOKES)
                + (GetRand(40) - 20) / 180.0 * DX_PI;

            double bx = pEnemyShotSet->x + radius * cos(angle);
            double by = pEnemyShotSet->y + radius * sin(angle);

            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = bx;
            pShot->y = by;
            pShot->muki = angle;                       // 発射方向（後に螺旋回転）
            pShot->speed = RADIAL_SPEED_MIN + (GetRand(100) / 100.0) * (RADIAL_SPEED_MAX - RADIAL_SPEED_MIN);
            pShot->kind = img_enemyShotSmallBall[6];   // 白小玉
            pShot->margin = 9999;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        if (pEnemyShotSet->count % 10 == 0) {
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK); // 周期的な軽い効果音
        }
    }

    // ---- クモから自機狙いのバースト（後半） ----
    if (pEnemyShotSet->count >= 220 && pEnemyShotSet->count % SPIDER_SHOT_INTERVAL == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        double aim = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        for (int i = 0; i < 6; ++i) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            // 自機方向を中心に±20度のばらつき
            pShot->muki = aim + (GetRand(40) - 20) / 180.0 * DX_PI;
            pShot->speed = 2.8 + (GetRand(100) / 100.0) * 1.2;
            pShot->kind = img_enemyShotScale[2];      // 緑の鱗弾（中くらいの威圧感）
            
            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // ---- 毎フレームの移動処理（螺旋回転を含む） ----
    const int spinDir = pEnemyShotSet->kind;   // 回転方向
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->kind == img_enemyShotSmallBall[6]) {  // クモ本体(速度0)は動かさない
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->muki += spinDir * ROTATION_SPEED; // 螺旋回転
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 敵本体パターン（クモの巣弾幕）
void EnemyPat_SpiderWeb_DeepSeek()
{
    static int muki; // 左右移動方向

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // 左右移動（端で折り返し）
        enemy.x += 1.0 * (double)muki;
        if (count % 250 == 125) muki *= -1;
    }

    // 約4.2秒(250フレーム)ごとに新たな巣を展開
    if (count == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = WebPattern;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        // 巣の初期方向は自機へ
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);

        // ダミーヘッダ作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 敵弾セットリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}