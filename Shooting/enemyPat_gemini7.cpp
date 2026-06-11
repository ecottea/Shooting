// enemyPat_Tmp.cpp
#include "DxLib.h"
#include "gv.h"
#include <math.h>

// 弾幕：彗星弾幕（コメットテイル）
static void ShotComet(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // --------------------------------------------------------
    // 1. 初回フレーム：彗星の「核（大玉）」を生成
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 0) {
        // 重い発射音を鳴らす
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki;
        pEnemyShot->speed = 5.0;                      // 核は高速で直進
        pEnemyShot->kind = img_enemyShotLargeBall[6]; // 白い大玉を核にする

        // リストの末尾に追加
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // --------------------------------------------------------
    // 2. 彗星の核の移動（尾を生成するための基準座標更新）
    // --------------------------------------------------------
    // 大玉のスピード（5.0）に合わせて、セット自体の座標も進める
    pEnemyShotSet->x += 5.0 * cos(pEnemyShotSet->muki);
    pEnemyShotSet->y += 5.0 * sin(pEnemyShotSet->muki);

    // --------------------------------------------------------
    // 3. 一定フレーム間、核の後方に「尾（粒子）」をリアルタイム生成
    // --------------------------------------------------------
    if (pEnemyShotSet->count < 50 && pEnemyShotSet->count % 2 == 0) {
        // 2フレームに1回、2個の尾の粒子を放出
        for (int i = 0; i < 2; i++) {
            pEnemyShot = new sEnemyShot;
            // 核の現在位置から、わずかにランダムに散らして配置
            pEnemyShot->x = pEnemyShotSet->x + (GetRand(10) - 5);
            pEnemyShot->y = pEnemyShotSet->y + (GetRand(10) - 5);

            // 進行方向の真後ろ（+DX_PI）を中心に、少し拡散させる
            pEnemyShot->muki = pEnemyShotSet->muki + DX_PI + (GetRand(30) - 15) / 180.0 * DX_PI;
            // 放出時の初速（後ろに吹き飛ぶエネルギー）
            pEnemyShot->speed = (50 + GetRand(150)) / 100.0;

            // 色は宇宙らしくシアン(3)、青(4)、白(6)をブレンド
            int c_rand = GetRand(2);
            int color = (c_rand == 0) ? 3 : (c_rand == 1) ? 4 : 6;

            // 形状は小玉と鱗弾をミックスして尾の質感を出す
            if (GetRand(1) == 0) {
                pEnemyShot->kind = img_enemyShotSmallBall[color];
            }
            else {
                pEnemyShot->kind = img_enemyShotScale[color];
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 尾の展開音（控えめに再生）
        if (pEnemyShotSet->count % 10 == 0) {
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    // --------------------------------------------------------
    // 4. 第二段階：一定時間後、残された尾の粒子が一斉に自機を狙う
    // --------------------------------------------------------
    if (pEnemyShotSet->count == 70) {
        // キラキランというニュアンスで中量音を鳴らす
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            // 大玉（核）以外を一斉にプレイヤー方向へ向け直す
            if (pEnemyShot->kind != img_enemyShotLargeBall[6]) {
                pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
                // 降ってくる速度にバラツキをもたせて奥行きを出す
                pEnemyShot->speed = 1.5 + (GetRand(80) / 100.0);
            }
            pEnemyShot = pEnemyShot->next;
        }
    }

    // --------------------------------------------------------
    // 5. すべての構成弾の移動・特殊処理の適用
    // --------------------------------------------------------
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        // 自機狙いに変化する前（count < 70）かつ大玉以外なら、
        // 急ブレーキをかけてその場に余韻として留まらせる（彗星の尾の表現）
        if (pEnemyShotSet->count < 70 && pEnemyShot->kind != img_enemyShotLargeBall[6]) {
            pEnemyShot->speed *= 0.92;
        }

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Comet_Gemini()
{
    if (count == 1) {
        // 初期位置とHPの設定
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 150;
    }
    else {
        // ボスらしく、ゆったりとした美しいサイン波で左右上下に浮遊移動
        enemy.x = 240.0 + sin(count / 60.0) * 150.0;
        enemy.y = 80.0 + cos(count / 40.0) * 15.0;
    }

    // 80フレームごとに、プレイヤーを挟み撃ちにするように2本の彗星を交差発射
    if (count % 80 == 0) {
        // プレイヤーへの直線角度
        double base_muki = atan2(player.y - enemy.y, player.x - enemy.x);

        // プレイヤーの左右をあえて狙い、弾幕の交差地点が美しくなるようにオフセット
        double angles[2] = { base_muki - 0.25, base_muki + 0.25 };

        for (int i = 0; i < 2; i++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotComet;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 10.0;
            pEnemyShotSet->muki = angles[i];

            // 弾リストのダミーヘッド生成
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            // 全体リストへの追加
            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}