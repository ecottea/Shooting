// enemyPat_IrritatingStick.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================
// イライラ棒モチーフ弾幕：ShotIrritatingStick
// =============================================
static void ShotIrritatingStick(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int CHAIN_LENGTH = 48;     // 鎖の長さ（細かく繋げる）
    const double BASE_SPEED = 1.8;   // 全体の下降速度

    if (pEnemyShotSet->count == 0) {
        // 出現時の効果音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 初期鎖生成（上部から細い線状に並べる）
        for (int i = 0; i < CHAIN_LENGTH; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y - i * 6.0;  // 上方向に連なる
            pEnemyShot->muki = 0.0;
            pEnemyShot->speed = 0.0;                     // 初期は静止、後にpatternFuncで制御
            pEnemyShot->kind = img_enemyShotSmallBall[4]; // 青寄りの小玉で細く金属線風
            pEnemyShot->param_i[0] = i;                  // 鎖内の順番（0が先頭）
            pEnemyShot->param_d[0] = 0.0;                // 時間オフセット用

            // 双方向リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 毎フレーム更新：鎖全体をイライラ棒風に蛇行させながら下降
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int idx = pShot->param_i[0];
        double t = (pEnemyShotSet->count + idx * 0.8) * 0.045;  // 時間進行（遅延で波を伝播）

        // イライラ棒らしい複雑な蛇行パス
        double baseX = pEnemyShotSet->x + sin(t * 1.8) * 55.0 + sin(t * 3.7) * 22.0;
        double wave = sin(t * 2.4 + idx * 0.13) * 38.0;
        double sharpTurn = (sin(t * 0.7) > 0.6) ? sin(t * 8.0) * 28.0 : 0.0;

        pShot->x = baseX + wave + sharpTurn;
        pShot->y = pEnemyShotSet->y + idx * 5.5 + pEnemyShotSet->count * BASE_SPEED;

        // 微振動（後半でイライラ増加）
        if (pEnemyShotSet->count > 80) {
            pShot->x += sin(pEnemyShotSet->count * 0.4 + idx) * 1.8;
        }

        pShot = pShot->next;
    }
}

// =============================================
// 敵本体パターン：イライラ棒モチーフ
// =============================================
void EnemyPat_Irairabou_Grok()
{
    static int muki = 1;
    static int shotTimer = 0;

    if (count == 1) {
        // 初期配置（ゲーム画面 480x480）
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shotTimer = 0;
    }
    else {
        // 左右往復移動（イライラ棒を構えるような動き）
        enemy.x += 1.35 * (double)muki;
        if (enemy.x < 120.0 || enemy.x > 360.0) muki *= -1;

        // 軽い上下揺れ
        enemy.y = 60.0 + sin(count * 0.03) * 12.0;
    }

    // 定期的にイライラ棒鎖を展開
    shotTimer++;
    if (shotTimer % 95 == 0) {  // 約1.5秒ごとに新鎖出現
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotIrritatingStick;
        pEnemyShotSet->x = enemy.x + (GetRand(80) - 40);  // 少しランダムオフセット
        pEnemyShotSet->y = enemy.y + 25.0;
        pEnemyShotSet->muki = DX_PI / 2.0;  // 下向き基準
        pEnemyShotSet->kind = 0;

        // 弾リスト初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // ShotSetを全体リストに連結
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}