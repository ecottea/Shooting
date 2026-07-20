// enemyPat_Swimmy.cpp
//
// 「群魚結像 -スイミー-」
// レオ・レオニ『スイミー』をモチーフにした弾幕パターン。
// 小魚(赤)がバラバラに泳ぎ、やがて整列して1匹の大魚を形作り、
// 大魚として前進・威嚇したのち、驚いたように散り散りになって本弾幕となる。
//
// ---- 使用素材の選定理由 ----
// ・img_enemyShotBullet（針状/銃弾型）: 小魚1匹1匹を細長い粒として表現するのに適しており、
//   既存コードの慣習（針状弾はimg_enemyShotBulletを優先）とも合致する。
// ・色は 0:赤 を群れの小魚に、7:黒 をスイミー本人（目の位置）に使用。
//   原作どおり「赤い小魚の群れに1匹だけ黒い魚（スイミー）」という構図をそのまま再現できる。
// ・img_enemyShotLaser は判定が大きすぎるため今回も不使用。
//
// ---- 設計方針 ----
// ・弾の位置は速度を積分するのではなく、pShot->count（生成からの経過フレーム）と
//   スポーン時に固定したparam_d[]の値から、毎フレーム式で直接計算する（formula駆動）。
// ・GetRand(x) は 0～x の x+1 通りを返す仕様に注意して範囲計算を行っている。
// ・count, pEnemyShotSet->count, pEnemyShot->count のインクリメントおよび
//   画面外弾の自動消去はメインルーチン側で行われる前提のため、本ファイルでは行わない。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  定数
// ============================================================
static const int    FISH_COUNT = 400;   // スイミー(目)を除いた小魚の数
static const int    PH1_END = 100;   // Phase1: 散開遊泳の終了フレーム
static const int    PH2_END = PH1_END + 180; // Phase2: 整列(大魚形成)の終了フレーム
static const int    PH3_END = PH2_END + 180; // Phase3: 前進・威嚇の終了フレーム（以降Phase4）

static const double FORM_CENTER_X = 240.0; // 大魚の隊列中心（画面は480x480想定）
static const double FORM_CENTER_Y = 160.0;
static const double ADVANCE_SPEED = 0.6;   // Phase3中、大魚が下方向（プレイヤー側）へ進む速さ

// 魚シルエットの拡大率。画面は480x480のため、上げすぎるとPhase2整列時に
// 輪郭が画面外にはみ出す点に注意（目安として2.0前後が上限）。
static const double SHAPE_SCALE = 1.8;

// ============================================================
//  魚のシルエット（中心基準のローカル座標）を返す
//  index: 0 ~ count-1 の小魚インデックス
//  胴体(紡錘形)を75%、尾びれ(三角形)を25%の点数で構成する
// ============================================================
static void GetFishLocalPos(int index, int count, double& lx, double& ly)
{
    int bodyNum = (int)(count * 0.85);

    if (index < bodyNum) {
        // 胴体：横長の紡錘形（楕円）の輪郭上に等間隔角度で分布させる。
        // 偶数番目/奇数番目で半径を変えることで、輪郭だけでなく面としての厚みを出す。
        double angle = (double)index / bodyNum * DX_PI * 2.0;
        double rx = 70.0 * SHAPE_SCALE;
        double ry = 22.0 * SHAPE_SCALE;
        double radiusScale = 1.0 - (index % 3) * 0.3;
        lx = cos(angle) * rx * radiusScale;
        ly = sin(angle) * ry * radiusScale;
    }
    else {
        // 尾びれ：本体後方(-x方向)に伸びる三角形の外周に分布させる。
        int tailIndex = index - bodyNum;
        int tailNum = count - bodyNum;
        double tt = (tailNum <= 1) ? 0.0 : (double)tailIndex / (tailNum - 1);

        double baseX = -70.0 * SHAPE_SCALE, tipX = -110.0 * SHAPE_SCALE;
        double topY = -25.0 * SHAPE_SCALE, botY = 25.0 * SHAPE_SCALE;

        if (tt < 0.5) {
            // 付け根の上側 → 尾の先端
            double s = tt / 0.5;
            lx = baseX + (tipX - baseX) * s;
            ly = topY + (0.0 - topY) * s;
        }
        else {
            // 尾の先端 → 付け根の下側
            double s = (tt - 0.5) / 0.5;
            lx = tipX + (baseX - tipX) * s;
            ly = 0.0 + (botY - 0.0) * s;
        }
    }
}

// ============================================================
//  弾幕本体：群魚結像
// ============================================================
static void ShotSwimmySchool(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium,
        //                    sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        int totalCount = FISH_COUNT + 1; // +1 はスイミー本人（目の位置の弾）

        for (int i = 0; i < totalCount; i++) {
            pEnemyShot = new sEnemyShot;

            bool isEye = (i == totalCount - 1);

            // ---- Phase1: 散開開始位置（生成元付近にランダム配置） ----
            // GetRand(x) は 0～x の x+1 通りを返すことに注意して範囲を組み立てる。
            double startX = pEnemyShotSet->x + GetRand(520) - 260;
            double startY = pEnemyShotSet->y + GetRand(80) - 40;

            // ---- Phase2: 目標位置（魚シルエット上のローカル座標） ----
            double lx, ly;
            if (isEye) {
                // スイミー(目)は頭部やや前方・非対称の位置に配置する
                lx = 55.0 * SHAPE_SCALE;
                ly = -8.0 * SHAPE_SCALE;
            }
            else {
                GetFishLocalPos(i, FISH_COUNT, lx, ly);
            }

            pEnemyShot->x = startX;
            pEnemyShot->y = startY;
            pEnemyShot->muki = 0.0; // 本パターンは formula 駆動のため未使用
            pEnemyShot->speed = 0.0; // 同上（速度を保持せず毎フレーム位置を式で算出する）

            pEnemyShot->param_d[0] = startX;                       // 散開開始X
            pEnemyShot->param_d[1] = startY;                       // 散開開始Y
            pEnemyShot->param_d[2] = lx;                           // シルエット ローカルX
            pEnemyShot->param_d[3] = ly;                           // シルエット ローカルY
            pEnemyShot->param_d[4] = (GetRand(200) - 100) / 100.0; // 遊泳ゆらぎの振幅比 (-1.0~1.0)
            pEnemyShot->param_d[5] = GetRand(628) / 100.0;         // 遊泳ゆらぎの位相 (0~2π程度)
            pEnemyShot->param_d[6] = 1.0 + GetRand(60) / 100.0;    // 個体差(降下速度・離脱速度の比率 1.0~1.6)
            pEnemyShot->param_i[0] = isEye ? 1 : 0;                // スイミー(目)フラグ

            // 針状の弾(img_enemyShotBullet)で小魚を表現。0:赤=群れ、7:黒=スイミー本人。
            pEnemyShot->kind = isEye ? img_enemyShotBullet[7] : img_enemyShotBullet[0];

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // ---- 隊列（大魚）の中心座標を毎フレーム計算 ----
    int t = pEnemyShotSet->count;
    int tClampedForAdvance = (t < PH3_END) ? t : PH3_END;

    double centerX = FORM_CENTER_X;
    double centerY = FORM_CENTER_Y;
    if (t >= PH2_END) {
        double advanceT = (double)(tClampedForAdvance - PH2_END);
        centerY += ADVANCE_SPEED * advanceT; // Phase3でプレイヤー側へゆっくり前進
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int st = pShot->count; // 個々の弾の経過フレーム（同時生成のためtと一致する）

        double startX = pShot->param_d[0];
        double startY = pShot->param_d[1];
        double lx = pShot->param_d[2];
        double ly = pShot->param_d[3];
        double wiggleAmp = pShot->param_d[4];
        double wiggleFreq = pShot->param_d[5];
        double individualRate = pShot->param_d[6];

        if (st < PH1_END) {
            // ---- Phase1: 散開遊泳 ----
            // 各弾が個別の正弦波でゆらぎながらゆっくり降下する。密度が低く回避しやすい。
            pShot->x = startX + sin(st * 0.05 + wiggleFreq) * 20.0 * wiggleAmp;
            pShot->y = startY + individualRate * st * 0.5;
        }
        else if (st < PH2_END) {
            // ---- Phase2: 整列・大魚形成 ----
            // 散開位置(Phase1終了時点の位置)から、魚シルエット上の目標座標へeaseで遷移する。
            double p = (st - PH1_END) / (double)(PH2_END - PH1_END);
            double ease = p * p * (3.0 - 2.0 * p); // smoothstep

            double fromX = startX + sin(PH1_END * 0.05 + wiggleFreq) * 20.0 * wiggleAmp;
            double fromY = startY + individualRate * PH1_END * 0.5;

            double toX = centerX + lx;
            double toY = centerY + ly;

            pShot->x = fromX + (toX - fromX) * ease;
            pShot->y = fromY + (toY - fromY) * ease;
        }
        else if (st < PH3_END) {
            // ---- Phase3: 大魚遊泳・威嚇 ----
            // 隊列全体で前進しつつ、体の前後位置(lx)に応じて位相をずらした正弦波で
            // 「体をくねらせて泳ぐ」うねりを表現する。
            double localWave = sin((st - PH2_END) * 0.05 - lx * 0.03) * 6.0;
            pShot->x = centerX + lx;
            pShot->y = centerY + ly + localWave;
        }
        else {
            // ---- Phase4: 散開・本弾幕 ----
            // 拘束を解いた瞬間の位置を基準に、中心から離れた弾は外向きへ、
            // 中心付近(スイミー本人など)はプレイヤー方向へ、驚いたように加速しながら飛び去る。
            double releaseT = st - PH3_END;

            double localWaveAtRelease = sin((PH3_END - PH2_END) * 0.05 - lx * 0.03) * 6.0;
            double releaseX = centerX + lx;
            double releaseY = centerY + ly + localWaveAtRelease;

            double dirX, dirY;
            double dirLen = sqrt(lx * lx + ly * ly);
            if (dirLen > 1.0) {
                dirX = lx / dirLen;
                dirY = ly / dirLen;
            }
            else {
                double toPlayer = atan2(player.y - releaseY, player.x - releaseX);
                dirX = cos(toPlayer);
                dirY = sin(toPlayer);
            }

            double speed = 1.2 * individualRate;
            double accel = 0.02;
            double dist = speed * releaseT + 0.5 * accel * releaseT * releaseT;

            pShot->x = releaseX + dirX * dist;
            pShot->y = releaseY + dirY * dist;
        }

        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Swimmy_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = FORM_CENTER_X;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
    }

    // 群れ(小魚+スイミー)は最初に1回だけ一斉生成する
    if (count % (PH3_END + 30) == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSwimmySchool;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
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
}