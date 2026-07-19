// enemyPat_Tmp.cpp
// 「新難題『金閣寺の一枚天井』」を再現した敵本体パターン

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：金閣寺の一枚天井（輝夜 / Level9-6）
// ============================================================
static void Shot_KinkakujiOnePlateCeiling(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回フレームのみ、天井弾と回転弾を生成
    if (pEnemyShotSet->count == 0) {
        // 効果音（中量弾の音）
        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 黄金の天井を模した横一列の黄色丸弾（左右交互）
        for (int i = 0; i < 15; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (i - 7) * 30.0; // 等間隔で横一列
            pEnemyShot->y = pEnemyShotSet->y - 80.0;           // 天井位置（敵より上）
            pEnemyShot->muki = DX_PI;                          // 真下方向
            pEnemyShot->speed = 1.5;
            pEnemyShot->kind = img_enemyShotSmallBall[1];      // 黄色（index 1）
            pEnemyShot->param_i[0] = (i % 2) * 2 - 1;          // 左右交互フラグ: -1 or 1
            pEnemyShot->margin = 100; // ※形崩れ防止

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 4方向バラ撒き回転弾（緑・水色・青・紫）の初期生成
        // param_i[1] で色種別を管理（0:緑, 1:水色, 2:青, 3:紫）
        for (int c = 0; c < 4; c++) {
            for (int i = 0; i < 36; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = (i * 10) * DX_PI / 180.0; // 10度刻みで全方向
                pEnemyShot->speed = 1.2 + c * 0.1;           // 色ごとに少し速度差
                pEnemyShot->param_i[0] = (c % 2 == 0) ? 1 : -1; // 回転方向: 1=時計, -1=反時計
                pEnemyShot->param_i[1] = c;                 // 色種別
                pEnemyShot->param_i[2] = 0;                 // 有効化フラグ（段階でONにする）

                // 色ごとの画像と速度調整
                switch (c) {
                case 0: // 緑（中速・時計回り）
                    pEnemyShot->kind = img_enemyShotSmallBall[2]; // 緑
                    pEnemyShot->speed = 1.8;
                    pEnemyShot->param_i[2] = 1; // 第一段階から有効
                    break;
                case 1: // 水色（反時計回り）
                    pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアン
                    pEnemyShot->speed = 2.0;
                    break;
                case 2: // 青（低速・時計回り）
                    pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青
                    pEnemyShot->speed = 1.5;
                    break;
                case 3: // 紫（中速・反時計回り）
                    pEnemyShot->kind = img_enemyShotSmallBall[5]; // マゼンタ
                    pEnemyShot->speed = 1.9;
                    break;
                }

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 毎フレームの弾移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        // 黄金天井弾（黄色）の左右振動
        if (pEnemyShot->kind == img_enemyShotSmallBall[1]) {
            pEnemyShot->x += 0.8 * pEnemyShot->param_i[0]; // 左右交互に揺らす
            pEnemyShot->y += pEnemyShot->speed;
        }
        // 回転バラ撒き弾（緑・水色・青・紫）
        else {
            // 段階ごとの有効化フラグをチェック
            int phase = pEnemyShotSet->param_i[0]; // 0〜3
            if (pEnemyShot->param_i[1] > phase) {
                // まだ有効化されていない弾は非表示扱い（画面外に退避）
                pEnemyShot->x = -1000.0;
                pEnemyShot->y = -1000.0;
            }
            else {
                // 有効な弾は回転＋放射移動
                double rotSpeed = 0.03 * pEnemyShot->param_i[0]; // 回転方向込み
                pEnemyShot->muki += rotSpeed;
                pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
                pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
            }
        }
        if (pEnemyShot->count >= 120 && pEnemyShot->kind == img_enemyShotSmallBall[2]) pEnemyShot->x = 9999; // ※プール枯渇防止
        pEnemyShot = pEnemyShot->next;
    }

    // 段階管理（撮影枚数に応じて param_i[0] を 0→1→2→3 に変化）
    // ここでは簡易的に count で段階を進める（実際の文花帖では撮影枚数に連動）
    if (pEnemyShotSet->count < 60) {
        pEnemyShotSet->param_i[0] = 0; // 第一段階（緑のみ）
    }
    else if (pEnemyShotSet->count < 120) {
        pEnemyShotSet->param_i[0] = 1; // 第二段階（緑＋水色）
    }
    else if (pEnemyShotSet->count < 180) {
        pEnemyShotSet->param_i[0] = 2; // 第三段階（緑＋水色＋青）
    }
    else {
        pEnemyShotSet->param_i[0] = 3; // 最終段階（全色）
    }
}

// ============================================================
// 敵本体パターン：金閣寺の一枚天井（輝夜）
// ============================================================
void EnemyPat_Kinkakuji_Sakana()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;      // 画面中央
        enemy.y = 80.0;       // やや上寄り
        enemy.maxHp = enemy.hp = 200;
        muki = 1;             // 右移動開始
        shot_count = 0;
    }
    else {
        // 敵の左右往復移動
        enemy.x += 1.2 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 10フレームごとに「金閣寺の一枚天井」弾幕セットを生成
    if (count % 10 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = Shot_KinkakujiOnePlateCeiling;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0; // 弾幕関数内で個別に制御
        pEnemyShotSet->kind = shot_count++; // 種類識別（任意）

        // 弾リストのダミーヘッド作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // グローバルリストに登録
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}