// enemyPat_shotgun.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ショットガン『散弾の華・バックショット・ブロッサム』
static void ShotGunBlossom(sEnemyShotSet* pEnemyShotSet)
{
    // 1. 事前動作：スラッグ弾の発射 (count == 0)
    if (pEnemyShotSet->count == 0) {
        // 重厚な発射音
        if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pSlug = new sEnemyShot;
        pSlug->x = pEnemyShotSet->x;
        pSlug->y = pEnemyShotSet->y;
        pSlug->muki = pEnemyShotSet->muki;
        pSlug->speed = 2.5; // 遅い単発重弾
        pSlug->kind = img_enemyShotLargeBall[0]; // 赤色の大玉
        pSlug->param_i[0] = 1; // 1:スラッグ弾フラグ

        // リストに追加
        pSlug->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pSlug->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pSlug;
        pEnemyShotSet->pEnemyShotHead->prev = pSlug;

        // 散弾発射までのウェイト（フレーム数）
        pEnemyShotSet->param_i[1] = 25;
    }

    // 2. 本撃：散弾の拡散 (count == param_i[1])
    if (pEnemyShotSet->count == pEnemyShotSet->param_i[1]) {
        // 大量の弾をばら撒く派手な音
        if (CheckSoundMem(sound_enemyShot_extreme) == 1) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        int groupCount = 5 + (2); // 5〜7つのグループ（花びら）
        int shotsPerGroup = 25;          // 1グループあたり25発
        double totalSpread = (120.0 + (30)) / 180.0 * DX_PI; // 120〜150度の扇状
        double groupSpread = 15.0 / 180.0 * DX_PI; // 1グループ内の広がり
        double gapSpread = totalSpread - groupSpread * groupCount;
        double gap = (groupCount > 1) ? (gapSpread / (groupCount - 1)) : 0.0;

        double currentAngle = pEnemyShotSet->muki - totalSpread / 2.0;

        for (int g = 0; g < groupCount; g++) {
            for (int i = 0; i < shotsPerGroup; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                pShot->muki = currentAngle;
                pShot->speed = 8.0; // 初速は非常に速い
                pShot->kind = img_enemyShotSmallBall[6]; // 白色の小玉（光る散弾）
                pShot->param_i[0] = 2; // 2:散弾フラグ
                pShot->param_i[1] = 0; // 残滓生成フラグ

                // リストに追加
                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;

                if (i < shotsPerGroup - 1) {
                    currentAngle += groupSpread / (shotsPerGroup - 1);
                }
            }
            currentAngle += gap; // グループ間の隙間
        }
    }

    // 3. 弾の更新と挙動制御
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        if (pShot->param_i[0] == 1) {
            // スラッグ弾の動き
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (pShot->param_i[0] == 2) {
            // 散弾の動き（極端な減速）
            pShot->speed *= 0.95; // 空気抵抗による急減速
            if (pShot->speed < 0.4) pShot->speed = 0.4; // 最低速度

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 速度が十分遅くなったら、燃えカスの残滓に変化
            if (pShot->speed <= 0.6 && pShot->param_i[1] == 0) {
                pShot->param_i[1] = 1; // 変化済みフラグ

                // 火花（残滓）を3〜4発生成
                int sparkCount = 3 + GetRand(1);
                for (int s = 0; s < sparkCount; s++) {
                    sEnemyShot* pSpark = new sEnemyShot;
                    pSpark->x = pShot->x;
                    pSpark->y = pShot->y;
                    pSpark->muki = DX_PI / 2.0 + (GetRand(60) - 30) / 180.0 * DX_PI; // 真下から少しランダム
                    pSpark->speed = 0.3 + GetRand(40) / 100.0 + 2; // ゆっくり落下
                    pSpark->kind = img_enemyShotSmallBall[8]; // 橙色の小玉（火花）
                    pSpark->param_i[0] = 3; // 3:残滓フラグ

                    // リストに追加
                    pSpark->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pSpark->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pSpark;
                    pEnemyShotSet->pEnemyShotHead->prev = pSpark;
                }

                // 散弾を消去（リストから外す）
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
        }
        else if (pShot->param_i[0] == 3) {
            // 残滓（火花）の動き
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot->speed *= 0.998; // さらに減速

            // 寿命管理（60フレームで消滅）
            if (pShot->count > 600) {
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
            }
        }

        pShot = pNext;
    }
}

// 敵本体のパターン
void EnemyPat_Shotgun_Qwen()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0; // 少し下げて配置
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
    }
    else {
        enemy.x += 1.2 * (double)muki;
        if (count % 150 == 75) muki *= -1; // 左右移動
    }

    // 3秒に1回程度（180フレーム）弾幕を発射
    if (count % 180 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGunBlossom;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}