// enemyPat_Tmp.cpp
// シェルピンスキーのギャスケットをモチーフにした弾幕パターン（独自の弾・色設定）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// シェルピンスキーのギャスケット風に弾を配置する補助関数
static void PlaceSierpinski(sEnemyShotSet* pEnemyShotSet, int level, double cx, double cy, double size)
{
    if (level <= 0) {
        // 再帰の末端で弾を1発配置
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = cx;
        pEnemyShot->y = cy;
        // プレイヤー方向へ発射
        pEnemyShot->muki = atan2(player.y - cy, player.x - cx);
        pEnemyShot->speed = 2.0 + (GetRand(50) / 100.0); // 2.0〜2.5程度

        // 弾の種類: 大玉・銃弾・菱形弾のみ使用
        // 弾の色:   赤(0)・青(4)・マゼンタ(5)・橙(8)のみ使用
        int color_choice = GetRand(3); // 0〜3 → 赤,青,マゼンタ,橙 のいずれか
        int color;
        switch (color_choice) {
        case 0: color = 0; break; // 赤
        case 1: color = 4; break; // 青
        case 2: color = 5; break; // マゼンタ
        case 3: color = 8; break; // 橙
        }

        int kind_choice = GetRand(2); // 0〜2 → 大玉,銃弾,菱形弾 のいずれか
        switch (kind_choice) {
        case 0:
            pEnemyShot->kind = img_enemyShotMediumBall[color];
            break;
        case 1:
            pEnemyShot->kind = img_enemyShotBullet[color];
            break;
        case 2:
            pEnemyShot->kind = img_enemyShotDiamond[color];
            break;
        }

        // リストに挿入
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        return;
    }

    // 三角形の3頂点に再帰
    double h = size * sqrt(3.0) / 2.0;
    PlaceSierpinski(pEnemyShotSet, level - 1, cx, cy - h / 2.0, size / 2.0); // 上頂点
    PlaceSierpinski(pEnemyShotSet, level - 1, cx - size / 2.0, cy + h / 2.0, size / 2.0); // 左下
    PlaceSierpinski(pEnemyShotSet, level - 1, cx + size / 2.0, cy + h / 2.0, size / 2.0); // 右下
}

// 弾幕：シェルピンスキーのギャスケット風
static void ShotSierpinski(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 初回のみ弾を生成
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // ダミーのヘッドノード
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // シェルピンスキーのギャスケット風に弾を配置
        PlaceSierpinski(pEnemyShotSet, 3, pEnemyShotSet->x, pEnemyShotSet->y, 80.0);
    }

    // 弾の移動処理
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Sierpinski_Sakana()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    if (count % 30 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSierpinski;
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
}