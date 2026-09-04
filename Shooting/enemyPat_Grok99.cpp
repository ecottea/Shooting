// enemyPat_Doppler.cpp
// ドップラー効果をモチーフにした円形弾幕パターン
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ドップラー円形弾幕（リング）
// プレイヤーが敵に接近しているほど弾速↑・弾数↑・青色（青方偏移）
// 遠ざかっているほど弾速↓・弾数↓・赤色（赤方偏移）
static void ShotDoppler(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 効果音（接近度合いで強さを変える）
        double v_rad = pEnemyShotSet->param_d[0];   // 相対放射速度（負＝接近）
        int snd = sound_enemyShot_medium;
        if (v_rad < -2.5) {
            snd = sound_enemyShot_heavy;
        }
        else if (v_rad > 2.0) {
            snd = sound_enemyShot_light;
        }
        if (CheckSoundMem(snd)) StopSoundMem(snd);
        PlaySoundMem(snd, DX_PLAYTYPE_BACK);

        // 弾数（接近で密、後退で疎）
        int num = 20;
        if (v_rad < -1.5)      num = 36;   // 強い接近 → 高密度
        else if (v_rad < -0.5) num = 28;
        else if (v_rad > 1.5)  num = 12;   // 強い後退 → 低密度
        else if (v_rad > 0.5)  num = 16;

        // 弾速（接近で速く、後退で遅く）
        double speed = 2.4 - v_rad * 0.35;  // v_radが負ほど速くなる
        if (speed < 1.2) speed = 1.2;
        if (speed > 4.5) speed = 4.5;

        // 色（0:赤 … 4:青）
        int color;
        if (v_rad < -1.8)      color = 4;   // 青
        else if (v_rad < -0.6) color = 3;   // シアン
        else if (v_rad > 1.8)  color = 0;   // 赤
        else if (v_rad > 0.6)  color = 8;   // 橙
        else                   color = 6;   // 白（中間）

        // リング生成
        for (int i = 0; i < num; i++) {
            double ang = (DX_PI * 2.0) * i / num;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = speed;
            // 小玉を使用（密集時に綺麗に見える）
            pEnemyShot->kind = img_enemyShotSmallBall[color];

            // リンク接続
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動（メインルーチンで count 増加・画面外消去）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体パターン（ドップラー）
void EnemyPat_Doppler_Grok()
{
    static double prev_player_x;   // 前フレームのプレイヤー位置
    static double prev_player_y;
    static int spawn_interval;     // 現在の発射間隔

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        prev_player_x = player.x;
        prev_player_y = player.y;
        spawn_interval = 18;
    }

    enemy.x = 240.0 + 180.0 * cos(count * 0.02);
    enemy.y = 120.0 + 90.0 * sin(count * 0.03);

    // プレイヤーの相対放射速度を計算
    // v_rad < 0 → 接近（青方偏移）、v_rad > 0 → 後退（赤方偏移）
    double dx = player.x - enemy.x;
    double dy = player.y - enemy.y;
    double dist = sqrt(dx * dx + dy * dy);
    if (dist < 1.0) dist = 1.0;

    double pdx = player.x - prev_player_x;
    double pdy = player.y - prev_player_y;
    double v_rad = (dx * pdx + dy * pdy) / dist;

    // 接近しているほど発射間隔を短くして波長を圧縮
    spawn_interval = 18;
    if (v_rad < -2.0)      spawn_interval = 8;
    else if (v_rad < -0.8) spawn_interval = 12;
    else if (v_rad > 1.5)  spawn_interval = 28;
    else if (v_rad > 0.6)  spawn_interval = 22;
    spawn_interval -= 3;

    // リング発射
    if (count % spawn_interval == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDoppler;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 8.0;
        pEnemyShotSet->muki = 0.0;               // 未使用
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->param_d[0] = v_rad;       // 相対放射速度を渡す

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 次フレーム用にプレイヤー位置を保存
    prev_player_x = player.x;
    prev_player_y = player.y;
}