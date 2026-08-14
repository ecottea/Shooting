// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：交差振動・クラドニの魔方陣
static void ShotChladni(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    int phase = pEnemyShotSet->param_i[0]; // 0:十字, 1:格子, 2:高密度格子

    if (pEnemyShotSet->count == 0) {
        // 予告音を再生
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // --- 1. 弦弾（中玉で表現した振動する線）の生成 ---
        int vLines[4] = { 240 + 20, 160 + 20, 320 + 20, 96 + 20 }; // 縦線のx座標
        int hLines[4] = { 240, 160, 320, 96 }; // 横線のy座標

        int vCount = 0, hCount = 0;
        if (phase == 0) { vCount = 1; hCount = 1; }       // 十字
        else if (phase == 1) { vCount = 2; hCount = 2; }  // 4つの四角
        else { vCount = 4; hCount = 4; }                  // 複雑な図形

        // 縦線（下に流れる）
        for (int i = 0; i < vCount; i++) {
            // 14ピクセル間隔で中玉を並べることで「線」を表現
            for (int y = -20; y <= 500; y += 14) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = (double)vLines[i];
                pEnemyShot->y = (double)y;
                pEnemyShot->muki = DX_PI / 2.0; // 下向き
                pEnemyShot->speed = 0.8;        // ゆっくり流す（振動の表現）
                pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白色の中玉
                // 線が途切れないように画面外へ出るまで消えないようマージンを広げる
                pEnemyShot->margin = 40.0;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 横線（右に流れる）
        for (int i = 0; i < hCount; i++) {
            for (int x = -20; x <= 500; x += 14) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = (double)x;
                pEnemyShot->y = (double)hLines[i];
                pEnemyShot->muki = 0.0;         // 右向き
                pEnemyShot->speed = 0.8;
                pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白色の中玉
                pEnemyShot->margin = 40.0;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // --- 2. 砂弾（小玉）の生成 ---
        // グリッド状に配置し、弦弾（線）の上にあるものは除外して「節（安全地帯）」を作る
        double safeMargin = 15.0; // 線の中心からこの距離以内は砂弾を置かない

        for (int gx = 12; gx < 480; gx += 20) {
            for (int gy = 12; gy < 480; gy += 20) {
                bool isOnLine = false;

                // 縦線との距離チェック
                for (int j = 0; j < vCount; j++) {
                    if (fabs(gx - vLines[j]) < safeMargin) {
                        isOnLine = true;
                        break;
                    }
                }
                // 横線との距離チェック
                if (!isOnLine) {
                    for (int j = 0; j < hCount; j++) {
                        if (fabs(gy - hLines[j]) < safeMargin) {
                            isOnLine = true;
                            break;
                        }
                    }
                }

                // 線の上なら砂弾を置かない（ここがクラドニ図形の安全地帯となる）
                if (isOnLine) continue;

                // 60%の確率で配置（砂の密度調整。GetRand(4)は0~4を返すため、0,1,2で60%）
                if (GetRand(4) < 3) {
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = (double)gx + (GetRand(8) - 4); // 少しランダムにばらけさせる
                    pEnemyShot->y = (double)gy + (GetRand(8) - 4);
                    // 砂弾はほぼ真下に落ちる
                    pEnemyShot->muki = DX_PI / 2.0 + (GetRand(20) - 10) / 180.0 * DX_PI;
                    pEnemyShot->speed = 1.5 + GetRand(10) / 10.0;
                    pEnemyShot->kind = img_enemyShotSmallBall[8]; // 橙色の小玉（砂）

                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }
    }

    // --- 弾の移動処理 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Chladni_Zai()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 180フレームごとに弾幕を生成
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotChladni;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;

        // フェーズの決定（周波数が上がって図形が複雑になっていく）
        if (shot_count < 3) {
            pEnemyShotSet->param_i[0] = 0; // 基音：十字
        }
        else if (shot_count < 6) {
            pEnemyShotSet->param_i[0] = 1; // 倍音：格子
        }
        else {
            pEnemyShotSet->param_i[0] = 2; // 高周波：高密度格子
        }
        shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}