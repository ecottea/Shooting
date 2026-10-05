//----------------------------------------------------------------
// 弾幕：針の苗木（サボテン）
//
// ・大玉(緑)と小玉(緑)を敵の周囲に配置してサボテンの胴体を表現
// ・胴体は敵本体に追従して動く
// ・プレイヤーが接近すると針（銃弾・白）を狙い撃ち（近づくと刺さる）
// ・一定間隔で全周方向へ針を一斉発射する「開花」攻撃
// ・頂上には白い小玉の花を飾る
//
// ※count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと
//   画面外の弾の消去はメインルーチンで行われる前提
//----------------------------------------------------------------
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾をリストに追加するヘルパー関数
static sEnemyShot* AddShot(sEnemyShotSet* pEnemyShotSet, int kind, double x, double y, double muki, double speed)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->kind = kind;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// サボテン本体＋針のパターン
static void ShotCactus(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // --- 胴体の芯：大玉(緑)を円形に配置 ---
        const int    coreNum = 8;
        const double coreRadius = 14.0;
        for (int i = 0; i < coreNum; i++) {
            double angle = i / (double)coreNum * 2.0 * DX_PI;
            pEnemyShot = AddShot(pEnemyShotSet, img_enemyShotLargeBall[2], // 緑
                pEnemyShotSet->x + coreRadius * cos(angle),
                pEnemyShotSet->y + coreRadius * sin(angle),
                0.0, 0.0);
            pEnemyShot->param_i[0] = 1;                      // 胴体フラグ（敵に追従）
            pEnemyShot->param_d[0] = coreRadius * cos(angle); // 敵からのオフセットX
            pEnemyShot->param_d[1] = coreRadius * sin(angle); // 敵からのオフセットY
        }

        // --- 外周のトゲ座：小玉(緑)を円形に配置 ---
        const int    rimNum = 14;
        const double rimRadius = 30.0;
        for (int i = 0; i < rimNum; i++) {
            double angle = i / (double)rimNum * 2.0 * DX_PI;
            pEnemyShot = AddShot(pEnemyShotSet, img_enemyShotSmallBall[2], // 緑
                pEnemyShotSet->x + rimRadius * cos(angle),
                pEnemyShotSet->y + rimRadius * sin(angle),
                0.0, 0.0);
            pEnemyShot->param_i[0] = 1;
            pEnemyShot->param_d[0] = rimRadius * cos(angle);
            pEnemyShot->param_d[1] = rimRadius * sin(angle);
        }

        // --- 左右の腕：大玉＋小玉 ---
        for (int side = 0; side < 2; side++) {
            double sx = (side == 0) ? -28.0 : 28.0;
            pEnemyShot = AddShot(pEnemyShotSet, img_enemyShotLargeBall[2], // 緑
                pEnemyShotSet->x + sx, pEnemyShotSet->y - 6.0,
                0.0, 0.0);
            pEnemyShot->param_i[0] = 1;
            pEnemyShot->param_d[0] = sx;
            pEnemyShot->param_d[1] = -6.0;

            pEnemyShot = AddShot(pEnemyShotSet, img_enemyShotSmallBall[2], // 緑
                pEnemyShotSet->x + sx, pEnemyShotSet->y - 20.0,
                0.0, 0.0);
            pEnemyShot->param_i[0] = 1;
            pEnemyShot->param_d[0] = sx;
            pEnemyShot->param_d[1] = -20.0;
        }

        // --- 頂上の花：小玉(白) ---
        pEnemyShot = AddShot(pEnemyShotSet, img_enemyShotSmallBall[6], // 白
            pEnemyShotSet->x, pEnemyShotSet->y - 44.0,
            0.0, 0.0);
        pEnemyShot->param_i[0] = 1;
        pEnemyShot->param_d[0] = 0.0;
        pEnemyShot->param_d[1] = -44.0;
    }

    // --- 弾の移動：胴体弾は敵に追従、針は直進 ---
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            pShot->x = enemy.x + pShot->param_d[0];
            pShot->y = enemy.y + pShot->param_d[1];
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }

    double dx = player.x - enemy.x;
    double dy = player.y - enemy.y;
    double dist = sqrt(dx * dx + dy * dy);
    double base = atan2(dy, dx);

    // --- 接近警告：プレイヤーが近いと狙い針を3way連射（近づくと刺さる） ---
    if (dist < 160.0 && pEnemyShotSet->count % 12 == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        for (int i = -1; i <= 1; i++) {
            AddShot(pEnemyShotSet, img_enemyShotBullet[6], // 白
                enemy.x + 40.0 * cos(base), enemy.y + 40.0 * sin(base),
                base + i * 0.15, 3.5);
        }
    }

    // --- 開花の予告 ---
    if (pEnemyShotSet->count % 180 == 120) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // --- 開花：全周方向へ針を一斉発射 ---
    if (pEnemyShotSet->count % 180 == 0 && pEnemyShotSet->count > 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        const int needleNum = 20;
        // 発射ごとに回転させることで全周の隙間を変化させる
        double rot = (pEnemyShotSet->count / 180 % 2) * (DX_PI / needleNum);
        for (int i = 0; i < needleNum; i++) {
            double angle = i / (double)needleNum * 2.0 * DX_PI + rot;
            AddShot(pEnemyShotSet, img_enemyShotBullet[6], // 白
                enemy.x + 40.0 * cos(angle), enemy.y + 40.0 * sin(angle),
                angle, 2.5);
        }
    }
}

// 敵本体のパターン
void EnemyPat_Tmp()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;

        // サボテン本体（弾の集合）を1つだけ生成する
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotCactus;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
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
    else {
        // リサジュー曲線で画面全体を動き回る
        // X: 速い横往復、Y: ゆっくりした縦往復（周期比 3:2）
        double t = (count - 1) * 0.005;
        enemy.x = 240.0 + 170.0 * sin(t * 3.0);
        enemy.y = 240.0 + 170.0 * sin(t * 2.0);
    }
}