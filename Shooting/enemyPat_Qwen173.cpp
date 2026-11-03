#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：流れる譜面・メロディ・ストリーム
// ============================================================
static void ShotMusicalStaff(sEnemyShotSet* pEnemyShotSet)
{
    int t = pEnemyShotSet->count;

    // 【フェーズ1: 譜面の提示】 (初回のみ)
    if (t == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 五線譜の「線」を5本生成 (Y座標: 100, 140, 180, 220, 260)
        for (int i = 0; i < 5; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = -40.0;
            pShot->y = 100.0 + i * 40.0+180;
            pShot->muki = 0.0;             // 右向き
            pShot->speed = 3.0;
            pShot->kind = img_enemyShotLaser[6]; // 6:白 (細い横レーザー)
            pShot->param_i[0] = 1;         // 1 = 五線譜の線
            pShot->margin = 240;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 【フェーズ2: 音符と小節線の生成】 (タイムライン管理)
    if (t > 0 && t < 300) {
        // 音符の生成 (30フレームごと = 一定リズムで出現)
        if (t % 30 == 0) {
            int lineIdx = (t / 30) % 5; // 0〜4を循環 (ドレミファソ)
            sEnemyShot* pNote = new sEnemyShot;
            pNote->x = -40.0;
            pNote->y = 100.0 + lineIdx * 40.0+180;
            pNote->muki = 0.0;
            pNote->speed = 3.0;
            pNote->kind = img_enemyShotMediumBall[lineIdx]; // 線に対応した色 (0:赤, 1:黄, 2:緑, 3:シアン, 4:青)
            pNote->param_i[0] = 2;       // 2 = 音符
            pNote->param_d[0] = pNote->y; // バウンド計算用の基準Y座標を保存
            pNote->param_i[1] = lineIdx;  // 色インデックスを保存
            pNote->margin = 240;

            pNote->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pNote->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pNote;
            pEnemyShotSet->pEnemyShotHead->prev = pNote;
        }

        // 小節線の生成 (120フレームごと)
        if (t % 120 == 60) {
            sEnemyShot* pBar = new sEnemyShot;
            pBar->x = -40.0;
            pBar->y = 180.0+180; // 画面中央
            pBar->muki = DX_PI / 2.0; // 下向き (縦レーザーとして表示)
            pBar->speed = 3.0;
            pBar->kind = img_enemyShotLaser[5]; // 5:マゼンタ
            pBar->param_i[0] = 3;       // 3 = 小節線
            pBar->margin = 240;

            pBar->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pBar->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pBar;
            pEnemyShotSet->pEnemyShotHead->prev = pBar;
        }
    }

    // 【フェーズ3: 弾の移動と挙動】
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next; // リスト変更時の安全策として次を保存

        if (pShot->param_i[0] == 2) {
            // 音符の挙動: 右に進みつつ、サイン波で上下にバウンド
            pShot->x += pShot->speed * cos(pShot->muki);
            // pShot->count はメインルーチンで自動インクリメントされるため、それを利用してリズムを作る
            pShot->y = pShot->param_d[0] + sin(pShot->count * 0.4) * 12.0;

            // 画面右端近くで炸裂 (自機狙い弾に変化)
            if (pShot->x > 400.0) {
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                pShot->speed = 4.5; // 加速
                pShot->kind = img_enemyShotSmallBall[pShot->param_i[1]]; // 小さな弾になって襲いかかる
                pShot->param_i[0] = 0; // 通常弾として扱わせる
            }
        }
        else if (pShot->param_i[0] == 3) {
            pShot->x += pShot->speed;
        }
        else {
            // 線と小節線の挙動: 等速直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = nextShot;
    }
}

static void ShotBlackLine(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < 9; i++) for (int j = 0; j < 2; j++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = i * 60;
            pEnemyShot->y = 270 + 180 * j;;
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLaser[7];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_StaffNotation_Qwen()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0; // 指揮者のように少し手前に配置
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵は左右にゆっくり移動しながら指揮を執る
        enemy.x += 0.5 * (double)muki;
        if (count % 180 == 0) muki *= -1;
    }

    if (count == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->patternFunc = ShotBlackLine;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 180フレーム(約3秒)ごとに「五線譜」パターンを発動
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMusicalStaff;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
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