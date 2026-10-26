// enemyPat_massGame.cpp
// 弾幕「人文字」: マスゲームの隊列行進・整列・人文字(隊形変形)・一斉解散を敵弾で表現する。
//
// 弾1発 = 演技者1人。10縦隊 x 20発 = 200発で1回の「演技」を行う。
//   小玉(白)   : 演技者の本体
//   中玉(赤)   : 各縦隊の先頭の「旗手」と、内周の演技者
//   小玉/中玉(黄): 号令の瞬間の発光、およびチャージ中の点滅
//   大玉(黄)   : フィニッシュの一発
//
// 進行(shotSet->count 基準, 60fps):
//   0〜210     フェーズ1 入場行進   左右の端から縦隊が入場し、減速して一斉に停止する
//   210〜330   フェーズ2 整列・静止 20列 x 10行の格子で静止(壁)。停止の瞬間に発光
//   330〜450   フェーズ3a 円形      格子から「二重線の円弧 x 5」の円へ変形
//   450〜570   フェーズ3b 円の回転  円全体が回転し、5か所の隙間(安全地帯)が回る
//   570〜630   フェーズ3c 二重円へ   外周(小玉)と内周(赤い中玉)に分かれる
//   630〜750   フェーズ3d 逆回転    外周と内周が逆向きに回る
//   750〜780   フェーズ4a 溜め      全弾が停止して点滅(笛の合図)
//   780〜1030  フェーズ4b 一斉解散   波のように順番に自機狙いで発射
//   1030〜     フィニッシュ         中央から大玉を1発、ゆっくり自機へ
//
// 隊形は「時刻 t から各弾の位置を求める関数」で表現している。
// 隊形の切り替えは、前後の隊形の位置を補間するだけなので、人文字の追加が容易。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace {

    struct MassPoint {
        double x, y;
    };

    // ============================================================
    //  定数 (名前の衝突を避けるため kMass プレフィックスを付けている)
    // ============================================================

    // --- 隊列 ---
    constexpr int    kMassRows = 10;    // 縦隊の数(行)
    constexpr int    kMassFileLen = 20;    // 1縦隊あたりの弾数(列)
    constexpr double kMassSpacingX = 20.0;  // 縦隊内の弾の間隔
    constexpr double kMassSpacingY = 18.0;  // 縦隊同士の間隔
    constexpr double kMassGridTopY = 96.0+20;  // 格子の最上段のY座標
    constexpr double kMassCenterX = 240.0; // 隊形の中心X
    constexpr double kMassCenterY = 215.0+25; // 円形隊形の中心Y

    // --- 入場行進 ---
    constexpr double kMassEntryX = 6.0;   // 入場時のX座標(左端。右端は 480 - この値)
    constexpr double kMassMarchSpeed = 2.4;   // 行進速度
    constexpr double kMassDecelFrames = 60.0; // 停止までの減速にかける時間
    constexpr double kMassGridLeftX = kMassCenterX - kMassSpacingX * (kMassFileLen - 1) / 2.0;
    constexpr double kMassHeadFinalX = kMassGridLeftX + kMassSpacingX * (kMassFileLen - 1); // 先頭の停止位置
    constexpr double kMassDecelStart = (kMassHeadFinalX - kMassEntryX - kMassMarchSpeed * kMassDecelFrames / 2.0) / kMassMarchSpeed;

    // --- 円形隊形(フェーズ3a,3b): 二重線の円弧 x 5 ---
    constexpr double kMassRingRadius = 140.0+100;
    constexpr double kMassRingLayerGap = 12.0;
    constexpr double kMassRingSpan = 54.0 * DX_PI / 180.0; // 1つの円弧の角度幅
    constexpr double kMassRingOmega = 0.020;                // 回転速度(rad/frame)

    // --- 二重円隊形(フェーズ3c,3d) ---
    constexpr double kMassOuterRadius = 155.0+80;               // 外周(小玉) 二重線の円弧 x 4
    constexpr double kMassOuterSpan = 60.0 * DX_PI / 180.0;
    constexpr double kMassOuterOmega = 0.018;
    constexpr double kMassInnerRadius = 105.0+60;               // 内周(赤い中玉) 円弧 x 2
    constexpr double kMassInnerSpan = 120.0 * DX_PI / 180.0;
    constexpr double kMassInnerOmega = 0.025;                // 外周と逆向きに回る
    constexpr int    kMassInnerFirstRow = 8;                 // この行以降が内周

    // --- タイムライン(フレーム) ---
    constexpr int kMassMarchEnd = 210;  // 行進が終わり静止
    constexpr int kMassHoldEnd = 330;  // 静止が終わり円形へ
    constexpr int kMassRingEnd = 450;  // 円形が完成
    constexpr int kMassRotEnd = 570;  // 回転が終わり二重円へ
    constexpr int kMassDoubleEnd = 630;  // 二重円が完成
    constexpr int kMassFreeze = 750;  // 逆回転が終わり全弾が停止
    constexpr int kMassFireStart = 780;  // 一斉解散の開始
    constexpr int kMassWaveInterval = 10; // 波と波の間隔
    constexpr int kMassRowDelay = 1;    // 同じ波の中で行ごとにずらすフレーム数
    constexpr int kMassFinish = 1030; // フィニッシュの大玉を撃つ時刻
    constexpr int kMassPeriod = 1260; // 演技を繰り返す周期
    constexpr int kMassFlashFrames = 6;   // 号令の発光時間

    // --- 解散時の弾速 ---
    constexpr double kMassSmallSpeed = 2.6;
    constexpr double kMassMediumSpeed = 2.0;
    constexpr double kMassFinishSpeed = 1.5;

    // --- 弾の色インデックス(0:赤 1:黄 6:白) ---
    constexpr int kMassColorRed = 0;
    constexpr int kMassColorYellow = 1;
    constexpr int kMassColorWhite = 6;

    // --- 弾ごとの param_i の使い道 ---
    constexpr int kMassPiRow = 0; // 縦隊の番号
    constexpr int kMassPiIdx = 1; // 縦隊内の番号(0が先頭=旗手)
    constexpr int kMassPiFired = 2; // 1なら発射済み(自由に飛ぶ)

    // ============================================================
    //  ユーティリティ
    // ============================================================
    void PlaySE(int handle)
    {
        if (CheckSoundMem(handle)) StopSoundMem(handle);
        PlaySoundMem(handle, DX_PLAYTYPE_BACK);
    }

    double PositiveOrZero(double v)
    {
        return v > 0.0 ? v : 0.0;
    }

    // 0〜1 のなめらかな補間(ease in-out)
    double Ease(double u)
    {
        if (u < 0.0) u = 0.0;
        if (u > 1.0) u = 1.0;
        return u * u * (3.0 - 2.0 * u);
    }

    MassPoint Lerp(const MassPoint& a, const MassPoint& b, double u)
    {
        return { a.x + (b.x - a.x) * u, a.y + (b.y - a.y) * u };
    }

    // ============================================================
    //  隊形: 時刻 t における各弾の位置
    // ============================================================

    // 縦隊の先頭のX座標(左から入場する縦隊基準)。一定速度で進み、最後に減速して停止する
    double HeadX(double t)
    {
        if (t <= kMassDecelStart) return kMassEntryX + kMassMarchSpeed * t;
        const double u = t - kMassDecelStart;
        if (u >= kMassDecelFrames) return kMassHeadFinalX;
        return kMassEntryX + kMassMarchSpeed * kMassDecelStart
            + kMassMarchSpeed * u - 0.5 * (kMassMarchSpeed / kMassDecelFrames) * u * u;
    }

    // フェーズ1,2: 入場行進と格子。偶数行は左から、奇数行は右から入場する
    MassPoint MarchPos(int row, int idx, double t)
    {
        double x = HeadX(t) - kMassSpacingX * idx;
        if (row % 2 == 1) x = 480.0 - x;
        return { x, kMassGridTopY + kMassSpacingY * row };
    }

    // 円弧上の idx 番目の弾の位置。idx=0 が dir 方向(回転の進行方向)の先頭になる
    MassPoint ArcPoint(double radius, double centerAngle, double span, double idx, double dir)
    {
        const double step = span / (double)(kMassFileLen - 1);
        const double angle = centerAngle + dir * ((kMassFileLen - 1) / 2.0 - idx) * step;
        return { kMassCenterX + radius * cos(angle), kMassCenterY + radius * sin(angle) };
    }

    // フェーズ3a,3b: 円形(二重線の円弧 x 5。円弧の間が隙間になる)
    MassPoint RingPos(int row, int idx, double t)
    {
        const int arc = row / 2;
        const int layer = row % 2;
        const double rot = kMassRingOmega * PositiveOrZero(t - kMassRingEnd);
        const double center = -DX_PI / 2.0 + arc * (DX_PI * 2.0 / 5.0) + rot;
        return ArcPoint(kMassRingRadius - kMassRingLayerGap * layer, center, kMassRingSpan,
            idx + 0.5 * layer, 1.0);
    }

    // フェーズ3c,3d: 二重円(外周は時計回り、内周は反時計回り)
    MassPoint DoublePos(int row, int idx, double t)
    {
        const double elapsed = PositiveOrZero(t - kMassRotEnd);
        if (row < kMassInnerFirstRow) {
            const int arc = row / 2;
            const int layer = row % 2;
            const double center = -DX_PI / 2.0 + arc * (DX_PI / 2.0) + kMassOuterOmega * elapsed;
            return ArcPoint(kMassOuterRadius - kMassRingLayerGap * layer, center, kMassOuterSpan,
                idx + 0.5 * layer, 1.0);
        }
        const int arc = row - kMassInnerFirstRow;
        const double center = -DX_PI / 2.0 + arc * DX_PI - kMassInnerOmega * elapsed;
        return ArcPoint(kMassInnerRadius, center, kMassInnerSpan, idx, -1.0);
    }

    // 全フェーズ通しての位置。フェーズの境目は前後の隊形を補間してつなぐ
    MassPoint MassPos(int row, int idx, int tNow)
    {
        const double t = (tNow > kMassFreeze) ? (double)kMassFreeze : (double)tNow;
        if (t <= kMassHoldEnd) {
            return MarchPos(row, idx, t);
        }
        if (t <= kMassRingEnd) {
            const double u = Ease((t - kMassHoldEnd) / (double)(kMassRingEnd - kMassHoldEnd));
            return Lerp(MarchPos(row, idx, t), RingPos(row, idx, t), u);
        }
        if (t <= kMassRotEnd) {
            return RingPos(row, idx, t);
        }
        if (t <= kMassDoubleEnd) {
            const double u = Ease((t - kMassRotEnd) / (double)(kMassDoubleEnd - kMassRotEnd));
            return Lerp(RingPos(row, idx, t), DoublePos(row, idx, t), u);
        }
        return DoublePos(row, idx, t);
    }

    // ============================================================
    //  弾の見た目
    // ============================================================

    // 号令の発光(黄色)と、チャージ中の点滅
    bool IsFlashFrame(int t)
    {
        const int cues[] = { kMassMarchEnd, kMassHoldEnd, kMassRotEnd };
        for (int i = 0; i < 3; i++) {
            if (t >= cues[i] && t < cues[i] + kMassFlashFrames) return true;
        }
        if (t >= kMassFreeze && t < kMassFireStart) {
            return ((t - kMassFreeze) / 5) % 2 == 0;
        }
        return false;
    }

    // 中玉(赤)になる弾: 各縦隊の先頭(旗手)と、二重円になってからの内周
    bool IsMediumShot(int row, int idx, int t)
    {
        return idx == 0 || (row >= kMassInnerFirstRow && t >= kMassRotEnd);
    }

    int ShotKind(int row, int idx, int t)
    {
        const bool medium = IsMediumShot(row, idx, t);
        if (IsFlashFrame(t)) {
            return medium ? img_enemyShotMediumBall[kMassColorYellow] : img_enemyShotSmallBall[kMassColorYellow];
        }
        return medium ? img_enemyShotMediumBall[kMassColorRed] : img_enemyShotSmallBall[kMassColorWhite];
    }

    // ============================================================
    //  弾の生成
    // ============================================================
    void LinkShot(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pEnemyShot)
    {
        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 演技者を1人入場させる
    void AddMassShot(sEnemyShotSet* pEnemyShotSet, int row, int idx)
    {
        const int t = pEnemyShotSet->count;
        const MassPoint pt = MassPos(row, idx, t);
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pt.x;
        pEnemyShot->y = pt.y;
        pEnemyShot->muki = (row % 2 == 0) ? 0.0 : DX_PI; // 進行方向(偶数行は右向き、奇数行は左向き)
        pEnemyShot->speed = 0.0;                          // 解散するまでは位置を直接指定して動かす
        pEnemyShot->kind = ShotKind(row, idx, t);
        pEnemyShot->param_i[kMassPiRow] = row;
        pEnemyShot->param_i[kMassPiIdx] = idx;
        pEnemyShot->param_i[kMassPiFired] = 0;
        LinkShot(pEnemyShotSet, pEnemyShot);
    }

    // フィニッシュの大玉(中央から自機へゆっくり飛ぶ)
    void AddFinishShot(sEnemyShotSet* pEnemyShotSet)
    {
        sEnemyShot* pEnemyShot = new sEnemyShot;
        pEnemyShot->x = kMassCenterX;
        pEnemyShot->y = kMassCenterY;
        pEnemyShot->muki = atan2(player.y - pEnemyShot->y, player.x - pEnemyShot->x);
        pEnemyShot->speed = kMassFinishSpeed;
        pEnemyShot->kind = img_enemyShotLargeBall[kMassColorYellow];
        pEnemyShot->param_i[kMassPiFired] = 1;
        LinkShot(pEnemyShotSet, pEnemyShot);
    }

    // ============================================================
    //  弾幕: 人文字
    // ============================================================
    void ShotMassGame(sEnemyShotSet* pEnemyShotSet)
    {
        const int t = pEnemyShotSet->count;

        // --- 入場: 先頭が進むたびに、画面端から次の列が1列(全縦隊ぶん)入場する ---
        // pEnemyShotSet->param_i[0] : 入場済みの列数
        while (pEnemyShotSet->param_i[0] < kMassFileLen &&
            HeadX((double)t) - kMassSpacingX * pEnemyShotSet->param_i[0] >= kMassEntryX) {
            for (int row = 0; row < kMassRows; row++) {
                AddMassShot(pEnemyShotSet, row, pEnemyShotSet->param_i[0]);
            }
            pEnemyShotSet->param_i[0]++;
        }

        // --- 号令(効果音) ---
        if (t == kMassMarchEnd) PlaySE(sound_enemyShot_medium); // 停止の瞬間(太鼓)
        if (t == kMassHoldEnd || t == kMassRotEnd || t == kMassFreeze) PlaySE(sound_enemyCharge); // 隊形変更・溜めの合図(笛)
        if (t == kMassFinish - 30) PlaySE(sound_enemyCharge);   // フィニッシュの予告
        if (t == kMassFinish) {
            PlaySE(sound_enemyShot_heavy);
            AddFinishShot(pEnemyShotSet);
        }

        // --- 各弾の更新 ---
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            const int row = pShot->param_i[kMassPiRow];
            const int idx = pShot->param_i[kMassPiIdx];

            if (pShot->param_i[kMassPiFired] == 0) {
                // 発射時刻: 列(idx)ごとに「波」になり、同じ波の中でも行ごとに少しずらす
                const int fireTime = kMassFireStart + idx * kMassWaveInterval + row * kMassRowDelay;
                if (t >= fireTime) {
                    // 解散: その瞬間の自機へ向けて発射
                    pShot->param_i[kMassPiFired] = 1;
                    pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                    pShot->speed = IsMediumShot(row, idx, t) ? kMassMediumSpeed : kMassSmallSpeed;
                    pShot->kind = ShotKind(row, idx, t);
                    if (row == 0) PlaySE(sound_enemyShot_light); // 1波につき1回だけ鳴らす
                }
                else {
                    // 隊形データから位置を求める。muki は移動方向、静止中は発射予定の自機方向にしておく
                    const MassPoint pt = MassPos(row, idx, t);
                    const double dx = pt.x - pShot->x;
                    const double dy = pt.y - pShot->y;
                    if (dx * dx + dy * dy > 1.0e-6) {
                        pShot->muki = atan2(dy, dx);
                    }
                    else {
                        pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                    }
                    pShot->x = pt.x;
                    pShot->y = pt.y;
                    pShot->kind = ShotKind(row, idx, t);
                }
            }

            if (pShot->param_i[kMassPiFired] != 0) {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pShot->next;
        }
    }

} // namespace

// 敵本体のパターン
// 敵は画面上部中央で動かず、「指揮者」として演技(弾幕)を繰り返し出す。
void EnemyPat_MassGame_Claude()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
    }

    // 演技を1回開始する。count==1 の時点で最初の演技が始まる
    if (count % kMassPeriod == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMassGame;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
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