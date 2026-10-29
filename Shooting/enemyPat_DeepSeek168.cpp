#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  前方宣言
// ============================================================
static void ShotZakoAimed(sEnemyShotSet* pEnemyShotSet);
static void EnemyPat_ZakoFormation(sEnemyShotSet* pEnemyShotSet);

// ============================================================
//  リスト操作・生成補助
// ============================================================
static void AddEnemyShotSet(sEnemyShotSet* pEnemyShotSet)
{
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

static sEnemyShotSet* CreateEnemyShotSet(
    sEnemyShotSet::PatternFunc func,
    double x, double y, double muki, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->kind = kind;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    AddEnemyShotSet(pEnemyShotSet);
    return pEnemyShotSet;
}

static void AddShotToSet(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pEnemyShot)
{
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

static void RemoveEnemyShot(sEnemyShot* pEnemyShot)
{
    pEnemyShot->prev->next = pEnemyShot->next;
    pEnemyShot->next->prev = pEnemyShot->prev;
    delete pEnemyShot;
}

static void RemovePlayerShot(sPlayerShot* pPlayerShot)
{
    pPlayerShot->prev->next = pPlayerShot->next;
    pPlayerShot->next->prev = pPlayerShot->prev;
    delete pPlayerShot;
}

// ============================================================
//  大玉ザコのHPに応じた色
// ============================================================
static int LargeBallKindByHp(int hp)
{
    // 弾の色一覧: 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
    if (hp >= 8) return img_enemyShotLargeBall[4]; // 青
    if (hp >= 6) return img_enemyShotLargeBall[3]; // シアン
    if (hp >= 4) return img_enemyShotLargeBall[2]; // 緑
    if (hp >= 2) return img_enemyShotLargeBall[1]; // 黄
    return img_enemyShotLargeBall[0];              // 赤
}

// ============================================================
//  大玉ザコと自機ショットの当たり判定
//  戻り値: true = 大玉が消滅した
// ============================================================
static bool HitZakoByPlayerShots(sEnemyShot* pZako)
{
    const double hitRadius = 20.0 + 4.0; // 大玉半径20.0 + 自機ショット半径目安4.0
    const double hitRadiusSq = hitRadius * hitRadius;

    sPlayerShot* pPlayerShot = playerShotHead.next;
    while (pPlayerShot != &playerShotHead) {
        sPlayerShot* pNextPlayerShot = pPlayerShot->next;

        double dx = pPlayerShot->x - pZako->x;
        double dy = pPlayerShot->y - pZako->y;

        if (dx * dx + dy * dy <= hitRadiusSq) {
            if (CheckSoundMem(sound_playerShotHit_default)) StopSoundMem(sound_playerShotHit_default);
            PlaySoundMem(sound_playerShotHit_default, DX_PLAYTYPE_BACK);

            // 自機ショットを消す
            RemovePlayerShot(pPlayerShot);

            // 大玉のHPを減らす
            pZako->param_i[15]--;

            if (pZako->param_i[15] <= 0) {
                // 大玉消滅。ボスに1ダメージ
                if (enemy.hp > 0) enemy.hp--;
                RemoveEnemyShot(pZako);
                return true;
            }

            // 残HPに応じて色を変更
            pZako->kind = LargeBallKindByHp(pZako->param_i[15]);
            return false;
        }

        pPlayerShot = pNextPlayerShot;
    }

    return false;
}

// ============================================================
//  ザコが撃つ弾（大玉以外）
// ============================================================
static void ShotZakoAimed(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = -2; i <= 2; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + i * 0.14;
            pEnemyShot->speed = 2.2 + fabs((double)i) * 0.15;
            pEnemyShot->margin = 20.0;

            // 大玉以外の弾を使用する
            if (pEnemyShotSet->kind % 2 == 0) {
                pEnemyShot->kind = img_enemyShotMediumBall[3]; // シアン中玉
            }
            else {
                pEnemyShot->kind = img_enemyShotDiamond[5];    // マゼンタ菱形弾
            }

            AddShotToSet(pEnemyShotSet, pEnemyShot);
        }
    }

    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

static void SpawnZakoBullet(sEnemyShot* pZako)
{
    double shotX = pZako->x;
    double shotY = pZako->y + 20.0;
    double muki = atan2(player.y - shotY, player.x - shotX);

    CreateEnemyShotSet(
        ShotZakoAimed,
        shotX,
        shotY,
        muki,
        pZako->param_i[0]
    );
}

// ============================================================
//  大玉ザコ隊列の制御
// ============================================================
static void EnemyPat_ZakoFormation(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pZako = pEnemyShotSet->pEnemyShotHead->next;

    while (pZako != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNextZako = pZako->next;
        bool alive = true;

        // 画面外から下方向へ飛来。少しだけ横に揺らす
        pZako->y += pZako->speed;
        pZako->x = pZako->param_d[0] + sin((pZako->count + pZako->param_i[0] * 20) * 0.04) * 12.0;

        // 自機ショットとの当たり判定
        if (HitZakoByPlayerShots(pZako)) {
            alive = false;
        }

        if (alive) {
            // 画面内に入ったら弾を撃つ
            if (pZako->y > 10.0 && pZako->y < 470.0 && pZako->count % 120 == 40) {
                SpawnZakoBullet(pZako);
            }
        }

        pZako = pNextZako;
    }
}

// ============================================================
//  大玉ザコの隊列を出現させる
// ============================================================
static void SpawnZakoWave(sEnemyShotSet* pZakoSet, int waveIndex)
{
    if (pZakoSet == nullptr) return;

    const int zakoNum = 5;
    const double baseX = 240.0;

    for (int i = 0; i < zakoNum; i++) {
        sEnemyShot* pZako = new sEnemyShot;

        double offsetX = (i - (zakoNum - 1) / 2.0) * 70.0;

        pZako->x = baseX + offsetX;
        pZako->y = -120.0 - fabs(offsetX) * 0.4;
        pZako->muki = DX_PI / 2.0;
        pZako->speed = 1.05 + (waveIndex % 3) * 0.05;
        pZako->margin = 240.0; // 大玉ザコのmarginは240.0

        // param_i[15]をHP代わりに使用
        pZako->param_i[15] = 8;
        pZako->param_i[0] = i;
        pZako->param_d[0] = pZako->x;

        pZako->kind = LargeBallKindByHp(pZako->param_i[15]);

        AddShotToSet(pZakoSet, pZako);
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_OnTheWay_DeepSeek()
{
    static sEnemyShotSet* zakoSet = nullptr;
    static int waveIndex = 0;

    if (count == 1) {
        // ボスは自機ショットの届かない上部画面外
        enemy.x = 240.0;
        enemy.y = -80.0;
        enemy.maxHp = enemy.hp = 20; // 200から変更しないこと

        waveIndex = 0;

        // 大玉ザコ隊列用のセットを1つ作成
        zakoSet = CreateEnemyShotSet(
            EnemyPat_ZakoFormation,
            240.0,
            -80.0,
            0.0,
            0
        );
    }

    // 一定間隔で隊列を出現させる
    if (zakoSet != nullptr && count % 200 == 1) {
        SpawnZakoWave(zakoSet, waveIndex++);
    }
}