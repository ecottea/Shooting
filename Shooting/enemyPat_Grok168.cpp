#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  ユーティリティ
// ============================================================
static inline double Dist2(double x1, double y1, double x2, double y2)
{
    double dx = x1 - x2;
    double dy = y1 - y2;
    return dx * dx + dy * dy;
}

// 自機ショットとの当たり判定（大玉専用）
// ヒットしたら自機ショットを消去し、trueを返す
static bool CheckHitPlayerShot(sEnemyShot* pLarge)
{
    const double largeR = 20.0;
    const double playerShotR = 4.0;
    const double hitR2 = (largeR + playerShotR) * (largeR + playerShotR);

    sPlayerShot* p = playerShotHead.next;
    while (p != &playerShotHead) {
        sPlayerShot* pNext = p->next;
        if (Dist2(pLarge->x, pLarge->y, p->x, p->y) < hitR2) {
            // ヒット → 自機ショット消去
            p->prev->next = p->next;
            p->next->prev = p->prev;
            delete p;
            return true;
        }
        p = pNext;
    }
    return false;
}

// HPに応じた大玉の色を返す（0:赤 1:黄 2:緑 8:橙）
static int GetLargeBallColor(int hp, int maxHp)
{
    if (hp <= 0) return 0;
    double rate = (double)hp / (double)maxHp;
    if (rate > 0.66) return 2;      // 緑
    if (rate > 0.33) return 1;      // 黄
    if (rate > 0.15) return 8;      // 橙
    return 0;                       // 赤
}

// ============================================================
//  大玉ザコ本体のパターン（移動・被弾・消滅・弾発射）
//  param_i[0] : 状態 (0=飛来中, 1=停止/攻撃, 2=退場)
//  param_i[1] : 最大HP
//  param_i[15]: 現在HP
//  param_d[0] : 目標x
//  param_d[1] : 目標y
//  param_d[2] : 移動速度
//  param_d[3] : 弾発射用角度オフセットなど
// ============================================================
static void ShotLargeZako(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;

        // --- 被弾処理 ---
        if (pShot->param_i[15] > 0 && CheckHitPlayerShot(pShot)) {
            pShot->param_i[15]--;
            // 色更新
            int col = GetLargeBallColor(pShot->param_i[15], pShot->param_i[1]);
            pShot->kind = img_enemyShotLargeBall[col];

            // ヒット音（軽い）
            if (CheckSoundMem(sound_playerShotHit_default)) StopSoundMem(sound_playerShotHit_default);
            PlaySoundMem(sound_playerShotHit_default, DX_PLAYTYPE_BACK);

            if (pShot->param_i[15] <= 0) {
                // 消滅 → ボスに1ダメージ
                enemy.hp--;
                if (enemy.hp < 0) enemy.hp = 0;

                // 消滅エフェクト代わりに少し派手な音
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                // リストから削除
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
                pShot = pNext;
                continue;
            }
        }

        // --- 移動 ---
        int state = pShot->param_i[0];
        if (state == 0) {
            // 目標位置へ移動
            double dx = pShot->param_d[0] - pShot->x;
            double dy = pShot->param_d[1] - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);
            double spd = pShot->param_d[2];
            if (dist < spd + 1.0) {
                pShot->x = pShot->param_d[0];
                pShot->y = pShot->param_d[1];
                pShot->param_i[0] = 1; // 攻撃状態へ
                pShot->count = 0;      // 攻撃用カウントリセット
            }
            else {
                pShot->x += spd * dx / dist;
                pShot->y += spd * dy / dist;
            }
        }
        else if (state == 1) {
            // 攻撃状態：少し左右に揺れる or その場で弾を撃つ
            // 簡易的に微小振動
            pShot->x += 0.4 * sin(pShot->count * 0.07 + pShot->param_d[3]);
            // 一定時間後に退場
            if (pShot->count > 180 + (pShot->param_i[1] * 8)) {
                pShot->param_i[0] = 2;
                // 退場方向を決定（画面外へ）
                if (pShot->x < 240.0) pShot->muki = DX_PI * 1.15; // 左下寄り
                else pShot->muki = DX_PI * -0.15; // 右下寄り
                pShot->speed = 2.8 + GetRand(40) / 20.0;
            }
        }
        else if (state == 2) {
            // 退場
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        // --- 弾発射（大玉以外の弾を使用） ---
        if (state == 1 && pShot->count > 20) {
            // 発射間隔
            int interval = 22 + (pShot->param_i[1] % 5);
            if (pShot->count % interval == 0) {
                // 効果音
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

                // 弾の種類を選択（大玉以外）
                int shotType = (pShot->param_i[1] + pShot->count / 30) % 6;
                // 0:小玉 1:中玉 2:鱗 3:菱形 4:中楕円 5:銃弾

                int num = 3 + GetRand(3); // 3〜6発
                double baseMuki = atan2(player.y - pShot->y, player.x - pShot->x);
                double spread = 0.18 + GetRand(20) / 100.0;

                for (int i = 0; i < num; i++) {
                    sEnemyShot* p = new sEnemyShot;
                    p->x = pShot->x;
                    p->y = pShot->y;
                    p->muki = baseMuki + (i - (num - 1) * 0.5) * spread;
                    p->speed = 2.4 + GetRand(80) / 100.0;
                    p->margin = 30.0;
                    p->count = 0;

                    int col = GetRand(5); // 0〜5 の色をランダムに
                    switch (shotType) {
                    case 0: p->kind = img_enemyShotSmallBall[col]; break;
                    case 1: p->kind = img_enemyShotMediumBall[col]; break;
                    case 2: p->kind = img_enemyShotScale[col]; break;
                    case 3: p->kind = img_enemyShotDiamond[col]; break;
                    case 4: p->kind = img_enemyShotMediumOval[col]; break;
                    default: p->kind = img_enemyShotBullet[col]; break;
                    }

                    p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    p->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = p;
                    pEnemyShotSet->pEnemyShotHead->prev = p;
                }
            }

            // たまに追加の環状弾（中盤以降の難易度上げ）
            if (pShot->count % 55 == 30 && pShot->param_i[1] >= 7) {
                if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
                PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

                int ringNum = 10 + GetRand(4);
                for (int i = 0; i < ringNum; i++) {
                    sEnemyShot* p = new sEnemyShot;
                    p->x = pShot->x;
                    p->y = pShot->y;
                    p->muki = (DX_PI * 2.0 * i) / ringNum + pShot->count * 0.02;
                    p->speed = 1.8 + GetRand(60) / 100.0;
                    p->margin = 28.0;
                    p->kind = img_enemyShotScale[3 + GetRand(2)]; // シアン〜青系
                    p->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    p->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = p;
                    pEnemyShotSet->pEnemyShotHead->prev = p;
                }
            }
        }

        pShot = pNext;
    }

    {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->kind != img_enemyShotLargeBall[0] &&
                pShot->kind != img_enemyShotLargeBall[1] &&
                pShot->kind != img_enemyShotLargeBall[2] &&
                pShot->kind != img_enemyShotLargeBall[8] &&
                pShot->kind != img_enemyShotLargeBall[3] &&
                pShot->kind != img_enemyShotLargeBall[4] &&
                pShot->kind != img_enemyShotLargeBall[5] &&
                pShot->kind != img_enemyShotLargeBall[6] &&
                pShot->kind != img_enemyShotLargeBall[7]) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pShot->next;
        }
    }
}

// ============================================================
//  大玉ザコを1体生成してショットセットに追加
// ============================================================
static void AddLargeZako(sEnemyShotSet* pSet, double startX, double startY,
    double targetX, double targetY, double speed, int hp)
{
    hp = 3;

    sEnemyShot* p = new sEnemyShot;
    p->x = startX;
    p->y = startY;
    p->muki = 0.0;
    p->speed = 0.0;
    p->margin = 240.0;               // 仕様通り
    p->param_i[0] = 0;               // 状態：飛来中
    p->param_i[1] = hp;              // 最大HP
    p->param_i[15] = hp;             // 現在HP
    p->param_d[0] = targetX;
    p->param_d[1] = targetY;
    p->param_d[2] = speed;
    p->param_d[3] = GetRand(1000) / 100.0; // 位相用
    p->kind = img_enemyShotLargeBall[GetLargeBallColor(hp, hp)];
    p->count = 0;

    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// ============================================================
//  隊列生成用ヘルパー（横列）
// ============================================================
static void SpawnLineFormation(sEnemyShotSet* pSet, double baseY, int num,
    double startX, double spacing, double targetY,
    double speed, int hp, int dir /* +1右 -1左 */)
{
    for (int i = 0; i < num; i++) {
        double sx = startX + i * spacing * dir;
        double sy = baseY - GetRand(30);
        double tx = 80.0 + i * (320.0 / (num > 1 ? num - 1 : 1));
        if (dir < 0) tx = 400.0 - i * (320.0 / (num > 1 ? num - 1 : 1));
        double ty = targetY + (i % 3) * 12.0;
        AddLargeZako(pSet, sx, sy, tx, ty, speed + GetRand(20) / 40.0, hp + GetRand(2));
    }
}

// ============================================================
//  敵本体パターン
// ============================================================
void EnemyPat_OnTheWay_Grok()
{
    // メインのショットセットを1つだけ持たせる（大玉＋その弾を管理）
    static sEnemyShotSet* pMainSet = nullptr;
    
    // ボスは常に画面外上部に固定
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = -80.0;
        enemy.maxHp = enemy.hp = 50; // 仕様通り変更禁止
        pMainSet = nullptr;
    }

    if (count == 1) {
        pMainSet = new sEnemyShotSet;
        pMainSet->count = 0;
        pMainSet->patternFunc = ShotLargeZako;
        pMainSet->x = 240.0;
        pMainSet->y = -80.0;
        pMainSet->muki = 0.0;
        pMainSet->kind = 0;
        pMainSet->alive = 99999;
        pMainSet->pEnemyShotHead = new sEnemyShot;
        pMainSet->pEnemyShotHead->prev = pMainSet->pEnemyShotHead;
        pMainSet->pEnemyShotHead->next = pMainSet->pEnemyShotHead;
        pMainSet->prev = enemyShotSetHead.prev;
        pMainSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pMainSet;
        enemyShotSetHead.prev = pMainSet;
    }

    if (!pMainSet) return;

    // -------------------------------------------------------
    //  波の生成スケジュール（難易度高めのステージ道中）
    // -------------------------------------------------------
    // 序盤〜中盤：左右から横列
    if (count == 90 || count == 210 || count == 360) {
        int side = (count / 90) % 2;
        double startX = (side == 0) ? -60.0 : 540.0;
        int dir = (side == 0) ? 1 : -1;
        SpawnLineFormation(pMainSet, -40.0, 5, startX, 38.0, 90.0 + GetRand(40),
            1.9, 6 + GetRand(2), dir);
    }

    // 中盤：上からV字気味に
    if (count == 480 || count == 620) {
        for (int i = 0; i < 7; i++) {
            double sx = 40.0 + i * 60.0 + (GetRand(20) - 10);
            double sy = -70.0 - abs(i - 3) * 18.0;
            double tx = 60.0 + i * 55.0;
            double ty = 110.0 + abs(i - 3) * 8.0;
            AddLargeZako(pMainSet, sx, sy, tx, ty, 2.1, 7 + GetRand(2));
        }
    }

    // 中盤後半：左右同時＋斜め
    if (count == 780 || count == 920) {
        // 左から
        SpawnLineFormation(pMainSet, -50.0, 4, -80.0, 45.0, 130.0, 2.3, 8, 1);
        // 右から
        SpawnLineFormation(pMainSet, -50.0, 4, 560.0, 45.0, 130.0, 2.3, 8, -1);
    }

    // 終盤寄り：密集＋高速
    if (count == 1080 || count == 1220 || count == 1380) {
        for (int i = 0; i < 6; i++) {
            double side = (i % 2 == 0) ? -70.0 : 550.0;
            double sy = -40.0 - i * 25.0;
            double tx = 70.0 + (i * 55) % 340;
            double ty = 80.0 + (i % 3) * 35.0;
            AddLargeZako(pMainSet, side, sy, tx, ty, 2.6 + GetRand(30) / 50.0, 9);
        }
    }

    // さらに後半：上から一気に
    if (count == 1550 || count == 1720) {
        for (int i = 0; i < 8; i++) {
            double sx = 30.0 + i * 55.0;
            double sy = -90.0 - (i % 4) * 20.0;
            double tx = 50.0 + i * 50.0;
            double ty = 70.0 + (i % 2) * 40.0;
            AddLargeZako(pMainSet, sx, sy, tx, ty, 2.4, 8 + GetRand(3));
        }
    }

    // ボスHPが減ってきたら追加波（持続的な圧力）
    if (enemy.hp < 120 && count > 1800 && count % 180 == 0) {
        int num = 3 + GetRand(2);
        for (int i = 0; i < num; i++) {
            double side = (GetRand(1) == 0) ? -60.0 : 540.0;
            double sy = -50.0 - GetRand(40);
            double tx = 60.0 + GetRand(360);
            double ty = 100.0 + GetRand(80);
            AddLargeZako(pMainSet, side, sy, tx, ty, 2.5, 7 + GetRand(3));
        }
    }

    // 最後の追い込み
    if (enemy.hp < 60 && count > 2200 && count % 120 == 30) {
        for (int i = 0; i < 5; i++) {
            double sx = 20.0 + i * 90.0 + GetRand(20);
            double sy = -80.0;
            double tx = sx;
            double ty = 60.0 + GetRand(50);
            AddLargeZako(pMainSet, sx, sy, tx, ty, 3.0, 10);
        }
    }
}