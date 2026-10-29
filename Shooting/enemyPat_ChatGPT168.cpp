#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 大玉ザコの画像を残りHPに応じて切り替える。
// 4:赤、3:橙、2:黄、1:シアン。
static int StageFormationLargeBallImage(int hp)
{
    switch (hp) {
    case 4: return img_enemyShotLargeBall[0];
    case 3: return img_enemyShotLargeBall[8];
    case 2: return img_enemyShotLargeBall[1];
    default: return img_enemyShotLargeBall[3];
    }
}

// 弾をセットの連結リスト末尾へ追加する。count の更新はメインルーチンに任せる。
static sEnemyShot* StageFormationAddShot(
    sEnemyShotSet* pSet,
    double x, double y,
    double muki, double speed,
    int kind, double margin,
    int role, int hp, int firePhase)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->margin = margin;
    pShot->param_i[13] = firePhase;
    pShot->param_i[14] = role; // 1: 大玉ザコ、0: 通常弾
    pShot->param_i[15] = hp;   // 大玉ザコの残りHP
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// 大玉ザコを隊列で画面外から進入させる。
static void StageFormationSpawnEnemies(sEnemyShotSet* pSet)
{
    const int memberCount = 8;
    const double center = (memberCount - 1) / 2.0;
    const int formation = pSet->kind % 6;

    for (int i = 0; i < memberCount; ++i) {
        const double d = i - center;
        double x = 0.0;
        double y = 0.0;
        double muki = DX_PI / 2.0;
        double speed = 1.8;

        switch (formation) {
        case 0:
            // 上端からV字気味に進入し、中央へ寄りながら降下。
            x = 240.0 + d * 82.0;
            y = -62.0 - fabs(d) * 10.0;
            muki = DX_PI / 2.0 + d * 0.035;
            speed = 1.9;
            break;
        case 1:
            // 左端から斜めの縦隊で進入。
            x = -78.0 - fabs(d) * 5.0;
            y = 38.0 + i * 49.0;
            muki = 0.06 + d * 0.028;
            speed = 1.85;
            break;
        case 2:
            // 右端から斜めの縦隊で進入。
            x = 558.0 + fabs(d) * 5.0;
            y = 38.0 + i * 49.0;
            muki = DX_PI - 0.06 + d * 0.028;
            speed = 1.85;
            break;
        case 3:
            // 高さをずらした逆V字の隊列。
            x = 240.0 + d * 59.0;
            y = -92.0 + fabs(d) * 11.0;
            muki = DX_PI / 2.0 + d * 0.045;
            speed = 2.0;
            break;
        case 4:
            // 左右両端から同時に進入する二列。前半と後半を鏡像にする。
            if (i < 4) {
                x = -66.0 - i * 13.0;
                y = 24.0 + i * 69.0;
                muki = 0.20 + i * 0.025;
            }
            else {
                const int j = i - 4;
                x = 546.0 + j * 13.0;
                y = 24.0 + j * 69.0;
                muki = DX_PI - 0.20 - j * 0.025;
            }
            speed = 1.9;
            break;
        default:
            // 上端から交互に高さを変えた横隊で進入。
            x = -18.0 + i * 74.0;
            y = -48.0 - (i % 2) * 34.0;
            muki = DX_PI / 2.0 + d * 0.023;
            speed = 2.05;
            break;
        }

        StageFormationAddShot(
            pSet, x, y, muki, speed,
            StageFormationLargeBallImage(4), 240.0,
            1, 4-2, GetRand(60));
    }
}

// 大玉ザコが自機狙いの扇状弾を撃つ。大玉以外の画像だけを使用する。
static void StageFormationFireVolley(sEnemyShotSet* pSet, sEnemyShot* pEnemy)
{
    const double aim = atan2(player.y - pEnemy->y, player.x - pEnemy->x);
    const double spread[] = { -0.19, 0.0, 0.19 };
    const int style = pSet->kind % 4;
    int bulletKind = img_enemyShotScale[(pSet->kind + 1) % 9];
    if (style == 1) bulletKind = img_enemyShotDiamond[(pSet->kind + 3) % 9];
    if (style == 2) bulletKind = img_enemyShotBullet[(pSet->kind + 0) % 9];
    if (style == 3) bulletKind = img_enemyShotMediumOval[(pSet->kind + 5) % 9];

    for (int i = 0; i < 3; ++i) {
        const double angle = aim + spread[i];
        StageFormationAddShot(
            pSet,
            pEnemy->x + cos(angle) * 15.0,
            pEnemy->y + sin(angle) * 15.0,
            angle,
            2.65 + style * 0.12,
            bulletKind, 20.0,
            0, 0, 0);
    }
}

// 隊列1波分の更新。大玉への自機ショット判定もここで行う。
static void StageFormationPattern(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        StageFormationSpawnEnemies(pSet);
        if (pSet->kind % 4 == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        // このフレームの処理中に削除・追加があっても、次の既存弾を安全にたどれるよう先に保存。
        sEnemyShot* pNext = pShot->next;

        if (pShot->param_i[14] == 1) {
            // 大玉ザコの移動。
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 自機ショットを大玉の円形当たり判定に対して検査する。
            sPlayerShot* pPlayerShot = playerShotHead.next;
            bool destroyed = false;
            while (pPlayerShot != &playerShotHead) {
                sPlayerShot* pNextPlayerShot = pPlayerShot->next;
                const double dx = pPlayerShot->x - pShot->x;
                const double dy = pPlayerShot->y - pShot->y;
                // 大玉の半径20pxに、自機ショット側の余裕を加えた判定。
                if (dx * dx + dy * dy <= 24.0 * 24.0) {
                    if (CheckSoundMem(sound_playerShotHit_default)) StopSoundMem(sound_playerShotHit_default);
                    PlaySoundMem(sound_playerShotHit_default, DX_PLAYTYPE_BACK);
                    
                    pPlayerShot->prev->next = pPlayerShot->next;
                    pPlayerShot->next->prev = pPlayerShot->prev;
                    delete pPlayerShot;

                    --pShot->param_i[15];
                    if (pShot->param_i[15] <= 0) {
                        // 大玉撃破につきボスへ1ダメージ。
                        pShot->prev->next = pShot->next;
                        pShot->next->prev = pShot->prev;
                        if (enemy.hp > 0) --enemy.hp;
                        delete pShot;
                        destroyed = true;
                        break;
                    }
                    pShot->kind = StageFormationLargeBallImage(pShot->param_i[15]);
                }
                pPlayerShot = pNextPlayerShot;
            }

            if (destroyed) {
                pShot = pNext;
                continue;
            }

            // 画面内に入ってから、各個体の位相をずらして高密度の自機狙い弾を撃つ。
            if (pShot->x > -5.0 && pShot->x < 485.0 &&
                pShot->y > 5.0 && pShot->y < 455.0 &&
                pShot->count % 72 == pShot->param_i[13]) {
                StageFormationFireVolley(pSet, pShot);
            }
        }
        else {
            // 通常弾の移動。muki は進行方向でもあるため、楕円・鱗・菱形弾の向きにも使える。
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pNext;
    }
}

// 敵本体のパターン
void EnemyPat_OnTheWay_ChatGPT()
{
    static int waveNo;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = -80.0; // 自機ショットが届かない画面上部の外側
        enemy.maxHp = enemy.hp = 80; // 200から変更しないこと
        waveNo = 0;
    }

    // ボスは画面外に固定。大玉ザコを次々と隊列で送り込む。
    if (enemy.hp > 0 && count % 120 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = StageFormationPattern;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = -80.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        pEnemyShotSet->kind = waveNo++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
