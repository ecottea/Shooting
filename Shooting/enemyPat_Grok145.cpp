#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 逃亡の軌跡（Escape Trail）
// 被弾するとボスがプレイヤーから離れる方向へダッシュし、
// ①ダッシュ開始時に進行方向の反対側へ扇状弾
// ②ダッシュ中に軌跡残像弾（一定時間後にホーミング）
// ③ダッシュ終了時に円形拡散弾
// を発生させる。
// ============================================================

// 扇状弾（ダッシュ開始時：進行方向の反対側）
static void ShotFanOpposite(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 反対方向を中心に約180度の扇を14発
        const int num = 14;
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            double angleOffset = ((double)i / (num - 1) - 0.5) * DX_PI; // -90°〜+90°
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = pEnemyShotSet->muki + angleOffset;
            pEnemyShot->speed = 3.2 + GetRand(80) / 100.0;
            // 中玉・赤〜橙系で威圧感
            int col = (i % 3 == 0) ? 0 : ((i % 3 == 1) ? 8 : 1); // 赤・橙・黄
            pEnemyShot->kind = img_enemyShotMediumBall[col];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    // 移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 軌跡残像弾（一定時間静止後、プレイヤーへホーミング）
static void ShotTrailHome(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 軽めの効果音
        //if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        //PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0; // 最初は静止
        // 小玉・シアン〜青で残像感
        int col = (GetRand(1) == 0) ? 3 : 4; // シアン or 青
        pEnemyShot->kind = img_enemyShotSmallBall[col];
        // param_i[0] = ホーミング開始までの待機フレーム
        pEnemyShot->param_i[0] = 45 + GetRand(15); // 45〜60F
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }
    // 更新
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->count == pShot->param_i[0]) {
            // この瞬間にプレイヤー方向へ向きを決定し、速度を与える
            pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
            pShot->speed = 1.4 + GetRand(60) / 100.0; // ゆっくりめ
        }
        if (pShot->count >= pShot->param_i[0]) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        // count < 待機時間の間は完全に静止（位置変更なし）
        pShot = pShot->next;
    }
}

// 円形拡散弾（ダッシュ終了時）
static void ShotCircleSpread(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int num = 24;
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = (DX_PI * 2.0 * i) / num;
            pEnemyShot->speed = 1.8 + GetRand(40) / 100.0; // やや遅めの余韻
            // 中楕円弾・白〜黄で広がり感
            int col = (i % 2 == 0) ? 6 : 1; // 白 or 黄
            pEnemyShot->kind = img_enemyShotMediumOval[col];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    // 移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体パターン
void EnemyPat_Escape_Grok()
{
    static int prev_hp;
    static int dash_timer;      // >0 の間ダッシュ中
    static double dash_muki;    // ダッシュ方向
    static int trail_cooldown;  // 軌跡弾の間隔制御

    if (count == 1) {
        // 初期化（ゲーム画面 480x480）
        enemy.x = 240.0;
        enemy.y = 200.0;
        enemy.maxHp = enemy.hp = 200; // 変更禁止
        prev_hp = enemy.hp;
        dash_timer = 0;
        trail_cooldown = 0;
    }
    else {
        // ----- 被弾検出 -----
        if (enemy.hp < prev_hp) {
            // プレイヤーから離れる方向を計算
            double dx = enemy.x - player.x;
            double dy = enemy.y - player.y;
            double len = sqrt(dx * dx + dy * dy);
            if (len < 1.0) {
                // ほぼ重なっている場合は上方向へ逃げる
                dash_muki = -DX_PI / 2.0;
            }
            else {
                dash_muki = atan2(dy, dx);
            }

            dash_timer = 28; // 約0.47秒（60FPS想定）
            trail_cooldown = 0;

            // ① 扇状弾セットを生成（進行方向の反対側）
            {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotFanOpposite;
                pSet->x = enemy.x;
                pSet->y = enemy.y;
                pSet->muki = dash_muki + DX_PI + (GetRand(100) - 50) / 1000.0; // 反対方向
                pSet->kind = 0;
                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
        }
        prev_hp = enemy.hp;

        // ----- ダッシュ処理 -----
        if (dash_timer > 0) {
            const double dash_speed = 7.5;
            enemy.x += dash_speed * cos(dash_muki);
            enemy.y += dash_speed * sin(dash_muki);

            // 画面内にクランプ（ボスが完全に消えないように）
            if (enemy.x < 40.0)  enemy.x = 40.0;
            if (enemy.x > 440.0) enemy.x = 440.0;
            if (enemy.y < 30.0)  enemy.y = 30.0;
            if (enemy.y > 280.0) enemy.y = 280.0;

            // ② 軌跡残像弾を一定間隔で残す
            if (trail_cooldown <= 0) {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotTrailHome;
                pSet->x = enemy.x;
                pSet->y = enemy.y;
                pSet->muki = 0.0;
                pSet->kind = 0;
                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;

                trail_cooldown = 4; // 4フレームごとに1発
            }
            else {
                trail_cooldown--;
            }

            dash_timer--;

            // ③ ダッシュ終了時に円形拡散
            if (dash_timer == 0) {
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = ShotCircleSpread;
                pSet->x = enemy.x;
                pSet->y = enemy.y;
                pSet->muki = 0.0;
                pSet->kind = 0;
                pSet->pEnemyShotHead = new sEnemyShot;
                pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
                pSet->prev = enemyShotSetHead.prev;
                pSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pSet;
                enemyShotSetHead.prev = pSet;
            }
        }
        // ダッシュしていない間はほぼ静止（わずかに上下に揺れる程度）
        else {
            enemy.y += 0.4 * sin(count * 0.05);
        }
    }
}