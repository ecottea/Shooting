// enemyPat_staffNocturne.cpp
// 弾幕「終止線のノクターン」
//
// 五線譜が画面を右から左へ流れ、その上を音符が通り過ぎて「演奏」される弾幕。
//   ・五線   : 白の小玉の点線 5本(弦)。発音のたびに振動する
//   ・音符   : 符頭=大玉(黒) / 白抜き=中玉の輪(白)、符幹=銃弾、旗=鱗弾
//   ・発音   : 符頭が画面中央(kSoundX)を通過した瞬間、音階の色の小玉が波紋状に広がる
//   ・小節線 : 銃弾の縦列。画面を縦に切り、毎小節 隙間の高さが変わる
//   ・終止線 : 細線(銃弾) + 太線(大玉)。中央に達すると五線が波打って左右へ吐き出される
//
// 1周期は 10小節。0小節目=休符 / 1〜8小節目=メロディ / 9小節目=全音符の和音。
// 難易度や密度は下の定数で調整できる。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace {

    // ============================================================
    //  定数
    // ============================================================

    // 弾の色(img_enemyShotXxx[色] のインデックス)
    constexpr int kColRed = 0;
    constexpr int kColYellow = 1;
    constexpr int kColGreen = 2;
    constexpr int kColCyan = 3;
    constexpr int kColBlue = 4;
    constexpr int kColMagenta = 5;
    constexpr int kColWhite = 6;
    constexpr int kColBlack = 7;
    constexpr int kColOrange = 8;

    // 音階(ド,レ,ミ,ファ,ソ,ラ,シ)ごとの波紋の色
    constexpr int kDegreeColor[7] = { kColRed, kColOrange, kColYellow, kColGreen, kColCyan, kColBlue, kColMagenta };
    // 終止線の吐き出しで使う色(五線の行ごと)
    constexpr int kRowColor[5] = { kColRed, kColYellow, kColGreen, kColCyan, kColBlue };

    // 弾ごとの動き方(param_i[0])
    constexpr int kBehaviorLinear = 0;  // 等速直線        : param_d[0]=vx, param_d[1]=vy
    constexpr int kBehaviorString = 1;  // 五線の弦        : param_d[0]=vx, param_d[2]=基準y
    constexpr int kBehaviorWave = 2;  // 波打つ吐き出し  : param_d[0]=vx, param_d[2]=基準y, param_d[3]=位相, param_d[4]=発射x

    // 音符の種類(数値は余韻リングの数と一致)
    constexpr int kNoteEighth = 0;
    constexpr int kNoteQuarter = 1;
    constexpr int kNoteHalf = 2;
    constexpr int kNoteWhole = 3;

    // ---- スクロール・五線 ----
    constexpr double kScrollSpeed = 2.0/2;    // 五線・音符・小節線が左へ流れる速さ(px/frame)
    constexpr double kSpawnX = 494.0-2;  // 右端の外(margin 20 以内)で弾を出す位置
    constexpr double kSoundX = 240.0;  // 音符が発音する縦ライン
    constexpr double kStaffBottomY = 340.0;  // 最下線のy(以降、40px間隔で上へ5本)
    constexpr double kLineStep = 40.0;   // 線と線の間隔(大玉の直径に合わせてある)
    constexpr double kHalfStep = 20.0;   // 「線⇔間」1段分(音符の高さの単位)
    constexpr int    kStaffInterval = 10*3;     // 弦の弾を出す間隔(frame)。弾同士の間隔は 10*2=20px

    // ---- テンポ・周期 ----
    constexpr int kBeatFrames = 40*2;                     // 1拍
    constexpr int kMeasureFrames = kBeatFrames * 4;        // 1小節
    constexpr int kCycleFrames = kMeasureFrames * 10;    // 1周期
    constexpr int kSongBase = 160;                    // 1周目の小節頭(off=0)が発音ラインに来るframe
    constexpr int kBarLead = 20*2;                     // 小節線は小節頭の音符より20frame(40px)先行する
    constexpr int kBarTravel = (int)((kSpawnX - kSoundX) / kScrollSpeed);  // 右端→発音ラインまでの飛行frame

    // 小節頭からの位置(eighth=8分音符を1単位)をframeに直す
    constexpr int Tick(int measure, int eighth)
    {
        return measure * kMeasureFrames + eighth * (kBeatFrames / 2);
    }

    // ---- 音符の形 ----
    constexpr double kStemOffsetX = 19.0;   // 符頭の中心から符幹までの横距離
    constexpr double kStemStep = 9.0;    // 符幹の銃弾の間隔
    constexpr int    kStemBullets = 16;     // 符幹の銃弾数(約3.5線間)
    constexpr double kHollowRadius = 15.0;   // 白抜きの符頭の輪の半径
    constexpr int    kHollowBullets = 10;     // 白抜きの符頭の輪の弾数
    constexpr int    kMaxNoteParts = 40;
    constexpr int    kNoteBuildFrames = 20*2;   // 音符のパーツを出し終えるまでのframe(余裕を見て)
    // 旗(8分音符): 符幹の先端からのオフセットと向き
    constexpr double kFlagX[4] = { 6.0, 12.0, 15.0, 14.0 };
    constexpr double kFlagY[4] = { 6.0, 15.0, 26.0, 37.0 };
    constexpr double kFlagMuki[4] = { 0.8, 1.2, 1.6, 1.95 };

    // ---- 小節線・終止線 ----
    constexpr double kGapHalf = 44.0;   // 小節線の隙間の半分(目印の中玉の中心まで)
    constexpr double kBarGapY[9] = { 170.0, 310.0, 200.0, 360.0, 250.0, 140.0, 320.0, 220.0, 300.0 };  // 1〜9小節目の隙間の高さ
    constexpr double kFinalGapY = 400.0;  // 終止線の隙間(五線の下へ逃がす)
    constexpr double kThickOffsetX = 26.0;   // 終止線: 太線は細線の右側
    constexpr double kThickGapHalf = 60.0;
    constexpr int    kFinaleEmitFrames = 90;  // 五線の吐き出しを続けるframe
    constexpr int    kFinaleEmitStep = 3;   // 吐き出しの間隔(frame)

    // ---- 譜面 ----
    struct sNoteEvent {
        int off;   // 周期内での発音frame
        int pos;   // 五線上の位置(最下線=0、半段ごとに+1、最上線=8)
        int type;  // 音符の種類
    };

    constexpr sNoteEvent kScore[] = {
        // 1小節目
        { Tick(1, 0), 3, kNoteQuarter }, { Tick(1, 2), 5, kNoteQuarter }, { Tick(1, 4), 7, kNoteHalf },
        // 2小節目
        { Tick(2, 0), 6, kNoteEighth }, { Tick(2, 1), 5, kNoteEighth }, { Tick(2, 2), 4, kNoteQuarter },
        { Tick(2, 4), 2, kNoteQuarter }, { Tick(2, 6), 3, kNoteQuarter },
        // 3小節目
        { Tick(3, 0), 3, kNoteQuarter }, { Tick(3, 2), 5, kNoteEighth }, { Tick(3, 3), 7, kNoteEighth },
        { Tick(3, 4), 8, kNoteHalf },
        // 4小節目
        { Tick(4, 0), 7, kNoteQuarter }, { Tick(4, 2), 5, kNoteQuarter }, { Tick(4, 4), 4, kNoteQuarter },
        { Tick(4, 6), 2, kNoteQuarter },
        // 5小節目
        { Tick(5, 0), 2, kNoteEighth }, { Tick(5, 1), 3, kNoteEighth }, { Tick(5, 2), 4, kNoteEighth },
        { Tick(5, 3), 5, kNoteEighth }, { Tick(5, 4), 7, kNoteQuarter }, { Tick(5, 6), 3, kNoteQuarter },
        // 6小節目
        { Tick(6, 0), 6, kNoteHalf }, { Tick(6, 4), 5, kNoteEighth }, { Tick(6, 5), 4, kNoteEighth },
        { Tick(6, 6), 3, kNoteQuarter },
        // 7小節目
        { Tick(7, 0), 1, kNoteQuarter }, { Tick(7, 2), 3, kNoteQuarter }, { Tick(7, 4), 5, kNoteEighth },
        { Tick(7, 5), 7, kNoteEighth }, { Tick(7, 6), 8, kNoteQuarter },
        // 8小節目
        { Tick(8, 0), 7, kNoteQuarter }, { Tick(8, 2), 6, kNoteQuarter }, { Tick(8, 4), 4, kNoteHalf },
        // 9小節目: 和音(ソ・ド・ミ)
        { Tick(9, 0), 2, kNoteWhole }, { Tick(9, 0), 5, kNoteWhole }, { Tick(9, 0), 7, kNoteWhole },
    };
    constexpr int kNumNotes = (int)(sizeof(kScore) / sizeof(kScore[0]));

    // 音符のパーツ(符頭の中心からの相対位置)
    struct sNotePart {
        double dx, dy;
        double muki;   // 見た目の向き
        int    kind;   // 弾の種類と色
    };

    // 弦の振動の大きさ(発音で跳ね上がり、フレームごとに減衰する)
    double g_vibration = 0.0;

    // ============================================================
    //  共通処理
    // ============================================================

    void PlaySE(int handle)
    {
        if (CheckSoundMem(handle)) StopSoundMem(handle);
        PlaySoundMem(handle, DX_PLAYTYPE_BACK);
    }

    // 弾をShotSetへ追加する。mukiは「見た目の向き」で、移動はvx,vyで行う。
    sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double vx, double vy, int kind)
    {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = x;
        pShot->y = y;
        pShot->muki = muki;
        pShot->speed = sqrt(vx * vx + vy * vy);
        pShot->kind = kind;
        pShot->param_d[0] = vx;
        pShot->param_d[1] = vy;
        pShot->margin = 120;
        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
        return pShot;
    }

    sEnemyShotSet* SpawnShotSet(void (*func)(sEnemyShotSet*))
    {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = func;
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
        return pSet;
    }

    // ShotSet内の全弾を動かす(動き方は param_i[0] で切り替え)
    void MoveShots(sEnemyShotSet* pSet)
    {
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            switch (pShot->param_i[0]) {
            case kBehaviorString:
                // 弦: 左へ流れつつ、振動の大きさに応じて上下に波打つ
                pShot->x += pShot->param_d[0];
                pShot->y = pShot->param_d[2] + g_vibration * sin(pShot->count * 0.45 + pShot->x * 0.07);
                break;
            case kBehaviorWave: {
                // 吐き出し: 発射点から離れるほど振幅が大きくなる波
                pShot->x += pShot->param_d[0];
                const double dist = fabs(pShot->x - pShot->param_d[4]);
                const double amp = 34.0 * (dist < 160.0 ? dist / 160.0 : 1.0);
                pShot->y = pShot->param_d[2] + amp * sin(dist * 0.07 + pShot->param_d[3]);
                break;
            }
            default:
                pShot->x += pShot->param_d[0];
                pShot->y += pShot->param_d[1];
                break;
            }
            pShot = pShot->next;
        }
    }

    // 距離distanceをスクロール速度で進むのにかかるframe(切り上げ)
    int FramesToReach(double distance)
    {
        return (int)ceil(distance / kScrollSpeed - 1e-9);
    }

    // このframeが「周期内で発音時刻soundOffのイベントを、飛行時間flightFrames前に出す時」か
    bool IsSpawnFrame(int soundOff, int flightFrames)
    {
        const int rel = count + flightFrames - kSongBase - soundOff;
        return rel >= 0 && rel % kCycleFrames == 0;
    }

    // ============================================================
    //  五線
    // ============================================================

    // 5本の弦を1列ぶん出して左へ流す。これが連続して点線の五線になる。
    void ShotStaffColumn(sEnemyShotSet* pSet)
    {
        if (pSet->count == 0) {
            for (int i = 0; i < 5; i++) {
                const double y = kStaffBottomY - kLineStep * i;
                sEnemyShot* pShot = AddShot(pSet, kSpawnX, y, DX_PI, -kScrollSpeed, 0.0, img_enemyShotDiamond[kColWhite]);
                pShot->param_i[0] = kBehaviorString;
                pShot->param_d[2] = y;
            }
        }
        MoveShots(pSet);
    }

    // ============================================================
    //  音符
    // ============================================================

    void AddPart(sNotePart parts[], int& n, double dx, double dy, double muki, int kind)
    {
        parts[n].dx = dx;
        parts[n].dy = dy;
        parts[n].muki = muki;
        parts[n].kind = kind;
        n++;
    }

    // 音符1個ぶんのパーツ一覧を作る。戻り値はパーツ数。
    int BuildNoteParts(int pos, int type, sNotePart parts[])
    {
        int n = 0;
        const bool hollow = (type == kNoteHalf || type == kNoteWhole);  // 二分・全音符は白抜き
        const bool hasStem = (type != kNoteWhole);
        const bool stemDown = (pos >= 4);                                 // 中央線(pos=4)以上は符幹が下向き
        const int  col = hollow ? kColWhite : kColBlack;

        // 符頭
        if (hollow) {
            for (int i = 0; i < kHollowBullets; i++) {
                const double a = 2.0 * DX_PI * i / kHollowBullets;
                AddPart(parts, n, kHollowRadius * cos(a), kHollowRadius * sin(a), a, img_enemyShotMediumBall[kColWhite]);
            }
        }
        else {
            AddPart(parts, n, 0.0, 0.0, DX_PI, img_enemyShotLargeBall[kColBlack]);
        }

        if (hasStem) {
            // 符幹: 上向きは符頭の右側、下向きは左側。銃弾を縦向きに並べる
            const double sx = stemDown ? -kStemOffsetX : kStemOffsetX;
            const double dirY = stemDown ? 1.0 : -1.0;
            for (int k = 0; k < kStemBullets; k++) {
                AddPart(parts, n, sx, dirY * kStemStep * k, DX_PI / 2.0, img_enemyShotBullet[col]);
            }
            // 旗(8分音符のみ): 符幹の先端から右へ張り出してカーブする
            if (type == kNoteEighth) {
                const double tipY = dirY * kStemStep * (kStemBullets - 1);
                for (int i = 0; i < 4; i++) {
                    AddPart(parts, n, sx + kFlagX[i], tipY - dirY * kFlagY[i], -dirY * kFlagMuki[i], img_enemyShotScale[kColBlack]);
                }
            }
        }
        return n;
    }

    double MinDx(const sNotePart parts[], int n)
    {
        double result = parts[0].dx;
        for (int i = 1; i < n; i++) {
            if (parts[i].dx < result) result = parts[i].dx;
        }
        return result;
    }

    // 音符を出してから、符頭が発音ラインに達するまでのframe
    int NoteSoundDelay(int pos, int type)
    {
        sNotePart parts[kMaxNoteParts];
        const int n = BuildNoteParts(pos, type, parts);
        return FramesToReach(kSpawnX - MinDx(parts, n) - kSoundX);
    }

    // 発音: 符頭を中心に、音階の色の小玉が波紋状に広がる。echo>0 は余韻のリング。
    void EmitRipple(sEnemyShotSet* pSet, double x, double y, int pos, int type, int echo)
    {
        const int col = kDegreeColor[(pos + 2) % 7];                     // 最下線(pos=0)は「ミ」
        int num = 28 + 4 * pos - 4 * echo + (type == kNoteWhole ? 16 : 0);   // 高い音ほど多い
        const double speed = (1.1 + 0.14 * pos) * (1.0 - 0.1 * echo);              // 高い音ほど速い
        const double base = 0.61 * pos + 0.35 * echo;                    // 音ごと・余韻ごとに角度をずらす
        num = 4;
        for (int i = 0; i < num; i++) {
            const double a = base + 2.0 * DX_PI * i / num;
            const bool isAccent = (echo == 0 && i % 4 == 0);             // 最初のリングだけ4発に1発を中玉にする
            const double s = isAccent ? speed * 0.9 : speed;
            const int kind = isAccent ? img_enemyShotMediumBall[col] : img_enemyShotSmallBall[col];
            AddShot(pSet, x, y, a, s * cos(a), s * sin(a), kind);
        }

        if (echo == 0) {
            switch (type) {
            case kNoteEighth:  PlaySE(sound_enemyShot_light);   break;
            case kNoteQuarter: PlaySE(sound_enemyShot_medium);  break;
            case kNoteHalf:    PlaySE(sound_enemyShot_heavy);   break;
            default:           PlaySE(sound_enemyShot_extreme); break;
            }
            // 弦を弾く
            const double amp = (2.0 + 0.4 * pos) * (type == kNoteWhole ? 1.6 : 1.0);
            if (amp > g_vibration) g_vibration = amp;
        }
    }

    // 音符1個ぶんのShotSet。param_i[0]=pos, param_i[1]=type, param_d[0]=パーツ最左のdx(内部用)
    void ShotNote(sEnemyShotSet* pSet)
    {
        const int pos = pSet->param_i[0];
        const int type = pSet->param_i[1];
        const int t = pSet->count;

        // パーツは「右端の画面外(margin内)に届いたものから順に」出す。
        // 符頭の中心から符幹・旗までは最大で約35pxあり、一斉に出すとmarginを越えて消されてしまうため。
        sNotePart parts[kMaxNoteParts];
        int numParts = 0;
        if (t <= kNoteBuildFrames) {
            numParts = BuildNoteParts(pos, type, parts);
            if (t == 0) pSet->param_d[0] = MinDx(parts, numParts);
        }
        const double dxMin = pSet->param_d[0];
        const double headX = kSpawnX - dxMin - kScrollSpeed * t;   // 符頭の現在位置(移動前)
        const double headY = kStaffBottomY - kHalfStep * pos;

        for (int i = 0; i < numParts; i++) {
            if (FramesToReach(parts[i].dx - dxMin) == t) {
                AddShot(pSet, headX + parts[i].dx, headY + parts[i].dy, parts[i].muki, -kScrollSpeed, 0.0, parts[i].kind);
            }
        }

        // 発音と余韻(8分=余韻なし、4分=1回、2分=2回、全音符=3回)
        const int tSound = FramesToReach(kSpawnX - dxMin - kSoundX);
        const int interval = (type == kNoteWhole) ? 20 : 15;
        const int rel = t - tSound;
        if (rel >= 0 && rel % interval == 0 && rel / interval <= type) {
            EmitRipple(pSet, headX, headY, pos, type, rel / interval);
        }

        MoveShots(pSet);
    }

    // ============================================================
    //  小節線
    // ============================================================

    // 縦に画面を切る小節線。param_d[0] = 隙間の中心y
    void ShotBarLine(sEnemyShotSet* pSet)
    {
        if (pSet->count == 0) {
            const double gapY = pSet->param_d[0];
            for (int i = 0; i < 54; i++) {
                const double y = 4.0 + 9.0 * i;
                if (fabs(y - gapY) < kGapHalf) continue;
                AddShot(pSet, kSpawnX, y, DX_PI / 2.0, -kScrollSpeed, 0.0, img_enemyShotBullet[kColWhite]);
            }
            // 隙間の縁の目印
            AddShot(pSet, kSpawnX, gapY - kGapHalf, DX_PI, -kScrollSpeed, 0.0, img_enemyShotMediumBall[kColOrange]);
            AddShot(pSet, kSpawnX, gapY + kGapHalf, DX_PI, -kScrollSpeed, 0.0, img_enemyShotMediumBall[kColOrange]);
        }
        MoveShots(pSet);
    }

    // ============================================================
    //  終止線(フィナーレ)
    // ============================================================

    // 五線の5行を、発音ラインから左右へ吐き出す(index = 何回目の吐き出しか)
    void EmitStaffWave(sEnemyShotSet* pSet, int index)
    {
        for (int row = 0; row < 5; row++) {
            const double rowY = kStaffBottomY - kLineStep * row;
            for (int dir = -1; dir <= 1; dir += 2) {
                sEnemyShot* pShot = AddShot(pSet, kSoundX, rowY, (dir < 0) ? DX_PI : 0.0, 3.2 * dir, 0.0,
                    img_enemyShotSmallBall[kRowColor[row]]);
                pShot->param_i[0] = kBehaviorWave;
                pShot->param_d[2] = rowY;
                pShot->param_d[3] = 0.9 * row + 0.12 * index;
                pShot->param_d[4] = kSoundX;
            }
        }
    }

    // 細線(銃弾)と太線(大玉)の二重線。param_d[0] = 隙間の中心y
    void ShotFinalBar(sEnemyShotSet* pSet)
    {
        const int t = pSet->count;
        const double gapY = pSet->param_d[0];

        // 細線
        if (t == 0) {
            for (int i = 0; i < 54; i++) {
                const double y = 4.0 + 9.0 * i;
                if (fabs(y - gapY) < kGapHalf) continue;
                AddShot(pSet, kSpawnX, y, DX_PI / 2.0, -kScrollSpeed, 0.0, img_enemyShotBullet[kColWhite]);
            }
            AddShot(pSet, kSpawnX, gapY - kGapHalf, DX_PI, -kScrollSpeed, 0.0, img_enemyShotMediumBall[kColOrange]);
            AddShot(pSet, kSpawnX, gapY + kGapHalf, DX_PI, -kScrollSpeed, 0.0, img_enemyShotMediumBall[kColOrange]);
        }

        // 太線: 細線の右側に並ぶので、その分だけ遅れて出す
        if (t == FramesToReach(kThickOffsetX)) {
            const double x = kSpawnX + kThickOffsetX - kScrollSpeed * t;
            for (int i = 0; i < 18; i++) {
                const double y = 14.0 + 28.0 * i;
                if (fabs(y - gapY) < kThickGapHalf) continue;
                AddShot(pSet, x, y, DX_PI, -kScrollSpeed, 0.0, img_enemyShotLargeBall[kColBlack]);
            }
        }

        // 細線が発音ラインに達したら、五線を波打たせながら左右へ吐き出す
        const int rel = t - kBarTravel;
        if (rel >= 0 && rel < kFinaleEmitFrames && rel % kFinaleEmitStep == 0) {
            if (rel == 0) PlaySE(sound_enemyShot_extreme);
            EmitStaffWave(pSet, rel / kFinaleEmitStep);
        }

        MoveShots(pSet);
    }

} // namespace

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_StaffNotation_Claude()
{
    static int s_delay[kNumNotes];   // 各音符が、出てから発音するまでのframe数

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 400;   // 200から変更しないこと
        g_vibration = 0.0;
        for (int i = 0; i < kNumNotes; i++) {
            s_delay[i] = NoteSoundDelay(kScore[i].pos, kScore[i].type);
        }
        PlaySE(sound_enemyCharge);       // 五線が引かれ始める予告音
    }
    else {
        enemy.x = 240.0 + 28.0 * sin(count * 0.02);   // 指揮者のようにゆっくり揺れる
    }

    // 弦の振動を減衰させる
    g_vibration *= 0.965;
    if (g_vibration < 0.05) g_vibration = 0.0;

    // 五線: 5本の弦を1列ずつ右端から送り出す
    if (count % kStaffInterval == 1) {
        SpawnShotSet(ShotStaffColumn);
    }

    // 音符: 発音ラインで鳴る時刻から逆算して、右端から送り出す
    for (int i = 0; i < kNumNotes; i++) {
        if (!IsSpawnFrame(kScore[i].off, s_delay[i])) continue;
        sEnemyShotSet* pSet = SpawnShotSet(ShotNote);
        pSet->param_i[0] = kScore[i].pos;
        pSet->param_i[1] = kScore[i].type;
    }

    // 小節線: 1〜9小節目の頭に、少し先行して出す
    for (int m = 1; m <= 9; m++) {
        if (IsSpawnFrame(m * kMeasureFrames - kBarLead, kBarTravel)) {
            sEnemyShotSet* pSet = SpawnShotSet(ShotBarLine);
            pSet->param_d[0] = kBarGapY[m - 1];
        }
    }

    // 終止線: 1周期の終わり(和音の後)に出す
    if (IsSpawnFrame(kCycleFrames - kBarLead, kBarTravel)) {
        PlaySE(sound_enemyCharge);
        sEnemyShotSet* pSet = SpawnShotSet(ShotFinalBar);
        pSet->param_d[0] = kFinalGapY;
    }
}