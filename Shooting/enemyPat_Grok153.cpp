#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  相転移弾幕：固体（結晶格子）→ 液体（流動）→ 気体（拡散）
//  使用素材：
//    固体：中玉 (img_enemyShotMediumBall) 色4(青) / 色3(シアン)
//    液体：小玉 (img_enemyShotSmallBall) 色3(シアン) / 色6(白)
//    気体：大玉 + 小玉 + 短レーザー  色6(白) / 色8(橙)
// ============================================================

// 固体フェーズ：結晶格子の回転・収縮
static void ShotSolidCrystal(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const double centerX = pEnemyShotSet->x;
    const double centerY = pEnemyShotSet->y;
    const int gridSize = 5;          // 5x5 格子
    const double spacing = 38.0;     // 格子間隔
    const double rotSpeed = 0.012;   // 回転速度
    const int phaseDuration = 180;   // このフェーズの長さ

    // 初回：格子状に中玉を配置
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        for (int gy = 0; gy < gridSize; gy++) {
            for (int gx = 0; gx < gridSize; gx++) {
                // 中心を空ける（ボス位置）
                if (gx == gridSize / 2 && gy == gridSize / 2) continue;

                pEnemyShot = new sEnemyShot;
                // 相対位置を param に保存
                double relX = (gx - (gridSize - 1) / 2.0) * spacing;
                double relY = (gy - (gridSize - 1) / 2.0) * spacing;
                pEnemyShot->param_d[0] = relX;          // 初期相対X
                pEnemyShot->param_d[1] = relY;          // 初期相対Y
                pEnemyShot->param_d[2] = atan2(relY, relX); // 初期角度
                pEnemyShot->param_d[3] = sqrt(relX * relX + relY * relY); // 半径
                pEnemyShot->param_i[0] = 0;             // フェーズ識別（0=固体）

                pEnemyShot->x = centerX + relX;
                pEnemyShot->y = centerY + relY;
                pEnemyShot->muki = 0.0;
                pEnemyShot->speed = 0.0;
                // 青〜シアンで結晶感
                pEnemyShot->kind = img_enemyShotMediumBall[(gx + gy) % 2 == 0 ? 4 : 3];

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // 毎フレーム：格子全体を回転＋緩やかに拡大しながら下へ
    double currentAngle = pEnemyShotSet->count * rotSpeed;
    double expand = 1.0 + pEnemyShotSet->count * 0.0015;
    double drop = pEnemyShotSet->count * 0.15;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            double r = pShot->param_d[3] * expand;
            double a = pShot->param_d[2] + currentAngle;
            pShot->x = centerX + r * cos(a);
            pShot->y = centerY + drop + r * sin(a);
        }
        pShot = pShot->next;
    }

    // フェーズ終了時に次のショットセットへバトンタッチするため何もしない
    // （EnemyPat_PhaseTransition_Grok 側で次フェーズを生成）
}

// 液体フェーズ：溶融して流れる小玉 + 液面レーザー
static void ShotLiquidFlow(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int phaseDuration = 200;

    // 初回と定期的に流れる弾を生成
    if (pEnemyShotSet->count == 0 || (pEnemyShotSet->count % 12 == 0 && pEnemyShotSet->count < phaseDuration - 30)) {
        if (pEnemyShotSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }

        // 複数の流れを左右にずらして生成
        for (int stream = 0; stream < 5; stream++) {
            double baseAngle = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
            double offsetAngle = (stream - 2) * 0.18;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + (stream - 2) * 28.0;
            pEnemyShot->y = pEnemyShotSet->y + 10.0;
            pEnemyShot->muki = baseAngle + offsetAngle;
            pEnemyShot->speed = 1.8 + GetRand(40) / 100.0;
            pEnemyShot->param_i[0] = 1;                 // 液体識別
            pEnemyShot->param_d[0] = pEnemyShot->muki;  // 基準角度
            pEnemyShot->param_d[1] = 0.035 + GetRand(20) / 1000.0; // 波の振幅速度
            pEnemyShot->param_d[2] = GetRand(628) / 100.0; // 波の初期位相
            // シアン〜白
            pEnemyShot->kind = img_enemyShotSmallBall[pEnemyShotSet->count % 2 == 0 ? 3 : 6];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 途中で横に走る「液面」レーザーを数本追加
    if (pEnemyShotSet->count == 60 || pEnemyShotSet->count == 120) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = 40.0 + i * 160.0;
            pEnemyShot->y = pEnemyShotSet->y + 80.0 + GetRand(60);
            pEnemyShot->muki = 0.0;          // 右方向
            pEnemyShot->speed = 3.5;
            pEnemyShot->param_i[0] = 1;
            pEnemyShot->kind = img_enemyShotLaser[3]; // シアンレーザー

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 移動：流れるように正弦波で軌道を揺らす
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // レーザー以外は波打たせる
            if (pShot->kind != img_enemyShotLaser[3] && pShot->kind != img_enemyShotLaser[6]) {
                double wave = sin(pShot->count * pShot->param_d[1] + pShot->param_d[2]) * 0.55;
                pShot->muki = pShot->param_d[0] + wave;
            }
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// 気体フェーズ：蒸発拡散（大玉・小玉・レーザーの爆発）
static void ShotGasDiffuse(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初回のみ大規模拡散
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 放射状に大玉＋小玉を撒く
        for (int i = 0; i < 36; i++) {
            double angle = i * (DX_PI * 2.0 / 36.0) + GetRand(20) / 100.0;

            // 大玉（外側）
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle;
            pEnemyShot->speed = 2.2 + GetRand(80) / 100.0;
            pEnemyShot->param_i[0] = 2;
            pEnemyShot->kind = img_enemyShotLargeBall[6]; // 白
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;

            // 小玉（内側・やや速い）
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = angle + 0.08;
            pEnemyShot->speed = 3.0 + GetRand(120) / 100.0;
            pEnemyShot->param_i[0] = 2;
            pEnemyShot->kind = img_enemyShotSmallBall[GetRand(1) == 0 ? 6 : 8]; // 白 or 橙
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }

        // 放射状の短いレーザー（分子の激しい運動を表現）
        for (int i = 0; i < 8; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = i * (DX_PI * 2.0 / 8.0);
            pEnemyShot->speed = 4.5;
            pEnemyShot->param_i[0] = 2;
            pEnemyShot->kind = img_enemyShotLaser[6]; // 白レーザー
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 追加のまばらな拡散（残りカスが飛び散る感じ）
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % 8 == 0 && pEnemyShotSet->count < 90) {
        for (int i = 0; i < 4; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(60) - 30;
            pEnemyShot->y = pEnemyShotSet->y + GetRand(40) - 20;
            pEnemyShot->muki = GetRand(628) / 100.0; // ほぼ全方位
            pEnemyShot->speed = 2.5 + GetRand(150) / 100.0;
            pEnemyShot->param_i[0] = 2;
            pEnemyShot->kind = img_enemyShotSmallBall[6];
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 単純な等速直線運動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 2) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体パターン
// ============================================================
void EnemyPat_PhaseTransition_Grok()
{
    static int phase;          // 0:固体 1:液体 2:気体 3:待機
    static int phaseTimer;
    static int muki;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 変更禁止
        phase = 0;
        phaseTimer = 0;
        muki = 1;
    }
    else {
        // 緩やかな左右移動
        enemy.x += 0.7 * (double)muki;
        if (enemy.x < 120.0) muki = 1;
        if (enemy.x > 360.0) muki = -1;
    }

    // フェーズ管理（全体で roughly 0-180固体 / 180-380液体 / 380-500気体）
    phaseTimer++;

    // --- 固体フェーズ開始 ---
    if (phase == 0 && phaseTimer == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSolidCrystal;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 固体 → 液体 遷移
    if (phase == 0 && phaseTimer >= 190) {
        phase = 1;
        phaseTimer = 0;

        // 液体ショットセット生成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLiquidFlow;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = atan2(player.y - enemy.y, player.x - enemy.x);
        pEnemyShotSet->kind = 1;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 液体 → 気体 遷移
    if (phase == 1 && phaseTimer >= 210) {
        phase = 2;
        phaseTimer = 0;

        // 気体ショットセット生成
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotGasDiffuse;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 2;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // 気体終了後、少し間を置いて固体に戻る（ループ）
    if (phase == 2 && phaseTimer >= 130) {
        phase = 0;
        phaseTimer = 0;
    }
}