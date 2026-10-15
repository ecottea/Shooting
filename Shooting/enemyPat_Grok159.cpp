#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：刀の横薙ぎ斬撃（既存弾で刃の軌跡を表現）
// 小玉を高密度に並べて「刃」を擬似再現し、振りの進行に合わせて順次発生させる
static void ShotSwordSlash(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // count==0 : 予兆（チャージ音）
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // param_d[0] : 振り開始角度（相対）
        // param_d[1] : 1フレームあたりの角度増分
        // param_d[2] : 基準角度（プレイヤー方向）
        pEnemyShotSet->param_d[0] = -DX_PI * 0.55;                 // 約 -99度から開始
        pEnemyShotSet->param_d[1] = (DX_PI * 1.10) / 24.0;          // 24フレームで約110度振り
        pEnemyShotSet->param_d[2] = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);
    }

    // count 18〜41 : 斬撃本体（順次スポーンで振りを表現）
    if (pEnemyShotSet->count >= 18 && pEnemyShotSet->count <= 41) {
        int frame = pEnemyShotSet->count - 18;
        double rel = pEnemyShotSet->param_d[0] + pEnemyShotSet->param_d[1] * frame;
        double angle = pEnemyShotSet->param_d[2] + rel;             // 現在の刀先方向

        // 斬撃開始フレームで重い効果音
        if (pEnemyShotSet->count == 18) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }

        // 刀先付近に高密度の小玉を5発発生（刃の厚みを出す）
        // 色は赤(0)を基調に、先端だけ少し白を混ぜて視認性を上げる
        for (int i = 0; i < 5; i++) {
            pEnemyShot = new sEnemyShot;

            double dist = 38.0 + (i - 2) * 1.2;                     // 刀先からの距離を少しずらす
            double side = (i - 2) * 3.8;                             // 進行方向に垂直なオフセット（刃の厚み）

            pEnemyShot->x = pEnemyShotSet->x + dist * cos(angle)
                + side * cos(angle + DX_PI * 0.5);
            pEnemyShot->y = pEnemyShotSet->y + dist * sin(angle)
                + side * sin(angle + DX_PI * 0.5);

            pEnemyShot->muki = angle;                               // 放射方向に飛ばす
            pEnemyShot->speed = 0.0;                                // 発生直後は静止（刃の残像）

            // 先端は白小玉、それ以外は赤小玉で刃らしく見せる
            if (i == 2) {
                pEnemyShot->kind = img_enemyShotSmallBall[6];       // 白
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[0];       // 赤
            }

            // リンクリスト接続
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // さらに刃の本体感を出すため、短いレーザーを1本追加（刀身）
        // レーザーは細長く、向きを振り方向に合わせると刃に見える
        if (frame % 3 == 0) {                                       // 密度を抑えつつ刃を強調
            pEnemyShot = new sEnemyShot;
            double laserDist = 28.0;
            pEnemyShot->x = pEnemyShotSet->x + laserDist * cos(angle);
            pEnemyShot->y = pEnemyShotSet->y + laserDist * sin(angle);
            pEnemyShot->muki = angle;                               // レーザーの向き＝振り方向
            pEnemyShot->speed = 0.0;
            pEnemyShot->kind = img_enemyShotLaser[0];               // 赤短レーザー
            pEnemyShot->margin = 80;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 全弾の移動処理（発生後一定フレーム静止 → 放射状に高速飛散）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 各弾のcountは生成時0から自動インクリメントされる
        // 発生から約12フレーム後に動き出す（刃が一瞬残ってから飛散）
        if (pShot->count >= 12 && pShot->speed < 0.1) {
            pShot->speed = 4.2 + GetRand(80) / 100.0;               // 少し速度にばらつき
        }

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Sword_Grok()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 155.0;
        enemy.maxHp = enemy.hp = 200;                               // 200から変更しないこと
        muki = 1;
    }
    else {
        // 緩やかな左右移動
        enemy.x += 0.85 * (double)muki;
        if (enemy.x < 90.0 || enemy.x > 390.0) {
            muki *= -1;
        }
    }

    // 約100フレームごとに斬撃を発動
    if (count >= 30 && (count - 30) % 100 == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSwordSlash;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 8.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // ショットセットをリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}