// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 壁弾（滞留弾）の管理パターン
// ============================================================
static void Pattern_Wall(sEnemyShotSet* pSet)
{
    // 生成直後は speed=0 で静止している
    // pSet->count が 240 (約4秒) を超えたら、壁が崩壊して弾け飛ぶ演出
    if (pSet->count == 240) {
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            // ランダムな方向へ加速させて画面外へ飛ばす（＝消去）
            p->muki = GetRand(360) * DX_PI / 180.0;
            p->speed = 6.0;
            p = p->next;
        }
        // 崩壊開始時のSE（必要に応じて）
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ============================================================
// ボス攻撃パターン：星喰いの回廊
// ============================================================
void EnemyPat_Tackle_Qwen()
{
    static int phase;
    static double target_x, target_y;
    static sEnemyShotSet* pWallSet; // 壁弾をまとめるセット

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        target_x = 0;
        target_y = 0;
        pWallSet = nullptr;
    }

    const int CYCLE = 300;

    if (count % CYCLE == 1) {
        // 壁弾管理用のセットを1つだけ確保し、ここに壁を蓄えていく
        pWallSet = new sEnemyShotSet;
        pWallSet->count = 0;
        pWallSet->patternFunc = Pattern_Wall;
        pWallSet->pEnemyShotHead = new sEnemyShot;
        pWallSet->pEnemyShotHead->prev = pWallSet->pEnemyShotHead;
        pWallSet->pEnemyShotHead->next = pWallSet->pEnemyShotHead;

        // グローバルリストへ接続
        pWallSet->prev = enemyShotSetHead.prev;
        pWallSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pWallSet;
        enemyShotSetHead.prev = pWallSet;
    }

    // --- フェーズ制御と移動 ---

    // ボスの移動速度（壁の密度に関わる）
    double boss_speed = 5.0;

    if (phase == 0) {
        // 待機・予告
        if (count % CYCLE == 60) {
            phase = 1;
            target_x = 80.0;  target_y = 240.0; // 左下へ
        }
    }
    else if (phase == 1) {
        // 左下へ突進
        double dx = target_x - enemy.x;
        double dy = target_y - enemy.y;
        double dist = sqrt(dx * dx + dy * dy);

        if (dist < boss_speed) {
            enemy.x = target_x; enemy.y = target_y;
            phase = 2;
            target_x = 400.0; target_y = 240.0; // 次は右下へ
        }
        else {
            enemy.x += (dx / dist) * boss_speed;
            enemy.y += (dy / dist) * boss_speed;
        }
    }
    else if (phase == 2) {
        // 右下へ突進
        double dx = target_x - enemy.x;
        double dy = target_y - enemy.y;
        double dist = sqrt(dx * dx + dy * dy);

        if (dist < boss_speed) {
            enemy.x = target_x; enemy.y = target_y;
            phase = 3;
            target_x = 240.0; target_y = 60.0; // 次は中央上へ戻る
        }
        else {
            enemy.x += (dx / dist) * boss_speed;
            enemy.y += (dy / dist) * boss_speed;
        }
    }
    else if (phase == 3) {
        // 中央上へ戻る
        double dx = target_x - enemy.x;
        double dy = target_y - enemy.y;
        double dist = sqrt(dx * dx + dy * dy);

        if (dist < boss_speed) {
            enemy.x = target_x; enemy.y = target_y;
            phase = 0; // 攻撃終了、待機
        }
        else {
            enemy.x += (dx / dist) * boss_speed;
            enemy.y += (dy / dist) * boss_speed;
        }
    }

    // --- 壁弾の生成 ---
    // 移動中 (phase 1, 2, 3) のみ、軌跡に対して垂直な壁を生成する
    if (phase >= 1 && phase <= 3) {
        sEnemyShot* pShot = new sEnemyShot;

        // 位置はボスの現在地
        pShot->x = enemy.x;
        pShot->y = enemy.y;

        // 角度は移動方向に対して垂直 (+90度)
        double move_ang = atan2(target_y - enemy.y, target_x - enemy.x);
        pShot->muki = move_ang + (DX_PI / 2.0);

        // 速度0で静止させる（壁として留まる）
        pShot->speed = 0.0;

        // 素材選定: 中楕円弾のシアン(3)
        // img_enemyShotMediumOval は 10.5 x 7.0 のサイズ
        pShot->kind = img_enemyShotMediumOval[3];

        // カウンタは0で初期化（メイン側でインクリメントされる）
        pShot->count = 0;

        // 壁弾セットのリストに接続
        pShot->prev = pWallSet->pEnemyShotHead->prev;
        pShot->next = pWallSet->pEnemyShotHead;
        pWallSet->pEnemyShotHead->prev->next = pShot;
        pWallSet->pEnemyShotHead->prev = pShot;
    }
}