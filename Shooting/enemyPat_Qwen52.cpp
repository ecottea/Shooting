// EnemyPat_Bee_Qwen.cpp
// 蜂をモチーフにした弾幕パターン「凶蜂の六芒毒針陣」の実装

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕パターン関数 1: チャージエフェクト (予備動作)
// ============================================================
static void ShotChargeEffect(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 敵の周囲に小さな白い弾を配置（溜めているエフェクト）
        for (int i = 0; i < 12; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            double angle = (double)i * (DX_PI * 2.0 / 12.0);

            // 初期位置を敵の周囲に設定
            pShot->param_d[0] = pEnemyShotSet->x + cos(angle) * 30.0;
            pShot->param_d[1] = pEnemyShotSet->y + sin(angle) * 30.0;

            // 白色の小玉
            pShot->kind = img_enemyShotSmallBall[6];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double target_x = pEnemyShotSet->x;
        double target_y = pEnemyShotSet->y;

        double dx = target_x - pShot->param_d[0];
        double dy = target_y - pShot->param_d[1];
        double dist = sqrt(dx * dx + dy * dy);

        // 中心に到達したら消去 (エフェクト終了)
        if (dist < 5.0) {
            sEnemyShot* next = pShot->next;
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
            pShot = next;
            continue;
        }

        // 中心に向かって移動
        double speed = 3.0 / 3.8;
        pShot->x = pShot->param_d[0] + (dx / dist) * speed;
        pShot->y = pShot->param_d[1] + (dy / dist) * speed;

        // 次フレームの基準位置として更新
        pShot->param_d[0] = pShot->x;
        pShot->param_d[1] = pShot->y;

        pShot = pShot->next;
    }
}

// ============================================================
// 弾幕パターン関数 2: ハニカム・リング (巣の壁)
// ============================================================
static void ShotHoneycombRing(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 六角形(6方向)に弾を配置
        for (int i = 0; i < 6; i++) {
            sEnemyShot* pShot = new sEnemyShot;
            pShot->param_d[0] = (double)i * (DX_PI * 2.0 / 6.0); // 初期角度オフセット
            
            // 黄色の鱗弾 (巣のイメージ)
            pShot->kind = img_enemyShotScale[1];
            pShot->margin = 480;

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 既存の弾の更新 (回転しながら拡大)
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double radius = 60.0 + t * 0.8;               // 徐々に拡大
        double angle = pShot->param_d[0] + t * 0.025; // 時計回りに回転

        pShot->x = pEnemyShotSet->x + cos(angle) * radius;
        pShot->y = pEnemyShotSet->y + sin(angle) * radius;
        pShot->muki = angle;

        pShot = pShot->next;
    }
}

// ============================================================
// 弾幕パターン関数 3: バズィング・ニードル (振動する針)
// ============================================================
static void ShotBuzzingNeedle(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 自機を狙う方向を計算
        double base_muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);

        // 3方向 × 3段階のズレ = 9発の群れを生成
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 5; j++) {
                sEnemyShot* pShot = new sEnemyShot;

                double muki_offset = (i - 2) * 0.15; // -0.15, 0, +0.15 ラジアンの角度ズレ
                double muki = base_muki + muki_offset;
                double speed = 3.5 + (j * 0.6);      // 速度に少しばらつきを持たせる

                // param_d に移動パラメータを保存
                pShot->param_d[0] = pEnemyShotSet->x;          // [0] start_x
                pShot->param_d[1] = pEnemyShotSet->y;          // [1] start_y
                pShot->param_d[2] = muki;                      // [2] 進行角度
                pShot->param_d[3] = speed;                     // [3] 速度
                pShot->param_d[4] = 18.0;                      // [4] 振動の振幅
                pShot->param_d[5] = 0.15;                      // [5] 振動の周波数

                // 黄色の銃弾 (針のイメージ)
                pShot->muki = muki;
                pShot->kind = img_enemyShotBullet[1];

                pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
                pEnemyShotSet->pEnemyShotHead->prev = pShot;
            }
        }
    }

    // 既存の弾の更新 (サイン波振動移動)
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double muki = pShot->param_d[2];
        double speed = pShot->param_d[3];
        double amp = pShot->param_d[4];
        double freq = pShot->param_d[5];

        // 直進成分
        double base_x = pShot->param_d[0] + t * speed * cos(muki);
        double base_y = pShot->param_d[1] + t * speed * sin(muki);

        // 垂直方向の単位ベクトル
        double perp_x = -sin(muki);
        double perp_y = cos(muki);

        // サイン波による振動オフセット
        double osc = amp * sin(t * freq);

        pShot->x = base_x + perp_x * osc;
        pShot->y = base_y + perp_y * osc;

        pShot = pShot->next;
    }
}

// ============================================================
// 弾幕パターン関数 4: ポイズン・ドロップ (毒蜜)
// ============================================================
static void ShotPoisonDrop(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 画面横方向にランダムに配置 (GetRand は 0～x を返す)
        for (int i = 0; i < 64; i++) {
            sEnemyShot* pShot = new sEnemyShot;

            // 画面外上部からスタート
            pShot->param_d[0] = (double)GetRand(440) + 20.0; // X: 20.0 ～ 460.0
            pShot->param_d[1] = -20.0;                       // Y: -20.0
            pShot->param_d[2] = 1.2 + (GetRand(100) / 100.0); // 落下速度: 1.2 ～ 2.2

            // 緑色の中玉 (毒のイメージ)
            pShot->kind = img_enemyShotMediumBall[2];

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;
        }
    }

    // 既存の弾の更新 (ゆらゆらと落下)
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double t = (double)pShot->count;
        double drop_speed = pShot->param_d[2];

        // 緩やかなサイン波の横揺れを加える
        pShot->x = pShot->param_d[0] + sin(t * 0.05) * 8.0;
        pShot->y = pShot->param_d[1] + t * drop_speed;

        pShot = pShot->next;
    }
}


// ============================================================
// 敵本体のパターン制御
// ============================================================
void EnemyPat_Bee_Qwen()
{
    // count はメインルーチンで毎フレーム自動インクリメントされるため書き換えない。
    // 代わりに剰余演算で 400フレーム(約6.6秒) のサイクルを作成して管理する。
    int cycle = count % 400;

    if (count == 1) {
        // 初期配置: 画面上部中央
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // 耐久値
    }
    else {
        // 【敵の挙動: ホバリング】
        // 上下にゆっくり揺れながら、左右に往復移動する
        enemy.y = 80.0 + sin(count * 0.05) * 15.0;
        enemy.x = 240.0 + sin(count * 0.015) * 120.0;
    }

    // --------------------------------------------------------
    // 攻撃フェーズのタイムライン管理 (cycle 基準)
    // --------------------------------------------------------

    // フェーズ1: cycle 60 - 予備動作（敵弾によるエフェクト）
    if (cycle == 60) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotChargeEffect;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // フェーズ2: cycle 90 - ハニカム・リング展開
    if (cycle >= 90 && cycle < 160 && cycle % 10 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotHoneycombRing;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->kind = 1;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // フェーズ3: cycle 160 - バズィング・ニードル発射
    // リングが展開しきった頃合いに、隙間から針を放つ
    if (cycle == 160) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotBuzzingNeedle;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0; // 少し下から発射
        pSet->kind = 2;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // フェーズ4: cycle 240 - ポイズン・ドロップ投下
    // 針で移動を制限されたところに、毒でさらに追い打ちをかける
    if (cycle == 240) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotPoisonDrop;
        pSet->x = 0.0; // 関数内でランダム配置するため未使用
        pSet->y = 0.0;
        pSet->kind = 3;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}