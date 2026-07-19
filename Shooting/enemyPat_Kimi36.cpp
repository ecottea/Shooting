// enemyPat_popcorn.cpp
// ポップコーンモチーフ弾幕「爆裂バターシャワー」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 弾幕パターン：ポップコーン「爆裂バターシャワー」
// ------------------------------------------------------------
// 【使った素材】
//   ・img_enemyShotSmallBall[7]  … 小玉・黒色（硬いカーネル）
//   ・img_enemyShotSmallBall[6]  … 小玉・白色（ふわふわポップコーン）
//   ・img_enemyShotMediumBall[6] … 中玉・白色（大きめのポップコーン）
//   ・img_enemyShotSmallBall[1]  … 小玉・黄色（溶けたバター）
//   ・sound_enemyShot_medium     … 破裂のインパクト音
//
// 【3段階の動き】
//   第1段階：カーネル（黒・小玉）をランダム方向に発射
//   第2段階：一定時間後にカーネルが爆裂し、扇状にポップコーン（白）を分裂
//   第3段階：ポップコーンからバター（黄・小玉）が降下し、画面下端で滞留
// ------------------------------------------------------------
static void ShotPopcorn(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // --- 初回フレーム：カーネル（未破裂の豆）を生成 ---
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int kernelCount = 6 + GetRand(4); // 6〜10個
        for (int i = 0; i < kernelCount; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(20) - 10;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(20) - 10;
            // 下方向中心にランダム拡散
            pEnemyShot->muki = DX_PI / 2.0 + (GetRand(200) - 100) / 180.0 * DX_PI;
            pEnemyShot->speed = (180 + GetRand(220)) / 100.0; // 1.8〜4.0

            // 小玉・黒色 = 硬いカーネル
            pEnemyShot->kind = img_enemyShotSmallBall[7];

            // param_i[0]: 状態識別子 (0=カーネル, 1=ポップコーン, 2=バター降下中, 3=バター滞留)
            pEnemyShot->param_i[0] = 0;
            // param_i[1]: 破裂までのフレーム数
            pEnemyShot->param_i[1] = 25 + GetRand(35); // 25〜60フレーム
            // param_i[2]: 分裂するポップコーンの数
            pEnemyShot->param_i[2] = 4 + GetRand(3);   // 4〜6個

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 弾の更新処理 ---
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pEnemyShot->next; // リスト変更対策

        if (pEnemyShot->param_i[0] == 0) {
            // ===== 第1段階：カーネル =====
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

            // 破裂判定
            if (pEnemyShot->count >= pEnemyShot->param_i[1]) {
                double popX = pEnemyShot->x;
                double popY = pEnemyShot->y;

                // カーネルを画面外に飛ばし、メインルーチンの画面外判定で消去されるようにする
                pEnemyShot->y = -100.0;
                pEnemyShot->speed = 0;

                // ポップコーン弾を生成
                int nPop = pEnemyShot->param_i[2];
                double spread = DX_PI / 3.0; // 扇の広さ：60度

                for (int j = 0; j < nPop; j++) {
                    sEnemyShot* pPop = new sEnemyShot;
                    pPop->x = popX;
                    pPop->y = popY;

                    // 扇状に広がる方向（下方向中心）
                    double angleOffset = -spread / 2.0 + spread * j / (nPop - 1);
                    pPop->muki = DX_PI / 2.0 + angleOffset + (GetRand(20) - 10) / 180.0 * DX_PI;
                    pPop->speed = (120 + GetRand(180)) / 100.0; // 1.2〜3.0

                    // 小玉または中玉・白色 = ふわふわポップコーン
                    if (GetRand(1) == 0) {
                        pPop->kind = img_enemyShotSmallBall[6];
                    }
                    else {
                        pPop->kind = img_enemyShotMediumBall[6];
                    }

                    pPop->param_i[0] = 1;                    // 状態：ポップコーン
                    pPop->param_i[1] = 15 + GetRand(25);     // バター生成までのフレーム数
                    pPop->param_i[2] = 0;
                    pPop->param_i[3] = 0;

                    pPop->param_d[0] = 0; // 重力用
                    pPop->param_d[1] = 0;
                    pPop->param_d[2] = 0;
                    pPop->param_d[3] = 0;

                    pPop->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pPop->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pPop;
                    pEnemyShotSet->pEnemyShotHead->prev = pPop;
                }
            }
        }
        else if (pEnemyShot->param_i[0] == 1) {
            // ===== 第2段階：ポップコーン =====
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

            // 空気抵抗で少し減速
            pEnemyShot->speed *= 0.995;

            // バター弾を生成（1回だけ）
            if (pEnemyShot->count >= pEnemyShot->param_i[1]) {
                sEnemyShot* pButter = new sEnemyShot;
                pButter->x = pEnemyShot->x;
                pButter->y = pEnemyShot->y;
                pButter->muki = DX_PI / 2.0; // 下向き
                pButter->speed = (50 + GetRand(100)) / 100.0; // 0.5〜1.5

                // 小玉・黄色 = バター
                pButter->kind = img_enemyShotSmallBall[1];

                pButter->param_i[0] = 2; // 状態：バター降下中
                pButter->param_i[1] = 0;
                pButter->param_i[2] = 0;
                pButter->param_i[3] = 0;

                pButter->param_d[0] = 0; // 重力加速度蓄積
                pButter->param_d[1] = 0;
                pButter->param_d[2] = 0;
                pButter->param_d[3] = 0;

                pButter->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pButter->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pButter;
                pEnemyShotSet->pEnemyShotHead->prev = pButter;

                // 重複生成防止
                pEnemyShot->param_i[1] = 99999;
            }
        }
        else if (pEnemyShot->param_i[0] == 2) {
            // ===== 第3段階：バター降下 =====
            pEnemyShot->param_d[0] += 0.03; // 重力加速
            pEnemyShot->y += pEnemyShot->speed + pEnemyShot->param_d[0];

            // 画面下端で滞留開始
            if (pEnemyShot->y > 460.0) {
                pEnemyShot->y = 460.0;
                pEnemyShot->param_i[0] = 3; // 状態：滞留
            }
        }
        else if (pEnemyShot->param_i[0] == 3) {
            // ===== 第3段階：バター滞留 =====
            // 少し左右に揺れながら滞留
            pEnemyShot->x += sin(pEnemyShot->count * 0.05) * 0.3;
        }

        pEnemyShot = pNext;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_Popcorn_Kimi()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右にゆっくり移動
        enemy.x += 0.8 * (double)muki;
        if (count % 150 == 75) muki *= -1;

        // 画面端での跳ね返り
        if (enemy.x < 40.0) { enemy.x = 40.0; muki = 1; }
        if (enemy.x > 440.0) { enemy.x = 440.0; muki = -1; }
    }

    // 80フレーム毎にポップコーン弾幕セットを生成
    if (count % 80 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPopcorn;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 15.0;
        pEnemyShotSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);
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