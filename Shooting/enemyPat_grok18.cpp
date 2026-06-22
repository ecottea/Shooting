// enemyPat_Tmp_spiderWeb.cpp
// クモの巣モチーフ弾幕パターン
// 敵本体関数: EnemyPat_SpiderWeb_Grok()

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================
// クモの巣モチーフ弾幕
// ・中心から放射状に「放射線（ spokes ）」を複数本展開
// ・同心円状にゆっくり広がる「環（ rings ）」を追加
// ・放射線に沿って小さな弾が這うような動き
// ・全体としてクモの巣のような網目状の視覚効果を目指す
// =============================================
static void ShotSpiderWeb(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int spokes = 12;           // 放射線の数
    const double ringInterval = 25.0; // 環の間隔

    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 1. 放射線（spokes）を作成
        for (int i = 0; i < spokes; i++) {
            double baseAngle = (DX_PI * 2.0 * i) / spokes;

            // 放射線上に複数の弾を配置（巣の糸を表現）
            for (int j = 1; j <= 6; j++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = baseAngle;
                pEnemyShot->speed = 0.0; // 最初は静止、後で加速

                // 距離に応じた種類（遠くなるほど大きい弾）
                int type = (j <= 2) ? 0 : (j <= 4 ? 1 : 2); // 小→中→大
                int color = (i % 3 == 0) ? 4 : 5; // 青とマゼンタで網目っぽく

                switch (type) {
                case 0: pEnemyShot->kind = img_enemyShotSmallBall[color]; break;
                case 1: pEnemyShot->kind = img_enemyShotMediumBall[color]; break;
                case 2: pEnemyShot->kind = img_enemyShotLargeBall[color]; break;
                }

                // 初期位置を少しずらす（自然な糸の感じ）
                double dist = j * 35.0;
                pEnemyShot->x += dist * cos(baseAngle);
                pEnemyShot->y += dist * sin(baseAngle);

                // リンク
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 2. 同心円状の環（ゆっくり広がる）
        for (int r = 1; r <= 4; r++) {
            int points = 24 + r * 4;
            for (int i = 0; i < points; i++) {
                double angle = (DX_PI * 2.0 * i) / points;
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = angle + (GetRand(30) - 15) / 180.0 * DX_PI; // 少し揺らぎ
                pEnemyShot->speed = 0.8 + r * 0.3; // 外側ほど速く

                pEnemyShot->kind = img_enemyShotScale[2]; // 緑の鱗弾で環を表現

                double dist = r * ringInterval;
                pEnemyShot->x += dist * cos(angle);
                pEnemyShot->y += dist * sin(angle);

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 放射線弾は徐々に加速して外へ
        if (pEnemyShot->speed < 4.5) {
            pEnemyShot->speed += 0.035;
        }

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 軽い波打つ動き（巣の糸が揺れる感じ）
        if (pEnemyShotSet->count % 8 == 0) {
            pEnemyShot->muki += 0.008 * sin((double)pEnemyShotSet->count / 10.0);
        }

        pEnemyShot = pEnemyShot->next;
    }

    pEnemyShotSet->count++;
}

// =============================================
// 敵本体パターン
// =============================================
void EnemyPat_SpiderWeb_Grok()
{
    static int phase = 0;
    static int muki = 1;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        muki = 1;
    }

    // 敵の動き：ゆったり左右移動 + 時々下降
    enemy.x += 1.1 * (double)muki;
    if (enemy.x < 80 || enemy.x > 400) muki *= -1;

    if (count % 540 == 500) {
        enemy.y += 35.0; // 少し下がる
    }

    // 弾幕発射タイミング
    if (count % 55 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSpiderWeb;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 18.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}