// enemyPat_tmp.cpp
// スイミーの大魚陣形 - 小さな黒い魚たちが群れで巨大な魚を形成し、追跡・崩壊する

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// スイミーの大魚陣形
// Phase 0 (0-119)  : 散開 - 四隅から小魚が集まってくる
// Phase 1 (120-239): 集結 - 魚の形を形成
// Phase 2 (240-479): 遊泳 - プレイヤー追跡、鱗が剥がれ落ちる
// Phase 3 (480-  ) : 崩壊 - コアが高速突進、残りは散開
static void ShotSwimmyBigFish(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    const int PHASE_SEED = 0;   // 散開
    const int PHASE_GATHER = 1;   // 集結
    const int PHASE_CRUISE = 2;   // 遊泳
    const int PHASE_COLLAPSE = 3;   // 崩壊

    int& phase = pEnemyShotSet->param_i[0];
    int& scaleTotal = pEnemyShotSet->param_i[1];   // 生成した鱗の総数
    double& fishX = pEnemyShotSet->param_d[0];   // 魚の中心X
    double& fishY = pEnemyShotSet->param_d[1];   // 魚の中心Y
    double& fishAng = pEnemyShotSet->param_d[2];   // 魚の向き
    double& fishSpd = pEnemyShotSet->param_d[3];   // 魚の移動速度

    // ===== 初期化 =====
    if (pEnemyShotSet->count == 0) {
        phase = PHASE_SEED;
        scaleTotal = 0;
        fishX = 240.0;
        fishY = 200.0;
        fishAng = DX_PI / 2.0;   // 下向き
        fishSpd = 0.0;

        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ===== フェーズ遷移 =====
    if (phase == PHASE_SEED && pEnemyShotSet->count >= 120) {
        phase = PHASE_GATHER;
    }
    else if (phase == PHASE_GATHER && pEnemyShotSet->count >= 240) {
        phase = PHASE_CRUISE;
        fishSpd = 0.9;
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }
    else if (phase == PHASE_CRUISE && pEnemyShotSet->count >= 480) {
        phase = PHASE_COLLAPSE;
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // ===== Phase 0: 散開（Seed）=====
    // 四隅から小さな黒い弾が中央へ向かって泳ぐ
    if (phase == PHASE_SEED && pEnemyShotSet->count < 120) {
        for (int i = 0; i < 2; i++) {
            pEnemyShot = new sEnemyShot;

            // 四隅から出現 (GetRand(3) は 0〜3 の4種類)
            int corner = GetRand(3);
            switch (corner) {
            case 0: pEnemyShot->x = GetRand(30);           pEnemyShot->y = GetRand(30);           break;
            case 1: pEnemyShot->x = 480 - GetRand(30);     pEnemyShot->y = GetRand(30);           break;
            case 2: pEnemyShot->x = GetRand(30);           pEnemyShot->y = 480 - GetRand(30);     break;
            case 3: pEnemyShot->x = 480 - GetRand(30);     pEnemyShot->y = 480 - GetRand(30);     break;
            }

            // 中央付近を目指す
            double targetX = 240.0 + GetRand(40) - 20.0;
            double targetY = 200.0 + GetRand(40) - 20.0;
            pEnemyShot->muki = atan2(targetY - pEnemyShot->y, targetX - pEnemyShot->x);
            pEnemyShot->speed = 1.8 + GetRand(120) / 100.0;   // 1.8〜3.0

            // 小玉・黒色 (img_enemyShotSmallBall[7])
            pEnemyShot->kind = img_enemyShotSmallBall[7];
            pEnemyShot->param_i[0] = 0;            // 状態: 小魚（鱗）
            pEnemyShot->param_i[1] = scaleTotal;     // 魚の形でのインデックス
            pEnemyShot->margin = 480;

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            scaleTotal++;
        }
    }

    // ===== Phase 1: 集結（Gathering）=====
    // 各弾を魚の輪郭上の位置へ誘導
    if (phase == PHASE_GATHER) {
        int idx = 0;
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[0] == 0) {
                double t = (double)idx / scaleTotal * 2.0 * DX_PI;

                // 魚の輪郭（先端が右を向く楕円系）
                double rx = 65.0 * cos(t) * (1.0 + 0.3 * cos(t));
                double ry = 32.0 * sin(t);

                double targetX = fishX + rx * cos(fishAng) - ry * sin(fishAng);
                double targetY = fishY + rx * sin(fishAng) + ry * cos(fishAng);

                double dx = targetX - p->x;
                double dy = targetY - p->y;
                double dist = sqrt(dx * dx + dy * dy);

                if (dist > 3.0) {
                    p->muki = atan2(dy, dx);
                    p->speed = dist * 0.1 + 0.5;
                    p->x += p->speed * cos(p->muki);
                    p->y += p->speed * sin(p->muki);
                }
                else {
                    p->speed = 0;
                    p->x = targetX;
                    p->y = targetY;
                }

                // 先端の弾をコア（目）に変化
                if (idx == 0 && dist < 8.0) {
                    p->param_i[0] = 1;                     // コア状態
                    p->kind = img_enemyShotMediumBall[0];   // 中玉・赤色
                    p->speed = 0;
                }
                idx++;
            }
            p = p->next;
        }
    }

    // ===== Phase 2: 遊泳（Cruise）=====
    // 魚の形を保ってプレイヤーを追跡。一定間隔で鱗が剥がれ落ちる
    if (phase == PHASE_CRUISE) {
        // プレイヤー方向を滑らかに向く
        double dx = player.x - fishX;
        double dy = player.y - fishY;
        double targetAngle = atan2(dy, dx);
        double diff = targetAngle - fishAng;
        while (diff > DX_PI) diff -= 2.0 * DX_PI;
        while (diff < -DX_PI) diff += 2.0 * DX_PI;
        fishAng += diff * 0.025;

        // 蛇行
        fishAng += 0.035 * sin(pEnemyShotSet->count * 0.07);

        // 移動
        fishX += fishSpd * cos(fishAng);
        fishY += fishSpd * sin(fishAng);

        // 画面内に制限
        if (fishX < 100.0) fishX = 100.0;
        if (fishX > 380.0) fishX = 380.0;
        if (fishY < 100.0) fishY = 100.0;
        if (fishY > 380.0) fishY = 380.0;

        // 鱗が剥がれる（35フレームごとに1枚）
        if (pEnemyShotSet->count % 35 == 0) {
            sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
            while (p != pEnemyShotSet->pEnemyShotHead) {
                if (p->param_i[0] == 0) {   // まだ魚の一部
                    p->param_i[0] = 2;      // 剥がれた鱗（誘導弾化）
                    p->speed = 1.0;
                    double aim = atan2(player.y - p->y, player.x - p->x);
                    p->muki = aim + (GetRand(50) - 25) / 180.0 * DX_PI;

                    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
                    break;   // 1枚だけ
                }
                p = p->next;
            }
        }

        // 各弾の位置更新
        int idx = 0;
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[0] == 0) {   // 魚の鱗
                double t = (double)idx / scaleTotal * 2.0 * DX_PI;
                double rx = 65.0 * cos(t) * (1.0 + 0.3 * cos(t));
                double ry = 32.0 * sin(t);
                p->x = fishX + rx * cos(fishAng) - ry * sin(fishAng);
                p->y = fishY + rx * sin(fishAng) + ry * cos(fishAng);
                idx++;
            }
            else if (p->param_i[0] == 1) {   // コア（目）
                p->x = fishX + 85.0 * cos(fishAng);
                p->y = fishY + 85.0 * sin(fishAng);
            }
            else if (p->param_i[0] == 2) {   // 剥がれた鱗（誘導弾）
                double aim = atan2(player.y - p->y, player.x - p->x);
                double diff2 = aim - p->muki;
                while (diff2 > DX_PI) diff2 -= 2.0 * DX_PI;
                while (diff2 < -DX_PI) diff2 += 2.0 * DX_PI;
                p->muki += diff2 * 0.025;
                p->speed += 0.015;
                if (p->speed > 2.8) p->speed = 2.8;

                p->x += p->speed * cos(p->muki);
                p->y += p->speed * sin(p->muki);
            }
            p = p->next;
        }
    }

    // ===== Phase 3: 崩壊（Collapse）=====
    // コアが高速でプレイヤーに突進、残りはランダムに散開
    if (phase == PHASE_COLLAPSE) {
        sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
        while (p != pEnemyShotSet->pEnemyShotHead) {
            if (p->param_i[0] == 1) {   // コア
                double dx = player.x - p->x;
                double dy = player.y - p->y;
                p->muki = atan2(dy, dx);
                p->speed = 3.5;
                p->x += p->speed * cos(p->muki);
                p->y += p->speed * sin(p->muki);
            }
            else if (p->param_i[0] == 0 || p->param_i[0] == 2) {
                // ランダムに散開
                if (p->speed < 4.0) p->speed += 0.2;
                p->x += p->speed * cos(p->muki);
                p->y += p->speed * sin(p->muki);
            }
            p = p->next;
        }
    }
}

// 敵本体のパターン
void EnemyPat_Swimmy_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 開始後120フレームでスイミーの大魚陣形を1回だけ発動
    if (count % 600 == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSwimmyBigFish;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}