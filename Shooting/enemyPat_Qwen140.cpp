#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：回転刃の紙吹雪（ペーパー・シュレッダー）
static void ShotShredder(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 回転刃の初期化 (8方向に配置)
        for (int i = 0; i < 8; i++) {
            sEnemyShot* pBlade = new sEnemyShot;
            pBlade->param_i[0] = 0; // 0: 回転刃
            pBlade->param_d[0] = (i * 45.0) * DX_PI / 180.0; // 初期角度
            pBlade->kind = img_enemyShotDiamond[0]; // 菱形弾, 赤(0)
            pBlade->x = 0.0;
            pBlade->y = 0.0;

            pBlade->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pBlade->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pBlade;
            pEnemyShotSet->pEnemyShotHead->prev = pBlade;
        }
    }

    // 紙の投入 (60フレームごと)
    if (pEnemyShotSet->count % 30 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        sEnemyShot* pPaper = new sEnemyShot;
        pPaper->param_i[0] = 1; // 1: 紙

        // 画面上部のランダムな位置から投入
        pPaper->x = 40.0 + (double)GetRand(400);
        pPaper->y = -20.0;
        pPaper->margin = 40;

        double dx = enemy.x - pPaper->x;
        double dy = enemy.y - pPaper->y;
        double dist = sqrt(dx * dx + dy * dy);

        pPaper->muki = atan2(dy, dx);
        pPaper->speed = 4.0;
        pPaper->kind = img_enemyShotMediumOval[6]; // 中楕円弾, 白(6)

        // 敵に到達するまでのフレーム数を計算して保持
        pPaper->param_i[1] = (int)(dist / pPaper->speed);

        pPaper->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pPaper->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pPaper;
        pEnemyShotSet->pEnemyShotHead->prev = pPaper;
    }

    // 各弾の更新処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* nextShot = pShot->next; // 消去時にnextポインタが壊れるため退避

        if (pShot->param_i[0] == 0) {
            // 【回転刃】の更新
            pShot->param_d[0] += 0.15; // 時計回りに高速回転
            pShot->x = enemy.x + cos(pShot->param_d[0]) * 30.0;
            pShot->y = enemy.y + sin(pShot->param_d[0]) * 30.0;

            // 代表として最初の刃の角度をセットに保存（紙片生成時の飛散方向計算に使用）
            if (pShot == pEnemyShotSet->pEnemyShotHead->next) {
                pEnemyShotSet->param_d[0] = pShot->param_d[0];
            }
        }
        else if (pShot->param_i[0] == 1) {
            // 【紙】の更新
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->param_i[1]--;

            // 敵(回転刃)に到達したら裁断処理
            if (pShot->param_i[1] <= 0) {
                double shredX = pShot->x;
                double shredY = pShot->y;
                double bladeAngle = pEnemyShotSet->param_d[0]; // 現在の回転刃の角度

                // 紙の消去
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;

                // 裁断音
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                // 【紙片】の生成 (6〜10個)
                int numPieces = 6 + GetRand(4) + 50;
                for (int i = 0; i < numPieces; i++) {
                    sEnemyShot* pPiece = new sEnemyShot;
                    pPiece->param_i[0] = 2; // 2: 紙片
                    pPiece->x = shredX;
                    pPiece->y = shredY;

                    // 回転刃の接線方向(角度+90度) + ランダムなばらつき(-30度〜+30度)
                    double spread = ((double)GetRand(60) - 30.0) / 180.0 * DX_PI;
                    pPiece->muki = bladeAngle + DX_PI / 2.0 + spread;

                    // 速度は 3.0 〜 5.0
                    pPiece->speed = 3.0 + (double)GetRand(200) / 100.0;
                    pPiece->kind = img_enemyShotBullet[6]; // 銃弾, 白(6)

                    pPiece->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pPiece->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pPiece;
                    pEnemyShotSet->pEnemyShotHead->prev = pPiece;
                }

                // pShotはdelete済みなので、continueで次の弾へ
                pShot = nextShot;
                continue;
            }
        }
        else if (pShot->param_i[0] == 2) {
            // 【紙片】の更新
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = nextShot;
    }
}

// 敵本体のパターン
void EnemyPat_Shredder_Qwen()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 100.0; // 紙吹雪が見えやすいように少し下げる
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;

        // シュレッダーパターンの初期化
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotShredder;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 敵の左右往復移動
        enemy.x += 0.5 * (double)muki;
        if (count % 120 == 60) {
            muki *= -1;
        }
    }
}