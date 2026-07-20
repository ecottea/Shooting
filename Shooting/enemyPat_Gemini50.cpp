// enemyPat_sampleForAI.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================================
// スイミー・フォーメーション用 弾幕定義
// ============================================================================

// 巨大魚を構成する小魚（赤弾）と目（黒弾）の相対座標 (Center_X, Center_Y からのオフセット)
// 魚の頭が下（プレイヤー方向）を向くように配置（計40発）
static const double FISH_OFFSETS[][2] = {
    {  0.0,  50.0 },   // 0: 頭の先端
    { -10.0,  35.0 },   // 1: 左の「目」ポジション（★ここを「スイミーの黒弾」にする）
    {  10.0,  35.0 },   // 2: 右の目の対称位置（赤弾）
    { -20.0,  25.0 }, {  0.0,  25.0 }, { 20.0,  25.0 },
    { -25.0,  15.0 }, { -10.0,  15.0 }, { 10.0,  15.0 }, { 25.0,  15.0 },
    { -28.0,   5.0 }, { -15.0,   5.0 }, {  0.0,   5.0 }, { 15.0,   5.0 }, { 28.0,   5.0 },
    { -30.0,  -5.0 }, { -20.0,  -5.0 }, { -10.0,  -5.0 }, { 10.0,  -5.0 }, { 20.0,  -5.0 }, { 30.0,  -5.0 },
    { -28.0, -15.0 }, { -15.0, -15.0 }, {  0.0, -15.0 }, { 15.0, -15.0 }, { 28.0, -15.0 },
    { -22.0, -25.0 }, { -10.0, -25.0 }, { 10.0, -25.0 }, { 22.0, -25.0 },
    { -15.0, -35.0 }, {  0.0, -35.0 }, { 15.0, -35.0 },
    {  -8.0, -45.0 }, {  8.0, -45.0 },
    { -20.0, -55.0 }, {  0.0, -55.0 }, { 20.0, -55.0 },
    { -30.0, -65.0 }, { 30.0, -65.0 }  // 尾ひれの先端
};

// 弾の役割を格納するパラメータインデックス
#define BULLET_ROLE      param_i[0] // 0:赤弾(小魚), 1:黒弾(目), 2:青弾(泡)
#define BULLET_INIT_X    param_d[2] // フェーズ0(集結)開始時の初期位置X
#define BULLET_INIT_Y    param_d[3] // フェーズ0(集結)開始時の初期位置Y

// 弾幕セットのパラメータインデックス
#define SET_PHASE        param_i[0] // 0:集結(Assembly), 1:遊泳(Swimming), 2:霧散(Route A), 3:決壊(Route B)
#define SET_PHASE_TIMER  param_i[1] // 現在のフェーズになってからの経過フレーム数
#define SET_CENTER_X     param_d[0] // 巨大魚の中心座標X
#define SET_CENTER_Y     param_d[1] // 巨大魚の中心座標Y

// スイミー弾幕の制御関数
static void SwimmyPatternFunc(sEnemyShotSet* pEnemyShotSet)
{
    // フェーズ内タイマーを自前でインクリメント
    pEnemyShotSet->SET_PHASE_TIMER++;
    int phase = pEnemyShotSet->SET_PHASE;
    int timer = pEnemyShotSet->SET_PHASE_TIMER;

    // ------------------------------------------------------------------------
    // フェーズ0: 集結（バラバラにスポーンした赤弾が徐々に魚型に並ぶ）
    // ------------------------------------------------------------------------
    if (phase == 0) {
        // 巨大魚の中心は少しずつ降下
        pEnemyShotSet->SET_CENTER_Y += 0.5;

        double t = (double)timer / 60.0; // 60フレーム(1秒)かけて集結
        if (t > 1.0) t = 1.0;

        // イージング（3次シグモイド風）で滑らかに吸着
        double ease = 1.0 - pow(1.0 - t, 3.0);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->BULLET_ROLE != 2) { // 泡弾は除外
                double targetX = pEnemyShotSet->SET_CENTER_X + pShot->param_d[0];
                double targetY = pEnemyShotSet->SET_CENTER_Y + pShot->param_d[1];

                // 小魚が群れの中で細かく泳いでいるような微細な震え（シェイク）
                double shiverX = (GetRand(10) - 5) / 5.0; // -1.0 〜 1.0
                double shiverY = (GetRand(10) - 5) / 5.0;

                pShot->x = pShot->BULLET_INIT_X + (targetX - pShot->BULLET_INIT_X) * ease + shiverX;
                pShot->y = pShot->BULLET_INIT_Y + (targetY - pShot->BULLET_INIT_Y) * ease + shiverY;
            }
            pShot = pShot->next;
        }

        if (timer >= 60) {
            pEnemyShotSet->SET_PHASE = 1;       // フェーズ1(遊泳)へ移行
            pEnemyShotSet->SET_PHASE_TIMER = 0; // タイマーリセット
        }
    }
    // ------------------------------------------------------------------------
    // フェーズ1: 巨大魚の遊泳（サインカーブを描いて降下 ＆ 口から泡を吐く）
    // ------------------------------------------------------------------------
    else if (phase == 1) {
        // 降下しながら、横に揺れて泳ぐ演出
        pEnemyShotSet->SET_CENTER_Y += 0.8;
        pEnemyShotSet->SET_CENTER_X += 1.8 * sin(timer * 0.05);

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->BULLET_ROLE != 2) { // 小魚・目の座標更新
                double shiverX = (GetRand(10) - 5) / 5.0;
                double shiverY = (GetRand(10) - 5) / 5.0;
                pShot->x = pEnemyShotSet->SET_CENTER_X + pShot->param_d[0] + shiverX;
                pShot->y = pEnemyShotSet->SET_CENTER_Y + pShot->param_d[1] + shiverY;
            }
            else { // 泡弾は自律移動
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pShot->next;
        }

        // 15フレームに1回、魚の「口（一番前: 0, 50）」からプレイヤーに向けて低速の青弾（泡）を放つ
        if (timer % 15 == 0) {
            sEnemyShot* pBubble = new sEnemyShot;
            pBubble->x = pEnemyShotSet->SET_CENTER_X;
            pBubble->y = pEnemyShotSet->SET_CENTER_Y + 50.0;
            // プレイヤー方向に少しブレを加えて狙い撃つ
            pBubble->muki = atan2(player.y - pBubble->y, player.x - pBubble->x) + (GetRand(20) - 10) / 180.0 * DX_PI;
            pBubble->speed = 1.2 + (GetRand(10) / 10.0); // 1.2 〜 2.2 の低速
            pBubble->kind = img_enemyShotSmallBall[4];   // 小玉：青（泡に見立てる）
            pBubble->BULLET_ROLE = 2;                    // 泡

            // 弾リストへ追加
            pBubble->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pBubble->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pBubble;
            pEnemyShotSet->pEnemyShotHead->prev = pBubble;

            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 時間経過（5秒）または画面下部(Y=400)に到達した場合、決壊ルート(Route B)へ移行
        if (timer >= 300 || pEnemyShotSet->SET_CENTER_Y + 50.0 >= 400.0) {
            pEnemyShotSet->SET_PHASE = 3;       // フェーズ3(Route B)
            pEnemyShotSet->SET_PHASE_TIMER = 0;

            if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

            // すべての赤弾と目を自機狙いへと変化させる
            sEnemyShot* pTargetShot = pEnemyShotSet->pEnemyShotHead->next;
            while (pTargetShot != pEnemyShotSet->pEnemyShotHead) {
                if (pTargetShot->BULLET_ROLE != 2) {
                    pTargetShot->muki = atan2(player.y - pTargetShot->y, player.x - pTargetShot->x);
                    pTargetShot->speed = 2.0 + (GetRand(10) / 10.0); // 2.0 〜 3.0

                    if (pTargetShot->BULLET_ROLE == 1) {
                        pTargetShot->kind = img_enemyShotMediumBall[8]; // 目を警告色(橙)に変える
                    }
                }
                pTargetShot = pTargetShot->next;
            }
        }
    }
    // ------------------------------------------------------------------------
    // フェーズ2/3: 決壊・霧散（すべての弾が個別のベクトルに従ってフリー移動）
    // ------------------------------------------------------------------------
    else {
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
            pShot = pShot->next;
        }
    }

    // ========================================================================
    // プレイヤーショットと「目（黒弾）」の衝突判定（フェーズ0, 1のみ機能）
    // ========================================================================
    if (phase == 0 || phase == 1) {
        // 黒弾(目)を探す
        sEnemyShot* pEye = nullptr;
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->BULLET_ROLE == 1) {
                pEye = pShot;
                break;
            }
            pShot = pShot->next;
        }

        // 目が存在していれば、プレイヤーのショットとの当たり判定を行う
        if (pEye != nullptr) {
            sPlayerShot* pPlShot = playerShotHead.next;
            bool isHit = false;

            while (pPlShot != &playerShotHead) {
                double dx = pPlShot->x - pEye->x;
                double dy = pPlShot->y - pEye->y;
                double distSq = dx * dx + dy * dy;

                if (distSq < 15.0 * 15.0) { // 判定距離15px
                    isHit = true;
                    // 当たった自機弾を消去（メモリプールに還元）
                    sPlayerShot* pNextPlShot = pPlShot->next;
                    pPlShot->prev->next = pPlShot->next;
                    pPlShot->next->prev = pPlShot->prev;
                    delete pPlShot;
                    pPlShot = pNextPlShot;
                    continue;
                }
                pPlShot = pPlShot->next;
            }

            // 目を撃破した場合：霧散ルート(Route A)へ移行
            if (isHit) {
                pEnemyShotSet->SET_PHASE = 2;       // フェーズ2(Route A)
                pEnemyShotSet->SET_PHASE_TIMER = 0;

                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                // 全弾を現在の「中心」から外側へ吹き飛ばす（高速放射弾）
                sEnemyShot* pBlastShot = pEnemyShotSet->pEnemyShotHead->next;
                while (pBlastShot != pEnemyShotSet->pEnemyShotHead) {
                    if (pBlastShot->BULLET_ROLE != 2) {
                        double angle = atan2(pBlastShot->y - pEnemyShotSet->SET_CENTER_Y, pBlastShot->x - pEnemyShotSet->SET_CENTER_X);
                        pBlastShot->muki = angle + (GetRand(30) - 15) / 180.0 * DX_PI; // 若干のバラつき
                        pBlastShot->speed = 3.5 + (GetRand(15) / 10.0);                // 3.5 〜 5.0 の高速

                        // 目が壊れた演出として、目を「シアン色の中玉」に変化
                        if (pBlastShot->BULLET_ROLE == 1) {
                            pBlastShot->kind = img_enemyShotMediumBall[3];
                        }
                    }
                    pBlastShot = pBlastShot->next;
                }
            }
        }
    }
}

// ============================================================================
// 敵本体のパターン
// ============================================================================
void EnemyPat_Swimmy_Gemini()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // HP固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵本体は画面上部をゆっくり左右にホバリング移動
        enemy.x += 0.6 * (double)muki;
        if (count % 160 == 80) muki *= -1;
    }

    // 360フレーム（約6秒）ごとに1回、スイミー・フォーメーションを生成する
    if (count % 240 == 1) for (int i = 0; i <= 4; i++) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = SwimmyPatternFunc;
        pEnemyShotSet->x = 40.0 * i / 4.0 + 440.0 * (4 - i) / 4.0;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = DX_PI / 2.0; // 下向き基準
        pEnemyShotSet->kind = shot_count++;

        // 独自パラメータ初期化
        pEnemyShotSet->SET_PHASE = 0;       // 最初は集結フェーズ
        pEnemyShotSet->SET_PHASE_TIMER = 0;
        pEnemyShotSet->SET_CENTER_X = pEnemyShotSet->x;
        pEnemyShotSet->SET_CENTER_Y = pEnemyShotSet->y + 30.0; // 敵の少し下に魚の重心を置く

        // 弾リストのダミーヘッドを作成
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 警告音（スイミー登場の合図）
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 40発の小魚・目を生成し初期配置
        int numBullets = sizeof(FISH_OFFSETS) / sizeof(FISH_OFFSETS[0]);
        for (int i = 0; i < numBullets; i++) {
            sEnemyShot* pBullet = new sEnemyShot;

            // スポーン時は画面上部(敵付近)のランダム位置にバラバラに配置（フェーズ0で集結する）
            pBullet->BULLET_INIT_X = pEnemyShotSet->x + (double)(GetRand(240) - 120);
            pBullet->BULLET_INIT_Y = pEnemyShotSet->y + (double)(GetRand(40) - 20);

            // 相対座標を保持
            pBullet->param_d[0] = FISH_OFFSETS[i][0];
            pBullet->param_d[1] = FISH_OFFSETS[i][1];

            // 1番インデックスのみを「スイミー(目)」に指定、他は赤い小魚
            if (i == 1) {
                pBullet->BULLET_ROLE = 1;                     // 目
                pBullet->kind = img_enemyShotMediumBall[7];  // 中玉：黒
            }
            else {
                pBullet->BULLET_ROLE = 0;                     // 赤い小魚
                pBullet->kind = img_enemyShotSmallBall[0];   // 小玉：赤
            }

            pBullet->x = pBullet->BULLET_INIT_X;
            pBullet->y = pBullet->BULLET_INIT_Y;

            pBullet->margin = 480;

            // 弾セット内のリストに繋ぐ
            pBullet->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pBullet->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pBullet;
            pEnemyShotSet->pEnemyShotHead->prev = pBullet;
        }

        // グローバル弾セットリスト（enemyShotSetHead）へ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}