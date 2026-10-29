#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  ザコ敵（大玉）の移動と射撃パターン
// ============================================================
static void MinionBulletMove(sEnemyShotSet* pEnemyShotSet) {
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

static void MinionFormation(sEnemyShotSet* pEnemyShotSet) {
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[14] == 1) { // ザコ敵フラグが立っている場合
            // ザコ敵の移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // ザコ敵の射撃（大玉以外の弾を使用）
            if (pShot->count % 45 == 0) {
                sEnemyShotSet* pNewSet = new sEnemyShotSet;
                pNewSet->count = 0;
                pNewSet->patternFunc = MinionBulletMove;
                pNewSet->x = pShot->x;
                pNewSet->y = pShot->y;
                pNewSet->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                pNewSet->kind = 0;
                pNewSet->pEnemyShotHead = new sEnemyShot;
                pNewSet->pEnemyShotHead->prev = pNewSet->pEnemyShotHead;
                pNewSet->pEnemyShotHead->next = pNewSet->pEnemyShotHead;

                // 5way狙い撃ち弾幕
                for (int i = 0; i < 5; i++) {
                    sEnemyShot* pBullet = new sEnemyShot;
                    pBullet->x = pShot->x;
                    pBullet->y = pShot->y;
                    pBullet->muki = pNewSet->muki + (i - 2) * 0.25;
                    pBullet->speed = 3.0 + GetRand(50) / 50.0;
                    // 大玉ではなく中玉や鱗弾を使用
                    pBullet->kind = (i % 2 == 0) ? img_enemyShotMediumBall[1] : img_enemyShotScale[3];

                    pBullet->prev = pNewSet->pEnemyShotHead->prev;
                    pBullet->next = pNewSet->pEnemyShotHead;
                    pNewSet->pEnemyShotHead->prev->next = pBullet;
                    pNewSet->pEnemyShotHead->prev = pBullet;
                }

                // 新しい弾幕セットをリストに追加
                pNewSet->prev = enemyShotSetHead.prev;
                pNewSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pNewSet;
                enemyShotSetHead.prev = pNewSet;
            }
        }
        else {
            // 通常の弾の移動（保険）
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  ボス本体の高密度弾幕パターン
// ============================================================
static void BossHeavyAttack(sEnemyShotSet* pEnemyShotSet) {
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double baseMuki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        // 速度差をつけた21way高密度弾幕
        for (int i = 0; i < 21; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = baseMuki + (i - 10) * 0.12;
            pShot->speed = 2.5 + (i % 3) * 1.5 + GetRand(50) / 50.0;
            // 鱗弾と菱形弾を交互に使用
            pShot->kind = (i % 2 == 0) ? img_enemyShotScale[4] : img_enemyShotDiamond[5];
            pShot->margin = 120;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  ザコ敵（大玉）と自機ショットの当たり判定処理
// ============================================================
static void CheckMinionCollision() {
    sEnemyShotSet* pSet = enemyShotSetHead.next;
    while (pSet != &enemyShotSetHead) {
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[14] == 1) { // ザコ敵フラグ
                sPlayerShot* pPShot = playerShotHead.next;
                while (pPShot != &playerShotHead) {
                    double dx = pPShot->x - pShot->x;
                    double dy = pPShot->y - pShot->y;
                    // 自機ショットの半径を約4.0、ザコ敵の半径をmargin(240.0)として判定
                    double hitRadius = 4.0 + pShot->margin; hitRadius = 44;

                    if (dx * dx + dy * dy < hitRadius * hitRadius) {
                        if (CheckSoundMem(sound_playerShotHit_default)) StopSoundMem(sound_playerShotHit_default);
                        PlaySoundMem(sound_playerShotHit_default, DX_PLAYTYPE_BACK);
                        
                        // 自機ショットを消去
                        sPlayerShot* pNextPShot = pPShot->next;
                        pPShot->prev->next = pPShot->next;
                        pPShot->next->prev = pPShot->prev;
                        delete pPShot;
                        pPShot = pNextPShot;

                        // ザコ敵のHPを減らす
                        pShot->param_i[15]--;

                        if (pShot->param_i[15] > 0) {
                            // 残HPに応じて色を変更 (0:赤 〜 8:橙)
                            int maxHp = pShot->param_i[13];
                            int currentHp = pShot->param_i[15];
                            int color = (currentHp * 8) / maxHp;
                            if (color > 8) color = 8;
                            if (color < 0) color = 0;
                            pShot->kind = img_enemyShotLargeBall[color];
                        }
                        else {
                            // HPが0になったらザコ敵を消去し、ボスに1ダメージ
                            enemy.hp--;

                            sEnemyShot* pNextShot = pShot->next;
                            pShot->prev->next = pShot->next;
                            pShot->next->prev = pShot->prev;
                            delete pShot;
                            pShot = pNextShot;
                            continue; // リスト構造が変わったため次の要素へスキップ
                        }
                        break; // 1フレームで1自機ショットのみ消費
                    }
                    pPShot = pPShot->next;
                }
            }
            pShot = pShot->next;
        }
        pSet = pSet->next;
    }
}

// ============================================================
//  敵本体のパターン関数
// ============================================================
void EnemyPat_OnTheWay_Qwen() {
    if (count == 1) {
        // ボスは自機ショットの届かない上部の画面外に配置
        enemy.x = 240.0;
        enemy.y = -80.0;
        enemy.maxHp = enemy.hp = 30;
    }

    // ザコ敵（大玉）の当たり判定とHP管理
    CheckMinionCollision();

    // ザコ敵の隊列飛来生成（180フレームごとにV字隊列で出現）
    if (count % 120 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = MinionFormation;
        pSet->x = 240.0;
        pSet->y = -100.0;
        pSet->muki = DX_PI / 2;
        pSet->kind = 0;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        // V字隊列の座標
        double startX[3] = { 160.0, 240.0, 320.0 };
        double startY[3] = { -140.0, -100.0, -140.0 };

        for (int i = 0; i < 3; i++) {
            sEnemyShot* pMinion = new sEnemyShot;
            pMinion->x = startX[i];
            pMinion->y = startY[i];
            pMinion->muki = DX_PI / 2; // 下向き
            pMinion->speed = 2.0;
            pMinion->kind = img_enemyShotLargeBall[8]; // 初期色は8:橙

            // 指定された仕様のパラメータ設定
            pMinion->margin = 240.0;       // marginは240.0に設定
            pMinion->param_i[14] = 1;      // ザコ敵識別フラグ
            pMinion->param_i[15] = 5;      // HP
            pMinion->param_i[13] = 5;      // 最大HP（色計算用）

            pMinion->prev = pSet->pEnemyShotHead->prev;
            pMinion->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pMinion;
            pSet->pEnemyShotHead->prev = pMinion;
        }

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // ボス本体による攻撃（難易度高めの道中ボス風弾幕）
    if (count % 60 == 1) {
        sEnemyShotSet* pBossSet = new sEnemyShotSet;
        pBossSet->count = 0;
        pBossSet->patternFunc = BossHeavyAttack;
        pBossSet->x = enemy.x;
        pBossSet->y = enemy.y;
        pBossSet->muki = DX_PI / 2;
        pBossSet->kind = 0;
        pBossSet->pEnemyShotHead = new sEnemyShot;
        pBossSet->pEnemyShotHead->prev = pBossSet->pEnemyShotHead;
        pBossSet->pEnemyShotHead->next = pBossSet->pEnemyShotHead;

        pBossSet->prev = enemyShotSetHead.prev;
        pBossSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pBossSet;
        enemyShotSetHead.prev = pBossSet;
    }
}