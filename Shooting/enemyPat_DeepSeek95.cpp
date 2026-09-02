// enemyPat_tmp.cpp
// 反則弾幕：ヒットボックス改竄（デバッグモード）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 反則で使用するシフト量
#define SHIFT_DIST_HITBOX  48.0   // 自機当たり判定のずらし量
#define SHIFT_DIST_BULLET  24.0   // 敵弾のずらし量

// ファイルスコープの状態変数（EnemyPat_Violate_DeepSeek と各パターン関数で共有）
static int    g_arrowDir = 0;          // 0:右, 1:下, 2:左, 3:上
static int    g_arrowTimer = 0;        // 矢印切り替え用タイマー
static int    g_rollbackDone = 0;      // HP巻き戻し済みフラグ
static int    g_rollbackTimer = 0;     // 巻き戻し後の無敵時間
static double g_hitboxOffX = SHIFT_DIST_HITBOX;
static double g_hitboxOffY = 0.0;
static double g_bulletOffX = SHIFT_DIST_BULLET;
static double g_bulletOffY = 0.0;

// ------------------------------------------------------------
// 汎用：弾を ShotSet のリストに追加する
// ------------------------------------------------------------
static void SpawnBullet(sEnemyShotSet* set, double x, double y, int kind)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = 0.0;
    p->speed = 0.0;
    p->kind = kind;
    p->margin = 120;

    p->prev = set->pEnemyShotHead->prev;
    p->next = set->pEnemyShotHead;
    set->pEnemyShotHead->prev->next = p;
    set->pEnemyShotHead->prev = p;
}

// ------------------------------------------------------------
// パターン：画面中央に "JUDGE SHIFT" と矢印を敵弾で描画
// ------------------------------------------------------------
static void PatternTextArrow(sEnemyShotSet* set)
{
    // 矢印方向が変わったら、弾を全て消して再構築する
    if (set->count == 0 || set->param_i[0] != g_arrowDir) {
        // 既存の弾を削除
        sEnemyShot* p = set->pEnemyShotHead->next;
        while (p != set->pEnemyShotHead) {
            sEnemyShot* next = p->next;
            delete p;
            p = next;
        }
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;
        set->param_i[0] = g_arrowDir;   // 現在の矢印方向を記録

        // 文字はピクセルフォントを簡略化し、実際には「J」「S」などの一部を弾で表現
        // ここでは時間の都合上、矢印のみを弾で明確に描く
        int colorArrow = 1;  // 黄色
        int colorText = 6;  // 白（テキスト用）
        double cx = 240.0, cy = 240.0;

        // 簡単な「J S」の文字（5x7フォントの一部）
        // 実際の実装では全文字を描いてもよいが、ここでは省略
        // SpawnBullet(set, 220, 230, img_enemyShotSmallBall[colorText]); // サンプル

        // 矢印を描画
        switch (g_arrowDir) {
        case 0: // 右
            for (int i = 0; i < 3; i++) {
                SpawnBullet(set, cx - 10 + i * 5, cy, img_enemyShotSmallBall[colorText]);
            }
            SpawnBullet(set, cx + 5, cy - 5, img_enemyShotSmallBall[colorArrow]);
            SpawnBullet(set, cx + 5, cy + 5, img_enemyShotSmallBall[colorArrow]);
            break;
        case 1: // 下
            for (int i = 0; i < 3; i++) {
                SpawnBullet(set, cx, cy - 10 + i * 5, img_enemyShotSmallBall[colorText]);
            }
            SpawnBullet(set, cx - 5, cy + 5, img_enemyShotSmallBall[colorArrow]);
            SpawnBullet(set, cx + 5, cy + 5, img_enemyShotSmallBall[colorArrow]);
            break;
        case 2: // 左
            for (int i = 0; i < 3; i++) {
                SpawnBullet(set, cx + 10 - i * 5, cy, img_enemyShotSmallBall[colorText]);
            }
            SpawnBullet(set, cx - 5, cy - 5, img_enemyShotSmallBall[colorArrow]);
            SpawnBullet(set, cx - 5, cy + 5, img_enemyShotSmallBall[colorArrow]);
            break;
        case 3: // 上
            for (int i = 0; i < 3; i++) {
                SpawnBullet(set, cx, cy + 10 - i * 5, img_enemyShotSmallBall[colorText]);
            }
            SpawnBullet(set, cx - 5, cy - 5, img_enemyShotSmallBall[colorArrow]);
            SpawnBullet(set, cx + 5, cy - 5, img_enemyShotSmallBall[colorArrow]);
            break;
        }
    }
    // 弾は静止しているので移動処理は不要
}

// ------------------------------------------------------------
// パターン：自機の「真の当たり判定位置」を示すリング
// ------------------------------------------------------------
static void PatternHitboxRing(sEnemyShotSet* set)
{
    double hx = player.x + g_hitboxOffX;
    double hy = player.y + g_hitboxOffY;

    if (set->count == 0) {
        // 初回のみリングを構成する8個の弾を生成
        for (int i = 0; i < 8; i++) {
            double angle = i * 2.0 * DX_PI / 8.0;
            double rx = hx + cos(angle) * 10.0;
            double ry = hy + sin(angle) * 10.0;
            SpawnBullet(set, rx, ry, img_enemyShotSmallBall[4]); // 青
        }
    }
    else {
        // 毎フレーム、リングの位置を真の当たり判定位置に追従させる
        sEnemyShot* p = set->pEnemyShotHead->next;
        int i = 0;
        while (p != set->pEnemyShotHead) {
            double angle = i * 2.0 * DX_PI / 8.0;
            p->x = hx + cos(angle) * 10.0;
            p->y = hy + sin(angle) * 10.0;
            p = p->next;
            i++;
        }
    }
}

// ------------------------------------------------------------
// パターン：HP巻き戻し時の表示
// ------------------------------------------------------------
static void PatternHpRollback(sEnemyShotSet* set)
{
    if (set->count == 0) {
        // 「HP = 9999」を弾で表現（簡略化のため「9999」のみ）
        int baseX = 200, baseY = 100;
        for (int i = 0; i < 4; i++) {
            SpawnBullet(set, baseX + i * 6, baseY, img_enemyShotSmallBall[5]); // マゼンタ
        }
        // 「NOW LOADING...」の代わりに3つの点
        baseY += 20;
        for (int i = 0; i < 3; i++) {
            SpawnBullet(set, baseX + i * 6, baseY, img_enemyShotSmallBall[6]); // 白
        }
    }
    // 静止表示
}

// ------------------------------------------------------------
// 通常弾幕パターン（オフセット付き散弾）
// ------------------------------------------------------------
static void ShotScatterWithOffset(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 9; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // 反則：敵弾の発生座標に24pxのオフセットを加算
            pEnemyShot->x = pEnemyShotSet->x + GetRand(480) - 240 + g_bulletOffX;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(40) - 20 + g_bulletOffY;
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(120) - 60) / 180.0 * DX_PI;
            pEnemyShot->speed = (200 + GetRand(200)) / 100.0;

            switch (pEnemyShotSet->kind % 8) {
            case 0: pEnemyShot->kind = img_enemyShotSmallBall[i]; break;
            case 1: pEnemyShot->kind = img_enemyShotMediumBall[i]; break;
            case 2: pEnemyShot->kind = img_enemyShotLargeBall[i]; break;
            case 3: pEnemyShot->kind = img_enemyShotBullet[i]; break;
            case 4: pEnemyShot->kind = img_enemyShotScale[i]; break;
            case 5: pEnemyShot->kind = img_enemyShotDiamond[i]; break;
            case 6: pEnemyShot->kind = img_enemyShotMediumOval[i]; break;
            case 7: pEnemyShot->kind = img_enemyShotLaser[i]; break;
            }

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体パターン
// ------------------------------------------------------------
void EnemyPat_Violate_DeepSeek()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;

        g_arrowDir = 0;
        g_arrowTimer = 0;
        g_rollbackDone = 0;
        g_rollbackTimer = 0;
        g_hitboxOffX = SHIFT_DIST_HITBOX;
        g_hitboxOffY = 0.0;
        g_bulletOffX = SHIFT_DIST_BULLET;
        g_bulletOffY = 0.0;

        // テキスト・矢印表示用の ShotSet を生成
        sEnemyShotSet* textSet = new sEnemyShotSet;
        textSet->count = 0;
        textSet->patternFunc = PatternTextArrow;
        textSet->x = 240.0;
        textSet->y = 240.0;
        textSet->muki = 0.0;
        textSet->kind = 0;
        textSet->param_i[0] = -1;   // 強制的に初回構築させる
        textSet->pEnemyShotHead = new sEnemyShot;
        textSet->pEnemyShotHead->prev = textSet->pEnemyShotHead;
        textSet->pEnemyShotHead->next = textSet->pEnemyShotHead;
        // グローバルリストへ追加
        textSet->prev = enemyShotSetHead.prev;
        textSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = textSet;
        enemyShotSetHead.prev = textSet;

        // 自機当たり判定リング用の ShotSet を生成
        sEnemyShotSet* ringSet = new sEnemyShotSet;
        ringSet->count = 0;
        ringSet->patternFunc = PatternHitboxRing;
        ringSet->x = player.x + g_hitboxOffX;
        ringSet->y = player.y + g_hitboxOffY;
        ringSet->muki = 0.0;
        ringSet->kind = 0;
        ringSet->pEnemyShotHead = new sEnemyShot;
        ringSet->pEnemyShotHead->prev = ringSet->pEnemyShotHead;
        ringSet->pEnemyShotHead->next = ringSet->pEnemyShotHead;
        ringSet->prev = enemyShotSetHead.prev;
        ringSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = ringSet;
        enemyShotSetHead.prev = ringSet;
    }
    else {
        // 敵機の水平移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 矢印方向の切り替え（4秒ごと）
    g_arrowTimer++;
    if (g_arrowTimer >= 240) {
        g_arrowTimer = 0;
        int oldDir = g_arrowDir;
        g_arrowDir = (g_arrowDir + 1) % 4;

        // 新しいオフセットを計算
        double newHitboxOffX = 0.0, newHitboxOffY = 0.0;
        double newBulletOffX = 0.0, newBulletOffY = 0.0;
        switch (g_arrowDir) {
        case 0: // 右
            newHitboxOffX = SHIFT_DIST_HITBOX;
            newBulletOffX = SHIFT_DIST_BULLET;
            break;
        case 1: // 下
            newHitboxOffY = SHIFT_DIST_HITBOX;
            newBulletOffY = SHIFT_DIST_BULLET;
            break;
        case 2: // 左
            newHitboxOffX = -SHIFT_DIST_HITBOX;
            newBulletOffX = -SHIFT_DIST_BULLET;
            break;
        case 3: // 上
            newHitboxOffY = -SHIFT_DIST_HITBOX;
            newBulletOffY = -SHIFT_DIST_BULLET;
            break;
        }

        // 既存の全敵弾をオフセット差分だけ移動（24pxシフト）
        double deltaBX = newBulletOffX - g_bulletOffX;
        double deltaBY = newBulletOffY - g_bulletOffY;
        if (deltaBX != 0.0 || deltaBY != 0.0) {
            sEnemyShotSet* set = enemyShotSetHead.next;
            while (set != &enemyShotSetHead) {
                sEnemyShot* shot = set->pEnemyShotHead->next;
                while (shot != set->pEnemyShotHead) {
                    shot->x += deltaBX;
                    shot->y += deltaBY;
                    shot = shot->next;
                }
                set = set->next;
            }
        }

        // オフセットを更新
        g_bulletOffX = newBulletOffX;
        g_bulletOffY = newBulletOffY;
        g_hitboxOffX = newHitboxOffX;
        g_hitboxOffY = newHitboxOffY;
    }

    // 通常弾幕を定期的に発射
    if (count % 20 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotScatterWithOffset;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = shot_count++;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // HP巻き戻し処理（1回だけ）
    if (!g_rollbackDone && enemy.hp <= 1) {
        g_rollbackDone = 1;
        g_rollbackTimer = 120; // 2秒間無敵
        enemy.hp = (int)(enemy.maxHp * 0.25);

        // HP巻き戻し表示用 ShotSet を生成
        sEnemyShotSet* rollbackSet = new sEnemyShotSet;
        rollbackSet->count = 0;
        rollbackSet->patternFunc = PatternHpRollback;
        rollbackSet->x = 240.0;
        rollbackSet->y = 120.0;
        rollbackSet->muki = 0.0;
        rollbackSet->kind = 0;
        rollbackSet->pEnemyShotHead = new sEnemyShot;
        rollbackSet->pEnemyShotHead->prev = rollbackSet->pEnemyShotHead;
        rollbackSet->pEnemyShotHead->next = rollbackSet->pEnemyShotHead;
        rollbackSet->prev = enemyShotSetHead.prev;
        rollbackSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = rollbackSet;
        enemyShotSetHead.prev = rollbackSet;
    }

    // 巻き戻し後の無敵時間を消化
    if (g_rollbackDone && g_rollbackTimer > 0) {
        g_rollbackTimer--;
        // ボスが無敵状態であることを示す（実際のダメージ処理は外部で行われる想定）
    }

    // 自機ショットの発射原点をリング位置に変更する処理は、
    // 実際のショット生成ルーチンに介入できないため、ここでは未実装。
    // 代わりに、リングが真の当たり判定位置を示している。
}