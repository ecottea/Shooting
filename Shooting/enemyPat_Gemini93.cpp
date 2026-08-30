// enemyPat_SignalChange.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ====================================================================
// リスト操作用ヘルパー関数
// ====================================================================

// 個別の弾をリストに追加する関数
static void AddEnemyShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind) {
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->count = 0;

    // 二重円環リストの末尾（ヘッドの直前）に挿入
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
}

// 弾幕セットをリストに追加する関数
static void AddEnemyShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki) {
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;

    // ダミーヘッドの初期化
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    // 二重円環リストの末尾（ヘッドの直前）に挿入
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}

// ====================================================================
// 各フェーズの弾幕パターン関数
// ====================================================================

// 【青信号フェーズ：進行・攻撃可】
// プレイヤーに攻撃のチャンスを与える、隙間の広い青色のレーン弾
static void ShotSignalBlue(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        // 青色(4) の小玉
        int kind = img_enemyShotSmallBall[4];

        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 中心から左右に等間隔で9レーン展開
        for (int i = -4-2; i <= 4+2; i++) {
            AddEnemyShot(pSet, pSet->x + i * 40.0, pSet->y, DX_PI / 2.0, 3.0, kind);
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 【黄信号フェーズ：警告・移動強制】
// 大きく動くことを強要する、自機狙いの高速黄色ウェイブ弾
static void ShotSignalYellow(sEnemyShotSet* pSet) {
    // 3フレーム間隔で5連射
    if (pSet->count % 3 == 0 && pSet->count <= 12) {
        // 黄色(1) の銃弾
        int kind = img_enemyShotBullet[1];

        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 自機狙い（pSet->muki）を中心に 5Way（7.5度間隔）
        for (int i = -2; i <= 2; i++) {
            double angle = pSet->muki + i * (DX_PI / 24.0);
            AddEnemyShot(pSet, pSet->x, pSet->y, angle, 6.0, kind);
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 【赤信号フェーズ：停止・精密回避】
// 画面を制圧し、プレイヤーの動きを封じる赤色の全方位大玉
static void ShotSignalRed(sEnemyShotSet* pSet) {
    if (pSet->count == 0) {
        // 赤色(0) の大玉
        int kind = img_enemyShotLargeBall[0];

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 30Way 全方位 (大玉同士の隙間を絶妙に狭くし、ミリ避けを強要する)
        int way = 30 + 15;
        double base_angle = GetRand(999) / 1000.0 * DX_PI * 2.0;
        for (int i = 0; i < way; i++) {
            double angle = base_angle + i * (DX_PI * 2.0 / way);
            AddEnemyShot(pSet, pSet->x, pSet->y, angle, 2.0, kind);
        }
    }

    // 弾の移動処理
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ====================================================================
// 敵本体の処理
// ====================================================================

// 敵本体のパターン：シグナル・チェンジ
void EnemyPat_TrafficLight_Gemini() {
    // 600フレーム（約10秒）で1サイクルのループ
    int cycle = count % 600;

    // --- 初回初期化 ---
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // --- ボスの移動パターン ---
    if (cycle < 240) {
        // 青フェーズ：左右にゆったりと往復移動し、弾幕のレーンを波打たせる（ワインダー効果）
        // 240フレームでちょうど1往復するように周期を調整
        enemy.x = 240.0 + 120.0 * sin(cycle * DX_PI / 120.0);
    }
    else {
        // 黄・赤フェーズ：画面中央上部にゆっくりと収束（停止状態へ）
        enemy.x += (240.0 - enemy.x) * 0.05;
        enemy.y += (80.0 - enemy.y) * 0.05;
    }

    // --- 弾幕発射パターン ---

    // 【青信号フェーズ】(0〜239)
    if (cycle >= 0 && cycle < 240) {
        // 15フレームに1回、下方向のレーン弾を撃つ
        if (cycle % 15 == 0) {
            AddEnemyShotSet(ShotSignalBlue, enemy.x, enemy.y, 0.0);
        }
    }
    // 【黄信号フェーズ】(240〜329)
    else if (cycle >= 240 && cycle < 330) {
        // フェーズ開始時にチャージ音（予告音）を鳴らす
        if (cycle == 240) {
            PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        }
        // 30フレーム間隔で3回、自機狙いの扇状連射
        if (cycle == 240 || cycle == 270 || cycle == 300) {
            // 発射瞬間の自機角度を取得して渡す
            double aim = atan2(player.y - enemy.y, player.x - enemy.x);
            AddEnemyShotSet(ShotSignalYellow, enemy.x, enemy.y, aim);
        }
    }
    // 【赤信号フェーズ】(330〜539)
    else if (cycle >= 330 && cycle < 540) {
        // 45フレームに1回、全方位の赤リング弾を発射
        if (cycle % 35 == 0) {
            AddEnemyShotSet(ShotSignalRed, enemy.x, enemy.y, 0.0);
        }
    }
    // 【インターバル】(540〜599)
    // 何も撃たない（次の青フェーズへの準備・休憩期間）
}