// enemyPat_memoryHack.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// -------------------------------------------------
// フェーズ1：スキャン - 自機座標を「記録」する演出
// -------------------------------------------------
static void ShotScan(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    int phase = pEnemyShotSet->param_i[0]; // 0:格子, 1:検索波, 2:ロックオン

    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        pEnemyShotSet->param_i[0] = 0;
    }

    // フェーズ0：メモリダンプ風格子（細長い短レーザーを格子状に）
    if (phase == 0 && pEnemyShotSet->count == 5) {
        for (int i = 0; i < 12; i++) {
            for (int j = 0; j < 12; j++) {
                pEnemyShot = new sEnemyShot;
                // 横方向のレーザー
                pEnemyShot->x = j * 40 + 20;
                pEnemyShot->y = i * 40 + 20;
                pEnemyShot->muki = 0;
                pEnemyShot->speed = 0; // 静止
                pEnemyShot->kind = img_enemyShotLaser[7]; // 白
                pEnemyShot->param_i[0] = 1; // 寿命管理用
                pEnemyShot->margin = 480;
                // リンク
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
        pEnemyShotSet->param_i[0] = 1;
    }

    // フェーズ1：検索波（同心円状のリング）
    if (phase == 1 && pEnemyShotSet->count % 20 == 10 && pEnemyShotSet->count < 80) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double angleToPlayer = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        double dist = hypot(player.x - pEnemyShotSet->x, player.y - pEnemyShotSet->y);

        for (int i = 0; i < 24; i++) {
            pEnemyShot = new sEnemyShot;
            double ang = i * (2.0 * DX_PI / 24.0);
            pEnemyShot->x = pEnemyShotSet->x + cos(ang) * 10;
            pEnemyShot->y = pEnemyShotSet->y + sin(ang) * 10;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = dist / 60.0 + 0.5; // プレイヤーに届く速度
            pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白
            pEnemyShot->param_i[0] = 2; // 寿命
            pEnemyShot->margin = 480;
            // リンク
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // フェーズ2：十字カーソル（プレイヤー位置に表示）
    if (phase == 1 && pEnemyShotSet->count == 85) {
        pEnemyShotSet->param_d[0] = player.x; // 記録したX
        pEnemyShotSet->param_d[1] = player.y; // 記録したY
        pEnemyShotSet->param_i[0] = 2;

        // 十字カーソル（5つの小玉）
        for (int i = -2; i <= 2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = player.x + i * 8;
            pEnemyShot->y = player.y;
            pEnemyShot->muki = 0;
            pEnemyShot->speed = 0;
            pEnemyShot->kind = img_enemyShotSmallBall[7]; // 黒
            pEnemyShot->param_i[0] = 3;
            pEnemyShot->margin = 480;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = player.x;
            pEnemyShot->y = player.y + i * 8;
            pEnemyShot->muki = 0;
            pEnemyShot->speed = 0;
            pEnemyShot->kind = img_enemyShotSmallBall[7]; // 黒
            pEnemyShot->param_i[0] = 3;
            pEnemyShot->margin = 480;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動と寿命管理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) { // 格子レーザー
            if (pEnemyShotSet->count > 40) {
                sEnemyShot* pDel = pShot;
                pShot = pShot->next;
                pDel->prev->next = pDel->next;
                pDel->next->prev = pDel->prev;
                delete pDel;
                continue;
            }
        }
        else if (pShot->param_i[0] == 2) { // 検索波
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            if (pShot->count > 60) {
                sEnemyShot* pDel = pShot;
                pShot = pShot->next;
                pDel->prev->next = pDel->next;
                pDel->next->prev = pDel->prev;
                delete pDel;
                continue;
            }
        }
        else if (pShot->param_i[0] == 3) { // カーソル
            if (pEnemyShotSet->count > 110) {
                sEnemyShot* pDel = pShot;
                pShot = pShot->next;
                pDel->prev->next = pDel->next;
                pDel->next->prev = pDel->prev;
                delete pDel;
                continue;
            }
        }
        pShot = pShot->next;
    }
}

// -------------------------------------------------
// フェーズ2：座標ロック - 自機を固定する
// -------------------------------------------------
static void ShotLock(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 四隅からのロックオン光線
        double px = pEnemyShotSet->param_d[0]; // 記録されたプレイヤーX
        double py = pEnemyShotSet->param_d[1]; // 記録されたプレイヤーY

        // 左上
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = 0; pEnemyShot->y = 0;
        pEnemyShot->muki = atan2(py - 0, px - 0);
        pEnemyShot->speed = hypot(px, py) / 30.0;
        pEnemyShot->kind = img_enemyShotLaser[4]; // 青
        pEnemyShot->param_i[0] = 1;
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 右上
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = 480; pEnemyShot->y = 0;
        pEnemyShot->muki = atan2(py - 0, px - 480);
        pEnemyShot->speed = hypot(px - 480, py) / 30.0;
        pEnemyShot->kind = img_enemyShotLaser[4]; // 青
        pEnemyShot->param_i[0] = 1;
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 左下
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = 0; pEnemyShot->y = 480;
        pEnemyShot->muki = atan2(py - 480, px - 0);
        pEnemyShot->speed = hypot(px, py - 480) / 30.0;
        pEnemyShot->kind = img_enemyShotLaser[4]; // 青
        pEnemyShot->param_i[0] = 1;
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

        // 右下
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = 480; pEnemyShot->y = 480;
        pEnemyShot->muki = atan2(py - 480, px - 480);
        pEnemyShot->speed = hypot(px - 480, py - 480) / 30.0;
        pEnemyShot->kind = img_enemyShotLaser[4]; // 青
        pEnemyShot->param_i[0] = 1;
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 光線到達後、枠を表示
    if (pEnemyShotSet->count == 35) {
        double px = pEnemyShotSet->param_d[0];
        double py = pEnemyShotSet->param_d[1];

        // 正方形の枠（16個の小玉を頂点と辺上に）
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = px - 30 + j * 20;
                pEnemyShot->y = py - 30 + i * 20;
                if (i == 0 || i == 3 || j == 0 || j == 3) { // 外周のみ
                    pEnemyShot->muki = 0;
                    pEnemyShot->speed = 0;
                    pEnemyShot->kind = img_enemyShotSmallBall[7]; // 黒
                    pEnemyShot->param_i[0] = 2; // 枠
                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
                else {
                    delete pEnemyShot; // 内側は不要
                }
            }
        }
    }

    // 弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) { // 光線
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            if (pShot->count > 40) {
                sEnemyShot* pDel = pShot;
                pShot = pShot->next;
                pDel->prev->next = pDel->next;
                pDel->next->prev = pDel->prev;
                delete pDel;
                continue;
            }
        }
        else if (pShot->param_i[0] == 2) { // 枠
            if (pEnemyShotSet->count > 120) {
                sEnemyShot* pDel = pShot;
                pShot = pShot->next;
                pDel->prev->next = pDel->next;
                pDel->next->prev = pDel->prev;
                delete pDel;
                continue;
            }
        }
        pShot = pShot->next;
    }
}

// -------------------------------------------------
// フェーズ3：無敵貫通 - 無敵を無視する弾
// -------------------------------------------------
static void ShotPierce(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 全方位に赤い大玉（中心に黒い核）
        for (int i = 0; i < 16; i++) {
            double ang = i * (2.0 * DX_PI / 16.0);

            // 外側：赤い大玉
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = enemy.x + cos(ang) * 20;
            pEnemyShot->y = enemy.y + sin(ang) * 20;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 1.5;
            pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤
            pEnemyShot->param_i[0] = 1; // 通常弾
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 中心：黒い核（無敵貫通演出）
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = enemy.x + cos(ang) * 20;
            pEnemyShot->y = enemy.y + sin(ang) * 20;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 1.5;
            pEnemyShot->kind = img_enemyShotSmallBall[7]; // 黒
            pEnemyShot->param_i[0] = 2; // 核
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

// -------------------------------------------------
// フェーズ4：HPリダイレクト - 敵が回復する弾
// -------------------------------------------------
static void ShotRedirect(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 緑色の弾を自機に向けて発射
        for (int i = 0; i < 6; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = enemy.x + GetRand(60) - 30;
            pEnemyShot->y = enemy.y + 20;
            pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x) + (GetRand(20) - 10) / 180.0 * DX_PI;
            pEnemyShot->speed = 1.0 + GetRand(100) / 100.0;
            pEnemyShot->kind = img_enemyShotMediumBall[2]; // 緑
            pEnemyShot->param_i[0] = 1; // 緑弾
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動とUターン処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // 自機に近づいたらUターン（演出上の処理）
            double distToPlayer = hypot(player.x - pShot->x, player.y - pShot->y);
            if (distToPlayer < 15 && pShot->param_i[1] == 0) {
                pShot->param_i[1] = 1; // Uターン済みフラグ
                pShot->muki = atan2(enemy.y - pShot->y, enemy.x - pShot->x);
                pShot->speed *= 2.0;
                // 軌跡演出：残像を残す
                for (int i = 0; i < 3; i++) {
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = pShot->x + GetRand(10) - 5;
                    pEnemyShot->y = pShot->y + GetRand(10) - 5;
                    pEnemyShot->muki = 0;
                    pEnemyShot->speed = 0;
                    pEnemyShot->kind = img_enemyShotSmallBall[2]; // 緑
                    pEnemyShot->param_i[0] = 2; // 軌跡
                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }

            // 敵に到達したら回復エフェクト
            double distToEnemy = hypot(enemy.x - pShot->x, enemy.y - pShot->y);
            if (distToEnemy < 20 && pShot->param_i[1] == 1) {
                // 回復エフェクト：同心円
                for (int r = 0; r < 3; r++) {
                    for (int i = 0; i < 12; i++) {
                        pEnemyShot = new sEnemyShot;
                        double ang = i * (2.0 * DX_PI / 12.0);
                        pEnemyShot->x = enemy.x + cos(ang) * (r * 15 + 10);
                        pEnemyShot->y = enemy.y + sin(ang) * (r * 15 + 10);
                        pEnemyShot->muki = ang;
                        pEnemyShot->speed = 0.5;
                        pEnemyShot->kind = img_enemyShotSmallBall[2]; // 緑
                        pEnemyShot->param_i[0] = 3; // 回復エフェクト
                        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                    }
                }
                // 弾を削除
                sEnemyShot* pDel = pShot;
                pShot = pShot->next;
                pDel->prev->next = pDel->next;
                pDel->next->prev = pDel->prev;
                delete pDel;
                continue;
            }

            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (pShot->param_i[0] == 2) { // 軌跡
            if (pShot->count > 20) {
                sEnemyShot* pDel = pShot;
                pShot = pShot->next;
                pDel->prev->next = pDel->next;
                pDel->next->prev = pDel->prev;
                delete pDel;
                continue;
            }
        }
        else if (pShot->param_i[0] == 3) { // 回復エフェクト
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            if (pShot->count > 30) {
                sEnemyShot* pDel = pShot;
                pShot = pShot->next;
                pDel->prev->next = pDel->next;
                pDel->next->prev = pDel->prev;
                delete pDel;
                continue;
            }
        }
        pShot = pShot->next;
    }
}

// -------------------------------------------------
// フェーズ5：自機ショット改竄 - 自機の弾が自機を狙う
// -------------------------------------------------
static void ShotHack(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 紫色の弾を自機ショットの軌道上に出現させる演出
        // 実際には自機ショットの座標を監視する代わりに、自機から一定距離の位置に出現
        for (int i = 0; i < 4; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = player.x + cos(i * DX_PI / 2) * 60;
            pEnemyShot->y = player.y + sin(i * DX_PI / 2) * 60;
            pEnemyShot->muki = 0;
            pEnemyShot->speed = 0;
            pEnemyShot->kind = img_enemyShotDiamond[5]; // マゼンタ（紫系）
            pEnemyShot->param_i[0] = 1; // 紫弾
            pEnemyShot->param_i[1] = i; // 方向
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 紫弾が自機に向かって移動（「改竄された」演出）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // 自機を追跡
            double ang = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->muki = ang;
            pShot->speed = 2.0;
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // ×印を出す演出（4本の短レーザー）
            if (pShot->count % 30 == 0) {
                for (int i = 0; i < 4; i++) {
                    pEnemyShot = new sEnemyShot;
                    pEnemyShot->x = pShot->x;
                    pEnemyShot->y = pShot->y;
                    pEnemyShot->muki = i * DX_PI / 4;
                    pEnemyShot->speed = 3.0;
                    pEnemyShot->kind = img_enemyShotLaser[7]; // 白
                    pEnemyShot->param_i[0] = 2; // ×印
                    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
                }
            }
        }
        else if (pShot->param_i[0] == 2) { // ×印
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            if (pShot->count > 15) {
                sEnemyShot* pDel = pShot;
                pShot = pShot->next;
                pDel->prev->next = pDel->next;
                pDel->next->prev = pDel->prev;
                delete pDel;
                continue;
            }
        }
        pShot = pShot->next;
    }
}

// -------------------------------------------------
// フェーズ6：強制リセット - 自機をワープさせ敵を回復
// -------------------------------------------------
static void ShotReset(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ノイズ演出（ランダムな極小弾）
    if (pEnemyShotSet->count < 60 && pEnemyShotSet->count % 2 == 0) {
        for (int i = 0; i < 20; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = GetRand(480);
            pEnemyShot->y = GetRand(480);
            pEnemyShot->muki = 0;
            pEnemyShot->speed = 0;
            pEnemyShot->kind = img_enemyShotSmallBall[GetRand(7)]; // ランダム色
            pEnemyShot->param_i[0] = 1; // ノイズ
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // WARNING表示（ピクセルフォント風に小玉を配置）
    if (pEnemyShotSet->count == 60) {
        // W
        int wx[17] = { 0,1,2,3,4, 0,4, 0,2,4, 0,4, 0,1,2,3,4 };
        int wy[17] = { 0,0,0,0,0, 1,1, 2,2,2, 3,3, 4,4,4,4,4 };
        // A
        int ax[12] = { 1,2,3, 0,4, 0,4, 0,1,2,3,4 };
        int ay[12] = { 0,0,0, 1,1, 2,2, 3,3, 3,3,3 };
        // R
        int rx[13] = { 0,1,2,3, 0,3, 0,1,2, 0,3, 0,3 };
        int ry[13] = { 0,0,0,0, 1,1, 2,2,2, 3,3, 4,4 };
        // N
        int nx[15] = { 0,4, 0,1,4, 0,2,4, 0,3,4, 0,4 };
        int ny[15] = { 0,0, 1,1,1, 2,2,2, 3,3,3, 4,4 };
        // I
        int ix[5] = { 2, 2, 2, 2, 2 };
        int iy[5] = { 0, 1, 2, 3, 4 };
        // N
        // G
        int gx[12] = { 1,2,3, 0,4, 0, 0,2,3,4, 4 };
        int gy[12] = { 0,0,0, 1,1, 2, 3,3,3,3, 4 };

        // 描画位置
        int baseX = 180;
        int baseY = 200;
        int scale = 6;

        auto drawChar = [&](int* cx, int* cy, int n, int offsetX) {
            for (int i = 0; i < n; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = baseX + offsetX + cx[i] * scale;
                pEnemyShot->y = baseY + cy[i] * scale;
                pEnemyShot->muki = 0;
                pEnemyShot->speed = 0;
                pEnemyShot->kind = img_enemyShotSmallBall[0]; // 赤
                pEnemyShot->param_i[0] = 2; // 文字
                pEnemyShot->margin = 480;
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        };

        // 各文字を描画（簡易版）
        // W
        for (int i = 0; i < 5; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = baseX + i * scale;
            pEnemyShot->y = baseY + (i % 2) * scale * 2;
            pEnemyShot->muki = 0; pEnemyShot->speed = 0;
            pEnemyShot->kind = img_enemyShotSmallBall[0];
            pEnemyShot->param_i[0] = 2;
            pEnemyShot->margin = 480;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
        // A, R, N, I, N, G も同様に...（省略して簡易版で）
        for (int c = 0; c < 7; c++) {
            for (int i = 0; i < 5; i++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = baseX + 30 + c * 25 + i * 3;
                pEnemyShot->y = baseY + i * 4;
                pEnemyShot->muki = 0; pEnemyShot->speed = 0;
                pEnemyShot->kind = img_enemyShotSmallBall[0];
                pEnemyShot->param_i[0] = 2;
                pEnemyShot->margin = 480;
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 収縮する輪（自機位置に）
    if (pEnemyShotSet->count == 90) {
        double px = player.x;
        double py = player.y;
        for (int i = 0; i < 32; i++) {
            double ang = i * (2.0 * DX_PI / 32.0);
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = px + cos(ang) * 100;
            pEnemyShot->y = py + sin(ang) * 100;
            pEnemyShot->muki = ang + DX_PI; // 中心向き
            pEnemyShot->speed = 3.0;
            pEnemyShot->kind = img_enemyShotMediumBall[7]; // 黒
            pEnemyShot->param_i[0] = 3; // 収縮輪
            pEnemyShot->margin = 480;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動と寿命管理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) { // ノイズ
            if (pShot->count > 5) {
                sEnemyShot* pDel = pShot;
                pShot = pShot->next;
                pDel->prev->next = pDel->next;
                pDel->next->prev = pDel->prev;
                delete pDel;
                continue;
            }
        }
        else if (pShot->param_i[0] == 2) { // 文字
            if (pEnemyShotSet->count > 150) {
                sEnemyShot* pDel = pShot;
                pShot = pShot->next;
                pDel->prev->next = pDel->next;
                pDel->next->prev = pDel->prev;
                delete pDel;
                continue;
            }
        }
        else if (pShot->param_i[0] == 3) { // 収縮輪
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            if (pShot->count > 40) {
                sEnemyShot* pDel = pShot;
                pShot = pShot->next;
                pDel->prev->next = pDel->next;
                pDel->next->prev = pDel->prev;
                delete pDel;
                continue;
            }
        }
        pShot = pShot->next;
    }
}

// -------------------------------------------------
// 敵本体のパターン
// -------------------------------------------------
void EnemyPat_Violate_Kimi()
{
    static int phase;
    static int shot_count;
    static double lockX, lockY;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        shot_count = 0;
    }

    // フェーズ遷移
    switch (phase) {
    case 0: // スキャン
        if (count % 180 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotScan;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
            phase = 1;
        }
        break;

    case 1: // 座標ロック
        if (count % 180 == 1) {
            // 前のフェーズで記録した座標を引き継ぐ（簡易実装）
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotLock;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->param_d[0] = player.x; // 記録
            pSet->param_d[1] = player.y;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
            phase = 2;
        }
        break;

    case 2: // 無敵貫通
        if (count % 180 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotPierce;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
            phase = 3;
        }
        break;

    case 3: // HPリダイレクト
        if (count % 180 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotRedirect;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
            phase = 4;
        }
        break;

    case 4: // ショット改竄
        if (count % 180 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotHack;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
            phase = 5;
        }
        break;

    case 5: // 強制リセット
        if (count % 180 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotReset;
            pSet->x = enemy.x;
            pSet->y = enemy.y;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
            phase = 0; // ループ
        }
        break;
    }

    // 敵の微動
    enemy.x += sin(count / 60.0) * 0.5;
}