// enemyPat_DelayedCrossBomber.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：ディレイド・クロス・ボンバー
// ボムの投下から十字の連鎖爆発までを1つの関数で管理します
// ============================================================
static void ShotBomber(sEnemyShotSet* pEnemyShotSet)
{
    // 1. 定期的にボムを設置（60フレーム毎に1個）
    if (pEnemyShotSet->count % 10 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium) == 1) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShot* bomb = new sEnemyShot;
        bomb->x = pEnemyShotSet->x;
        bomb->y = pEnemyShotSet->y;
        bomb->kind = img_enemyShotLargeBall[7]; // 大玉（黒）

        // 目的地の設定 (画面内ランダム: 画面端に寄り過ぎないよう調整)
        // GetRand(x)は 0〜x なので、X:40〜440, Y:40〜280 の範囲になる
        bomb->param_d[0] = 40.0 + GetRand(400);
        bomb->param_d[1] = 40.0 + GetRand(240);

        bomb->muki = atan2(bomb->param_d[1] - bomb->y, bomb->param_d[0] - bomb->x);
        bomb->speed = 4.0;

        bomb->param_i[0] = 0;   // 状態 (0:目的地へ移動, 1:起爆待機, 2:爆発中, 10:爆風弾)
        bomb->param_i[1] = 120 + GetRand(60); // 爆発までのタイマー (2〜3秒でランダム)
        bomb->param_i[2] = 0;   // 爆発の持続フレームカウント

        // リストへ追加
        bomb->prev = pEnemyShotSet->pEnemyShotHead->prev;
        bomb->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = bomb;
        pEnemyShotSet->pEnemyShotHead->prev = bomb;
    }

    // 2. 登録されている全弾（ボム本体と爆風）の更新
    sEnemyShot* shot = pEnemyShotSet->pEnemyShotHead->next;
    while (shot != pEnemyShotSet->pEnemyShotHead) {

        if (shot->param_i[0] == 0) {
            // --- 状態0：目的地へ移動中のボム ---
            shot->x += shot->speed * cos(shot->muki);
            shot->y += shot->speed * sin(shot->muki);

            // 目的地への到達判定
            double dx = shot->param_d[0] - shot->x;
            double dy = shot->param_d[1] - shot->y;
            if (dx * dx + dy * dy < 20.0) {
                shot->x = shot->param_d[0];
                shot->y = shot->param_d[1];
                shot->param_i[0] = 1; // 待機状態へ移行
                shot->speed = 0.0;
            }
        }
        else if (shot->param_i[0] == 1) {
            // --- 状態1：起爆待機のボム ---
            shot->param_i[1]--; // 自前タイマーを減らす

            // 点滅エフェクト（爆発が近づくと黒と赤が激しく点滅）
            if (shot->param_i[1] < 60) {
                int cycle = (shot->param_i[1] < 30) ? 4 : 8; // 爆発直前は点滅を速くする
                if (shot->param_i[1] % cycle < cycle / 2) {
                    shot->kind = img_enemyShotLargeBall[0]; // 赤
                }
                else {
                    shot->kind = img_enemyShotLargeBall[7]; // 黒
                }
            }

            // 連鎖判定（誘爆）
            // 他の爆風弾（状態10）との当たり判定を行い、接触したら即起爆させる
            sEnemyShot* other = pEnemyShotSet->pEnemyShotHead->next;
            while (other != pEnemyShotSet->pEnemyShotHead) {
                if (other->param_i[0] == 10) {
                    double odx = other->x - shot->x;
                    double ody = other->y - shot->y;
                    // 距離が近い場合（半径30ピクセル程度以内）、即座に起爆
                    if (odx * odx + ody * ody < 900.0) {
                        shot->param_i[1] = 0; // タイマーをゼロにして次フレームで爆発
                        break;
                    }
                }
                other = other->next;
            }

            // タイマーゼロで爆発状態へ
            if (shot->param_i[1] <= 0) {
                shot->param_i[0] = 2; // 爆発中へ移行
                shot->kind = img_enemyShotLargeBall[8]; // 一瞬だけ橙色にして爆発感を出す
                if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            }
        }
        else if (shot->param_i[0] == 2) {
            // --- 状態2：爆発中のボム ---
            shot->param_i[2]++; // 爆発持続カウンター

            // 20フレームの間、上下左右の十字方向に爆風弾（レーザー）を生成
            if (shot->param_i[2] <= 20) {
                for (int dir = 0; dir < 4; dir++) {
                    sEnemyShot* laser = new sEnemyShot;
                    laser->x = shot->x;
                    laser->y = shot->y;
                    laser->muki = dir * DX_PI / 2.0; // 0, 90, 180, 270度
                    laser->speed = 12.0; // 高速で直進させる
                    laser->kind = img_enemyShotSmallBall[8]; // 橙色の小玉で炎（レーザー）を表現
                    laser->param_i[0] = 10; // 状態：爆風弾

                    laser->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    laser->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = laser;
                    pEnemyShotSet->pEnemyShotHead->prev = laser;
                }
            }

            // 爆発（弾の生成）が終わったら、ボム本体を画面外へ飛ばしてメインルーチンに消去させる
            if (shot->param_i[2] > 20) {
                shot->y = -9999.0;
            }
        }
        else if (shot->param_i[0] == 10) {
            // --- 状態10：爆風弾（十字レーザー） ---
            shot->x += shot->speed * cos(shot->muki);
            shot->y += shot->speed * sin(shot->muki);
        }

        shot = shot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Bomberman_Gemini()
{
    static int muki;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // ボスとして耐えるよう高めに設定
        muki = 1;

        // 最初から弾幕セットを「1つだけ」作成し、すべてのボムと爆発をその中で一元管理する
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotBomber;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // ボス本体は画面上部をゆっくり左右に往復する
        enemy.x += 1.0 * (double)muki;
        if (enemy.x > 400.0) muki = -1;
        if (enemy.x < 80.0) muki = 1;
    }

    // 生成済みの弾幕セットの発生源(x, y)をボスに追従させる
    sEnemyShotSet* set = enemyShotSetHead.next;
    while (set != &enemyShotSetHead) {
        set->x = enemy.x;
        set->y = enemy.y;
        set = set->next;
    }
}