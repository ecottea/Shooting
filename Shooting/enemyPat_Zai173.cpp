// ============================================================
//  五線譜弾幕「レクイエム・スコア」  EnemyPat_StaffNotation_Zai.cpp
// ------------------------------------------------------------
//  <アイデア>
//   画面に「五線」(白い点線5本)と「演奏ライン」(マゼンタの点線)を敷く。
//   音符型の弾の塊(符頭+符幹+旗+加線)が五線の上を流れてきて、
//   符頭が演奏ラインに達した瞬間に「演奏」= 円環弾+自機狙いへ展開する。
//   音程は「音符のx位置」「円環の速さ」に変換され(高い音ほど速い)、
//   16拍ループのメロディ配列がそのまま弾幕のスケジュールになる。
//   HPが減るとテンポアップ(♩=90 → 120 → 150)し、小節後半8拍が
//   8分→16分の走句に変化。第3フェーズは16拍ごとにフィナーレとして
//   3和音の同時演奏(大型円環×3)→フェルマータ(休止)→次の小節へ。
//
//  <素材の選定(enemyPat_sampleForAI.cpp の一覧より抜粋)>
//   五線/符幹/旗/加線 : 小玉 白(6)   … 細かい玉を並べると「線」に見える
//   拍の明滅          : 小玉 橙(8)   … 拍ごとに五線が一瞬光る(テンポの見える化)
//   演奏ライン        : 小玉 マゼンタ(5)
//   符頭              : 大玉 黄(1)   … 線間40pxに収まる音符サイズ/走句は中玉
//   円環(和音)        : 中玉 赤(0)
//   自機狙い2WAY      : 銃弾 マゼンタ(5)
//   音                : light=拍 / medium=発射(〜第2フェーズ) / heavy=発射(第3フェーズ)
//                       extreme=フィナーレ発射 / charge=フィナーレ予告
//   鱗弾・菱形弾・中楕円弾・短レーザーは不使用(必要なものを抜粋)
//
//  <仕様上の注意>
//   ・count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと
//     画面外の弾の消去はメインルーチンが行うため、ここでは一切行わない。
//     弾は削除せず役割を「付け替え」て攻撃弾へ変化させ、画面外に出た弾は
//     メインルーチンの自動消去(margin)に回収を任せる。
//   ・GetRand(x) は 0〜x の x+1 種類を返す。使用は1か所のみで
//     (GetRand(12) - 6) とすることで -6〜+6 の揺らぎにしている。
//   ・弾セットは1つだけ生成し、全ての弾をそこにぶら下げて管理する。
//     同時最大弾数は約600のため sEnemyShot のプール(4096)に余裕がある。
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---- 五線譜のパラメータ(調整はここだけでOK) ----
static const double STAFF_TOP_Y = 96.0;   // 一番上の線のy
static const double STAFF_GAP_Y = 40.0;   // 線と線の間隔
static const int    STAFF_SLOT_X = 20;     // 線上の小玉の間隔(隙間は自機が通れる幅)
static const int    STAFF_SLOT_N = 24;     // 横方向の弾数(480 / 20)
static const double NOTE_SPAWN_Y = 56.0;   // 音符の出現位置(五線の上)
static const double NOTE_SPEED = 1.2;    // 譜面が流れる速さ
static const double PLAY_Y = 320.0;  // 演奏ライン(ここで音符が「鳴る」)

// メロディ(音程)。0=低音…10=高音(上加線)。
// 音程が大きいほど音符は右へ置かれ、円環弾が速くなる。後半8拍は上昇の走句。
static const int MELODY[16] = { 8,7,5,4, 5,7,8,10, 2,4,5,7, 8,9,10,7 };

// 弾の役割(param_i[0]): 0=五線 1=符頭 2=符幹 3=旗 4=加線 5=演奏ライン 6=発射後の弾

// ---- 効果音(重なり防止は sample と同じ作法) ----
static void PlaySe(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// ---- 弾をセットの末尾に追加 ----
static sEnemyShot* AddShot(sEnemyShotSet* pEnemyShotSet, double x, double y,
    double muki, double speed, int kind, int role)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->param_i[0] = role;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// ---- 音符1個(符頭+符幹+旗+加線)を譜面に乗せる ----
// compact=true は16分走句用の小型音符(中玉の符頭+短い符幹)
// 符頭には発射情報(円環の弾数・速さ・開始角度)を仕込んでおく
static void SpawnNote(sEnemyShotSet* pEnemyShotSet, double x, int pitch,
    int ringN, double ringSpeed, double ringAngle,
    bool withFlag, bool compact, bool aimed)
{
    double hx = x;
    double hy = NOTE_SPAWN_Y;
    double stemDx = compact ? 8.0 : 20.0;   // 符幹を付けるx(符頭の右端)
    int    stems = compact ? 2 : 4;

    // 符頭
    sEnemyShot* pHead = AddShot(pEnemyShotSet, hx, hy, DX_PI * 0.5, NOTE_SPEED,
        compact ? img_enemyShotMediumBall[1] : img_enemyShotLargeBall[1], 1);
    pHead->param_i[1] = 0;                // 0=未発射
    pHead->param_i[2] = ringN;            // 演奏時の円環弾数
    pHead->param_i[3] = aimed ? 1 : 0;    // 発射後に自機を狙うか
    pHead->param_i[4] = compact ? 0 : 1;  // 1=フルサイズの音符(発射音あり)
    pHead->param_d[0] = ringSpeed;        // 円環の速さ
    pHead->param_d[1] = ringAngle;        // 円環の開始角度(音程ごとに回す)

    // 符幹(符頭の右端から真上)/発射時は下方向へ扇状に散る
    for (int k = 0; k < stems; k++) {
        sEnemyShot* p = AddShot(pEnemyShotSet, hx + stemDx, hy - 14.0 - 12.0 * k,
            DX_PI * 0.5, NOTE_SPEED, img_enemyShotSmallBall[6], 2);
        p->param_d[0] = DX_PI * 0.5 + (k - (stems - 1) * 0.5) * 0.28;
        p->param_d[1] = compact ? 1.6 : 2.0;
    }

    // 旗(八分音符のばさき)
    if (withFlag && !compact) {
        double tx = hx + stemDx;
        double ty = hy - 14.0 - 12.0 * (stems - 1);
        const double fx[3] = { 7.0, 13.0, 17.0 };
        const double fy[3] = { 4.0, 10.0, 18.0 };
        for (int k = 0; k < 3; k++) {
            sEnemyShot* p = AddShot(pEnemyShotSet, tx + fx[k], ty + fy[k],
                DX_PI * 0.5, NOTE_SPEED, img_enemyShotSmallBall[6], 3);
            p->param_d[0] = DX_PI * 0.5 - 0.45 - 0.22 * k;   // 右下方向へ散る
            p->param_d[1] = 2.2;
        }
    }

    // 加線(音程が五線からはみ出しているとき p>=10)
    if (pitch >= 10) {
        double half = compact ? 16.0 : 30.0;
        for (int s = -1; s <= 1; s += 2) {
            sEnemyShot* p = AddShot(pEnemyShotSet, hx + half * s, hy,
                DX_PI * 0.5, NOTE_SPEED, img_enemyShotSmallBall[6], 4);
            p->param_d[0] = DX_PI * 0.5 - 1.1 * s;   // 左は左下、右は右下へ
            p->param_d[1] = 1.8;
        }
    }
}

// ---- 譜面全体(五線の維持+スケジューラ+演奏)を制御 ----
static void ShotScore(sEnemyShotSet* pEnemyShotSet)
{
    // HPでフェーズ判定(テンポと弾数が変わる)
    int phase = (enemy.hp > 140) ? 0 : ((enemy.hp > 70) ? 1 : 2);

    // 五線と演奏ラインは最初の1回だけ生成(速度0の固定弾/消去はメインに任せる)
    if (pEnemyShotSet->param_i[4] == 0) {
        pEnemyShotSet->param_i[4] = 1;
        for (int line = 0; line < 5; line++) {
            for (int i = 0; i < STAFF_SLOT_N+1; i++) {
                AddShot(pEnemyShotSet, 10.0-10 + STAFF_SLOT_X * i, STAFF_TOP_Y + STAFF_GAP_Y * line,
                    0.0, 0.0, img_enemyShotSmallBall[6], 0);
            }
        }
        for (int i = 0; i < 12*2+1; i++) {
            AddShot(pEnemyShotSet, 20.0-20 + 40.0 * i/2, PLAY_Y, 0.0, 0.0, img_enemyShotSmallBall[5], 5);
        }
    }

    // 拍の明滅の残り時間を減らす
    if (pEnemyShotSet->param_i[5] > 0) pEnemyShotSet->param_i[5]--;

    // ---- 指揮(スケジューラ):拍が来たら音符を譜面に乗せる ----
    if (pEnemyShotSet->param_i[2] > 0) {
        // フィナーレ後のフェルマータ(休止)
        if (count >= pEnemyShotSet->param_i[2]) {
            pEnemyShotSet->param_i[2] = 0;
            pEnemyShotSet->param_i[1] = count;
        }
    }
    else if (count >= pEnemyShotSet->param_i[1]) {
        int slot = pEnemyShotSet->param_i[0];
        if (phase == 2 && slot == 0 && pEnemyShotSet->param_i[3] == 1) {
            // フィナーレ:3和音(低・中・高)を同時に演奏してしばし休止
            PlaySe(sound_enemyCharge);
            for (int c = 0; c < 3; c++) {
                int chordPitch = 2 + c * 3;                     // 2,5,8
                SpawnNote(pEnemyShotSet, 90.0 + chordPitch * 30.0, chordPitch,
                    20, 2.6, chordPitch * 0.35, false, false, true);
            }
            pEnemyShotSet->param_i[5] = 10;           // 五線を強めに光らせる
            pEnemyShotSet->param_i[3] = 0;            // 予約を消化
            pEnemyShotSet->param_i[2] = count + 460/2;  // 休止後に次の小節へ
        }
        else {
            int  pitch = MELODY[slot];
            bool run = (slot >= 8);         // 小節後半8拍は8分/16分の走句
            int  interval;
            int  ringN;
            bool flag = false, compact = false, aimed = false;
            if (phase == 0) {                 // ♩=90 4分音符のみ
                interval = 40;
                ringN = 8;
            }
            else if (phase == 1) {            // ♩=120 8分音符混在
                interval = run ? 15 : 30;
                ringN = run ? 8 : 12;
                flag = run;
                aimed = !run;
            }
            else {                            // ♩=150 16分音符の走句
                interval = run ? 6 : 24;
                ringN = run ? 8 : 16;
                compact = run;
                aimed = !run;
            }
            double ringSpeed = 1.7 + 0.25 * phase + 0.08 * pitch;  // 高い音ほど速い
            // GetRand(12)は0〜12を返すので、(GetRand(12)-6)で-6〜+6の揺らぎ(±6%)
            ringSpeed *= 1.0 + (GetRand(12) - 6) / 100.0;
            SpawnNote(pEnemyShotSet, 90.0 + pitch * 30.0, pitch,
                ringN, ringSpeed, pitch * 0.35, flag, compact, aimed);
            if (!compact) {
                PlaySe(sound_enemyShot_light);    // 拍の音
                pEnemyShotSet->param_i[5] = 8;    // 五線を一瞬光らせる
            }
            if (phase == 2) pEnemyShotSet->param_i[3] = 1;  // 小節の終わりでフィナーレ予約
            pEnemyShotSet->param_i[0] = (slot + 1) % 16;
            pEnemyShotSet->param_i[1] = count + interval;
        }
    }

    // ---- 弾の移動と「演奏」(発射) ----
    int staffKind = (pEnemyShotSet->param_i[5] > 0) ? img_enemyShotSmallBall[8]   // 拍:橙
        : img_enemyShotSmallBall[6];  // 通常:白
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        switch (pShot->param_i[0]) {
        case 0: // 五線:拍に合わせて明滅させるだけ(固定弾)
            pShot->kind = staffKind;
            break;
        case 5: // 演奏ライン:常時マゼンタ
            break;
        case 1: // 符頭:演奏ラインに達したら演奏=発射
            if (pShot->param_i[1] == 0 && pShot->y >= PLAY_Y) {
                pShot->param_i[1] = 1;
                double aim = atan2(player.y - pShot->y, player.x - pShot->x);
                double cx = pShot->x;
                double cy = pShot->y;
                int    n = pShot->param_i[2];
                double rs = pShot->param_d[0];
                double base = pShot->param_d[1];
                if (pShot->param_i[3] == 1) {     // フル音符の符頭は自機を狙う
                    pShot->muki = aim;
                    pShot->speed = 2.8;
                }
                else {                            // 走句の符頭はそのまま落ちる
                    pShot->muki = DX_PI * 0.5;
                    pShot->speed = 1.8;
                }
                pShot->kind = (pShot->param_i[4] == 1) ? img_enemyShotLargeBall[0]
                    : img_enemyShotMediumBall[0];
                // 円環(和音)
                for (int k = 0; k < n; k++) {
                    AddShot(pEnemyShotSet, cx, cy, base + DX_PI * 2.0 * k / n, rs,
                        img_enemyShotMediumBall[0], 6);
                }
                // 自機狙い2WAY
                if (pShot->param_i[3] == 1) {
                    AddShot(pEnemyShotSet, cx, cy, aim - 0.12, 3.6, img_enemyShotBullet[5], 6);
                    AddShot(pEnemyShotSet, cx, cy, aim + 0.12, 3.6, img_enemyShotBullet[5], 6);
                }
                // 演奏音(フル音符のみ)
                if (pShot->param_i[4] == 1) {
                    if (n >= 20)      PlaySe(sound_enemyShot_extreme);
                    else if (n >= 16) PlaySe(sound_enemyShot_heavy);
                    else              PlaySe(sound_enemyShot_medium);
                }
            }
            break;
        case 2: // 符幹
        case 3: // 旗
        case 4: // 加線:符頭に少し遅れて追い打ちのように散る
            if (pShot->param_i[1] == 0 && pShot->y >= PLAY_Y) {
                pShot->param_i[1] = 1;
                pShot->muki = pShot->param_d[0];
                pShot->speed = pShot->param_d[1];
            }
            break;
        default: // 6=発射後の弾(円環・銃弾)は直進のみ
            break;
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン:五線譜弾幕「レクイエム・スコア」
// ============================================================
void EnemyPat_StaffNotation_Zai()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと

        // 譜面(弾幕)全体を管理するセットはこの1つだけ生成する
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotScore;
        pEnemyShotSet->x = 0.0;
        pEnemyShotSet->y = 0.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->param_i[0] = 0;   // メロディのインデックス(次に鳴らす拍)
        pEnemyShotSet->param_i[1] = 90;  // 次に音符を置くフレーム(前奏として少し待つ)
        pEnemyShotSet->param_i[2] = 0;   // フィナーレ後の休止(再開フレーム)
        pEnemyShotSet->param_i[3] = 0;   // フィナーレの予約フラグ
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        // 指揮者のようにゆったり両手を振るう
        enemy.x = 240.0 + sin(count * 0.011) * 130.0;
        enemy.y = 40.0 + sin(count * 0.045) * 5.0;
    }
}