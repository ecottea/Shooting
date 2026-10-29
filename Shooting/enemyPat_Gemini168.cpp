#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ザコ敵（大玉）の編隊行動、HP管理、当たり判定、および攻撃パターン
static void ShotZakoFormation(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 3機編隊の大玉（ザコ敵）を生成
        for (int i = 0; i < 3; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = 120.0 + i * 120.0; // 120, 240, 360
            pEnemyShot->y = -40.0 - i * 20.0;  // 画面外上部から登場
            pEnemyShot->speed = 1.5;
            pEnemyShot->muki = DX_PI / 2.0;    // 下向き
            pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤（初期HP 3の色）
            pEnemyShot->margin = 240.0;                  // 指定されたマージン
            pEnemyShot->param_i[15] = 3;                 // param_i[15]をHP代わりに利用（初期HP: 3）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNextShot = pShot->next;

        // --- 1. 大玉（ザコ敵）の移動パターン ---
        if (pShot->count < 80) {
            pShot->y += 2.0; // 画面内に侵入
        }
        else if (pShot->count < 320) {
            // 左右にウェーブしながら定位置付近に滞在
            double base_x = (pShot->x < 240.0) ? 140.0 : 340.0;
            pShot->x = base_x + 50.0 * sin((pShot->count - 80) * 0.04);
            pShot->y = 120.0 + 15.0 * cos((pShot->count - 80) * 0.04);
        }
        else {
            // 時間経過で下方へ去っていく
            pShot->y += 1.0;
        }

        // --- 2. 自機ショットとの当たり判定 ---
        bool destroyed = false;
        sPlayerShot* pPlayerShot = playerShotHead.next;
        while (pPlayerShot != &playerShotHead) {
            sPlayerShot* pNextPlayerShot = pPlayerShot->next;

            double dx = pShot->x - pPlayerShot->x;
            double dy = pShot->y - pPlayerShot->y;
            double distSq = dx * dx + dy * dy;
            double hitDist = 20.0 + 6.0; // 大玉半径(20.0) + 自機ショット半径(約6.0)

            if (distSq < hitDist * hitDist) {
                if (CheckSoundMem(sound_playerShotHit_default)) StopSoundMem(sound_playerShotHit_default);
                PlaySoundMem(sound_playerShotHit_default, DX_PLAYTYPE_BACK);

                // ヒットした自機ショットを消去
                pPlayerShot->prev->next = pPlayerShot->next;
                pPlayerShot->next->prev = pPlayerShot->prev;
                delete pPlayerShot;

                // 大玉のHPを減らす
                pShot->param_i[15]--;

                // 残HPに応じた色変更
                if (pShot->param_i[15] == 2) {
                    pShot->kind = img_enemyShotLargeBall[8]; // 橙
                }
                else if (pShot->param_i[15] == 1) {
                    pShot->kind = img_enemyShotLargeBall[1]; // 黄
                }

                // HPが0になったら大玉を消してボスに1ダメージを与える
                if (pShot->param_i[15] <= 0) {
                    if (enemy.hp > 0) {
                        enemy.hp -= 1;
                    }
                    if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                    // 大玉の削除
                    pShot->prev->next = pShot->next;
                    pShot->next->prev = pShot->prev;
                    delete pShot;
                    destroyed = true;
                    break;
                }
            }
            pPlayerShot = pNextPlayerShot;
        }

        if (destroyed) {
            pShot = pNextShot;
            continue;
        }

        // --- 3. 大玉から弾（大玉以外の弾）を発射する処理 ---
        if (pShot->count > 80 && pShot->count % 90 == 0) {
            sEnemyShotSet* pBulletSet = new sEnemyShotSet;
            pBulletSet->count = 0;
            pBulletSet->x = pShot->x;
            pBulletSet->y = pShot->y;
            pBulletSet->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            pBulletSet->patternFunc = [](sEnemyShotSet* set) {
                sEnemyShot* pBullet;
                if (set->count == 0) {
                    if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                    PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                    // 5way弾（中楕円弾と小玉をミックス：大玉以外を使用）
                    for (int i = -2; i <= 2; i++) {
                        pBullet = new sEnemyShot;
                        pBullet->x = set->x;
                        pBullet->y = set->y;
                        pBullet->muki = set->muki + i * 0.25;
                        pBullet->speed = 3.0;
                        pBullet->kind = (i % 2 == 0) ? img_enemyShotMediumOval[3] : img_enemyShotSmallBall[4]; // シアンの中楕円弾、青の小玉
                        pBullet->margin = 50.0;

                        pBullet->prev = set->pEnemyShotHead->prev;
                        pBullet->next = set->pEnemyShotHead;
                        set->pEnemyShotHead->prev->next = pBullet;
                        set->pEnemyShotHead->prev = pBullet;
                    }
                }
                // 弾の移動処理
                sEnemyShot* s = set->pEnemyShotHead->next;
                while (s != set->pEnemyShotHead) {
                    s->x += s->speed * cos(s->muki);
                    s->y += s->speed * sin(s->muki);
                    s = s->next;
                }
            };

            pBulletSet->pEnemyShotHead = new sEnemyShot;
            pBulletSet->pEnemyShotHead->prev = pBulletSet->pEnemyShotHead;
            pBulletSet->pEnemyShotHead->next = pBulletSet->pEnemyShotHead;

            pBulletSet->prev = enemyShotSetHead.prev;
            pBulletSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pBulletSet;
            enemyShotSetHead.prev = pBulletSet;
        }

        pShot = pNextShot;
    }
}

// 敵本体のパターン
void EnemyPat_OnTheWay_Gemini()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        // ボスは自機ショットの届かない上部の画面外に配置する
        enemy.x = 240.0;
        enemy.y = -80.0;
        enemy.maxHp = enemy.hp = 70; // 200から変更しないこと
    }

    // 一定間隔でザコ敵（大玉の編隊）を画面外から飛来させる
    if (count % 60 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotZakoFormation;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = -80.0;
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
}