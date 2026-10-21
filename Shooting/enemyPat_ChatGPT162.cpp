#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// 弾幕「静謐なるミルクの流れ」
// 白い小玉・中玉・大玉を牛乳に見立てる。
// count / sEnemyShotSet::count / sEnemyShot::count の加算と、
// 画面外の弾・空の弾幕セットの削除はメインルーチン側に任せる。
// ------------------------------------------------------------

// 弾を既存の弾幕セットのリスト末尾に追加する。各種 count は変更しない。
static void AddMilkShot(sEnemyShotSet* pSet,
    double x, double y,
    double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    // margin と count は既定値のままにする。

    sEnemyShot* pHead = pSet->pEnemyShotHead;
    pShot->prev = pHead->prev;
    pShot->next = pHead;
    pHead->prev->next = pShot;
    pHead->prev = pShot;
}

// 新しい弾幕セットを作成して、メインルーチンが管理するリストへ登録する。
static sEnemyShotSet* CreateMilkShotSet(sEnemyShotSet::PatternFunc patternFunc,
    double baseAngle)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = patternFunc;
    pSet->x = enemy.x;
    pSet->y = enemy.y + 10.0;
    pSet->muki = baseAngle;
    pSet->kind = 0;
    pSet->param_d[0] = baseAngle;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

static void PlayMilkSound(int soundHandle)
{
    if (CheckSoundMem(soundHandle)) StopSoundMem(soundHandle);
    PlaySoundMem(soundHandle, DX_PLAYTYPE_BACK);
}

// 弾を移動させる。大玉が一定の高さに達した場合は小玉を放射状に散らす。
static void UpdateMilkShots(sEnemyShotSet* pSet, bool burstLargeBall)
{
    sEnemyShot* pHead = pSet->pEnemyShotHead;
    sEnemyShot* pShot = pHead->next;
    while (pShot != pHead) {
        // 更新中に弾を追加しても、その弾は次フレームから移動させる。
        sEnemyShot* pNext = pShot->next;

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        if (burstLargeBall &&
            pShot->kind == img_enemyShotLargeBall[6] &&
            pShot->y >= 205.0) {
            // 大玉が画面中段に達したら、飛沫のように白い小玉を散らす。
            const int bulletNum = 14*2;
            for (int i = 0; i < bulletNum; i++) {
                const double angle = 2.0 * DX_PI * i / bulletNum;
                const double speed = 2.05 + 0.12 * (i % 3);
                AddMilkShot(pSet, pShot->x, pShot->y,
                    angle, speed, img_enemyShotSmallBall[6]);
            }

            // 元の大玉も小玉に変えて進ませ、同じ弾が再度破裂しないようにする。
            pShot->kind = img_enemyShotSmallBall[6];
            pShot->speed = 2.05;
        }

        pShot = pNext;
    }
}

// 第1段階：牛乳を注ぐ細い流れ。
// ボスの移動と発射角のゆるやかな変化で、連続した曲線状の弾列を描く。
static void ShotMilkStream(sEnemyShotSet* pSet)
{
    if (pSet->count < 145) {
        const double angle = pSet->param_d[0]
            + pSet->param_d[1] * sin(pSet->count * 0.035)*4;
        AddMilkShot(pSet, enemy.x, enemy.y + 10.0,
            angle, 2.15, img_enemyShotSmallBall[6]);
    }

    UpdateMilkShots(pSet, false);
}

// 第2段階：注がれた牛乳が波紋のように広がる扇状弾幕。
static void ShotMilkRipple(sEnemyShotSet* pSet)
{
    if (pSet->count < 245 && pSet->count % 42 == 1) {
        const double originX = enemy.x;
        const double originY = enemy.y + 10.0;
        const double aim = atan2(player.y - originY, player.x - originX);
        const double centerAngle = aim
            + 0.22 * sin(pSet->count * 0.027);
        const int bulletNum = 17*2;

        for (int i = 0; i < bulletNum; i++) {
            const double angle = centerAngle + (i - (bulletNum - 1) / 2.0) * 0.065;
            const double speed = 2.0 + 0.10 * (i % 3);
            AddMilkShot(pSet, originX, originY,
                angle, speed, img_enemyShotSmallBall[6]);
        }
    }

    UpdateMilkShots(pSet, false);
}

// 第3段階：大きなミルクの塊が落ち、画面中段で飛沫に変わる。
static void ShotMilkOverflow(sEnemyShotSet* pSet)
{
    if (pSet->count < 145 && pSet->count % 36 == 1) {
        const double offsets[4] = { -0.18, -0.06, 0.06, 0.18 };
        const int shotIndex = pSet->param_i[1]++;
        const double angle = DX_PI / 2.0 + offsets[shotIndex % 4];
        AddMilkShot(pSet, enemy.x, enemy.y + 10.0,
            angle, 1.40, img_enemyShotLargeBall[6]);
    }

    UpdateMilkShots(pSet, true);
}

// 敵本体のパターン
void EnemyPat_Milkmaid_ChatGPT()
{
    static int moveDirection;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 42.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        moveDirection = 1;
    }
    else {
        enemy.x += 0.68 * moveDirection;
        if (enemy.x >= 380.0) {
            enemy.x = 380.0;
            moveDirection = -1;
        }
        else if (enemy.x <= 100.0) {
            enemy.x = 100.0;
            moveDirection = 1;
        }
        enemy.y = 42.0 + 4.0 * sin(count * 0.018);
    }

    // 540フレーム周期で「注ぐ流れ → 波紋 → あふれる飛沫」を繰り返す。
    const int cycleCount = (count - 1) / 540;
    const int phase = (count - 1) % 540;

    if (phase == 0) {
        // 流れの向きを周期ごとに左右交互に変える。
        const double lean = (cycleCount % 2 == 0) ? -0.28 : 0.28;
        sEnemyShotSet* pSet = CreateMilkShotSet(
            ShotMilkStream, DX_PI / 2.0 + lean);
        pSet->param_i[0] = 145;
        pSet->param_d[1] = 0.10;
        PlayMilkSound(sound_enemyShot_medium);
    }
    else if (phase == 180) {
        sEnemyShotSet* pSet = CreateMilkShotSet(
            ShotMilkRipple, DX_PI / 2.0);
        pSet->param_i[0] = 245;
        PlayMilkSound(sound_enemyShot_medium);
    }
    else if (phase == 330) {
        // 大玉が落ちてくる前に予告音を鳴らす。
        PlayMilkSound(sound_enemyCharge);
    }
    else if (phase == 360) {
        sEnemyShotSet* pSet = CreateMilkShotSet(
            ShotMilkOverflow, DX_PI / 2.0);
        pSet->param_i[0] = 145;
        PlayMilkSound(sound_enemyShot_heavy);
    }
}
