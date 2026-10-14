#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：スリットの壁（障害物）を形成するパターン
static void ShotSlitBoard(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 画面中央付近（y = 180）に横一列の壁となる弾を配置し、中央に2箇所のスリット（隙間）を開ける
        // 弾の色一覧・種類: 中玉(7.0x7.0)、色6: 白
        for (double wx = 80.0; wx <= 400.0; wx += 16.0) {
            // スリットの穴（x = 200〜220 と x = 260〜280 付近）は弾を置かない
            if ((wx >= 196.0 && wx <= 220.0) || (wx >= 260.0 && wx <= 284.0)) {
                continue;
            }
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = wx;
            pEnemyShot->y = 180.0;
            pEnemyShot->speed = 0.0; // 固定（壁）
            pEnemyShot->muki = 0.0;
            pEnemyShot->kind = img_enemyShotMediumBall[6]; // 白の中玉

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    // 壁の弾は移動しないためループ内での位置更新は不要ですが、リスト構造を維持します。
}

// 弾幕：2つのスリットから量子波（干渉縞を生む波）を放射するパターン
static void ShotQuantumWave(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    // 一定フレームごとに2つのスリット位置から波の粒子を放射
    if (pEnemyShotSet->count % 25 == 0) {
        // 効果音の再生
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 2つのスリットの穴の中心座標
        double slitX[2] = { 208.0, 272.0 };
        double slitY = 180.0;

        for (int s = 0; s < 2; s++) {
            // 各スリットから下方へ向けて扇状に波の粒子（小玉）を発射
            // 小玉(2.5x2.5)、色3: シアン（量子・光子のイメージ）
            for (int i = 0; i < 11; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = slitX[s];
                pEnemyShot->y = slitY;

                // 下方向（PI / 2）を中心に左右に広げる
                double baseAngle = DX_PI_F * 0.5;
                pEnemyShot->muki = baseAngle + (i - 5) * (DX_PI * 0.6 / 10.0);
                pEnemyShot->speed = 2.2 + (i % 2) * 0.2; // わずかな速度差で干渉縞の美しさを強調
                pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアンの小玉

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 放射された弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_DoubleSlit_Gemini()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;

        // 1. スリットの壁（障害物）を生成する弾幕セットを登録
        sEnemyShotSet* pSetWall = new sEnemyShotSet;
        pSetWall->count = 0;
        pSetWall->patternFunc = ShotSlitBoard;
        pSetWall->x = enemy.x;
        pSetWall->y = enemy.y;
        pSetWall->pEnemyShotHead = new sEnemyShot;
        pSetWall->pEnemyShotHead->prev = pSetWall->pEnemyShotHead;
        pSetWall->pEnemyShotHead->next = pSetWall->pEnemyShotHead;
        pSetWall->prev = enemyShotSetHead.prev;
        pSetWall->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetWall;
        enemyShotSetHead.prev = pSetWall;

        // 2. 干渉波を放出し続ける弾幕セットを登録
        sEnemyShotSet* pSetWave = new sEnemyShotSet;
        pSetWave->count = 0;
        pSetWave->patternFunc = ShotQuantumWave;
        pSetWave->x = enemy.x;
        pSetWave->y = enemy.y;
        pSetWave->pEnemyShotHead = new sEnemyShot;
        pSetWave->pEnemyShotHead->prev = pSetWave->pEnemyShotHead;
        pSetWave->pEnemyShotHead->next = pSetWave->pEnemyShotHead;
        pSetWave->prev = enemyShotSetHead.prev;
        pSetWave->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSetWave;
        enemyShotSetHead.prev = pSetWave;
    }
    else {
        // ボスは上部でゆっくり往復移動
        enemy.x += 1.2 * (double)muki;
        if (count % 200 == 100) muki *= -1;
    }
}