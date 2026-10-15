#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 斬月一閃
// 短レーザー弾を刀身、楕円弾を斬撃の残像、小玉を斬撃後の扇状弾幕として使う。
// count / sEnemyShotSet::count / sEnemyShot::count の加算と、画面外の弾の削除はメインルーチンに任せる。

// 弾を弾幕セットの双方向リスト末尾へ追加する。
static void AppendEnemyShot(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pEnemyShot)
{
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// 弾幕セットを作り、敵弾管理リストへ登録する。
// angle は弾の見た目の向き、moveAngle は弾を移動させる向き。
static sEnemyShotSet* CreateSlashShotSet(
    double x, double y, double angle, double moveAngle,
    sEnemyShotSet::PatternFunc patternFunc)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = patternFunc;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = angle;
    pEnemyShotSet->param_d[0] = moveAngle;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
    return pEnemyShotSet;
}

// param_d[0] に保存した移動方向へ弾を進める。
// muki は画像の向きとして保持するため、斬撃の軸と突進方向を別々に設定できる。
static void MoveSlashShots(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const double moveAngle = pShot->param_d[0];
        pShot->x += pShot->speed * cos(moveAngle);
        pShot->y += pShot->speed * sin(moveAngle);
        pShot = pShot->next;
    }
}

// 刀身：白い短レーザー弾を重ね、長い斬撃の線を作る。
// 各レーザーは同じ方向へ進むため、ボスの突進と重なって飛ぶ斬撃になる。
static void ShotSlashBlade(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        const double bladeAngle = pEnemyShotSet->muki;
        const double moveAngle = pEnemyShotSet->param_d[0];
        for (int i = -2; i <= 2; ++i) {
            sEnemyShot* pShot = new sEnemyShot;
            const double offset = i * 46.0;
            pShot->x = pEnemyShotSet->x + cos(bladeAngle) * offset;
            pShot->y = pEnemyShotSet->y + sin(bladeAngle) * offset;
            pShot->muki = bladeAngle;
            pShot->speed = 8.5;
            pShot->kind = img_enemyShotLaser[6]; // 白い短レーザー弾
            pShot->param_d[0] = moveAngle;
            AppendEnemyShot(pEnemyShotSet, pShot);
        }
    }
    MoveSlashShots(pEnemyShotSet);
}

// 残像：マゼンタの中楕円弾を刀身に沿って並べ、ゆっくり進ませる。
static void ShotSlashAfterimage(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        const double bladeAngle = pEnemyShotSet->muki;
        const double moveAngle = pEnemyShotSet->param_d[0];
        for (int i = -3; i <= 3; ++i) {
            sEnemyShot* pShot = new sEnemyShot;
            const double offset = i * 19.0;
            pShot->x = pEnemyShotSet->x + cos(bladeAngle) * offset;
            pShot->y = pEnemyShotSet->y + sin(bladeAngle) * offset;
            pShot->muki = bladeAngle;
            pShot->speed = 2.4;
            pShot->kind = img_enemyShotMediumOval[5]; // マゼンタの中楕円弾
            pShot->param_d[0] = moveAngle;
            AppendEnemyShot(pEnemyShotSet, pShot);
        }
    }
    MoveSlashShots(pEnemyShotSet);
}

// 斬撃後の飛び散り：突進方向を中心に小玉を扇状に発射する。
static void ShotSlashFan(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        const double baseAngle = pEnemyShotSet->muki;
        for (int i = 0; i <= 10; ++i) {
            const double spread = (i - 5) * 0.13;
            const double angle = baseAngle + spread;
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = angle;
            const int distanceFromCenter = (i >= 5) ? i - 5 : 5 - i;
            pShot->speed = 3.8 + (5 - distanceFromCenter) * 0.24;
            pShot->kind = img_enemyShotSmallBall[1]; // 黄色の小玉
            pShot->param_d[0] = angle;
            AppendEnemyShot(pEnemyShotSet, pShot);
        }
    }
    MoveSlashShots(pEnemyShotSet);
}

static void PlaySlashSound(int soundHandle)
{
    if (CheckSoundMem(soundHandle)) StopSoundMem(soundHandle);
    PlaySoundMem(soundHandle, DX_PLAYTYPE_BACK);
}

// 敵本体のパターン
void EnemyPat_Sword_ChatGPT()
{
    static double dashAngle;
    static double bladeAngle;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 55.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        dashAngle = DX_PI / 2.0;
        bladeAngle = 0.0;
    }

    // 3種類の斬撃を順番に繰り返す。
    // 各斬撃は、構え・突進斬り・残像と弾の放出・退避で構成する。
    const int cycleFrame = (count - 1) % 180;
    const int slashIndex = cycleFrame / 60;
    const int phase = cycleFrame % 60;

    const double homeX = 240.0 + 75.0 * sin(count * 0.018);
    const double homeY = 58.0 + 10.0 * sin(count * 0.027);

    if (phase == 0) {
        // 構え始めにプレイヤーを狙い、突進方向を固定する。
        dashAngle = atan2(player.y - enemy.y, player.x - enemy.x);

        // 横薙ぎ、斜め斬り、逆斜め斬りを視覚的に区別する。
        if (slashIndex == 0) bladeAngle = 0.0;
        else if (slashIndex == 1) bladeAngle = 0.78;
        else bladeAngle = -0.78;

        //PlaySlashSound(sound_enemyCharge);
    }

    if (phase < 18) {
        // 斬撃前：少し静止して構え、攻撃の予兆を見せる。
        enemy.x += (homeX - enemy.x) * 0.08;
        enemy.y += (homeY - enemy.y) * 0.08;
    }
    else if (phase < 36) {
        if (phase == 18) {
            // 白い刀身を発生させ、ボス自身も同じ方向へ突進する。
            CreateSlashShotSet(enemy.x, enemy.y, bladeAngle, dashAngle, ShotSlashBlade);
            PlaySlashSound(sound_enemyShot_extreme);
        }

        enemy.x += cos(dashAngle) * 10.0;
        enemy.y += sin(dashAngle) * 10.0;

        // ボスが画面外へ飛び出し続けないよう、位置のみ制限する。
        if (enemy.x < 25.0) enemy.x = 25.0;
        if (enemy.x > 455.0) enemy.x = 455.0;
        if (enemy.y < 30.0) enemy.y = 30.0;
        if (enemy.y > 420.0) enemy.y = 420.0;

        if (phase == 35) {
            // 振り抜いた位置に残像を残し、突進方向へ扇状の小玉を放つ。
            CreateSlashShotSet(enemy.x, enemy.y, bladeAngle, dashAngle, ShotSlashAfterimage);
            CreateSlashShotSet(enemy.x, enemy.y, dashAngle, dashAngle, ShotSlashFan);
        }
    }
    else {
        // 退避：次の斬撃に備えて上方へ戻る。
        enemy.x += (homeX - enemy.x) * 0.08;
        enemy.y += (homeY - enemy.y) * 0.08;
    }
}
