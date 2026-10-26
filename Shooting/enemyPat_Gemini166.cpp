#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：マスゲーム風ウェーブ弾幕
static void ShotMassGame(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // グリッド状（横10列 × 縦6行）に小丸弾を配置して、マスゲームのドット感を演出
        int cols = 10;
        int rows = 6;
        double spacingX = 22.0;
        double spacingY = 16.0;
        double startX = pEnemyShotSet->x - (cols - 1) * spacingX / 2.0;
        double startY = pEnemyShotSet->y + 20.0;

        for (int r = 0; r < rows; r++) {
            for (int c = 0; c < cols; c++) {
                pEnemyShot = new sEnemyShot;

                double initX = startX + c * spacingX;
                double initY = startY + r * spacingY;

                pEnemyShot->x = initX;
                pEnemyShot->y = initY;

                // 後続のフェーズで使うための初期位置やインデックスをパラメータに保持
                pEnemyShot->param_d[0] = initX; // 初期X
                pEnemyShot->param_d[1] = initY; // 初期Y
                pEnemyShot->param_d[2] = (double)c;     // 列インデックス（ウェーブ用）

                // 弾の種類と色一覧: 0:赤, 1:黄, 2:緑, 3:シアン, 4:青, 5:マゼンタ, 6:白, 7:黒, 8:橙
                // 小玉(2.5x2.5): img_enemyShotSmallBall[i]
                // 青(4)と赤(0)の市松模様でマスゲームのカード表現を再現
                int colorIdx = ((c + r) % 2 == 0) ? 4 : 0;
                pEnemyShot->kind = img_enemyShotSmallBall[colorIdx];

                pEnemyShot->speed = 0.0;
                pEnemyShot->muki = 0.0;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 弾の挙動（フェーズ管理：pEnemyShotSet->count はメインルーチンで毎フレーム自動インクリメントされます）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int t = pEnemyShotSet->count;

        if (t < 140) {
            // フェーズ1 & 2：整列とウェーブ演出（0〜139フレーム）
            double c = pShot->param_d[2];
            double initX = pShot->param_d[0];
            double initY = pShot->param_d[1];

            // 列ごとに位相をずらしたサイン波で、観客席のウェーブのように上下にうねらせる
            double wave = sin(t * 0.12 + c * 0.4) * 10.0;
            pShot->x = initX;
            pShot->y = initY + wave;
        }
        else if (t == 140) {
            // フェーズ3への切り替わり（一斉拡散の瞬間）
            if (pShot == pEnemyShotSet->pEnemyShotHead->next) {
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }
            // プレイヤーの方向へ発射角度を決定し、スピードを与える
            // GetRand(x) は 0 以上 x 以下の (x+1) 種類の整数を返す
            double targetAngle = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->muki = targetAngle + (GetRand(40) - 20) / 100.0; // 若干のばらつきを持たせる
            pShot->speed = 2.5 + GetRand(150) / 100.0;
        }
        else {
            // フェーズ3：一斉拡散（140フレーム以降は等速直線運動）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_MassGame_Gemini()
{
    static int muki;
    static int shot_count;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵自身は左右にゆっくり移動
        enemy.x += 0.8 * (double)muki;
        if (enemy.x < 100.0 || enemy.x > 380.0) {
            muki *= -1;
        }
    }

    // 一定間隔でマスゲーム弾幕セットを生成
    if (count % 100 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMassGame;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0 + (count / 100) % 3 * 150;
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