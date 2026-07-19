// enemyPat_DNA_ComplementaryHelix.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：相補的二重螺旋（コンプリメンタリー・ヘリックス）
static void ShotDNA_ComplementaryHelix(sEnemyShotSet* pEnemyShotSet)
{
    // 120フレームの間、一定間隔（6フレーム毎）でDNAのハシゴを1段ずつ生成
    if (pEnemyShotSet->count % 6 == 0 && pEnemyShotSet->count < 120) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double centerX = pEnemyShotSet->x;
        double startY = pEnemyShotSet->y;

        // 1段のハシゴを7個の弾で構成する
        for (int i = 0; i < 7; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 汎用パラメータの使い方
            // param_d[0] : 基準となるX座標 (らせんの中心)
            // param_d[1] : 振幅 (中心からの距離)
            // param_d[2] : 位相 (0 または PI)
            // param_i[0] : 解離する方向 (0=左下へスライド, 1=右下へスライド)
            // param_i[1] : 状態フラグ (0=らせん結合中, 1=解離後)

            pEnemyShot->param_d[0] = centerX;
            pEnemyShot->y = startY;
            pEnemyShot->speed = 2.5; // らせん全体の落下速度

            double amp = 0;
            double phase = 0;
            int side = 0;

            if (i < 3) {
                // 左半分のパーツ
                amp = 120.0 - i * 40.0;
                phase = 0.0;
                side = 0;
                // 外側(i==0)は骨格(中玉・赤), 内側は塩基対(小玉・黄)
                pEnemyShot->kind = (i == 0) ? img_enemyShotMediumBall[0] : img_enemyShotSmallBall[1];
            }
            else if (i > 3) {
                // 右半分のパーツ
                amp = 120.0 - (6 - i) * 40.0;
                phase = DX_PI;
                side = 1;
                // 外側(i==6)は骨格(中玉・青), 内側は塩基対(小玉・緑)
                pEnemyShot->kind = (i == 6) ? img_enemyShotMediumBall[4] : img_enemyShotSmallBall[2];
            }
            else {
                // 中央の結合部
                amp = 0.0;
                phase = 0.0;
                side = (GetRand(1) == 0) ? 0 : 1; // 解離時はランダムに左右へ割れる
                pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白
            }

            pEnemyShot->param_d[1] = amp;
            pEnemyShot->param_d[2] = phase;
            pEnemyShot->param_i[0] = side;
            pEnemyShot->param_i[1] = 0; // 初期状態は「結合中」

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動・状態遷移処理
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->param_i[1] == 0) {
            // 【結合フェーズ】 サインカーブを描いて落下
            pEnemyShot->y += pEnemyShot->speed;

            // 弾の生存フレーム(count)に応じて回転角度を進める
            double angle = pEnemyShot->count * 0.05;
            pEnemyShot->x = pEnemyShot->param_d[0] + pEnemyShot->param_d[1] * sin(angle + pEnemyShot->param_d[2]);

            // プレイヤーの目の前付近(y > 280)に来たら、DNAを真っ二つに解離（ジッパー展開）
            if (pEnemyShot->y > 280.0) {
                pEnemyShot->param_i[1] = 1; // 解離状態へ移行

                // 左右にスライドするようにベクトル（向きと速度）を再設定
                if (pEnemyShot->param_i[0] == 0) {
                    pEnemyShot->muki = DX_PI * 0.85 - (pEnemyShotSet->count - 80) * 0.02; // 左下方向
                }
                else {
                    pEnemyShot->muki = DX_PI * 0.15 + (pEnemyShotSet->count - 80) * 0.02; // 右下方向
                }
                // バラバラに散るよう速度にランダムな揺らぎを入れる
                pEnemyShot->speed = 2.5 + GetRand(15) / 30.0;

                // 弾が割れるタイミングで効果音を鳴らす（連続再生での音割れ防止）
                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            }
        }
        else {
            // 【解離フェーズ】 直線移動で飛んでいく
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }

        pEnemyShot = pEnemyShot->next;
    }
}

static void ShotWall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {        
        for (int i = 0; i < 480; i += 3) {
            pEnemyShot = new sEnemyShot;

            // GetRand(x) は 0 から x までの x+1 種類の整数をランダムに返す関数なので注意！
            pEnemyShot->x = i;
            pEnemyShot->y = 350;
            pEnemyShot->muki = 0;
            pEnemyShot->speed = 0;
            pEnemyShot->kind = img_enemyShotSmallBall[6];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
}

// 敵本体のパターン
void EnemyPat_DNA_Gemini()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0; // やや上部に配置
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        // ボスはゆっくり左右に揺れ続ける
        enemy.x += 0.5 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 240フレーム（約4秒）ごとに新しいDNA弾幕セットを生成
    if (count % 240 == 60) {
        // 予告音
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDNA_ComplementaryHelix;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    if (count == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->patternFunc = ShotWall;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}