// enemyPat_Tmp.cpp
// 弾幕パターン：「ニュートンの落花（Flower of Kent）」難易度強化版
// ケントの花（ニュートンのリンゴ）をモチーフに、
// 花が咲く → 花弁が激しく散る → 実が連続で落ちる を既存弾の組み合わせで表現。
// 使用弾：中楕円弾（花弁）、小玉（芯・花粉・追加散弾）、大玉（落果）、菱形弾（補助散弾）

#include "gv.h"  // 必要に応じてプロジェクトのヘッダに合わせて調整

// 使用可能な効果音: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge
// 弾種類: SmallBall, MediumBall, LargeBall, Bullet, Scale, Diamond, MediumOval, Laser
// 色index: 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙

// ============================================================
// 花弁・芯・散り・落果を管理する共通パターン関数（高密度版）
// param_i[0] : 弾の役割 (0=花弁, 1=芯, 2=花粉/小弾, 3=落果リンゴ, 4=追加散弾)
// param_d[0] : vx (主に落果用)
// param_d[1] : vy (主に落果用)
// param_d[2] : 補助（重力や回転量など）
// ============================================================
static void ShotFlowerOfKent(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // --- 生成フェーズ (count == 0) : 二重の花を一気に展開 ---
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        const double baseAngle = pEnemyShotSet->muki;
        const int petalCount = 7;          // 5弁から7弁へ増加
        const int layerCount = 2;          // 二重花

        // 1. 花弁（中楕円弾・白/赤/マゼンタ混在）二重に展開
        for (int layer = 0; layer < layerCount; layer++) {
            double radius = 8.0 + layer * 14.0;
            double speedBase = 0.9 + layer * 0.4;
            for (int i = 0; i < petalCount; i++) {
                pEnemyShot = new sEnemyShot;
                double ang = baseAngle + (DX_PI * 2.0 * i) / petalCount + layer * 0.22;

                pEnemyShot->x = pEnemyShotSet->x + radius * cos(ang);
                pEnemyShot->y = pEnemyShotSet->y + radius * sin(ang);
                pEnemyShot->muki = ang;
                pEnemyShot->speed = speedBase;

                // 色を層と位置で変化（白・赤・マゼンタ）
                int col;
                if (layer == 0) col = (i % 2 == 0) ? 6 : 0;
                else            col = (i % 3 == 0) ? 5 : 6;
                pEnemyShot->kind = img_enemyShotMediumOval[col];

                pEnemyShot->param_i[0] = 0;               // 花弁
                pEnemyShot->param_d[0] = 0.0;
                pEnemyShot->param_d[1] = 0.0;
                pEnemyShot->param_d[2] = (i % 2 == 0) ? 0.012 : -0.012; // 回転散り用

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 2. 花の芯（小玉・白）少し厚めに
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(6) - 3);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(6) - 3);
            pEnemyShot->muki = baseAngle + (GetRand(30) - 15) / 180.0 * DX_PI;
            pEnemyShot->speed = 0.5 + i * 0.15;
            pEnemyShot->kind = img_enemyShotSmallBall[6];
            pEnemyShot->param_i[0] = 1;
            pEnemyShot->param_d[0] = 0.0;
            pEnemyShot->param_d[1] = 0.0;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 3. 初期花粉・小弾を多めに撒く
        for (int i = 0; i < 16; i++) {
            pEnemyShot = new sEnemyShot;
            double ang = baseAngle + (DX_PI * 2.0 * i) / 16.0 + (GetRand(24) - 12) / 180.0 * DX_PI;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(16) - 8;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(16) - 8;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 1.6 + GetRand(120) / 100.0;
            pEnemyShot->kind = img_enemyShotSmallBall[(i % 2 == 0) ? 6 : 0];
            pEnemyShot->param_i[0] = 2;
            pEnemyShot->param_d[0] = 0.0;
            pEnemyShot->param_d[1] = 0.0;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 中盤追加散弾（開花が広がった頃に菱形弾を追加で撒く） ---
    if (pEnemyShotSet->count == 28) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 12; i++) {
            pEnemyShot = new sEnemyShot;
            double ang = pEnemyShotSet->muki + (DX_PI * 2.0 * i) / 12.0 + (GetRand(20) - 10) / 180.0 * DX_PI;
            pEnemyShot->x = pEnemyShotSet->x + 20.0 * cos(ang);
            pEnemyShot->y = pEnemyShotSet->y + 20.0 * sin(ang);
            pEnemyShot->muki = ang + (GetRand(40) - 20) / 180.0 * DX_PI;
            pEnemyShot->speed = 2.2 + GetRand(100) / 100.0;
            pEnemyShot->kind = img_enemyShotDiamond[(i % 3 == 0) ? 5 : 6]; // マゼンタ/白
            pEnemyShot->param_i[0] = 4;
            pEnemyShot->param_d[0] = 0.0;
            pEnemyShot->param_d[1] = 0.0;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 落果フェーズ（複数波で連続落下） ---
    if (pEnemyShotSet->count == 40 || pEnemyShotSet->count == 55 || pEnemyShotSet->count == 70) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int appleCount = 4 + (pEnemyShotSet->count / 20); // 徐々に増やす
        for (int i = 0; i < appleCount; i++) {
            pEnemyShot = new sEnemyShot;
            // 横に広めにばらけさせる
            pEnemyShot->x = pEnemyShotSet->x + (i - appleCount / 2.0) * 22.0 + (GetRand(24) - 12);
            pEnemyShot->y = pEnemyShotSet->y - 10.0 - GetRand(8);
            double vx = (GetRand(100) - 50) / 80.0;
            double vy = 1.1 + GetRand(60) / 100.0;
            pEnemyShot->muki = atan2(vy, vx);
            pEnemyShot->speed = 0.0;
            int col = (i % 3 == 0) ? 8 : 0; // 橙/赤
            pEnemyShot->kind = img_enemyShotLargeBall[col];
            pEnemyShot->param_i[0] = 3;
            pEnemyShot->param_d[0] = vx;
            pEnemyShot->param_d[1] = vy;
            pEnemyShot->param_d[2] = 0.055 + GetRand(20) / 1000.0; // やや強めの重力

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 毎フレームの移動更新 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int role = pShot->param_i[0];

        if (role == 3) {
            // 落果：強めの重力付き放物線
            pShot->param_d[1] += pShot->param_d[2];
            pShot->x += pShot->param_d[0];
            pShot->y += pShot->param_d[1];
            pShot->muki = atan2(pShot->param_d[1], pShot->param_d[0]);
        }
        else {
            // 花弁・芯・花粉・追加散弾
            if (role == 0) {
                // 花弁：開花加速＋回転散り
                if (pEnemyShotSet->count < 35) {
                    pShot->speed += 0.055;
                }
                else {
                    // 散り始め：角度を継続的に回転させて隙間を埋める
                    pShot->muki += pShot->param_d[2];
                    pShot->speed += 0.028;
                }
            }
            else if (role == 1) {
                // 芯
                if (pEnemyShotSet->count > 15) pShot->speed += 0.025;
            }
            else if (role == 2) {
                // 花粉：早めに加速
                pShot->speed += 0.018;
            }
            else if (role == 4) {
                // 追加散弾：さらに加速
                pShot->speed += 0.022;
            }

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体パターン
// 関数名は指定通り void EnemyPat_FlowerOfKent_Grok()
// ============================================================
void EnemyPat_FlowerOfKent_Grok()
{
    static int muki;
    static int shotPhase;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shotPhase = 0;
    }
    else {
        // 左右にしっかり移動
        enemy.x += 1.15 * (double)muki;
        if (enemy.x < 70.0)  muki = 1;
        if (enemy.x > 410.0) muki = -1;

        // 上下もやや大きめに動く
        if (count % 70 < 35) enemy.y += 0.28;
        else                 enemy.y -= 0.28;
        if (enemy.y < 35.0)  enemy.y = 35.0;
        if (enemy.y > 120.0) enemy.y = 120.0;
    }

    // 攻撃間隔を短縮し、序盤から高密度に
    // 最初の数回は短め、以降も比較的頻繁に
    int interval;
    if (count < 100)      interval = 55;
    else if (count < 300) interval = 70;
    else                  interval = 85;

    if (count % interval == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFlowerOfKent;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 14.0;
        // 自機方向を基本に、少しランダムで狙いをずらす
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x)
            + (GetRand(50) - 25) / 180.0 * DX_PI;
        pEnemyShotSet->kind = shotPhase++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}