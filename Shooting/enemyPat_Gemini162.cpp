#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ----------------------------------------------------
// ヘルパー関数：弾リストへの追加処理
// ----------------------------------------------------
static void AddShotToSet(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pEnemyShot)
{
    pEnemyShot->margin = 240;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// ----------------------------------------------------
// ヘルパー関数：新しい弾幕セットの登録
// ----------------------------------------------------
static void AddEnemyShotSet(void (*patternFunc)(sEnemyShotSet*), double x, double y)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = patternFunc;
    pSet->x = x;
    pSet->y = y;

    // ダミーヘッドの初期化
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
}


// ====================================================
// 弾幕パーツの挙動定義
// ====================================================

// 1. 器（鉢）の設置
static void ShotBowl(sEnemyShotSet* pEnemyShotSet)
{
    // 最初のみ、黄色の大玉をその場に配置して固定する
    if (pEnemyShotSet->count == 0) {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = 0;
        pEnemyShot->speed = 0.0; // 動かさない
        pEnemyShot->kind = img_enemyShotLargeBall[1]; // 1:黄
        AddShotToSet(pEnemyShotSet, pEnemyShot);
    }
    // 移動処理は行わない
}

// 2. 注がれるミルクの線
static void ShotMilkStream(sEnemyShotSet* pEnemyShotSet)
{
    // 高密度で真下に白の弾を流し続ける
    if (pEnemyShotSet->count % 2 == 0) {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        // 少しだけ左右に揺らぎを与えて液体の質感を出す
        pEnemyShot->x = pEnemyShotSet->x + (GetRand(4) - 2) / 2.0;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = DX_PI / 2.0; // 真下
        pEnemyShot->speed = 6.0 + (GetRand(10) / 10.0);
        pEnemyShot->kind = img_enemyShotMediumOval[6]; // 6:白
        AddShotToSet(pEnemyShotSet, pEnemyShot);
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // 鉢（y=320）付近に到達したら消す（画面外へ飛ばしてメイン処理に消去を任せる）
        if (pShot->y > 310.0) {
            pShot->y = 9999.0;
        }
        pShot = pShot->next;
    }
}

// 3. 鉢から弾け飛ぶ飛散エフェクト（波状拡散）
static void ShotMilkSplash(sEnemyShotSet* pEnemyShotSet)
{
    // 飛沫を定期的に打ち上げる
    if (pEnemyShotSet->count % 4 == 0) {
        // 効果音が連続しすぎないように間隔をあける
        if (pEnemyShotSet->count % 24 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 一度に 2〜4 発飛ばす
        int num = 2 + GetRand(2);
        for (int i = 0; i < num; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x + GetRand(16) - 8;
            pEnemyShot->y = pEnemyShotSet->y - 10; // 鉢の少し上から発射

            // 角度：真上（-PI/2）から左右に ±60度 程度
            double angle = -DX_PI / 2.0 + (GetRand(120) - 60) / 180.0 * DX_PI;
            double spd = 2.0 + GetRand(40) / 10.0;

            // 重力落下をさせるため、param_dに初期のX速度・Y速度を記憶
            pEnemyShot->param_d[0] = spd * cos(angle);
            pEnemyShot->param_d[1] = spd * sin(angle);

            // 飛散する液体の色：白(6)、シアン(3)、青(4)
            int color = 6;
            int r = GetRand(2);
            if (r == 1) color = 3;
            else if (r == 2) color = 4;

            pEnemyShot->kind = img_enemyShotSmallBall[color];
            AddShotToSet(pEnemyShotSet, pEnemyShot);
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // Y方向の速度（param_d[1]）に重力を加算
        pShot->param_d[1] += 0.08;

        pShot->x += pShot->param_d[0];
        pShot->y += pShot->param_d[1];

        pShot = pShot->next;
    }
}

// 4. 窓からの斜光（斜めに降り注ぐレーザー列）
static void ShotWindowLight(sEnemyShotSet* pEnemyShotSet)
{
    // 90フレームごとに斜めのレーザー列を放つ
    if (pEnemyShotSet->count % 90 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 画面左上の領域を基点に、右下（45度付近）へ
        double muki = DX_PI / 4.0 + (GetRand(20) - 10) / 100.0;
        double startX = (GetRand(400) - 200);
        double startY = -40;

        // 35個のレーザー弾で直線の光を形成
        for (int i = 0; i < 35; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            // 弾を進行方向と逆向きにオフセットさせて並べる
            pEnemyShot->x = startX - i * 16 * cos(muki);
            pEnemyShot->y = startY - i * 16 * sin(muki);
            pEnemyShot->muki = muki;
            pEnemyShot->speed = 2.5; // 安全地帯を見極められるように低速

            // 色は黄色(1)をベースに、時折橙色(8)を混ぜる
            int color = (GetRand(10) > 8) ? 8 : 1;
            pEnemyShot->kind = img_enemyShotLaser[color];

            AddShotToSet(pEnemyShotSet, pEnemyShot);
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ====================================================
// 敵本体のパターン
// ====================================================
void EnemyPat_Milkmaid_Gemini()
{
    // 初期化
    if (count == 1) {
        enemy.x = 320.0; // 画面右寄りに配置
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200;

        // 1. 器（鉢）の設置 (固定)
        AddEnemyShotSet(ShotBowl, enemy.x, 320.0);
    }

    // ボス本体は動かない（フェルメールの静寂を表現）

    // 2. ミルクの注ぎ開始
    if (count == 30) {
        AddEnemyShotSet(ShotMilkStream, enemy.x, enemy.y + 10.0);
    }

    // 3. ミルクの飛散開始（ミルクが鉢に到達する頃に同期）
    if (count == 60) {
        AddEnemyShotSet(ShotMilkSplash, enemy.x, 320.0);
    }

    // 4. 窓からの斜光開始
    if (count == 120) {
        AddEnemyShotSet(ShotWindowLight, 0.0, 0.0);
    }
}