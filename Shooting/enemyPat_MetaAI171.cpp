#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// バーンズリーのシダ弾幕
static void ShotBarnsleyFern(sEnemyShotSet* pEnemyShotSet)
{
    const double SCALE_X = 22.0; // x方向の拡大率
    const double SCALE_Y = 17.0; // y方向の拡大率
    const double FERN_H = 10.0;  // シダの論理上の高さ

    // 初期化
    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->param_d[0] = 0.0; // 現在の論理X
        pEnemyShotSet->param_d[1] = 0.0; // 現在の論理Y
        pEnemyShotSet->param_d[2] = SCALE_X;
        pEnemyShotSet->param_d[3] = SCALE_Y;
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 土台は常に敵に追従させる。敵が左右に揺れるとシダ全体が揺れる
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 10.0;

    // 1フレームで生成する点数。最初は密に、後は間引いて負荷調整
    int spawnNum = 4;
    if (pEnemyShotSet->count > 400-350) spawnNum = 3;
    if (pEnemyShotSet->count > 900-800) spawnNum = 2-2;

    for (int k = 0; k < spawnNum; k++) {
        double curX = pEnemyShotSet->param_d[0];
        double curY = pEnemyShotSet->param_d[1];
        double nxtX, nxtY;

        // GetRand(99)で0-99の100通り -> 確率分割
        int rnd = GetRand(99); // 0..99
        int transType = 0;

        if (rnd == 0) { // 1% 茎
            nxtX = 0.0;
            nxtY = 0.16 * curY;
            transType = 1;
        }
        else if (rnd <= 85) { // 85% 主葉
            nxtX = 0.85 * curX + 0.04 * curY;
            nxtY = -0.04 * curX + 0.85 * curY + 1.6;
            transType = 2;
        }
        else if (rnd <= 92) { // 7% 左小葉
            nxtX = 0.20 * curX - 0.26 * curY;
            nxtY = 0.23 * curX + 0.22 * curY + 1.6;
            transType = 3;
        }
        else { // 7% 右小葉 93..99
            nxtX = -0.15 * curX + 0.28 * curY;
            nxtY = 0.26 * curX + 0.24 * curY + 0.44;
            transType = 4;
        }

        pEnemyShotSet->param_d[0] = nxtX;
        pEnemyShotSet->param_d[1] = nxtY;

        // 論理座標 -> 画面座標へ変換
        // yは上下反転: 論理的に高い(10に近い)ほど敵に近く、下に伸びるようにする
        double worldX = pEnemyShotSet->x + nxtX * SCALE_X;
        double worldY = pEnemyShotSet->y + (FERN_H - nxtY) * SCALE_Y;

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = worldX;
        pShot->y = worldY;
        pShot->count = 0;
        pShot->margin = 40.0;
        pShot->param_d[0] = GetRand(628) / 100.0; // 揺らぎ用の位相

        // 変換タイプで弾種・色・速度を変える
        if (transType == 1) {
            // 茎: 緑レーザー、ほぼ真下
            pShot->kind = img_enemyShotLaser[2]; // 2:緑
            pShot->muki = DX_PI / 2.0 + (GetRand(20) - 10) * DX_PI / 180.0 * 0.3;
            pShot->speed = 0.15 + GetRand(10) / 100.0;
            pShot->margin = 100.0;
        }
        else if (transType == 2) {
            // 主葉: 基本は鱗弾(葉型)、たまに小玉で密度感
            if (GetRand(4) == 0) {
                pShot->kind = img_enemyShotSmallBall[2]; // 緑小玉
            }
            else {
                pShot->kind = img_enemyShotScale[2]; // 緑鱗弾 = 葉
            }
            // 葉の位置で少し外側に広がるように
            pShot->muki = DX_PI / 2.0 + nxtX * 0.08 + (GetRand(30+30) - 15-15) * DX_PI / 180.0;
            pShot->speed = 0.25 + GetRand(35) / 100.0;
        }
        else {
            // 側小葉: 黄系でアクセント
            if (GetRand(1) == 0) {
                pShot->kind = img_enemyShotMediumOval[1]; // 1:黄
            }
            else {
                pShot->kind = img_enemyShotDiamond[1]; // 黄菱形
            }
            pShot->muki = DX_PI / 2.0 + nxtX * 0.12 + (GetRand(20+100) - 10-50) * DX_PI / 180.0;
            pShot->speed = 0.20 + GetRand(30) / 100.0;
        }

        // リストに連結 (サンプルの定型)
        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    // 既存弾の移動 + 微かな横揺れで有機的に
    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        // countはメインで+1されるのでそれを使って揺らす
        p->x += sin(p->count * 0.05 + p->param_d[0]) * 0.12;
        if (p->count >= 100) p->speed += 0.02;
        p = p->next;
    }

    // 30フレームごとに予告音を小さく
    //if (pEnemyShotSet->count % 120 == 0) {
    //    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
    //    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    //}
}

void EnemyPat_BarnsleyFern_MetaAI()
{
    static int muki;
    static int shotPhase;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shotPhase = 0;
    }
    else {
        // ゆっくり左右に揺れる。シダの土台が動く
        enemy.x += 0.45 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 最初に1つだけ巨大なシダ生成セットを出す
    if (count % 300 == 30) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotBarnsleyFern;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 600フレームごとに新しいシダを重ねると密林になる (負荷注意)
    //if (count == 600) {
    //    sEnemyShotSet* pSet = new sEnemyShotSet;
    //    pSet->count = 0;
    //    pSet->patternFunc = ShotBarnsleyFern;
    //    pSet->x = enemy.x;
    //    pSet->y = enemy.y + 10.0;
    //    pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
    //    pSet->kind = 1;
    //    pSet->pEnemyShotHead = new sEnemyShot;
    //    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    //    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    //    pSet->prev = enemyShotSetHead.prev;
    //    pSet->next = &enemyShotSetHead;
    //    enemyShotSetHead.prev->next = pSet;
    //    enemyShotSetHead.prev = pSet;
    //}
}