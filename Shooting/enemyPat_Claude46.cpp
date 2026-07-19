// enemyPat_daisajin.cpp
//
// 「大砂嵐 ―地平を喰らう者―」
//
// 実際のハブーブ（砂嵐）が持つ
//   1. 壁のように迫る砂の前線
//   2. 視界を奪う渦状の砂塵（サンドデビル）
//   3. 突風（ガスト）
//   4. 嵐の目
// という段階構造を、そのまま弾幕のフェーズ展開に落とし込んだパターン。
//
// フェーズはcount（このパターン開始からのフレーム数）で管理する。
//   count <= 600            : 砂の壁（常時稼働、以降も続く）
//   count <= 1200            : 上記 + 砂塵の渦
//   count <= 1800            : 上記 + 突風
//   count >  1800（嵐の目）  : 上記すべてが同時展開。中央付近に
//                              移動する安全地帯（嵐の目）が出現し、
//                              砂の壁は嵐の目の位置に合わせた通路を、
//                              突風は嵐の目を避けた場所から発生する。
//
// [素材選定メモ]
//   ・砂粒表現の基本 … img_enemyShotSmallBall（2.5x2.5, 橙8）
//     細かい砂粒の密集感を出すため、最も小さい弾を採用。
//   ・渦の腕（粗い砂塊） … img_enemyShotScale（4.0x3.0, 黄1）
//     鱗弾は非対称形状で、回転する砂塵の荒々しさに合う。
//   ・突風の高速弾 … img_enemyShotBullet（5.0x2.0, 赤0）
//     細長い針状の見た目に適するのはimg_enemyShotBullet
//     （img_enemyShotLaserは当たり判定が64x4と大きすぎるため不採用）。
//   ・突風の予告マーカー … img_enemyShotSmallBall（橙8）
//     本体と統一感を持たせつつ、静止していることで「これから来る」
//     予兆であることを示す。
//
// [実装上の注意]
//   sEnemyShotSetは、保持する弾（sEnemyShot）のリストが空になると
//   エンジン側の判定で自動的に解放されてしまう。そのため、
//   ・砂塵の渦の中心位置や自転位相
//   ・突風の発生間隔タイマー
//   ・嵐の目の座標
//   といった「shotSetの生死をまたいで持続すべき状態」は、
//   すべてファイルスコープのstatic変数、またはEnemyPat_SandStorm_Claude()自身の
//   static局所変数として保持し、個々のshotSetには依存しない。
//   （突風の予告時間中は弾リストが空になってshotSetごと消えて
//   しまわないよう、速度0のマーカー弾を1つ置いてリストを維持する）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//====================================================================
// 定数
//====================================================================
static const double SCREEN_W = 480.0;
static const double SCREEN_H = 480.0;

static const int PHASE1_END = 300;   // 砂の壁のみ
static const int PHASE2_END = 600;  // + 砂塵の渦
static const int PHASE3_END = 900;  // + 突風
// PHASE3_END 以降は「嵐の目」フェーズ（全要素同時展開）

static const int SAND_DEVIL_NUM = 4;          // 砂塵の渦の数
static const int GUST_TELEGRAPH_FRAMES = 30;  // 突風の予告フレーム数

//====================================================================
// 永続状態（shotSetの生死に依存しない）
//====================================================================
static double sandDevilX[SAND_DEVIL_NUM];
static double sandDevilY[SAND_DEVIL_NUM];
static double sandDevilAngle[SAND_DEVIL_NUM]; // 自転位相
static double sandDevilVy[SAND_DEVIL_NUM];    // ゆっくり下方向へドリフト

static double stormEyeX, stormEyeY; // 嵐の目の中心（嵐の目フェーズでのみ移動）

//====================================================================
// 弾幕：砂の壁（横一列、隙間あり、ゆっくり降下）
//====================================================================
static void ShotSandWall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 隙間（風で削れた砂丘の谷）を2箇所設定
        int gapCenter[2];
        if (pEnemyShotSet->param_i[0] == 1) {
            // 嵐の目フェーズ：嵐の目の位置に合わせた強制的な安全通路
            gapCenter[0] = (int)pEnemyShotSet->param_d[0];
        }
        else {
            gapCenter[0] = 60 + GetRand((int)(SCREEN_W - 120.0));
        }
        gapCenter[1] = 60 + GetRand((int)(SCREEN_W - 120.0));
        int gapWidth = 48;

        for (int x = 8; x <= (int)SCREEN_W - 8; x += 16) {
            bool inGap = false;
            for (int g = 0; g < 2; g++) {
                int diff = x - gapCenter[g];
                if (diff < 0) diff = -diff;
                if (diff < gapWidth / 2) { inGap = true; break; }
            }
            if (inGap) continue;

            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = (double)x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = DX_PI / 2.0; // 真下
            pEnemyShot->speed = 0.9 + GetRand(20) / 100.0; // 0.90〜1.09
            pEnemyShot->kind = img_enemyShotSmallBall[8]; // 砂色（橙）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

//====================================================================
// 弾幕：砂塵の渦（サンドデビル）から放たれる対2方向の螺旋弾
// 呼び出し側（EnemyPat_SandStorm_Claude）が中心位置・自転位相を更新し続け、
// このパターンは「その瞬間の位置から2way弾を撃つだけ」の
// 使い捨てshotSetとして毎回生成される。
//====================================================================
static void ShotSandDevilEmit(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        double baseAngle = pEnemyShotSet->muki;
        for (int i = 0; i < 2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = baseAngle + DX_PI * i; // 対角2方向
            pEnemyShot->speed = 1.6;
            pEnemyShot->kind = img_enemyShotScale[1]; // 黄色い鱗弾（粗い砂塊）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

//====================================================================
// 弾幕：突風（ガスト）
// 予告（マーカー弾を静止表示＋予告音）→ 放射状高速弾 → 急減衰
//====================================================================
static void ShotGust(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK); // 予告音

        // 予告用マーカー弾（速度0）。
        // shotSetの弾リストを空のままにするとエンジンにより
        // shotSet自体が自動削除されてしまうため、予告時間中は
        // このマーカーでリストを維持しておく。
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0;
        pEnemyShot->param_i[0] = 1; // マーカー識別フラグ
        pEnemyShot->kind = img_enemyShotSmallBall[8]; // 砂色（橙）

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    if (pEnemyShotSet->count == GUST_TELEGRAPH_FRAMES) {
        // マーカー弾を取り除いてから放射状バーストへ切り替える
        pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
            sEnemyShot* nextShot = pEnemyShot->next;
            if (pEnemyShot->param_i[0] == 1) {
                pEnemyShot->prev->next = pEnemyShot->next;
                pEnemyShot->next->prev = pEnemyShot->prev;
                delete pEnemyShot;
            }
            pEnemyShot = nextShot;
        }

        int wayNum = 16;
        for (int i = 0; i < wayNum; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = DX_PI * 2.0 * i / wayNum;
            pEnemyShot->speed = 4.5; // 突風の初速は速い
            pEnemyShot->kind = img_enemyShotBullet[0]; // 赤い針状弾（危険色）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        if (pEnemyShot->param_i[0] != 1) { // マーカー弾は移動させない
            if (pEnemyShot->count == 20) pEnemyShot->speed = 1.0; // 急減衰
            pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
            pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        }
        pEnemyShot = pEnemyShot->next;
    }
}

//====================================================================
// 敵本体のパターン
//====================================================================
void EnemyPat_SandStorm_Claude()
{
    static int muki;
    static int nextGustTime;
    static int devilEmitTimer[SAND_DEVIL_NUM];

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;

        nextGustTime = 90; // フェーズ3開始後、少し間を置いて初回突風

        for (int i = 0; i < SAND_DEVIL_NUM; i++) {
            sandDevilX[i] = 60.0 + i * (SCREEN_W - 120.0) / (SAND_DEVIL_NUM - 1);
            sandDevilY[i] = 80.0;
            sandDevilAngle[i] = DX_PI * 2.0 * i / SAND_DEVIL_NUM;
            sandDevilVy[i] = 0.15; // ゆっくり自機側（画面下方向）へドリフト
            devilEmitTimer[i] = i * 5;
        }

        stormEyeX = SCREEN_W / 2.0;
        stormEyeY = SCREEN_H / 2.0;
    }
    else {
        enemy.x += 0.5 * (double)muki;
        if (count % 180 == 90) muki *= -1;
    }

    bool devilActive = (count > PHASE1_END);
    bool gustActive = (count > PHASE2_END);
    bool stormEyePhase = (count > PHASE3_END);

    //----------------------------------------------------------
    // 嵐の目：中心座標をリサージュ曲線で移動（フェーズ4のみ）
    //----------------------------------------------------------
    if (stormEyePhase) {
        double t = (double)(count - PHASE3_END) * 0.01;
        stormEyeX = SCREEN_W / 2.0 + 120.0 * sin(t * 1.3);
        stormEyeY = SCREEN_H / 2.0 + 100.0 * sin(t * 2.0);
    }

    //----------------------------------------------------------
    // フェーズ1〜：砂の壁（常時稼働。フェーズが進むほど間隔を調整）
    //----------------------------------------------------------
    int wallInterval = 90;
    if (devilActive)   wallInterval = 100;
    if (gustActive)     wallInterval = 110;
    if (stormEyePhase) wallInterval = 80; // 嵐の目フェーズは再び密に

    if (count % wallInterval == 0) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSandWall;
        pEnemyShotSet->x = 0.0;
        pEnemyShotSet->y = -10.0; // 画面上端から出現

        if (stormEyePhase) {
            pEnemyShotSet->param_i[0] = 1;
            pEnemyShotSet->param_d[0] = stormEyeX; // 嵐の目に合わせた通路
        }

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    //----------------------------------------------------------
    // フェーズ2〜：砂塵の渦（サンドデビル）
    //----------------------------------------------------------
    if (devilActive) {
        for (int i = 0; i < SAND_DEVIL_NUM; i++) {
            // 中心位置・自転位相はshotSetの生死と無関係にここで更新する
            sandDevilY[i] += sandDevilVy[i];
            if (sandDevilY[i] > 340.0) sandDevilY[i] = 340.0; // 下がりすぎ防止
            sandDevilAngle[i] += 0.10;

            devilEmitTimer[i]--;
            if (devilEmitTimer[i] <= 0) {
                devilEmitTimer[i] = stormEyePhase ? 5 : 8;

                sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
                pEnemyShotSet->count = 0;
                pEnemyShotSet->patternFunc = ShotSandDevilEmit;
                pEnemyShotSet->x = sandDevilX[i];
                pEnemyShotSet->y = sandDevilY[i];
                pEnemyShotSet->muki = sandDevilAngle[i];

                pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

                pEnemyShotSet->prev = enemyShotSetHead.prev;
                pEnemyShotSet->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pEnemyShotSet;
                enemyShotSetHead.prev = pEnemyShotSet;
            }
        }
    }

    //----------------------------------------------------------
    // フェーズ3〜：突風（ガスト）
    //----------------------------------------------------------
    if (gustActive) {
        nextGustTime--;
        if (nextGustTime <= 0) {
            nextGustTime = 150 + GetRand(90); // 約2.5〜4秒間隔（60fps想定）

            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotGust;

            if (GetRand(1) == 0) {
                pEnemyShotSet->x = 40.0 + GetRand((int)(SCREEN_W - 80.0));
                pEnemyShotSet->y = 40.0 + GetRand((int)(SCREEN_H - 80.0));
            }
            else {
                int d = GetRand(SAND_DEVIL_NUM - 1);
                pEnemyShotSet->x = sandDevilX[d];
                pEnemyShotSet->y = sandDevilY[d];
            }

            if (stormEyePhase) {
                // 嵐の目の中には発生しないよう、範囲外へ押し出す
                double dx = pEnemyShotSet->x - stormEyeX;
                double dy = pEnemyShotSet->y - stormEyeY;
                double dist = sqrt(dx * dx + dy * dy);
                double safeDist = 90.0;
                if (dist < 0.001) {
                    pEnemyShotSet->x = stormEyeX + safeDist;
                    pEnemyShotSet->y = stormEyeY;
                }
                else if (dist < safeDist) {
                    pEnemyShotSet->x = stormEyeX + dx / dist * safeDist;
                    pEnemyShotSet->y = stormEyeY + dy / dist * safeDist;
                }
                if (pEnemyShotSet->x < 20.0) pEnemyShotSet->x = 20.0;
                if (pEnemyShotSet->x > SCREEN_W - 20.0) pEnemyShotSet->x = SCREEN_W - 20.0;
                if (pEnemyShotSet->y < 20.0) pEnemyShotSet->y = 20.0;
                if (pEnemyShotSet->y > SCREEN_H - 20.0) pEnemyShotSet->y = SCREEN_H - 20.0;
            }

            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }
    }
}