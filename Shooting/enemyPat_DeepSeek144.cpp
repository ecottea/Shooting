#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：秒針の抜け道（Slow Spiral Gate）
static void ShotSlowSpiralGate(sEnemyShotSet* pEnemyShotSet)
{
    // 45フレームごとに1リング発射
    if (pEnemyShotSet->count % 30 == 0) {
        // 缺口中心角度の初期化と更新
        if (pEnemyShotSet->count == 0) {
            // 最初の缺口中心（7.5度）
            pEnemyShotSet->param_d[0] = 7.5 * DX_PI / 180.0;
        }
        else {
            // 5秒（315フレーム = 7リング）ごとに回転方向を反転
            int dir = ((pEnemyShotSet->count / 315) % 2 == 0) ? 1 : -1;
            pEnemyShotSet->param_d[0] += 12.0 * DX_PI / 180.0 * dir;
        }

        double gapCenter = pEnemyShotSet->param_d[0];
        int ringIndex = pEnemyShotSet->count / 45;

        // 発射音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 24発を15度間隔で配置。缺口（中心から±30度）だけ弾を生成しない
        for (int i = 0; i < 24*8; i++) {
            double angle = i * 15.0/8 * DX_PI / 180.0;

            // 缺口との角度差を -π～π に正規化
            double diff = angle - gapCenter;
            while (diff <= -DX_PI) diff += 2.0 * DX_PI;
            while (diff > DX_PI) diff -= 2.0 * DX_PI;

            // 缺口なら弾を生成しない
            if (fabs(diff) <= 30.0 * DX_PI / 180.0/4) continue;

            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 0.75; // 1px/frame以下
            // 水色と白を交互に
            pEnemyShot->kind = (ringIndex % 2 == 0)
                ? img_enemyShotSmallBall[3]  // シアン
                : img_enemyShotSmallBall[6]; // 白

            // リンクリストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全ての弾を移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_SlowBullet_DeepSeek()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと

        // 弾幕セットを1つ作成し、以降はこのセットが継続してリングを発射する
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSlowSpiralGate;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0; // 未使用
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    // 敵は静止（必要ならここで移動処理を追加）
}