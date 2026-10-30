#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// スプラトゥーン風カラーパレット (3:シアン, 5:マゼンタ, 1:黄, 2:緑)
static const int INK_COLORS[] = { 3, 5, 1, 2 };
static const int NUM_INK_COLORS = 4;

// ============================================================
// 弾幕パターン関数群（高難易度版）
// ============================================================

// フェーズ1: インクローラー（高密度・着弾拡散）
static void ShotNawabariHard(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int color = INK_COLORS[GetRand(NUM_INK_COLORS - 1)];

        // インクの塊を落とす
        sEnemyShot* pDrop = new sEnemyShot;
        pDrop->x = pEnemyShotSet->x;
        pDrop->y = pEnemyShotSet->y;
        pDrop->muki = pEnemyShotSet->muki + (GetRand(40) - 20) / 180.0 * DX_PI;
        pDrop->speed = 4.5 + GetRand(100) / 50.0;
        pDrop->kind = img_enemyShotLargeBall[color];
        pDrop->param_i[0] = 1; // 着弾拡散フラグ

        pDrop->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pDrop->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pDrop;
        pEnemyShotSet->pEnemyShotHead->prev = pDrop;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 画面下部に到達したら着弾拡散
        if (pShot->param_i[0] == 1 && pShot->y > 470.0) {
            pShot->param_i[0] = 0; // 拡散済み
            pShot->speed = 0.0;
            pShot->kind = img_enemyShotMediumOval[INK_COLORS[GetRand(NUM_INK_COLORS - 1)]]; // リングに変化

            // 着弾点から全方位に飛び散るインク（小玉）を生成
            for (int i = 0; i < 12; i++) {
                sEnemyShot* pSub = new sEnemyShot;
                pSub->x = pShot->x;
                pSub->y = pShot->y;
                pSub->muki = (double)i / 12.0 * DX_PI * 2.0;
                pSub->speed = 3.0 + GetRand(150) / 100.0;
                pSub->kind = img_enemyShotSmallBall[INK_COLORS[GetRand(NUM_INK_COLORS - 1)]];

                pSub->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pSub->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pSub;
                pEnemyShotSet->pEnemyShotHead->prev = pSub;
            }
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// フェーズ2: イカジャンプ（自機狙い＋壁反射＋複雑な波打ち）
static void ShotIkaJumpHard(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int color = INK_COLORS[GetRand(NUM_INK_COLORS - 1)];

        // 5wayで発射し、密度を上げる
        for (int i = 0; i < 5; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = pEnemyShotSet->muki + (i - 2) * 0.25;
            pShot->speed = 4.0;
            pShot->kind = img_enemyShotScale[color];
            pShot->param_d[0] = (double)GetRand(100) / 100.0 * DX_PI * 2.0; // 波打ちの初期位相
            pShot->param_i[0] = 0; // 反射カウンタ

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 基本移動 ＋ 激しい正弦波による横揺れ（ぬるぬるした軌道）
        double move_x = pShot->speed * cos(pShot->muki) + sin(pShot->count * 0.25/2 + pShot->param_d[0]) * 3.5/3;
        double move_y = pShot->speed * sin(pShot->muki);

        pShot->x += move_x;
        pShot->y += move_y;

        // 画面左右端での反射（最大2回まで）
        if ((pShot->x < 20.0 || pShot->x > 460.0) && pShot->param_i[0] < 2) {
            pShot->muki = DX_PI - pShot->muki;
            pShot->param_d[0] = (double)GetRand(100) / 100.0 * DX_PI * 2.0; // 反射で位相をリセットして軌道を変化させる
            pShot->param_i[0]++;
        }
        pShot = pShot->next;
    }
}

// フェーズ3: キューバンボム（高密度・時間差爆発）
static void ShotCubeBombHard(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        int color = INK_COLORS[GetRand(NUM_INK_COLORS - 1)];

        // 3x3 のキューブ状に配置
        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                sEnemyShot* pShot = new sEnemyShot;
                pShot->x = pEnemyShotSet->x + dx * 20.0;
                pShot->y = pEnemyShotSet->y + dy * 20.0;
                pShot->muki = 0.0;
                pShot->speed = 0.0;
                pShot->kind = img_enemyShotDiamond[color];
                pShot->param_i[0] = 0; // 0:待機中, 1:爆発中
                pShot->param_i[1] = 30 + GetRand(40); // 爆発までの時間を各弾でズラす

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            pShot->param_i[1]--;
            if (pShot->param_i[1] <= 0) {
                pShot->param_i[0] = 1;
                pShot->kind = img_enemyShotMediumBall[INK_COLORS[GetRand(NUM_INK_COLORS - 1)]];

                // 爆発時に8方向＋自機狙い1方向の計9方向へ拡散
                for (int i = 0; i < 9; i++) {
                    sEnemyShot* pSub = new sEnemyShot;
                    pSub->x = pShot->x;
                    pSub->y = pShot->y;
                    if (i == 8) {
                        pSub->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                    }
                    else {
                        pSub->muki = (double)i / 8.0 * DX_PI * 2.0;
                    }
                    pSub->speed = 4.0 + GetRand(100) / 100.0;
                    pSub->kind = img_enemyShotSmallBall[INK_COLORS[GetRand(NUM_INK_COLORS - 1)]];
                    pSub->param_i[0] = 2;

                    pSub->prev = pEnemyShotSet->pEnemyShotHead->prev;
                    pSub->next = pEnemyShotSet->pEnemyShotHead;
                    pEnemyShotSet->pEnemyShotHead->prev->next = pSub;
                    pEnemyShotSet->pEnemyShotHead->prev = pSub;
                }
            }
        }
        else if (pShot->param_i[0] == 1) {
            // 爆発済みの親弾は画面外へ飛ばしてメインルーチンに消去させる
            pShot->y = -1000.0;
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// フェーズ4: ウルトラショット（高速回転レーザー＋螺旋弾）
static void ShotUltraShotHard(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        int color = INK_COLORS[GetRand(NUM_INK_COLORS - 1)];

        sEnemyShot* pLaser = new sEnemyShot;
        pLaser->x = pEnemyShotSet->x;
        pLaser->y = pEnemyShotSet->y;
        pLaser->muki = DX_PI * 0.8; // 初期角度（左上から右下へ）
        pLaser->speed = 0.0;
        pLaser->kind = img_enemyShotLaser[color];
        pLaser->param_d[0] = 0.06; // 高速回転速度
        pLaser->param_i[0] = 1;

        pLaser->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pLaser->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pLaser;
        pEnemyShotSet->pEnemyShotHead->prev = pLaser;
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // レーザーの回転に連動して、螺旋状に巻き込むように弾を放出
        if (pShot->param_i[0] == 1) {
            // レーザーを高速で回転させる
            pShot->muki += pShot->param_d[0];
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;

            if (pEnemyShotSet->count % 3 == 0) {
                sEnemyShot* pSub = new sEnemyShot;
                pSub->x = pEnemyShotSet->x;
                pSub->y = pEnemyShotSet->y;
                pSub->muki = pShot->muki + DX_PI / 2.0; // レーザーの進行方向に対して垂直に飛ばす
                pSub->speed = 4.5;
                pSub->kind = img_enemyShotBullet[INK_COLORS[GetRand(NUM_INK_COLORS - 1)]];

                pSub->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pSub->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pSub;
                pEnemyShotSet->pEnemyShotHead->prev = pSub;
            }
        }
        else {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン制御（600フレーム周期でループ）
// ============================================================
void EnemyPat_Splatoon_Qwen()
{
    static int cycle_count = 0;
    static int shot_count = 0;
    static int enemy_muki = 1;

    // 600フレーム(約10秒)周期でループさせる
    cycle_count = count % 600;

    if (count == 1) {
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }

    if (cycle_count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        shot_count = 0;
        enemy_muki = 1;
    }

    // Phase 1: ナワバリインクローラー (cycle: 1 ~ 150)
    if (cycle_count >= 1 && cycle_count <= 150) {
        enemy.x += 2.5 * (double)enemy_muki;
        if (enemy.x > 400.0 || enemy.x < 80.0) enemy_muki *= -1;

        if (cycle_count % 12 == 1) { // 発射頻度を上げて密度を高める
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotNawabariHard;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            pSet->muki = DX_PI / 2.0;
            pSet->kind = shot_count++;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
    // Phase 2: イカジャンプ (cycle: 151 ~ 300)
    else if (cycle_count >= 151 && cycle_count <= 300) {
        enemy.x = 240.0 + sin(cycle_count * 0.08) * 70.0;
        enemy.y = 80.0 + cos(cycle_count * 0.1) * 30.0;

        if (cycle_count % 25 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotIkaJumpHard;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
            pSet->kind = shot_count++;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
    // Phase 3: キューバンボム (cycle: 301 ~ 450)
    else if (cycle_count >= 301 && cycle_count <= 450) {
        enemy.x = 240.0;
        enemy.y = 80.0;

        // 周期内に2回発動して間断なく攻める
        if (cycle_count == 301 || cycle_count == 380) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotCubeBombHard;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
            pSet->kind = shot_count++;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
    // Phase 4: ウルトラショット (cycle: 451 ~ 600)
    else if (cycle_count >= 451 && cycle_count <= 600) {
        enemy.x = 240.0;
        enemy.y = 40.0;

        if (cycle_count == 451) {
            if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }

        if (cycle_count == 511) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotUltraShotHard;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 10.0;
            pSet->muki = DX_PI * 0.7;
            pSet->kind = shot_count++;
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
}