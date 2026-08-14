// EnemyPat_FlickeringLight_DeepSeek.cpp
// 切れかけの電球モチーフ弾幕「終焉のフィラメント」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---------------------------------------------------------------
// フェーズ1：安定点灯 - 螺旋状に拡散する光弾
// ---------------------------------------------------------------
static void Pattern_StableLightSpiral(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        // 螺旋の初期角度と回転方向
        pSet->param_i[0] = GetRand(360);          // 開始角度（度単位、後でラジアン変換）
        pSet->param_i[1] = (GetRand(1) == 0) ? 1 : -1; // 回転方向
        pSet->param_d[0] = 1.2;                   // 半径の広がり速度
    }

    // 約5秒間（300フレーム）だけ弾を生成
    if (pSet->count < 270) {
        const double angleStep = 0.12;            // 1フレームあたりの角度変化
        const double radius0 = 20.0;
        double radius = radius0 + pSet->count * pSet->param_d[0];
        double baseAngle = pSet->param_i[0] * DX_PI / 180.0 + pSet->count * angleStep * pSet->param_i[1];

        // 2本の螺旋アーム
        for (int arm = 0; arm < 2; ++arm) {
            double ang = baseAngle + arm * DX_PI;
            double sx = pSet->x + radius * cos(ang);
            double sy = pSet->y + radius * sin(ang);

            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = sx;
            pShot->y = sy;
            pShot->muki = ang;                     // 外側へ飛ぶ
            pShot->speed = 1.2 * 1.5;
            pShot->kind = img_enemyShotSmallBall[8]; // オレンジ小玉

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------------
// フェーズ2：ちらつき 明状態 - 光の輪が広がり内側へ針弾
// ---------------------------------------------------------------
static void Pattern_FlickerBright(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        pSet->param_d[0] = 3.0; // 輪の拡大速度（ピクセル/フレーム）
    }

    // 0.5秒後に針弾発射
    const int fireTime = 30; // 60fpsで0.5秒
    if (pSet->count == fireTime) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        double R = fireTime * pSet->param_d[0]; // この時点での輪の半径
        const int numBullets = 36 * 2;              // 10度間隔
        for (int i = 0; i < numBullets; ++i) {
            double ang = i * (2.0 * DX_PI) / numBullets;
            double fx = pSet->x + R * cos(ang);
            double fy = pSet->y + R * sin(ang);

            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = fx;
            pShot->y = fy;
            // 中心に向かう方向（若干のばらつきあり）
            pShot->muki = ang + DX_PI + (GetRand(20) - 10) * DX_PI / 180.0;
            pShot->speed = 4.0 + GetRand(200) / 100.0; // 4.0～6.0
            pShot->kind = img_enemyShotBullet[3];      // シアン銃弾（針弾イメージ）
            pShot->margin = 240;

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------------
// フェーズ2：ちらつき 暗状態 - 低速追尾弾
// ---------------------------------------------------------------
static void Pattern_FlickerDark(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 5発の追尾弾を生成
        for (int i = 0; i < 5; ++i) {
            double ang = GetRand(360) * DX_PI / 180.0; // ランダム方向
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = ang;
            pShot->speed = 0.6;                    // 低速
            pShot->kind = img_enemyShotSmallBall[5]; // マゼンタ小玉
            pShot->param_d[0] = 0.025;             // 1フレームあたりの最大旋回角

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // 毎フレーム、弾の向きを自機方向に旋回させる
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        double target = atan2(player.y - pShot->y, player.x - pShot->x);
        double diff = target - pShot->muki;
        // -PI ～ PI に正規化
        while (diff > DX_PI) diff -= 2.0 * DX_PI;
        while (diff < -DX_PI) diff += 2.0 * DX_PI;
        double turn = diff;
        double maxTurn = pShot->param_d[0];
        if (turn > maxTurn) turn = maxTurn;
        else if (turn < -maxTurn) turn = -maxTurn;
        pShot->muki += turn;
        pShot = pShot->next;
    }

    pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------------
// フェーズ2：スパーク - 突然のランダム高速破片
// ---------------------------------------------------------------
static void Pattern_Spark(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 10 * 5; ++i) {
            double ang = GetRand(360) * DX_PI / 180.0;
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x;
            pShot->y = pSet->y;
            pShot->muki = ang;
            pShot->speed = 3.0 + GetRand(200) / 100.0; // 3.0～5.0
            pShot->kind = img_enemyShotScale[6];       // 白鱗弾

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------------
// フェーズ3：瀕死の消灯 - 残光弾の網と爆発
// ---------------------------------------------------------------
static void Pattern_DyingWeb(sEnemyShotSet* pSet)
{
    const int burstTime = 120; // 2秒後に加速＆破裂

    if (pSet->count == 0) {
        // 20発の低速残光弾をランダムな位置に生成
        for (int i = 0; i < 20 * 5; ++i) {
            double offsetX = GetRand(200) - 100.0;
            double offsetY = GetRand(200) - 100.0;
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pSet->x + offsetX;
            pShot->y = pSet->y + offsetY;
            pShot->speed = 0.3;
            // ボスから見た外側方向を向く
            pShot->muki = atan2(pShot->y - pSet->y, pShot->x - pSet->x);
            pShot->kind = img_enemyShotSmallBall[7];   // 黒小玉（視認性低）
            pShot->param_i[0] = 0;                     // 未加速フラグ

            pShot->prev = pSet->pEnemyShotHead->prev;
            pShot->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pShot;
            pSet->pEnemyShotHead->prev = pShot;
        }
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }

    // 2秒後に全残光弾を加速＆破裂
    if (pSet->count == burstTime) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                // 外側へ急加速
                pShot->speed = 4.5;
                pShot->muki = atan2(pShot->y - pSet->y, pShot->x - pSet->x)
                    + (GetRand(30) - 15) * DX_PI / 180.0;
                pShot->param_i[0] = 1;

                // 破裂：各残光弾の位置から破片を3発ずつ
                for (int j = 0; j < 3; ++j) {
                    double fragAng = GetRand(360) * DX_PI / 180.0;
                    sEnemyShot* pFrag = new sEnemyShot;
                    pFrag->x = pShot->x;
                    pFrag->y = pShot->y;
                    pFrag->muki = fragAng;
                    pFrag->speed = 2.0 + GetRand(100) / 100.0;
                    pFrag->kind = img_enemyShotMediumBall[0]; // 赤中玉
                    pFrag->param_i[0] = 2;

                    pFrag->prev = pSet->pEnemyShotHead->prev;
                    pFrag->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = pFrag;
                    pSet->pEnemyShotHead->prev = pFrag;
                }
            }
            pShot = pShot->next;
        }
    }

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------------
// フェーズ3：レーザーの壁（左右からゆっくり降りてくる）
// ---------------------------------------------------------------
static void Pattern_LaserWalls(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 左壁
        {
            sEnemyShot* pL = new sEnemyShot;
            pL->x = 60.0;
            pL->y = -30.0;                    // 画面上部の外からスタート
            pL->muki = 0.0;                   // 水平右向き（画像の基準方向）
            pL->speed = 1.0;                  // ゆっくり下へ
            pL->kind = img_enemyShotLaser[4]; // 青レーザー
            pL->margin = 100;
            pL->prev = pSet->pEnemyShotHead->prev;
            pL->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pL;
            pSet->pEnemyShotHead->prev = pL;
        }
        // 右壁
        {
            sEnemyShot* pR = new sEnemyShot;
            pR->x = 420.0;
            pR->y = -30.0;
            pR->muki = 0.0;
            pR->speed = 1.0;
            pR->kind = img_enemyShotLaser[4];
            pR->margin = 100;
            pR->prev = pSet->pEnemyShotHead->prev;
            pR->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pR;
            pSet->pEnemyShotHead->prev = pR;
        }
        // 無音または軽い音
    }
    // 弾は自動で下に移動し、画面外で消える

    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->y += pShot->speed;

        pShot = pShot->next;
    }
}

// ---------------------------------------------------------------
// 敵本体パターン（終焉のフィラメント）
// ---------------------------------------------------------------
void EnemyPat_FlickeringLight_DeepSeek()
{
    // 静的ローカル変数で状態を保持
    static int  muki = 1;
    static int  phase = 1;
    static int  prevPhase = 0;

    // フェーズ2用
    static int  flickerTimer = 0;
    static int  flickerState = 0;   // 0=明, 1=暗
    static int  sparkCycleCount = 0;

    // フェーズ3用
    static int  phase3Timer = 0;
    static int  phase3DarkLight = 0; // 0=暗, 1=明
    static int  phase3TeleportTimer = 0;
    static int  phase3LaserTimer = 0;

    // ---- 初回初期化 ----
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 1;
        prevPhase = 0;
        flickerTimer = 0;   flickerState = 0;  sparkCycleCount = 0;
        phase3Timer = 0;    phase3DarkLight = 0;
        phase3TeleportTimer = 80 + GetRand(70);
        phase3LaserTimer = 300;
        return;
    }

    // ---- HPからフェーズ判定 ----
    if (enemy.hp > 120)
        phase = 1;
    else if (enemy.hp > 60)
        phase = 2;
    else
        phase = 3;

    // フェーズが切り替わったら関連タイマーをリセット
    if (phase != prevPhase) {
        flickerTimer = 0;  flickerState = 0;  sparkCycleCount = 0;
        phase3Timer = 0;   phase3DarkLight = 0;
        phase3TeleportTimer = 80 + GetRand(70);
        phase3LaserTimer = 300;
        prevPhase = phase;
    }

    // ---- 移動 ----
    if (phase == 1) {
        // ゆっくり左右に移動
        enemy.x += 0.5 * muki;
        if (count % 120 == 60) muki *= -1;
        // 画面内に留める
        if (enemy.x < 30) enemy.x = 30;
        if (enemy.x > 450) enemy.x = 450;
    }
    else if (phase == 2) {
        // 横方向に正弦波＋上下にも揺れる
        enemy.x = 240.0 + 100.0 * sin(count * 0.03);
        enemy.y = 40.0 + 10.0 * sin(count * 0.05);
    }
    else { // フェーズ3
        // 不規則なテレポート
        if (phase3TeleportTimer <= 0) {
            enemy.x = 120.0 + GetRand(240); // 120～360
            phase3TeleportTimer = 80 + GetRand(70);
        }
        else {
            phase3TeleportTimer--;
        }
        enemy.y = 40.0 + 5.0 * sin(count * 0.1);
    }

    // ---- 弾幕セット生成 ----
    if (phase == 1) {
        // 約1秒ごとに螺旋セットを発射
        if (count % 60 == 1) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = Pattern_StableLightSpiral;
            pSet->x = enemy.x;
            pSet->y = enemy.y + 20.0;
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
    else if (phase == 2) {
        // ちらつきの状態遷移
        if (flickerTimer <= 0) {
            if (flickerState == 0) {
                // 明状態開始
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = Pattern_FlickerBright;
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

                flickerTimer = 60; // 1秒間続く
            }
            else {
                // 暗状態開始
                // 5サイクルに1回、切り替わり直前にスパーク
                if (sparkCycleCount % 5 == 0) {
                    sEnemyShotSet* pSpark = new sEnemyShotSet;
                    pSpark->count = 0;
                    pSpark->patternFunc = Pattern_Spark;
                    pSpark->x = enemy.x;
                    pSpark->y = enemy.y;
                    pSpark->muki = 0.0;
                    pSpark->kind = 0;
                    pSpark->pEnemyShotHead = new sEnemyShot;
                    pSpark->pEnemyShotHead->prev = pSpark->pEnemyShotHead;
                    pSpark->pEnemyShotHead->next = pSpark->pEnemyShotHead;
                    pSpark->prev = enemyShotSetHead.prev;
                    pSpark->next = &enemyShotSetHead;
                    enemyShotSetHead.prev->next = pSpark;
                    enemyShotSetHead.prev = pSpark;
                }

                // 追尾弾セット
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = Pattern_FlickerDark;
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

                flickerTimer = 60;
            }
        }
        else {
            flickerTimer--;
        }

        // タイマーが0になったら状態反転、明→暗の切り替わりでサイクルカウントアップ
        if (flickerTimer == 0) {
            if (flickerState == 1) { // 暗→明 の切り替わり
                sparkCycleCount++;
            }
            flickerState = 1 - flickerState;
        }
    }
    else { // フェーズ3
        // 暗／明 周期
        if (phase3Timer <= 0) {
            if (phase3DarkLight == 0) {
                // 暗期開始 -> 残光弾セット
                sEnemyShotSet* pSet = new sEnemyShotSet;
                pSet->count = 0;
                pSet->patternFunc = Pattern_DyingWeb;
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

                phase3Timer = 180; // 3秒間暗い
            }
            else {
                // 明期開始（ここでは特にセットは出さずに次の暗期を待つ）
                phase3Timer = 30;  // 0.5秒だけ明るい
            }
            phase3DarkLight = 1 - phase3DarkLight;
        }
        else {
            phase3Timer--;
        }

        // 一定間隔でレーザー壁
        if (phase3LaserTimer <= 0) {
            sEnemyShotSet* pLaser = new sEnemyShotSet;
            pLaser->count = 0;
            pLaser->patternFunc = Pattern_LaserWalls;
            pLaser->x = enemy.x; // 使わないが一応
            pLaser->y = enemy.y;
            pLaser->muki = 0.0;
            pLaser->kind = 0;
            pLaser->pEnemyShotHead = new sEnemyShot;
            pLaser->pEnemyShotHead->prev = pLaser->pEnemyShotHead;
            pLaser->pEnemyShotHead->next = pLaser->pEnemyShotHead;
            pLaser->prev = enemyShotSetHead.prev;
            pLaser->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pLaser;
            enemyShotSetHead.prev = pLaser;

            phase3LaserTimer = 300; // 5秒に1回
        }
        else {
            phase3LaserTimer--;
        }
    }
}