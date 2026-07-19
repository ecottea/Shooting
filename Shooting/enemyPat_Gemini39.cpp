// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// インベーダー型のクラスター弾幕
static void ShotInvaderCluster(sEnemyShotSet* pEnemyShotSet)
{
    const double SPACING = 10.0; // 弾同士の間隔

    // ==========================================
    // フェーズ: 配置（初期化）
    // ==========================================
    if (pEnemyShotSet->count == 0) {
        if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        pEnemyShotSet->param_i[0] = 35; // ステップ移動の間隔（フレーム数）
        pEnemyShotSet->param_i[1] = 1;  // 進行方向（1: 右, -1: 左）
        pEnemyShotSet->param_i[2] = 0;  // 現在の状態（0: 行進, 1: バースト）
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x; // クラスターの基準X座標
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y; // クラスターの基準Y座標

        // 8x8のドット絵パターン
        const char* pattern[] = {
            "...##...",
            "..####..",
            ".######.",
            "##.##.##",
            "########",
            "..#..#..",
            ".#.##.#.",
            "#......#"
        };

        // 投下時に割り当てられた色を取得
        int color = pEnemyShotSet->kind % 8;

        // パターン通りに中玉を配置
        for (int row = 0; row < 8; row++) {
            for (int col = 0; col < 8; col++) {
                if (pattern[row][col] == '#') {
                    sEnemyShot* pEnemyShot = new sEnemyShot;

                    // クラスター中心からの相対座標を記憶しておく
                    pEnemyShot->param_d[0] = (col - 3.5) * SPACING;
                    pEnemyShot->param_d[1] = (row - 3.5) * SPACING;

                    pEnemyShot->x = pEnemyShotSet->param_d[0] + pEnemyShot->param_d[0];
                    pEnemyShot->y = pEnemyShotSet->param_d[1] + pEnemyShot->param_d[1];
                    pEnemyShot->speed = 0.0; // 行進中はspeedを使わず直接座標を更新する
                    pEnemyShot->kind = img_enemyShotMediumBall[color]; // 中玉を使用

                    // リストに連結
                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }
    }

    // ==========================================
    // フェーズ: 更新
    // ==========================================
    if (pEnemyShotSet->param_i[2] == 0) {
        // --- 状態0: グリッド状の行進 ---

        // 指定フレーム間隔ごとにステップ移動
        if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % pEnemyShotSet->param_i[0] == 0) {
            // ステップ移動ごとに音を鳴らしてプレッシャーを与える
            if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            // 基準X座標を横移動
            pEnemyShotSet->param_d[0] += 54.0 * pEnemyShotSet->param_i[1];

            // 画面端で折り返し＆一段下がる
            if (pEnemyShotSet->param_d[0] > 420.0 || pEnemyShotSet->param_d[0] < 60.0) {
                pEnemyShotSet->param_i[1] *= -1;   // 進行方向反転
                pEnemyShotSet->param_d[1] += 80.0; // 一段下がる

                // 加速ギミック: ステップ間隔を短くする（最速10フレーム）
                if (pEnemyShotSet->param_i[0] > 10) {
                    pEnemyShotSet->param_i[0] -= 5;
                }
            }

            // 自機ラインに近づいたらバースト（侵略完了）
            if (pEnemyShotSet->param_d[1] > 320.0) {
                pEnemyShotSet->param_i[2] = 1; // 状態1へ移行
            }

            // クラスター内の全弾の座標を更新
            sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
                // 基準座標 ＋ 相対座標
                pEnemyShot->x = pEnemyShotSet->param_d[0] + pEnemyShot->param_d[0];
                pEnemyShot->y = pEnemyShotSet->param_d[1] + pEnemyShot->param_d[1];

                // バーストへの移行時、一斉に自機狙い＋拡散のベクトルを与える
                if (pEnemyShotSet->param_i[2] == 1) {
                    double angleToPlayer = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
                    double randomSpread = (GetRand(60) - 30) / 180.0 * DX_PI; // -30度〜+30度のブレ
                    pEnemyShot->muki = angleToPlayer + randomSpread;
                    // GetRand(x) は 0 から x を返すため、速度は 2.5 〜 4.0
                    pEnemyShot->speed = (250 + GetRand(150)) / 100.0;
                    pEnemyShot->kind = img_enemyShotMediumBall[0]; // 危険を示す「赤色」に変化
                }

                pEnemyShot = pEnemyShot->next;
            }
        }
    }
    else {
        // --- 状態1: バースト（通常の弾として移動） ---
        sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
            pEnemyShot = pEnemyShot->next;
        }
    }
}

// 敵本体のパターン
void EnemyPat_Invader_Gemini()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // UFOを模した上空の往復移動
        enemy.x += 2.0 * (double)muki;
        if (enemy.x > 420.0) muki = -1;
        if (enemy.x < 60.0)  muki = 1;
    }

    // 一定間隔でインベーダークラスターを投下
    if (count % 100 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotInvaderCluster;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;

        // クラスターの色を 緑(2), 青(4), マゼンタ(5) からランダムに選択
        int colors[3] = { 2, 4, 5 };
        pEnemyShotSet->kind = colors[GetRand(2)];

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // リストに連結
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}