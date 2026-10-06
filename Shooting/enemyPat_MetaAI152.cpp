#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// ミュラーリヤー共通処理
// isOutward = true : <----> 外向、長く見える
// isOutward = false : >----< 内向、短く見える
// param_d[0] : 実際の長さ L
// param_d[1] : 落下速度
// pShot->param_i[0] : 0=軸 1=矢羽
// ------------------------------------------------------------
static void ShotMullerLyer_Common(sEnemyShotSet* pSet, bool isOutward)
{
    const double FALL_SPEED = pSet->param_d[1] > 0.0 ? pSet->param_d[1] : 1.1;
    const double L = pSet->param_d[0] > 0.0 ? pSet->param_d[0] : 160.0;
    const double half = L * 0.5;

    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double cx = pSet->x;
        double cy = pSet->y;

        // --- 軸: 同じ長さで21発 ---
        const int shaftNum = 17;
        for (int i = 0; i < shaftNum; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double t = (double)i / (shaftNum - 1);
            pShot->x = cx - half + L * t;
            pShot->y = cy;
            pShot->muki = DX_PI * 0.5; // 見た目は下だが移動は手動で行う
            pShot->speed = 0.0;
            pShot->count = 0;
            pShot->kind = img_enemyShotMediumBall[6]; // 白 中玉 7.0x7.0
            pShot->param_i[0] = 0; // 軸
            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }

        // 矢羽の角度定義
        // 外向 <----> : 左135°,225° 右45°,315°
        // 内向 >----< : 左45°,315° 右135°,225°
        double leftA1, leftA2, rightA1, rightA2;
        if (isOutward) {
            leftA1 = DX_PI * 0.75; // 135°
            leftA2 = DX_PI * 1.25; // 225°
            rightA1 = DX_PI * 0.25; // 45°
            rightA2 = DX_PI * 1.75; // 315° (-45°)
        }
        else {
            leftA1 = DX_PI * 0.25;
            leftA2 = DX_PI * 1.75;
            rightA1 = DX_PI * 0.75;
            rightA2 = DX_PI * 1.25;
        }
        const double wingAngles[4] = { leftA1, leftA2, rightA1, rightA2 };
        const double endX[4] = { cx - half, cx - half, cx + half, cx + half };

        const int wingLen = 4+2; // 1つの羽根あたり4発
        const double step = 9.0;

        for (int w = 0; w < 4; w++) {
            for (int j = 1; j <= wingLen; j++) for (int k = 0; k < 3; k++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = endX[w] + cos(wingAngles[w]) * j * step;
                pShot->y = cy + sin(wingAngles[w]) * j * step;
                pShot->muki = wingAngles[w]; // 銃弾の向き = 矢羽の向き
                pShot->speed = 0.0; // 90Fまでは落下のみ
                pShot->count = 0;
                pShot->kind = img_enemyShotMediumBall[6]; // 白 銃弾 5.0x2.0 向き依存
                pShot->param_i[0] = 1; // 矢羽
                pShot->param_d[0] = wingAngles[w]; // 元角度保存
                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // --- 移動 ---
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pSet->count < 90) {
            // 錯視フェーズ: 全体が同じ速度で落下、フォーメーション維持
            pShot->y += FALL_SPEED;
        }
        else {
            if (pShot->param_i[0] == 0) {
                // 軸は落下し続ける
                pShot->y += FALL_SPEED;
            }
            else {
                // 矢羽は90F目に射出、錯視が実弾化
                if (pShot->count == 90) { // 弾側countが90F相当を超えた初回
                    // pSet->count==90のタイミングで弾はまだspeed 0なのでここで付与
                    pShot->speed = 2.1 + (GetRand(100) - 50) * 0.03;
                    pShot->muki += (GetRand(100) - 50) * 0.012;
                }
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
        }
        pShot = pShot->next;
    }
}

static void ShotMullerLyer_Out(sEnemyShotSet* pSet) { ShotMullerLyer_Common(pSet, true); }
static void ShotMullerLyer_In(sEnemyShotSet* pSet) { ShotMullerLyer_Common(pSet, false); }

// ------------------------------------------------------------
// 敵本体
// ------------------------------------------------------------
void EnemyPat_MullerLyer_MetaAI()
{
    static int muki;
    static int wave;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        wave = 0;
    }
    else {
        enemy.x += 0.9 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 180Fごとに2本ペアで出現、上下60px間隔で比較させる
    if (count % 180 == 1) {
        const double L = 160.0;
        const double fall = 1.1;

        for (int k = 0; k < 2; k++) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->x = 240.0; // 画面中央揃えで長さ比較
            pSet->y = enemy.y + 20.0 + k * 70.0;
            pSet->muki = DX_PI * 0.5;
            pSet->param_d[0] = L;
            pSet->param_d[1] = fall;
            pSet->kind = k; // 0=外向 1=内向
            pSet->patternFunc = (k == 0) ? ShotMullerLyer_Out : ShotMullerLyer_In;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
        wave++;
    }
}