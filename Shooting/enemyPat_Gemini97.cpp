// enemyPat_perfectFreeze.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// --------------------------------------------------------
// 弾幕パターンA：パーフェクトフリーズ（ばら撒き → 氷結 → 融解）
// --------------------------------------------------------
static void ShotPerfectFreeze_Scatter(sEnemyShotSet* pSet)
{
    // [Phase 1: ばら撒き] 0〜149フレーム
    if (pSet->count < 150) {
        // 2フレームに1回、ランダム方向に弾を生成
        if (pSet->count % 2 == 0) {
            // 音が重なりすぎないように間引いて再生
            if (pSet->count % 10 == 0 && CheckSoundMem(sound_enemyShot_light) == 0) {
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
            }

            // 1回につき4発発射
            for (int i = 0; i < 4; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = enemy.x; // 常にボスの現在位置から発射
                pShot->y = enemy.y;

                // GetRand(359) で 0〜359 までの整数を取得し、ラジアンに変換
                pShot->muki = GetRand(359) * DX_PI / 180.0;

                // 速度は 1.0 〜 3.5 の間でランダム
                pShot->speed = (100 + GetRand(250)) / 100.0;

                // 色(0:赤 〜 6:白)をランダムに選び、中玉と小玉を混ぜる
                int color = GetRand(6);
                if (GetRand(1) == 0) {
                    pShot->kind = img_enemyShotMediumBall[color];
                }
                else {
                    pShot->kind = img_enemyShotSmallBall[color];
                }

                // リンクへの追加
                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }
    // [Phase 2: 氷結合図] 150フレーム目
    else if (pSet->count == 150) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }
    // [Phase 3: 融解合図] 270フレーム目
    else if (pSet->count == 270) {
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // 既存弾の挙動更新（移動・減速・再始動）
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {

        // 150〜180フレームにかけて急ブレーキ（氷結）
        if (pSet->count >= 150 && pSet->count < 180) {
            pShot->speed *= 0.85; // 減速
            if (pShot->speed < 0.1) pShot->speed = 0.0; // 完全に停止
        }
        // 270フレーム目で全弾一斉にランダムな方向へ再始動（融解）
        else if (pSet->count == 270) {
            // 進行方向を完全ランダムに再設定
            pShot->muki = GetRand(359) * DX_PI / 180.0;
            // 速度を 1.2 〜 3.0 の間で再設定
            pShot->speed = (120 + GetRand(180)) / 100.0;
        }

        // 速度と向きに従って座標更新
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}


// --------------------------------------------------------
// 弾幕パターンB：氷結中のサブ攻撃（自機狙い＆自機外し WAY弾）
// --------------------------------------------------------
static void ShotPerfectFreeze_Way(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // pSet->param_i[0] に 0(自機狙い) または 1(自機外し) が渡される
        int isEven = pSet->param_i[0];
        int way = isEven ? 16 : 15;     // 偶数(自機外し)16WAY / 奇数(自機狙い)15WAY
        double angle_range = 140.0 * DX_PI / 180.0; // 扇の広がり角（140度）

        // Lunaticらしく、速度の違う3層の弾幕を重ねる
        for (int layer = 0; layer < 3; layer++) {
            double speed = 2.5 + layer * 0.7; // 速度 2.5, 3.2, 3.9

            for (int i = 0; i < way; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pSet->x;
                pShot->y = pSet->y;

                // WAY弾の角度計算
                pShot->muki = pSet->muki - (angle_range / 2.0) + (angle_range / (way - 1)) * i;
                pShot->speed = speed;

                // 鱗弾を使用。自機狙いと自機外しで色を分ける（3:シアン, 4:青）
                pShot->kind = img_enemyShotScale[isEven ? 3 : 4];

                pShot->prev = pSet->pEnemyShotHead->prev;
                pShot->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pShot;
                pSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // サブ弾は氷結の影響を受けず、直進し続ける
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}


// --------------------------------------------------------
// 敵本体の挙動・管理ルーチン
// --------------------------------------------------------
void EnemyPat_PerfectFreeze_Gemini()
{
    // 1サイクルを400フレーム（約6.6秒）として管理
    int c = (count - 1) % 400;

    // 初期化処理
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // --- ボスの移動処理 ---
    if (c < 150) {
        // ばら撒き中：サイン波を使って左右上下にゆらゆらと動く
        enemy.x += 1.2 * sin(count * 0.04);
        enemy.y += 0.5 * cos(count * 0.03);
    }
    else if (c >= 150 && c < 270) {
        // 氷結中：移動を停止してプレイヤーにプレッシャーを与える
    }
    else {
        // 融解後：緩やかに初期位置(240, 80)付近へ戻る
        enemy.x += (240.0 - enemy.x) * 0.03;
        enemy.y += (80.0 - enemy.y) * 0.03;
    }


    // --- 弾幕セットの生成 ---

    // サイクル開始時(0F): ばら撒き＆氷結用の弾幕セットを登録
    if (c == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotPerfectFreeze_Scatter;
        pSet->x = enemy.x; // 座標はShotPerfectFreeze_Scatter内で適宜参照
        pSet->y = enemy.y;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 氷結中の追撃(170F, 200F, 230F): サブWAY弾の弾幕セットを登録
    if (c == 170 || c == 200 || c == 230) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotPerfectFreeze_Way;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        // プレイヤーへの角度を計算
        pSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);

        // c==200(2波目)だけ偶数WAY(自機外し)にして避け方を狂わせる
        pSet->param_i[0] = (c == 200) ? 1 : 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}