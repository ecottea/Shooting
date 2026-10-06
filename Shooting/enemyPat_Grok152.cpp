#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ミュラーリヤー錯視をモチーフにした弾幕
// 同じ長さの水平弾列2本を、外側矢印（長く見える）と内側矢印（短く見える）で構成し、
// ゆっくり真下に降下させる。既存の小玉のみを使用。
static void ShotMullerLyer(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 効果音
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 共通パラメータ
        const double spacing = 14.0;          // 本体弾の間隔
        const int main_num = 15;              // 本体の弾数（奇数推奨）
        const double half = (main_num - 1) * spacing / 2.0;
        const double arrow_dist = 16.0;       // 矢印の長さ
        const double arrow_angle = 38.0 * DX_PI / 180.0; // 水平からの角度
        const double speed = 1.35*1.5;            // 降下速度（ゆっくり）
        const double muki_down = DX_PI / 2.0; // 真下

        // 2本の相対Y位置（上下に少し離す）
        const double y_long = -28.0;  // 外側矢印（長く見える方）
        const double y_short = 28.0;  // 内側矢印（短く見える方）

        // 色: 0=赤（外側・長く見える）, 4=青（内側・短く見える）
        // 小玉を使用（等方的で位置が正確に出る）
        auto createShot = [&](double rx, double ry, int color) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + rx;
            pEnemyShot->y = pEnemyShotSet->y + ry;
            pEnemyShot->muki = muki_down;
            pEnemyShot->speed = speed;
            pEnemyShot->kind = img_enemyShotSmallBall[color];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        };

        // ========== パターンA: 外側矢印（長く見える） ==========
        // 本体
        for (int i = 0; i < main_num; i++) {
            double rx = -half + i * spacing;
            createShot(rx, y_long, 0);
        }
        // 左外側矢印（左上・左下）
        {
            double lx = -half;
            double dx = -arrow_dist * cos(arrow_angle);
            double dy = arrow_dist * sin(arrow_angle);
            createShot(lx + dx, y_long - dy, 0); // 左上
            createShot(lx + dx, y_long + dy, 0); // 左下
        }
        // 右外側矢印（右上・右下）
        {
            double rx = half;
            double dx = arrow_dist * cos(arrow_angle);
            double dy = arrow_dist * sin(arrow_angle);
            createShot(rx + dx, y_long - dy, 0); // 右上
            createShot(rx + dx, y_long + dy, 0); // 右下
        }

        // ========== パターンB: 内側矢印（短く見える） ==========
        // 本体
        for (int i = 0; i < main_num; i++) {
            double rx = -half + i * spacing;
            createShot(rx, y_short, 4);
        }
        // 左内側矢印（右上・右下方向 = 内側）
        {
            double lx = -half;
            double dx = arrow_dist * cos(arrow_angle);
            double dy = arrow_dist * sin(arrow_angle);
            createShot(lx + dx, y_short - dy, 4); // 内側上
            createShot(lx + dx, y_short + dy, 4); // 内側下
        }
        // 右内側矢印（左上・左下方向 = 内側）
        {
            double rx = half;
            double dx = -arrow_dist * cos(arrow_angle);
            double dy = arrow_dist * sin(arrow_angle);
            createShot(rx + dx, y_short - dy, 4); // 内側上
            createShot(rx + dx, y_short + dy, 4); // 内側下
        }
    }

    // 全弾を同じ速度で真下に移動（相対位置を維持）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_MullerLyer_Grok()
{
    static int muki;
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
    }
    else {
        // 左右にゆっくり往復
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 120.0) muki = 1;
        if (enemy.x > 360.0) muki = -1;
    }

    // 一定間隔でミュラーリヤー弾幕を発射
    if (count % 45 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMullerLyer;
        pEnemyShotSet->x = enemy.x + (GetRand(20) - 10) * 15.0;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = DX_PI / 2.0; // 参考値（実際は関数内で固定）
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