#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン：逃走中の牽制弾（後方へ扇状にばら撒く）
// ============================================================
static void ShotRearGuard(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int shotNum = 7*2;
        for (int i = 0; i < shotNum; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // プレイヤーへの角度を基準に、-30度〜+30度の範囲にばら撒く
            double baseMuki = pEnemyShotSet->muki;
            double spread = (GetRand(60) - 30) * DX_PI / 180.0;
            pEnemyShot->muki = baseMuki + spread;

            // 速度は 3.0 〜 4.0
            pEnemyShot->speed = 3.0 + GetRand(100) / 100.0;

            // 中玉、黄色(1)または橙色(8)
            int color = (GetRand(1) == 0) ? 1 : 8;
            pEnemyShot->kind = img_enemyShotMediumBall[color];
            pEnemyShot->margin = 120;

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 弾幕パターン：パニック硬直時の放射弾（周囲に隙間を作る）
// ============================================================
static void ShotPanic(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        int shotNum = 32*3;
        for (int i = 0; i < shotNum; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            // ボスの中心から半径30ずらした位置から発生させることで、ボス直下に安全地帯（隙間）を作る
            double muki = GetRand(359) * DX_PI / 180.0;
            pEnemyShot->x = pEnemyShotSet->x + cos(muki) * 30.0;
            pEnemyShot->y = pEnemyShotSet->y + sin(muki) * 30.0;

            pEnemyShot->muki = muki;

            // 速度は 2.0 〜 3.5 とやや遅めにし、避ける猶予を与える
            pEnemyShot->speed = 2.0 + GetRand(150) / 100.0;
            pEnemyShot->margin = 120;

            // 小玉、赤(0)または白(6)
            int color = (GetRand(1) == 0) ? 0 : 6;
            pEnemyShot->kind = img_enemyShotSmallBall[color];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン：恐怖の後退（バックワード・パニック）
// ============================================================
void EnemyPat_Escape_Qwen()
{
    static int muki = 1;
    static int phase = 0;          // 0:通常, 1:逃走中, 2:パニック硬直, 3:復帰
    static int target_x = 240;
    static int target_y = 40;
    static int panic_timer = 0;
    static bool has_fled = false;  // 一度でも逃走したかのフラグ

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = -1;
        phase = 0;
        target_x = 240;
        target_y = 40;
        panic_timer = 0;
        has_fled = false;
    }
    else {
        if (phase == 0) {
            // 【通常待機】左右に移動
            muki = -1;
            enemy.x += 0.98 * (double)muki;
            if (count % 120 == 60) muki *= -1;

            // HPが減ったら（＝撃たれたら）逃走開始
            if (!has_fled && enemy.hp < enemy.maxHp) {
                phase = 1;
                has_fled = true;

                // 逃走予告音
                if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
                PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

                // プレイヤーから最も遠いコーナーを計算（距離の2乗で比較）
                double dist00 = (player.x - 0) * (player.x - 0) + (player.y - 0) * (player.y - 0);
                double dist480_0 = (player.x - 480) * (player.x - 480) + (player.y - 0) * (player.y - 0);
                double dist0_480 = (player.x - 0) * (player.x - 0) + (player.y - 480) * (player.y - 480);
                double dist480_480 = (player.x - 480) * (player.x - 480) + (player.y - 480) * (player.y - 480);

                double max_dist = dist00;
                target_x = 0; target_y = 0;
                if (dist480_0 > max_dist) { max_dist = dist480_0; target_x = 480; target_y = 0; }
                if (dist0_480 > max_dist) { max_dist = dist0_480; target_x = 0; target_y = 480; }
                if (dist480_480 > max_dist) { max_dist = dist480_480; target_x = 480; target_y = 480; }
            }
            else {
                // 通常時の簡易的な自機狙い弾（サンプルのばら撒きを参考に簡略化）
                if (count % 30 == 1) {
                    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
                    pEnemyShotSet->count = 0;
                    pEnemyShotSet->patternFunc = [](sEnemyShotSet* pSet) {
                        if (pSet->count == 0) {
                            sEnemyShot* pEnemyShot = new sEnemyShot;
                            pEnemyShot->x = pSet->x;
                            pEnemyShot->y = pSet->y;
                            pEnemyShot->muki = atan2(player.y - pSet->y, player.x - pSet->x);
                            pEnemyShot->speed = 4.0;
                            pEnemyShot->kind = img_enemyShotSmallBall[3]; // シアン
                            pEnemyShot->prev = pSet->pEnemyShotHead->prev;
                            pEnemyShot->next = pSet->pEnemyShotHead;
                            pSet->pEnemyShotHead->prev->next = pEnemyShot;
                            pSet->pEnemyShotHead->prev = pEnemyShot;
                        }
                        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
                        while (pShot != pSet->pEnemyShotHead) {
                            pShot->x += pShot->speed * cos(pShot->muki);
                            pShot->y += pShot->speed * sin(pShot->muki);
                            pShot = pShot->next;
                        }
                    };
                    pEnemyShotSet->x = enemy.x;
                    pEnemyShotSet->y = enemy.y + 10.0;
                    pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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
        }
        else if (phase == 1) {
            // 【逃走中】ターゲットコーナーへ向かう
            double dx = target_x - enemy.x;
            double dy = target_y - enemy.y;
            double dist = sqrt(dx * dx + dy * dy);

            if (dist < 5.0) {
                // コーナーに到達
                enemy.x = (double)target_x;
                enemy.y = (double)target_y;
                phase = 2;
                panic_timer = 0;
            }
            else {
                // 移動
                double speed = 3.5;
                enemy.x += (dx / dist) * speed;
                enemy.y += (dy / dist) * speed;

                // 牽制弾を定期的に発射（20フレームごと）
                if (count % 20 == 1) {
                    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
                    pEnemyShotSet->count = 0;
                    pEnemyShotSet->patternFunc = ShotRearGuard;
                    pEnemyShotSet->x = enemy.x;
                    pEnemyShotSet->y = enemy.y;
                    // プレイヤーへの角度を基準にする
                    pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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
        }
        else if (phase == 2) {
            // 【パニック硬直】その場で停止し、弾をばら撒く
            panic_timer++;
            if (panic_timer == 1) {
                // 硬直開始時にパニック弾を撒く
                sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
                pEnemyShotSet->count = 0;
                pEnemyShotSet->patternFunc = ShotPanic;
                pEnemyShotSet->x = enemy.x;
                pEnemyShotSet->y = enemy.y;
                pEnemyShotSet->muki = 0;
                pEnemyShotSet->kind = 0;
                pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->prev = enemyShotSetHead.prev;
                pEnemyShotSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pEnemyShotSet;
                enemyShotSetHead.prev = pEnemyShotSet;
            }

            // 120フレーム（約2秒）硬直したら復帰フェーズへ
            if (panic_timer >= 120) {
                phase = 3;
            }
        }
        else if (phase == 3) {
            // 【復帰】中央へ戻る
            double dx = 240.0 - enemy.x;
            double dy = 40.0 - enemy.y;
            double dist = sqrt(dx * dx + dy * dy);

            if (dist < 5.0) {
                enemy.x = 240.0;
                enemy.y = 40.0;
                phase = 0;
                // 再戦のためにHPを全回復させる（ゲームとして成立させるため）
                //enemy.hp = enemy.maxHp;
                has_fled = false;
            }
            else {
                double speed = 3.5;
                enemy.x += (dx / dist) * speed;
                enemy.y += (dy / dist) * speed;
            }
        }
    }
}