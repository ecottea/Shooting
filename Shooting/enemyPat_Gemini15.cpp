// enemyPat_washingMachine.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：洗濯機（水流の反転と泡立ち）
static void ShotWashingMachine(sEnemyShotSet* pEnemyShotSet)
{
    // 敵本体が振動するため、常に発射地点（セットの座標）を敵の中心に追従させる
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    // --- 1. 水流（メインのうずまき）の生成 ---
    // 4フレームに1回のペースで発射
    if (pEnemyShotSet->count % 4 == 0) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 角速度をサイン波で変動させ、右回り・左回りを周期的に反転させる（洗濯機の挙動）
        // countが進むにつれて滑らかに回転方向が切り替わります
        double ang_vel = 0.25 * sin(pEnemyShotSet->count * DX_PI / 150.0);
        pEnemyShotSet->muki += ang_vel;

        // 十字（4方向）に水流を放つ
        for (int i = 0; i < 8; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + i * (DX_PI / 2.0) / 2;

            // 水圧の強弱をサイン波で表現し、弾速に波を持たせる
            pEnemyShot->speed = 2.5 + 1.0 * sin(pEnemyShotSet->count * DX_PI / 60.0);

            // シアン(3)と青(4)の中玉を交互に出して「水」を表現
            int color = (pEnemyShotSet->count % 8 == 0) ? 3 : 4;
            pEnemyShot->kind = img_enemyShotMediumBall[color];

            // リストへの追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 2. 洗剤の泡の生成 ---
    // 6フレームに1回、ランダムな方向に白い泡を散らす
    if (pEnemyShotSet->count % 2 == 0) {
        for (int i = 0; i < 2; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // GetRand(360) は 0〜360 を返すので、全方位にランダム発射
            pEnemyShot->muki = GetRand(360) / 180.0 * DX_PI;
            // 弾速もランダムにして、ふわふわ・弾ける泡を表現
            pEnemyShot->speed = 1.0 + GetRand(200) / 100.0;

            // 白(6)の小玉
            pEnemyShot->kind = img_enemyShotSmallBall[6];

            // リストへの追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // --- 3. 弾の移動処理 ---
    // セットに登録されている全ての弾の座標を更新する
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン（洗濯機）
void EnemyPat_WashingMachine_Gemini()
{
    if (count == 1) {
        // ゲーム画面（480x480想定）のやや上部に配置
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 60フレーム目以降、脱水機のように激しくガタガタと振動し続ける
    if (count > 30) {
        // ベースの座標を中心に、乱数で小刻みに位置を揺らす
        enemy.x = 240.0 + (GetRand(4) - 2);
        enemy.y = 100.0 + (GetRand(4) - 2);
    }

    // 60フレーム目に弾幕セット（洗濯機の水流ジェネレータ）を1つだけ生成
    // 以降は ShotWashingMachine が毎フレーム呼ばれ、自動で弾幕を展開・管理し続ける
    if (count == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotWashingMachine;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;

        // ダミーのヘッドノードを生成して環状リストを初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 全体のリスト（enemyShotSetHead）へ接続
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}