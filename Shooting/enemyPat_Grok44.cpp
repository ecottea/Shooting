// enemyPat_Tmp_ArrowTorrent.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================
// 矢の奔流パターン用 専用弾幕関数
// =============================================
static void ShotArrowTorrent(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    int phase = pEnemyShotSet->count / 60;  // 60フレームごとにフェーズ進行

    // =============================================
    // フェーズ0: 大型矢印の生成と直進 (0〜3秒相当)
    // =============================================
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);  // 重厚な矢の音

        // 大型矢印を4方向から生成
        for (int i = 0; i < 4; i++) {
            pEnemyShot = new sEnemyShot;
            double angle = i * (DX_PI / 2.0);  // 0, 90, 180, 270度

            pEnemyShot->x = pEnemyShotSet->x + cos(angle) * 180.0;
            pEnemyShot->y = pEnemyShotSet->y + sin(angle) * 180.0 - 60.0;
            pEnemyShot->muki = angle + DX_PI;  // 中央に向かう
            pEnemyShot->speed = 1.8;
            pEnemyShot->kind = img_enemyShotBullet[1];  // 赤い銃弾（矢印らしい細長い形）
            pEnemyShot->param_i[0] = 1;  // 大型フラグ
            pEnemyShot->margin = 480;

            // 連結
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // =============================================
    // フェーズ1: 分裂 (大型矢印が中央付近に到達したら小型矢に分裂)
    // =============================================
    if (pEnemyShotSet->count == 45) {  // 大型が中央に近づいたタイミング
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            if (pEnemyShot->param_i[0] == 1) {  // 大型矢印のみ
                double baseAngle = pEnemyShot->muki;
                for (int i = 0; i < 9; i++) {
                    sEnemyShot* smallp = new sEnemyShot;
                    smallp->x = pEnemyShot->x;
                    smallp->y = pEnemyShot->y;
                    smallp->muki = baseAngle + (i - 4) * 0.22;  // 扇状に広がる
                    smallp->speed = 2.8 + GetRand(80) / 100.0;
                    smallp->kind = img_enemyShotBullet[0];  // 小型矢（白/薄赤）
                    smallp->param_i[0] = 0;  // 小型フラグ

                    // 弱追尾用にプレイヤー方向を少し記憶
                    smallp->param_d[0] = atan2(player.y - smallp->y, player.x - smallp->x);

                    smallp->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    smallp->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = smallp;
                    pEnemyShotSet->pEnemyShotHead->prev = smallp;
                }
                // 大型は一旦無効化（速度を0に）
                pEnemyShot->speed = 0.0;
            }
            pEnemyShot = pEnemyShot->next;
        }
    }

    // =============================================
    // 毎フレーム移動処理
    // =============================================
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->param_i[0] == 0) {  // 小型矢
            // 弱追尾（少し滑らかにプレイヤー方向へ）
            double target = pEnemyShot->param_d[0];
            pEnemyShot->muki = pEnemyShot->muki * 0.92 + target * 0.08;
        }

        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// =============================================
// 敵本体パターン（矢の奔流）
// =============================================
void EnemyPat_Arrow_Grok()
{
    static int muki = 1;
    static int shot_count = 0;

    if (count == 1) {
        // 初期位置・体力設定
        enemy.x = 240.0;
        enemy.y = 160.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        // 左右往復移動
        enemy.x += 1.1 * (double)muki;
        if (count % 110 == 55) muki *= -1;

        // 軽い上下揺れ
        enemy.y = 160.0 + sin(count / 30.0) * 12.0;
    }

    // 定期的に新しいShotSetを生成（約1.2秒間隔）
    if (count % 72 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotArrowTorrent;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 双方向リストに連結
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}