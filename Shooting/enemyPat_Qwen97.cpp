// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾の画像ハンドルを取得するヘルパー関数
// typeIdx: 0=小玉, 1=中玉, 2=鱗弾, 3=菱形弾, 4=中楕円弾
// colorIdx: 0=赤, 1=黄, 2=緑, 3=シアン, 4=青, 5=マゼンタ, 6=白, 7=黒, 8=橙
static int GetKindImage(int typeIdx, int colorIdx) {
    switch (typeIdx) {
    case 0: return img_enemyShotSmallBall[colorIdx];
    case 1: return img_enemyShotMediumBall[colorIdx];
    case 2: return img_enemyShotScale[colorIdx];
    case 3: return img_enemyShotDiamond[colorIdx];
    case 4: return img_enemyShotMediumOval[colorIdx];
    default: return img_enemyShotMediumBall[colorIdx];
    }
}

// 凍結中に発射される自機狙い弾（追撃）
static void ShotAimedFreeze(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // Lunaticらしく 5way または 6way で発射
        int way = 5 + GetRand(1);
        double baseMuki = pEnemyShotSet->muki;
        double spread = 15.0 / 180.0 * DX_PI; // 拡散角

        for (int i = 0; i < way; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = baseMuki - spread * (way - 1) / 2.0 + spread * i;
            pShot->speed = 3.5;
            pShot->kind = img_enemyShotMediumBall[4]; // 4:青色

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }
    else {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }

        // 一定時間経過後にパターン終了（メインルーチンで削除処理が行われる想定）
        if (pEnemyShotSet->count > 60) {
            // pEnemyShotSet->patternFunc = nullptr;
        }
    }
}

// 凍符「パーフェクトフリーズ」本体
static void ShotPerfectFreeze(sEnemyShotSet* pEnemyShotSet)
{
    // param_i[0]: 0=発射中, 1=凍結中, 2=融解・再始動中

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int numShots = 30 + GetRand(10) + 150; // 30〜40発
        for (int i = 0; i < numShots; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;

            // ランダムな角度
            double angle = GetRand(360) / 180.0 * DX_PI;
            pShot->muki = angle;
            pShot->speed = 2.0 + GetRand(100) / 100.0 * 4; // 2.0 〜 3.0

            // 種類と色のランダム選定（カラフルな弾）
            int typeIdx = GetRand(4); // 0〜4
            int colorIdx = GetRand(5); // 0(赤)〜5(マゼンタ)

            pShot->kind = GetKindImage(typeIdx, colorIdx);

            // 凍結・融解用に現在の状態を退避
            pShot->param_i[1] = typeIdx;
            pShot->param_i[2] = colorIdx;
            pShot->param_d[0] = pShot->speed;
            pShot->param_d[1] = pShot->muki;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
        pEnemyShotSet->param_i[0] = 0;
    }
    else if (pEnemyShotSet->count == 30) {
        // 【第2段階：凍結】
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->speed = 0.0; // 急停止
            pShot->kind = GetKindImage(pShot->param_i[1], 6); // 6:白 に変更
            pShot = pShot->next;
        }
        pEnemyShotSet->param_i[0] = 1;

        // 【第3段階：凍結中の自機狙い弾発射】
        sEnemyShotSet* pAimedSet = new sEnemyShotSet;
        pAimedSet->count = -1;
        pAimedSet->patternFunc = ShotAimedFreeze;
        pAimedSet->x = pEnemyShotSet->x;
        pAimedSet->y = pEnemyShotSet->y + 10.0;
        pAimedSet->muki = atan2(player.y - pAimedSet->y, player.x - pAimedSet->x);
        pAimedSet->kind = 0;
        pAimedSet->pEnemyShotHead = new sEnemyShot;
        pAimedSet->pEnemyShotHead->prev = pAimedSet->pEnemyShotHead;
        pAimedSet->pEnemyShotHead->next = pAimedSet->pEnemyShotHead;
        pAimedSet->prev = enemyShotSetHead.prev;
        pAimedSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pAimedSet;
        enemyShotSetHead.prev = pAimedSet;
    }
    else if (pEnemyShotSet->count == 90) {
        // 【第4段階：融解・再始動】
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            // 元の速度の60%程度でじわーっと動き出す
            pShot->speed = pShot->param_d[0] * 0.6;
            // 角度を±20度程度ランダムにずらして不規則性を出す
            pShot->muki = pShot->param_d[1] + (GetRand(40) - 20) / 180.0 * DX_PI;
            // 色を元に戻す
            pShot->kind = GetKindImage(pShot->param_i[1], pShot->param_i[2]);
            pShot = pShot->next;
        }
        pEnemyShotSet->param_i[0] = 2;
    }
    else if (pEnemyShotSet->count > 150) {
        // パターン終了
        //pEnemyShotSet->patternFunc = nullptr;
    }

    // 移動処理 (フェーズ0と2のみ実行)
    if (pEnemyShotSet->param_i[0] == 0 || pEnemyShotSet->param_i[0] == 2) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->speed > 0.0) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pShot->next;
        }
    }
}

// 敵本体のパターン
void EnemyPat_PerfectFreeze_Qwen()
{
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 180.0;
        enemy.maxHp = enemy.hp = 200;
        shot_count = 0;
    }
    else {
        // 左右にゆっくり滑らかに往復
        enemy.x += 1.5 * sin(count / 60.0 * DX_PI);
    }

    // 120フレーム(約2秒)周期でスペルカード発動
    if (count % 120 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPerfectFreeze;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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