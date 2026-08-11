// enemyPat_KajitsuTensei.cpp
// 「果実転生」― スイカゲーム(合体パズル)モチーフの弾幕パターン
//
// フェーズ構成:
//   1. 果実投下・堆積   : 小玉(赤)が壺の中にランダムな列で降り積もる
//   2. 連鎖合体(小→中) : 隣接する同tierの玉同士が「触れた」ことを距離判定で検出し、
//                         中間点へ寄って合体し中玉(橙)に育つ
//   3. 連鎖合体(中→大) : 同じ仕組みで中玉が大玉(緑)へ育つ
//   4. スイカ降臨       : 大玉5個を十字クラスターにして特大スイカを表現し、
//                         予告点滅の後、同心3段リング爆散+自機狙い3way(種)で終幕
//
// 弾の大きさは小・中・大の3種類しかないため、
// 「小玉→中玉→大玉」の2段階合体にそのまま対応させ、
// フィナーレのみ大玉5個を十字に組んでワンランク上の巨大さを表現する。


#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 定数
// ============================================================
static const int    NUM_COLS = 6;
static const double COL_X[NUM_COLS] = { 150.0, 190.0, 230.0, 270.0, 310.0, 350.0 };
static const double FLOOR_Y = 470.0;      // 果実が積み上がる床面(壺の底)
static const double TOP_LIMIT_Y = 50.0;  // これより上には積み上げない
static const double ROW_SPACING = 12.0;
static const int    MAX_ROWS = (int)((FLOOR_Y - TOP_LIMIT_Y) / ROW_SPACING);

static const double WALL_LEFT_X = 120.0;
static const double WALL_RIGHT_X = 380.0;

static const int    MERGE_DURATION = 20;         // 合体演出にかけるフレーム数
static const double COLLIDE_DIST = 32.0;          // 合体判定に使う距離(px)
static const double COLLIDE_DIST2 = COLLIDE_DIST * COLLIDE_DIST;

// 果実の色: tier0=小玉(赤/サクランボ), tier1=中玉(橙/中間果実), tier2=大玉(緑/スイカ)
static const int FRUIT_COLOR[3] = { 0, 8, 2 };

// sEnemyShot の param 割り当て(ShotFruitField 内で使用):
//   param_d[0] = 落下開始y      param_d[1] = 着地y(床面上のy)
//   param_d[2] = 合体開始x      param_d[3] = 合体目標x(中間点)
//   param_i[1] = tier(0/1/2)
//   param_i[2] = 合体状態(0:待機 1:吸収役 2:消滅役)
//   param_i[3] = 着地フラグ(0:落下中 1:着地済み)
//   param_i[4] = 合体後のtier(吸収役のみ使用)
//   param_i[5] = 合体開始時のpShot->count

// ============================================================
// 弾幕: 壺の縁(左右の壁) ―― 静止したまま画面を区切るだけの障害物
// ============================================================
static void ShotJarWall(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        for (double y = FLOOR_Y + 10.0; y >= TOP_LIMIT_Y - 20.0; y -= 10.0) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = y;
            pShot->muki = -DX_PI / 2.0;
            pShot->speed = 0.0;
            pShot->kind = img_enemyShotSmallBall[3]; // シアン: ガラス壺の縁

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }
    // 以降は完全に静止(移動処理なし)
}

// ============================================================
// 弾幕: 合体ポップ(果実が融合した瞬間の小爆散)
// pEnemyShotSet->kind に tier(1 or 2)を格納し、規模・色に反映する
// ============================================================
static void ShotPop(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        int tier = pEnemyShotSet->kind;
        int num = (tier == 1) ? 6 : 12;
        int color = (tier == 1) ? 1 : 8; // 中玉合体=黄、大玉合体=橙のポップ

        for (int i = 0; i < num; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = (2.0 * DX_PI) * i / num + (GetRand(20) - 10) / 100.0;
            pShot->speed = (tier == 1) ? 2.0 : 3.0;
            pShot->kind = img_enemyShotSmallBall[color];

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
// 弾幕: 果実フィールド ―― 投下・堆積・連鎖合体をまとめて管理する本体
// ============================================================
static void ShotFruitField(sEnemyShotSet* pEnemyShotSet)
{
    static int columnHeight[NUM_COLS];

    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < NUM_COLS; i++) columnHeight[i] = 0;
    }

    // ---- 果実投下(小玉) ----
    if (pEnemyShotSet->count >= 1 && pEnemyShotSet->count <= 720 &&
        pEnemyShotSet->count % 12 == 1) {
        int col = -1;
        for (int tryCnt = 0; tryCnt < NUM_COLS; tryCnt++) {
            int c = GetRand(NUM_COLS - 1);
            if (columnHeight[c] < MAX_ROWS) { col = c; break; }
        }
        if (col != -1) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = COL_X[col];
            pShot->y = 60.0;
            pShot->muki = DX_PI / 2.0;
            pShot->speed = 0.0;
            pShot->kind = img_enemyShotSmallBall[FRUIT_COLOR[0]];
            pShot->param_d[0] = 60.0;
            pShot->param_d[1] = FLOOR_Y - columnHeight[col] * ROW_SPACING;
            pShot->param_i[1] = 0; // tier
            pShot->param_i[2] = 0; // 合体状態: 待機
            pShot->param_i[3] = 0; // 落下中

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;

            columnHeight[col]++;
        }
    }

    // ---- 落下/着地判定(待機中の弾のみ) ----
    for (sEnemyShot* s = pEnemyShotSet->pEnemyShotHead->next; s != pEnemyShotSet->pEnemyShotHead; s = s->next) {
        if (s->param_i[2] != 0 || s->param_i[3] != 0) continue;
        double y = fmin(s->param_d[1], s->param_d[0] + 3.0 * s->count);
        s->y = y;
        if (y >= s->param_d[1] - 0.01) s->param_i[3] = 1; // 着地
    }

    // ---- 当たり判定による連鎖合体(1回のスキャンで最大1組だけ成立させる) ----
    if (pEnemyShotSet->count >= 1 && pEnemyShotSet->count % 8 == 0) {
        sEnemyShot* found1 = nullptr;
        sEnemyShot* found2 = nullptr;

        for (sEnemyShot* a = pEnemyShotSet->pEnemyShotHead->next;
            a != pEnemyShotSet->pEnemyShotHead && found1 == nullptr; a = a->next) {
            if (a->param_i[2] != 0 || a->param_i[3] != 1 || a->param_i[1] >= 2) continue;

            for (sEnemyShot* b = a->next; b != pEnemyShotSet->pEnemyShotHead; b = b->next) {
                if (b->param_i[2] != 0 || b->param_i[3] != 1) continue;
                if (b->param_i[1] != a->param_i[1]) continue;

                double dx = a->x - b->x;
                double dy = a->y - b->y;
                if (dx * dx + dy * dy <= COLLIDE_DIST2) {
                    found1 = a;
                    found2 = b;
                    break;
                }
            }
        }

        if (found1 != nullptr) {
            int tier = found1->param_i[1];
            double midX = (found1->x + found2->x) / 2.0;

            found1->param_i[2] = 1; // 吸収役(合体して育つ)
            found1->param_i[4] = tier + 1;
            found1->param_d[2] = found1->x;
            found1->param_d[3] = midX;
            found1->param_i[5] = found1->count;

            found2->param_i[2] = 2; // 消滅役(相手に合体を譲る)
            found2->param_d[2] = found2->x;
            found2->param_d[3] = midX;
            found2->param_i[5] = found2->count;
        }
    }

    // ---- 合体アニメーション処理 ----
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        if (pShot->param_i[2] == 1) {
            // 吸収(合体して育つ)
            int elapsed = pShot->count - pShot->param_i[5];
            double t = (double)elapsed / MERGE_DURATION;
            if (t > 1.0) t = 1.0;
            pShot->x = pShot->param_d[2] + (pShot->param_d[3] - pShot->param_d[2]) * t;

            if (elapsed >= MERGE_DURATION) {
                int newTier = pShot->param_i[4];
                pShot->param_i[1] = newTier;
                pShot->param_i[2] = 0; // 待機に戻る(さらに上のtierとも合体できる)
                pShot->x = pShot->param_d[3];

                int color = FRUIT_COLOR[newTier];
                pShot->kind = (newTier == 1) ? img_enemyShotMediumBall[color]
                    : img_enemyShotLargeBall[color];

                sEnemyShotSet* pPop = new sEnemyShotSet;
                pPop->count = 0;
                pPop->patternFunc = ShotPop;
                pPop->x = pShot->x;
                pPop->y = pShot->y;
                pPop->kind = newTier;
                pPop->pEnemyShotHead = new sEnemyShot;
                pPop->pEnemyShotHead->prev = pPop->pEnemyShotHead;
                pPop->pEnemyShotHead->next = pPop->pEnemyShotHead;
                pPop->prev = enemyShotSetHead.prev;
                pPop->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pPop;
                enemyShotSetHead.prev = pPop;

                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(newTier == 1 ? sound_enemyShot_light : sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }
        }
        else if (pShot->param_i[2] == 2) {
            // 消滅(相手に合体を譲る)
            int elapsed = pShot->count - pShot->param_i[5];
            double t = (double)elapsed / MERGE_DURATION;
            if (t > 1.0) t = 1.0;
            pShot->x = pShot->param_d[2] + (pShot->param_d[3] - pShot->param_d[2]) * t;

            if (elapsed >= MERGE_DURATION) {
                pShot->x = -9999.0; // 画面外へ逃がしメインルーチンの自動消去に委ねる
            }
        }

        pShot = pNext;
    }
}

// ============================================================
// 弾幕: フィナーレ・同心リング爆散(スイカの実)
// ============================================================
static void ShotFinaleRing(sEnemyShotSet* pEnemyShotSet)
{
    static const int RING_TIMING[3] = { 0, 15, 30 };
    static const int RING_NUM[3] = { 16, 20, 24 };
    static const double RING_SPEED[3] = { 2.0, 2.6, 3.2 };

    for (int r = 0; r < 3; r++) {
        if (pEnemyShotSet->count == RING_TIMING[r] + 1) {
            double offset = (r % 2 == 0) ? 0.0 : DX_PI / RING_NUM[r];
            for (int i = 0; i < RING_NUM[r]; i++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x;
                pShot->y = pEnemyShotSet->y;
                pShot->muki = (2.0 * DX_PI) * i / RING_NUM[r] + offset;
                pShot->speed = RING_SPEED[r];
                pShot->kind = img_enemyShotMediumBall[2]; // 緑: スイカの果肉
                pShot->param_d[0] = pEnemyShotSet->x;
                pShot->param_d[1] = pEnemyShotSet->y;

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double r = pShot->speed * pShot->count; // countのみから導出
        pShot->x = pShot->param_d[0] + r * cos(pShot->muki);
        pShot->y = pShot->param_d[1] + r * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 弾幕: フィナーレ・自機狙い3way(種)
// ============================================================
static void ShotFinaleAimed(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 1) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double spread = DX_PI / 10.0; // 約18度間隔
        for (int i = -1; i <= 1; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = pEnemyShotSet->muki + spread * i;
            pShot->speed = 3.6;
            pShot->kind = img_enemyShotBullet[7]; // 黒: スイカの種
            pShot->param_d[0] = pEnemyShotSet->x;
            pShot->param_d[1] = pEnemyShotSet->y;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x = pShot->param_d[0] + pShot->speed * cos(pShot->muki) * pShot->count;
        pShot->y = pShot->param_d[1] + pShot->speed * sin(pShot->muki) * pShot->count;
        pShot = pShot->next;
    }
}

// ============================================================
// 弾幕: スイカ降臨 ―― 大玉5個を十字クラスターにして特大スイカを表現
// (大きさは小・中・大の3種類しかないため、複数弾を組み合わせて表現)
// ============================================================
static void ShotWatermelonCore(sEnemyShotSet* pEnemyShotSet)
{
    static sEnemyShot* clusterShots[5];
    static const double OFFSET_X[5] = { 0.0, -18.0, 18.0, 0.0, 0.0 };
    static const double OFFSET_Y[5] = { 0.0, 0.0, 0.0, -18.0, 18.0 };

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 5; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x + OFFSET_X[i];
            pShot->y = pEnemyShotSet->y + OFFSET_Y[i];
            pShot->muki = -DX_PI / 2.0;
            pShot->speed = 0.0;
            pShot->kind = img_enemyShotLargeBall[2];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;

            clusterShots[i] = pShot;
        }
    }

    // 予告点滅(90フレーム): 緑と白を交互に切り替える
    if (pEnemyShotSet->count < 90) {
        int color = ((pEnemyShotSet->count / 6) % 2 == 0) ? 2 : 6;
        for (int i = 0; i < 5; i++) {
            clusterShots[i]->kind = img_enemyShotLargeBall[color];
        }
    }

    // 破裂の瞬間
    if (pEnemyShotSet->count == 90) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 5; i++) {
            clusterShots[i]->x = -9999.0; // クラスターを消去(スイカが割れて消える)
        }

        sEnemyShotSet* pRing = new sEnemyShotSet;
        pRing->count = 0;
        pRing->patternFunc = ShotFinaleRing;
        pRing->x = pEnemyShotSet->x;
        pRing->y = pEnemyShotSet->y;
        pRing->pEnemyShotHead = new sEnemyShot;
        pRing->pEnemyShotHead->prev = pRing->pEnemyShotHead;
        pRing->pEnemyShotHead->next = pRing->pEnemyShotHead;
        pRing->prev = enemyShotSetHead.prev;
        pRing->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pRing;
        enemyShotSetHead.prev = pRing;

        sEnemyShotSet* pAimed = new sEnemyShotSet;
        pAimed->count = 0;
        pAimed->patternFunc = ShotFinaleAimed;
        pAimed->x = pEnemyShotSet->x;
        pAimed->y = pEnemyShotSet->y;
        pAimed->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pAimed->pEnemyShotHead = new sEnemyShot;
        pAimed->pEnemyShotHead->prev = pAimed->pEnemyShotHead;
        pAimed->pEnemyShotHead->next = pAimed->pEnemyShotHead;
        pAimed->prev = enemyShotSetHead.prev;
        pAimed->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pAimed;
        enemyShotSetHead.prev = pAimed;
    }
}

// ============================================================
// 敵本体のパターン: 「果実転生」
// ============================================================
void EnemyPat_SuikaGame_Claude()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 70.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定

        // 壺の縁(左右の壁)
        sEnemyShotSet* pWallL = new sEnemyShotSet;
        pWallL->count = 0;
        pWallL->patternFunc = ShotJarWall;
        pWallL->x = WALL_LEFT_X;
        pWallL->y = 0.0;
        pWallL->pEnemyShotHead = new sEnemyShot;
        pWallL->pEnemyShotHead->prev = pWallL->pEnemyShotHead;
        pWallL->pEnemyShotHead->next = pWallL->pEnemyShotHead;
        pWallL->prev = enemyShotSetHead.prev;
        pWallL->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pWallL;
        enemyShotSetHead.prev = pWallL;

        sEnemyShotSet* pWallR = new sEnemyShotSet;
        pWallR->count = 0;
        pWallR->patternFunc = ShotJarWall;
        pWallR->x = WALL_RIGHT_X;
        pWallR->y = 0.0;
        pWallR->pEnemyShotHead = new sEnemyShot;
        pWallR->pEnemyShotHead->prev = pWallR->pEnemyShotHead;
        pWallR->pEnemyShotHead->next = pWallR->pEnemyShotHead;
        pWallR->prev = enemyShotSetHead.prev;
        pWallR->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pWallR;
        enemyShotSetHead.prev = pWallR;

        // 果実フィールド(投下・堆積・連鎖合体)
        sEnemyShotSet* pField = new sEnemyShotSet;
        pField->count = 0;
        pField->patternFunc = ShotFruitField;
        pField->x = 0.0;
        pField->y = 0.0;
        pField->pEnemyShotHead = new sEnemyShot;
        pField->pEnemyShotHead->prev = pField->pEnemyShotHead;
        pField->pEnemyShotHead->next = pField->pEnemyShotHead;
        pField->prev = enemyShotSetHead.prev;
        pField->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pField;
        enemyShotSetHead.prev = pField;
    }

    // 壺がゆらゆら揺れるだけの控えめな動き(countのみから導出)
    enemy.x = 240.0 + 15.0 * sin(count * 0.02);
    enemy.y = 70.0 + 5.0 * sin(count * 0.013);

    // スイカ降臨(フィナーレ)
    if (count == 900) {
        sEnemyShotSet* pCore = new sEnemyShotSet;
        pCore->count = 0;
        pCore->patternFunc = ShotWatermelonCore;
        pCore->x = 240.0;
        pCore->y = 260.0;
        pCore->pEnemyShotHead = new sEnemyShot;
        pCore->pEnemyShotHead->prev = pCore->pEnemyShotHead;
        pCore->pEnemyShotHead->next = pCore->pEnemyShotHead;
        pCore->prev = enemyShotSetHead.prev;
        pCore->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pCore;
        enemyShotSetHead.prev = pCore;
    }
}