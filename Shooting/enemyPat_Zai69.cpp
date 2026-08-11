// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：重力合成弾幕・果実のカスケード
static void ShotWatermelonCascade(sEnemyShotSet* pEnemyShotSet)
{
    // チェリー（小玉）の散布
    if (pEnemyShotSet->count % 10 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 5; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = enemy.x + GetRand(40) - 20;
            pEnemyShot->y = enemy.y + 10.0;

            // 下方向を基準にランダムな角度で散布
            double angle = DX_PI / 2.0 + (GetRand(120) - 60) / 180.0 * DX_PI;
            double spd = (100 + GetRand(100)) / 100.0;

            // 重力計算のため、速度をX・Y成分で保持する
            pEnemyShot->param_d[0] = spd * cos(angle); // vx
            pEnemyShot->param_d[1] = spd * sin(angle); // vy
            pEnemyShot->speed = 0;
            pEnemyShot->muki = 0;

            // レベル0：チェリー（小玉・赤）
            pEnemyShot->kind = img_enemyShotSmallBall[0];
            pEnemyShot->param_i[0] = 0; // レベル (0:チェリー, 1:イチゴ, 2:メロン, 3:スイカ, 4:停止スイカ)
            pEnemyShot->param_i[1] = 0; // 生存フラグ (0:生存, -1:消去予約)

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾同士の合成判定（レベル0〜2まで）
    sEnemyShot* p1 = pEnemyShotSet->pEnemyShotHead->next;
    while (p1 != pEnemyShotSet->pEnemyShotHead) {
        if (p1->param_i[0] < 3 && p1->param_i[1] != -1) {
            sEnemyShot* p2 = p1->next;
            while (p2 != pEnemyShotSet->pEnemyShotHead) {
                if (p2->param_i[0] == p1->param_i[0] && p2->param_i[1] != -1) {
                    double dx = p1->x - p2->x;
                    double dy = p1->y - p2->y;
                    double dist = sqrt(dx * dx + dy * dy);

                    // 果実のサイズに応じた吸引合成判定距離
                    double threshold = 0;
                    if (p1->param_i[0] == 0) threshold = 6.0;   // 小玉同士
                    else if (p1->param_i[0] == 1) threshold = 12.0; // 中玉同士
                    else if (p1->param_i[0] == 2) threshold = 25.0; // 大玉同士

                    if (dist < threshold) {
                        // 合成効果音（進化レベルに応じて変化）
                        if (p1->param_i[0] == 0) {
                            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                        }
                        else if (p1->param_i[0] == 1) {
                            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
                        }
                        else {
                            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
                        }

                        // 座標と速度の平均化（運動量保存を模倣）
                        p1->x = (p1->x + p2->x) / 2.0;
                        p1->y = (p1->y + p2->y) / 2.0;
                        p1->param_d[0] = (p1->param_d[0] + p2->param_d[0]) / 2.0;
                        p1->param_d[1] = (p1->param_d[1] + p2->param_d[1]) / 2.0;
                        p1->param_i[0]++; // レベルアップ

                        // 見た目の変更（メロン・スイカは緑色に）
                        int color = (p1->param_i[0] >= 2) ? 2 : 0;
                        switch (p1->param_i[0]) {
                        case 1: p1->kind = img_enemyShotMediumBall[color]; break; // イチゴ
                        case 2: p1->kind = img_enemyShotLargeBall[color]; break;  // メロン
                        case 3: p1->kind = img_enemyShotLargeBall[color]; break;  // スイカ(移動中)
                        }

                        p2->param_i[1] = -1; // 合成された方を消去予約
                        break; // p1は合成済みなので内側ループを抜けて次のp1へ
                    }
                }
                p2 = p2->next;
            }
        }
        p1 = p1->next;
    }

    // 移動・重力・停止処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 消去予約されている弾はスキップ（メインルーチンが消去してくれます）
        if (pShot->param_i[1] == -1) {
            pShot->margin = -9999;
            pShot = pShot->next;
            continue;
        }

        // 停止スイカ（レベル4）の処理
        if (pShot->param_i[0] == 4) {
            pShot->param_i[2]--; // 寿命を減らす
            if (pShot->param_i[2] <= 0) {
                pShot->param_i[1] = -1; // 寿命切れで消去予約
            }
            pShot = pShot->next;
            continue;
        }

        // 重力加速度（レベルが高いほど重くする）
        double gravity = 0.03;
        if (pShot->param_i[0] == 2) gravity = 0.05;
        if (pShot->param_i[0] == 3) gravity = 0.08;
        pShot->param_d[1] += gravity;

        // 移動
        pShot->x += pShot->param_d[0];
        pShot->y += pShot->param_d[1];

        // 左右の壁バウンド（スイカゲームの箱をイメージ）
        double radius = 1.25;
        if (pShot->param_i[0] == 1) radius = 3.5;
        if (pShot->param_i[0] >= 2) radius = 10.0;

        if (pShot->x - radius < 0.0) {
            pShot->x = radius;
            pShot->param_d[0] = fabs(pShot->param_d[0]) * 0.8; // 反発と減衰
        }
        if (pShot->x + radius > 480.0) {
            pShot->x = 480.0 - radius;
            pShot->param_d[0] = -fabs(pShot->param_d[0]) * 0.8;
        }

        // スイカ（レベル3）が画面下部に到達したら、複数弾による停止スイカ（レベル4）に変化
        if (pShot->param_i[0] == 3 && pShot->y > 480.0 - count / 5.0) {
            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            pShot->param_i[1] = -1; // 移動中のスイカを消去予約

            // スイカの見た目を大玉7個で表現（中心1個＋六角形に6個）
            double baseX = pShot->x;
            double baseY = pShot->y; // 画面下部に接地するY座標(大玉の半径10を考慮)

            // 壁にめり込まないようにクランプ
            if (baseX < 20.0) baseX = 20.0;
            if (baseX > 460.0) baseX = 460.0;

            double offsets[7][2] = {
                {  0.0,   0.0},
                { 20.0,   0.0},
                { 10.0,  17.3},
                {-10.0,  17.3},
                {-20.0,   0.0},
                {-10.0, -17.3},
                { 10.0, -17.3}
            };

            for (int i = 0; i < 7; i++) {
                sEnemyShot* pNew = new sEnemyShot;
                pNew->x = baseX + offsets[i][0];
                pNew->y = baseY + offsets[i][1];
                pNew->param_d[0] = 0;
                pNew->param_d[1] = 0;
                pNew->speed = 0;
                pNew->muki = 0;
                pNew->kind = img_enemyShotLargeBall[2]; // 緑の大玉
                pNew->param_i[0] = 4; // レベル4（停止スイカ構成弾）
                pNew->param_i[1] = 1; // 生存
                pNew->param_i[2] = 99999; // 寿命（10秒後に自然消滅して画面が埋まるのを防止）

                pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pNew->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                pEnemyShotSet->pEnemyShotHead->prev = pNew;
            }
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_SuikaGame_Zai()
{
    static int muki;
    static int isSetCreated;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        isSetCreated = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 全ての弾を1つのSetで管理し、Set同士で合成できない問題を解消
    if (count == 60 && isSetCreated == 0) {
        isSetCreated = 1;
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotWatermelonCascade;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}