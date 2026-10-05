#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾の追加処理を簡略化するヘルパー関数
static sEnemyShot* CreateShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind, int type = 0)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->param_i[0] = type; // 0: 通常弾, 1: サボテン胴体パーツ弾

    // リスト末尾に追加
    pEnemyShot->prev = pSet->pEnemyShotHead->prev;
    pEnemyShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pEnemyShot;
    pSet->pEnemyShotHead->prev = pEnemyShot;

    return pEnemyShot;


}

// 弾幕：開花仙人掌（フラワー・カクタス）
static void ShotFlowerCactus(sEnemyShotSet* pEnemyShotSet)
{
    // ============================================================
    // 1. 各フェーズでの弾生成 logic
    // ============================================================

    // 【フェーズ1：サボテンの配置（設置）】
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        double base_x = pEnemyShotSet->x;
        double base_y = pEnemyShotSet->y;

        // 素材割り当て: 2:緑, 5:マゼンタ, 1:黄
        int img_stem = img_enemyShotLargeBall[2];  // 主茎(緑大玉)
        int img_arm = img_enemyShotMediumBall[2]; // 枝(緑中玉)

        // --- 主茎の作成 (縦に3個) ---
        CreateShot(pEnemyShotSet, base_x, base_y + 30.0, 0, 0.0, img_stem, 1);
        CreateShot(pEnemyShotSet, base_x, base_y + 58.0, 0, 0.0, img_stem, 1);
        CreateShot(pEnemyShotSet, base_x, base_y + 86.0, 0, 0.0, img_stem, 1);

        // --- 左枝の作成 (L字型に2個) ---
        CreateShot(pEnemyShotSet, base_x - 24.0, base_y + 58.0, 0, 0.0, img_arm, 1);
        CreateShot(pEnemyShotSet, base_x - 24.0, base_y + 40.0, 0, 0.0, img_arm, 1);

        // --- 右枝の作成 (L字型に2個) ---
        CreateShot(pEnemyShotSet, base_x + 24.0, base_y + 58.0, 0, 0.0, img_arm, 1);
        CreateShot(pEnemyShotSet, base_x + 24.0, base_y + 40.0, 0, 0.0, img_arm, 1);
    }

    // 【フェーズ2：開花（ピンクの花びら拡散）】
    if (pEnemyShotSet->count == 45) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int img_flower = img_enemyShotSmallBall[5]; // 花(マゼンタ小玉)
        double flower_x = pEnemyShotSet->x;
        double flower_y = pEnemyShotSet->y + 30.0; // 主茎の最上部

        int way_count = 16*3;
        double base_angle = (GetRand(360) / 360.0) * DX_PI * 2.0; // ランダムな初期角度 offset

        for (int i = 0; i < way_count; i++) {
            double angle = base_angle + (DX_PI * 2.0 / way_count) * i;
            CreateShot(pEnemyShotSet, flower_x, flower_y, angle, 1.6, img_flower, 0);
        }
    }

    // 【フェーズ3：トゲ一斉射出 (第1波 & 第2波)】
    if (pEnemyShotSet->count >= 85 && pEnemyShotSet->count <= 115+100 && (pEnemyShotSet->count - 85) % 15 == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int img_needle = img_enemyShotBullet[1]; // トゲ(黄銃弾)
        double angle_offset = (pEnemyShotSet->count - 85) * (DX_PI / 8.0) / 30; // 2波目で角度をずらして格子模様化

        // 現在存在する「サボテンの胴体パーツ弾」の全位置から放射状に針弾を発射
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            // 次の要素を保持（ループ維持用）
            sEnemyShot* pNext = pShot->next;

            if (pShot->param_i[0] == 1) { // 胴体パーツである場合
                for (int i = 0; i < 4; i++) {
                    double needle_angle = angle_offset + (DX_PI / 2.0) * i + (DX_PI / 4.0);
                    CreateShot(pEnemyShotSet, pShot->x, pShot->y, needle_angle, 2.2, img_needle, 0);
                }
            }
            pShot = pNext;
        }
    }

    // 【サボテン胴体パーツの消去】
    if (pEnemyShotSet->count == 145+100) {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* pNext = pShot->next;

            if (pShot->param_i[0] == 1) { // 胴体パーツを消去
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
            pShot = pNext;
        }
    }

    // ============================================================
    // 2. 毎フレームの移動処理
    // ============================================================
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 胴体パーツ(param_i[0] == 1)は speed = 0 なので静止し、それ以外の弾は移動
        if (pShot->speed > 0.0) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }


}

// 敵本体のパターン
void EnemyPat_Cactus_Gemini()
{
    static int muki, T, R;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        T = 180;
        R = 30;
    }
    else {
        // 左右にゆったり移動
        enemy.x += 0.8 * (double)muki;
        if (count % 160 == 80) {
            muki *= -1;
        }
    }
    R--;

    // 180フレーム（約3秒）サイクルでサボテン弾幕を1回配置
    if (R == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFlowerCactus;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // リスト初期化と挿入
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;

        R = T;
        T -= 30;
        T = max(T, 60);
    }


}