#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// ペアノ曲線の再帰的座標計算
// ------------------------------------------------------------
// level: 階層（3 なら 3x3 の再帰で 27x27=729 個の点）
// index: 0 〜 (9^level - 1) の通し番号
// x, y : 出力されるグリッド座標（0 〜 3^level - 1）
static void GetPeanoCoord(int index, int level, int& x, int& y) {
    if (level == 0) {
        x = 0; y = 0; return;
    }

    int pow9 = 1, pow3 = 1;
    for (int i = 0; i < level - 1; i++) {
        pow9 *= 9;
        pow3 *= 3;
    }

    int d = index / pow9;       // 現在のレベルにおける 3x3 ブロックの番号 (0〜8)
    int remain = index % pow9;  // 下位レベルに渡す残りのインデックス

    int cx, cy;
    GetPeanoCoord(remain, level - 1, cx, cy);

    // ペアノ曲線の規則に基づくブロックごとの反転処理
    // d = 1, 4, 7 のブロックは Y 軸方向に反転
    if (d == 1 || d == 4 || d == 7) {
        cy = (pow3 - 1) - cy;
    }
    // d = 3, 4, 5 のブロックは X 軸方向に反転
    if (d == 3 || d == 4 || d == 5) {
        cx = (pow3 - 1) - cx;
    }

    // ブロック番号に応じたオフセット (3x3グリッド)
    int ox = 0, oy = 0;
    switch (d) {
    case 0: ox = 0; oy = 0; break;
    case 1: ox = 0; oy = 1; break;
    case 2: ox = 0; oy = 2; break;
    case 3: ox = 1; oy = 2; break;
    case 4: ox = 1; oy = 1; break;
    case 5: ox = 1; oy = 0; break;
    case 6: ox = 2; oy = 0; break;
    case 7: ox = 2; oy = 1; break;
    case 8: ox = 2; oy = 2; break;
    }

    x = cx + ox * pow3;
    y = cy + oy * pow3;
}


// ------------------------------------------------------------
// 弾幕：空間充填「ペアノ・ドメイン」
// ------------------------------------------------------------
static void ShotPeanoDomain(sEnemyShotSet* pEnemyShotSet)
{
    int scount = pEnemyShotSet->count;

    // --------------------------------------------------------
    // フェーズ1・2：一筆書きの軌跡による弾の配置 (0〜364フレーム)
    // --------------------------------------------------------
    if (scount < 365) {
        // 毎フレーム2発ずつスポーン (全729発)
        for (int k = 0; k < 2; k++) {
            int idx = scount * 2 + k;
            if (idx >= 729) break;

            int px = 0, py = 0;
            GetPeanoCoord(idx, 3, px, py); // レベル3 (27x27グリッド)

            sEnemyShot* pEnemyShot = new sEnemyShot;
            // 画面中央(240, 240)を中心に、中心インデックス(13, 13)を基準として15ピクセル間隔で配置
            pEnemyShot->x = 240.0 + (px - 13) * 15.0;
            pEnemyShot->y = 240.0 + (py - 13) * 15.0;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotSmallBall[4]; // 予告線としての青小玉

            pEnemyShot->param_i[0] = idx; // 自身のペアノ軌道上の順番を記憶
            pEnemyShot->param_i[1] = 0;   // 状態フラグ (0:待機, 1:パルス点灯, 2:拡散)

            // リストに接続
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 配置中のSE（チッチッチッと時計のように鳴らす）
        if (scount % 10 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    // --------------------------------------------------------
    // フェーズ3：実体化・絶望感（一斉に危険な赤玉に変化）
    // --------------------------------------------------------
    if (scount == 400) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->kind = img_enemyShotMediumBall[0]; // 赤中玉へ変化
            pShot = pShot->next;
        }
    }

    // --------------------------------------------------------
    // フェーズ4：1次元位相走査（パルス波）と弾幕化
    // --------------------------------------------------------
    if (scount >= 460) {
        // パルスの先端インデックス（毎フレーム3マス分進む）
        int pulse_idx = (scount - 460) * 3;

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            int idx = pShot->param_i[0];
            int state = pShot->param_i[1];

            // ① パルス先端が到達
            if (state == 0 && idx <= pulse_idx) {
                pShot->kind = img_enemyShotLargeBall[6]; // 白大玉で発光
                pShot->param_i[1] = 1;
            }
            // ② パルス通過後（先端から15マス後方）
            else if (state == 1 && idx <= pulse_idx - 15) {
                pShot->kind = img_enemyShotScale[3]; // シアン色の鱗弾に変化
                // 画面中心から外側へ向かう角度を計算
                pShot->muki = atan2(pShot->y - 240.0, pShot->x - 240.0);
                pShot->speed = 1.0 + GetRand(100) / 100.0; // 1.0 〜 2.0の速度でゆっくり拡散
                pShot->param_i[1] = 2;
            }

            // ③ 拡散状態になった弾のみ移動を開始
            if (pShot->param_i[1] == 2) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }

            pShot = pShot->next;
        }

        // パルス進行中のSE（連続音でプレイヤーのリズムを刻む）
        if (pulse_idx < 729 + 15 && scount % 4 == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
    }
}


// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_PeanoCurve_Gemini()
{
    // 初期化
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0; // 画面上部中央へ
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }

    // 敵の移動（ゆらゆらと漂う）
    if (count > 60) {
        enemy.x = 240.0 + 30.0 * sin((count - 60) * DX_PI / 120.0);
    }

    // 一度だけ弾幕セットを生成して委譲
    if (count % 600 == 60) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPeanoDomain;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
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