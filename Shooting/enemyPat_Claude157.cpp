// ============================================================
//  enemyPat_supernova.cpp
//  弾幕「超新星爆発 ― 星の終焉」
//
//  流れ(1サイクル = 720フレーム、以降ループ)
//    t=  0〜 59 : 敵が画面中央上部へ移動(初回のみ)
//    t= 60〜196 : フェーズ1 重力崩壊  青・シアンのガスが渦を巻いて敵(星のコア)へ吸い込まれる
//    t=     264 : フェーズ2 超新星爆発  光球リング + 5重の衝撃波 + 残光
//    t=294〜    : フェーズ3 残骸の放出  菱形弾・鱗弾の腕16本 + 中楕円弾の星雲ガス
//    t=424〜663 : フェーズ4 パルサー    中性子星 + 回転する対向ジェット(短レーザー)
//    t=     664 : フィナーレ  シアンの小玉を全方位へ
//
//  使用素材
//    星のコア      : 大玉(橙→黄→白、最後は白/シアン点滅)
//    崩壊するガス  : 小玉(シアン・青)、4波に1回だけ中玉(白)
//    衝撃波        : 中玉(白・黄)、小玉(橙・赤)
//    残骸          : 菱形弾・鱗弾(橙・黄)
//    星雲          : 中楕円弾(マゼンタ・緑)
//    中性子星      : 大玉(マゼンタ/白の点滅)
//    パルサー噴流  : 短レーザー(白・シアン)、周囲に小玉(マゼンタ・白)
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

namespace {

    // ------------------------------------------------------------
    //  定数
    // ------------------------------------------------------------
    const double kPi = DX_PI;
    const double kTwoPi = DX_PI * 2.0;

    // 弾の色インデックス: img_enemyShotXxx[色]
    const int kColRed = 0;
    const int kColYellow = 1;
    const int kColGreen = 2;
    const int kColCyan = 3;
    const int kColBlue = 4;
    const int kColMagenta = 5;
    const int kColWhite = 6;
    const int kColOrange = 8;

    // 弾の形状
    enum SnShape {
        kShapeSmall,    // 小玉
        kShapeMedium,   // 中玉
        kShapeLarge,    // 大玉
        kShapeBullet,   // 銃弾
        kShapeScale,    // 鱗弾
        kShapeDiamond,  // 菱形弾
        kShapeOval,     // 中楕円弾
        kShapeLaser     // 短レーザー
    };

    // 星の位置(敵の待機位置)
    const double kHomeX = 240.0;
    const double kHomeY = 130.0;

    // タイムライン(1サイクル内のフレーム)
    const int kCycle = 720;
    const int kStartCollapse = 60;   // 重力崩壊の開始
    const int kCollapseStep = 8;    // 崩壊ガスの波の間隔
    const int kCollapseWaves = 18;   // 波の数(最後の発射は t=196)
    const int kCollapseNum = 60;   // 1波あたりの弾数
    const double kCollapseR0 = 130.0; // 崩壊ガスの初期半径(画面内に収まる範囲)
    const double kCoreRadius = 22.0;  // この半径に達したガスは吸収されて消える
    const int kExplode = 264;  // 爆発の瞬間
    const int kEjecta0 = kExplode + 30; // 残骸放出の開始
    const int kEjectaStep = 6;
    const int kEjectaTicks = 20;
    const int kGasStep = 10;
    const int kGasTicks = 16;
    const int kPulsar0 = 424;  // パルサーの開始
    const int kPulsarDur = 240;
    const int kPulsarStep = 3;
    const int kFinale = kPulsar0 + kPulsarDur;

    // 衝撃波リングの定義(5重の衝撃波 + 遅れてくる残光)
    struct SnRingDef {
        int    shape;
        int    color;
        double speed;
        int    num;
        int    delay;   // 爆発からの遅れ(フレーム)
    };
    const SnRingDef kRingDef[6] = {
        { kShapeMedium, kColWhite,  5.2, 72,  0 },
        { kShapeMedium, kColYellow, 4.4, 72,  3 },
        { kShapeSmall,  kColOrange, 3.7, 72,  6 },
        { kShapeSmall,  kColRed,    3.0, 72,  9 },
        { kShapeSmall,  kColOrange, 2.4, 72, 12 },
        { kShapeSmall,  kColRed,    1.9, 48, 24 },  // 残光(やや疎)
    };

    // ------------------------------------------------------------
    //  共通ヘルパー
    // ------------------------------------------------------------
    // 形状と色から弾の kind 値を得る
    int SnKind(int shape, int color)
    {
        switch (shape) {
        case kShapeSmall:   return img_enemyShotSmallBall[color];
        case kShapeMedium:  return img_enemyShotMediumBall[color];
        case kShapeLarge:   return img_enemyShotLargeBall[color];
        case kShapeBullet:  return img_enemyShotBullet[color];
        case kShapeScale:   return img_enemyShotScale[color];
        case kShapeDiamond: return img_enemyShotDiamond[color];
        case kShapeOval:    return img_enemyShotMediumOval[color];
        default:            return img_enemyShotLaser[color];
        }
    }

    // 効果音(前回の再生が残っていれば止めてから鳴らす)
    void SnPlay(int sound)
    {
        if (CheckSoundMem(sound)) StopSoundMem(sound);
        PlaySoundMem(sound, DX_PLAYTYPE_BACK);
    }

    // ShotSet を生成してリストに連結する(位置は現在の敵本体)
    sEnemyShotSet* SnCreateSet(sEnemyShotSet::PatternFunc func, double muki, int kind)
    {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = func;
        pSet->x = enemy.x;
        pSet->y = enemy.y;
        pSet->muki = muki;
        pSet->kind = kind;
        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
        return pSet;
    }

    // 弾を1発生成して ShotSet に連結する
    sEnemyShot* SnAddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
    {
        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = x;
        pShot->y = y;
        pShot->muki = muki;
        pShot->speed = speed;
        pShot->kind = kind;
        pShot->margin = 80;
        pShot->prev = pSet->pEnemyShotHead->prev;
        pShot->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = pShot;
        pSet->pEnemyShotHead->prev = pShot;
        return pShot;
    }

    // muki / speed に従って直進
    void SnAdvance(sEnemyShot* pShot)
    {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
    }

    // 弾を消す: 画面外の遠方へ飛ばし、実際の消去は画面外消去(メインルーチン)に任せる
    // param_i[15] を「消滅済み」フラグとして使う
    void SnDiscard(sEnemyShot* pShot)
    {
        pShot->param_i[15] = 1;
        pShot->speed = 0.0;
        pShot->x = -1000.0;
        pShot->y = -1000.0;
    }
    bool SnIsDead(const sEnemyShot* pShot)
    {
        return pShot->param_i[15] != 0;
    }

    double SnMax(double a, double b) { return (a > b) ? a : b; }
    double SnMin(double a, double b) { return (a < b) ? a : b; }

    // ------------------------------------------------------------
    //  フェーズ1: 重力崩壊
    // ------------------------------------------------------------
    // 星のコア: 大玉を数個重ね、色を橙→黄→白と変えて明るくしていく。爆発の直前は点滅。
    // 爆発の瞬間に消え、代わりに ShotFireball の光球が飛び出す。
    void ShotCore(sEnemyShotSet* pSet)
    {
        const int life = kExplode - kStartCollapse;
        if (pSet->count == 0) {
            for (int i = 0; i < 3; i++) {
                const double a = kTwoPi * i / 3.0;
                SnAddShot(pSet, pSet->x + cos(a) * 4.0, pSet->y + sin(a) * 4.0, a, 0.0,
                    SnKind(kShapeLarge, kColOrange));
            }
        }
        int color;
        if (pSet->count >= life - 30)  color = ((pSet->count / 2) % 2 == 0) ? kColWhite : kColCyan; // 臨界の点滅
        else if (pSet->count < 60)     color = kColOrange;
        else if (pSet->count < 130)    color = kColYellow;
        else                           color = kColWhite;

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (!SnIsDead(pShot)) {
                if (pSet->count >= life) {
                    SnDiscard(pShot);
                }
                else {
                    pShot->kind = SnKind(kShapeLarge, color);
                    pShot->muki += 0.03;
                }
            }
            pShot = pShot->next;
        }
    }

    // 崩壊するガス: 画面内の円周上から、渦を巻きながら加速して中心へ吸い込まれる。
    // 波ごとに回転方向を反転させる。4波に1回は白の中玉で「濃いガス」にする。
    //   pSet->kind    : 波の番号
    //   pSet->muki    : 位相オフセット
    //   param_d[0]    : 現在の半径 / [1]: 現在の角度 / [2]: 内向きの速さ / [3]: 回転方向(+1/-1)
    void ShotCollapse(sEnemyShotSet* pSet)
    {
        if (pSet->count == 0) {
            const int w = pSet->kind;
            const double dir = (w % 2 == 0) ? 1.0 : -1.0;
            const bool dense = (w % 4 == 3);
            for (int i = 0; i < kCollapseNum; i++) {
                const double theta = pSet->muki + kTwoPi * i / kCollapseNum;
                int shape, color;
                if (dense) { shape = kShapeMedium; color = kColWhite; }
                else { shape = kShapeSmall;  color = (i % 2 == 0) ? kColCyan : kColBlue; }
                sEnemyShot* pShot = SnAddShot(pSet,
                    pSet->x + cos(theta) * kCollapseR0,
                    pSet->y + sin(theta) * kCollapseR0,
                    theta + kPi, 0.0, SnKind(shape, color));  // 向きは中心方向
                pShot->param_d[0] = kCollapseR0;
                pShot->param_d[1] = theta;
                pShot->param_d[2] = 0.4;
                pShot->param_d[3] = dir;
            }
        }
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (!SnIsDead(pShot)) {
                double r = pShot->param_d[0];
                double th = pShot->param_d[1];
                double rv = pShot->param_d[2] + 0.05;  // 重力で加速
                r -= rv;
                if (r < kCoreRadius) {
                    SnDiscard(pShot);  // コアに吸収
                }
                else {
                    // 角運動量保存のイメージ: 中心に近いほど速く回る
                    th += pShot->param_d[3] * 0.035 * kCollapseR0 / SnMax(r, 26.0);
                    const double nx = pSet->x + cos(th) * r;
                    const double ny = pSet->y + sin(th) * r;
                    const double dx = nx - pShot->x;
                    const double dy = ny - pShot->y;
                    pShot->muki = atan2(dy, dx);
                    pShot->speed = sqrt(dx * dx + dy * dy);
                    pShot->x = nx;
                    pShot->y = ny;
                    pShot->param_d[0] = r;
                    pShot->param_d[1] = th;
                    pShot->param_d[2] = rv;
                }
            }
            pShot = pShot->next;
        }
    }

    // ------------------------------------------------------------
    //  フェーズ2: 超新星爆発
    // ------------------------------------------------------------
    // 光球: 白の大玉を全方位に24発。ゆっくり減速しながら広がる。
    void ShotFireball(sEnemyShotSet* pSet)
    {
        if (pSet->count == 0) {
            for (int i = 0; i < 24; i++) {
                const double theta = pSet->muki + kTwoPi * i / 24.0;
                SnAddShot(pSet, pSet->x, pSet->y, theta, 2.6, SnKind(kShapeLarge, kColWhite));
            }
        }
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->speed > 1.0) {
                pShot->speed *= 0.985;
                if (pShot->speed < 1.0) pShot->speed = 1.0;
            }
            SnAdvance(pShot);
            pShot = pShot->next;
        }
    }

    // 衝撃波リング(5重) + 残光リング。定義は kRingDef を参照。
    //   pSet->kind : リング番号 / pSet->muki : 角度オフセット(1つおきに半ピッチずらす)
    //   param_d[0] : 減速の下限速度
    void ShotShockRing(sEnemyShotSet* pSet)
    {
        const SnRingDef& def = kRingDef[pSet->kind];
        if (pSet->count == 0) {
            for (int i = 0; i < def.num; i++) {
                const double theta = pSet->muki + kTwoPi * i / def.num;
                sEnemyShot* pShot = SnAddShot(pSet, pSet->x, pSet->y, theta, def.speed,
                    SnKind(def.shape, def.color));
                pShot->param_d[0] = def.speed * 0.55;
            }
        }
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->speed > pShot->param_d[0]) {
                pShot->speed *= 0.993;  // 衝撃波は進むほど減衰
                if (pShot->speed < pShot->param_d[0]) pShot->speed = pShot->param_d[0];
            }
            SnAdvance(pShot);
            pShot = pShot->next;
        }
    }

    // ------------------------------------------------------------
    //  フェーズ3: 残骸の放出
    // ------------------------------------------------------------
    // 残骸の腕: 16本の腕それぞれが違う初速・ブレーキ・曲がり方を持つ。
    // 初速の速い腕ほどブレーキが弱く、遠くまで伸びる。左右交互に曲がって渦を巻く。
    //   pSet->kind : 何発目の連射か
    //   param_d[0] : 曲がり(rad/frame) / [1]: 最低速度 / [2]: 減速率
    void ShotEjecta(sEnemyShotSet* pSet)
    {
        if (pSet->count == 0) {
            const int j = pSet->kind;
            for (int a = 0; a < 16; a++) {
                const int g = a % 4;
                const double theta = kTwoPi * a / 16.0 + 0.15 + j * 0.012;
                const int shape = (a % 2 == 0) ? kShapeDiamond : kShapeScale;
                const int color = ((a / 2 + j) % 2 == 0) ? kColOrange : kColYellow;
                sEnemyShot* pShot = SnAddShot(pSet, pSet->x, pSet->y, theta,
                    1.8 + 0.7 * g, SnKind(shape, color));
                pShot->param_d[0] = ((a % 2 == 0) ? 1.0 : -1.0) * (0.0035 + 0.0006 * g);
                pShot->param_d[1] = 0.7 + 0.12 * g;
                pShot->param_d[2] = 0.982 + 0.004 * g;
            }
        }
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            pShot->speed *= pShot->param_d[2];
            if (pShot->speed < pShot->param_d[1]) pShot->speed = pShot->param_d[1];
            pShot->muki += pShot->param_d[0];  // 進行方向と一緒に弾の向きも回る
            SnAdvance(pShot);
            pShot = pShot->next;
        }
    }

    // 星雲ガス: 腕と腕の間を、遅い中楕円弾(マゼンタ・緑)がゆるやかに曲がりながら漂う。
    //   pSet->kind : 何回目の放出か / param_d[0]: 曲がり(rad/frame)
    void ShotGas(sEnemyShotSet* pSet)
    {
        if (pSet->count == 0) {
            const int j = pSet->kind;
            for (int a = 0; a < 16; a++) {
                // GetRand(100) は 0〜100 の101通り → -50〜+50 に直して ±0.05rad の揺らぎ
                const double theta = kTwoPi * (a + 0.5) / 16.0 + 0.15 + j * 0.02
                    + (GetRand(100) - 50) / 1000.0;
                const double speed = 0.9 + GetRand(70) / 100.0;
                const int color = ((a + j) % 2 == 0) ? kColMagenta : kColGreen;
                sEnemyShot* pShot = SnAddShot(pSet, pSet->x, pSet->y, theta, speed,
                    SnKind(kShapeOval, color));
                pShot->param_d[0] = ((a % 2 == 0) ? 1.0 : -1.0) * 0.002;
            }
        }
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            pShot->muki += pShot->param_d[0];
            SnAdvance(pShot);
            pShot = pShot->next;
        }
    }

    // ------------------------------------------------------------
    //  フェーズ4: パルサー
    // ------------------------------------------------------------
    // 中性子星: マゼンタの大玉を1発、中心に静止させる。マゼンタ/白で点滅。
    void ShotPulsarStar(sEnemyShotSet* pSet)
    {
        if (pSet->count == 0) {
            SnAddShot(pSet, pSet->x, pSet->y, 0.0, 0.0, SnKind(kShapeLarge, kColMagenta));
        }
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (!SnIsDead(pShot)) {
                if (pSet->count >= kPulsarDur) {
                    SnDiscard(pShot);
                }
                else {
                    pShot->kind = SnKind(kShapeLarge, ((pSet->count / 4) % 2 == 0) ? kColMagenta : kColWhite);
                    pShot->muki += 0.05;
                }
            }
            pShot = pShot->next;
        }
    }

    // パルサーのジェット: 対向する2方向へ、短レーザーを3本ずつ扇状に発射。
    // 1回の発射は1個の ShotSet。ジェットは発射後も少しずつ加速する。
    //   pSet->muki : 発射時のジェット角度
    void ShotPulsarJet(sEnemyShotSet* pSet)
    {
        if (pSet->count == 0) {
            for (int jet = 0; jet < 2; jet++) {
                for (int f = -1; f <= 1; f++) {
                    const double theta = pSet->muki + kPi * jet + f * 0.07;
                    const int color = (f == 0) ? kColWhite : kColCyan;
                    SnAddShot(pSet, pSet->x, pSet->y, theta, 3.0, SnKind(kShapeLaser, color));
                }
            }
        }
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            pShot->speed = SnMin(pShot->speed + 0.04, 6.5);
            SnAdvance(pShot);
            pShot = pShot->next;
        }
    }

    // パルサー周囲のリング: ジェットが半周するたびに、小玉のリングをゆっくり広げて重ねる。
    //   pSet->kind : リングの通し番号 / pSet->muki : 角度オフセット
    void ShotPulsarRing(sEnemyShotSet* pSet)
    {
        if (pSet->count == 0) {
            const double speed = 1.1 + 0.2 * (pSet->kind % 3);
            for (int i = 0; i < 36; i++) {
                const double theta = pSet->muki + kTwoPi * i / 36.0;
                const int color = (i % 2 == 0) ? kColMagenta : kColWhite;
                SnAddShot(pSet, pSet->x, pSet->y, theta, speed, SnKind(kShapeSmall, color));
            }
        }
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            SnAdvance(pShot);
            pShot = pShot->next;
        }
    }

    // フィナーレ: シアンの小玉を3層(速度違い)で全方位へ。星の最期の余韻。
    void ShotFinale(sEnemyShotSet* pSet)
    {
        if (pSet->count == 0) {
            for (int layer = 0; layer < 3; layer++) {
                for (int i = 0; i < 120; i++) {
                    const double theta = pSet->muki + kTwoPi * i / 120.0 + layer * (kPi / 120.0);
                    SnAddShot(pSet, pSet->x, pSet->y, theta, 1.2 + 0.4 * layer,
                        SnKind(kShapeSmall, kColCyan));
                }
            }
        }
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            SnAdvance(pShot);
            pShot = pShot->next;
        }
    }

}  // namespace

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Supernova_Claude()
{
    static double s_pulsarTheta;     // ジェットの現在角度
    static int    s_pulsarHalfTurns; // ジェットが何回半周したか

    if (count == 1) {
        // ゲーム画面は 480x480。画面上部から星の位置へ降りてくる
        enemy.x = kHomeX;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;  // 200から変更しないこと
        s_pulsarTheta = 0.0;
        s_pulsarHalfTurns = 0;
    }
    else if (enemy.y < kHomeY) {
        enemy.y += 1.5;
        if (enemy.y > kHomeY) enemy.y = kHomeY;
    }

    const int t = (count - 1) % kCycle;  // サイクル内の経過フレーム

    // ---- フェーズ1: 重力崩壊 ----
    const int tc = t - kStartCollapse;
    if (tc >= 0 && tc % kCollapseStep == 0 && tc / kCollapseStep < kCollapseWaves) {
        const int w = tc / kCollapseStep;
        if (w == 0) {
            SnPlay(sound_enemyCharge);
            SnCreateSet(ShotCore, 0.0, 0);
        }
        SnCreateSet(ShotCollapse, w * 0.37, w);
    }

    // ---- フェーズ2: 超新星爆発 ----
    if (t == kExplode) {
        SnPlay(sound_enemyShot_extreme);
        SnCreateSet(ShotFireball, 0.0, 0);
    }
    if (t == kExplode + 9) {
        SnPlay(sound_enemyShot_heavy);
    }
    for (int k = 0; k < 6; k++) {
        if (t == kExplode + kRingDef[k].delay) {
            SnCreateSet(ShotShockRing, (k % 2) * (kPi / 72.0), k);  // 1つおきに半ピッチずらす
        }
    }

    // ---- フェーズ3: 残骸の放出 ----
    const int te = t - kEjecta0;
    if (te >= 0 && te % kEjectaStep == 0 && te / kEjectaStep < kEjectaTicks) {
        const int j = te / kEjectaStep;
        if (j % 3 == 0) SnPlay(sound_enemyShot_light);
        SnCreateSet(ShotEjecta, 0.0, j);
    }
    if (te >= 0 && te % kGasStep == 0 && te / kGasStep < kGasTicks) {
        SnCreateSet(ShotGas, 0.0, te / kGasStep);
    }

    // ---- フェーズ4: パルサー ----
    const int tp = t - kPulsar0;
    if (tp >= 0 && tp < kPulsarDur && tp % kPulsarStep == 0) {
        if (tp == 0) {
            s_pulsarTheta = 0.0;
            s_pulsarHalfTurns = 0;
            SnPlay(sound_enemyShot_medium);
            SnCreateSet(ShotPulsarStar, 0.0, 0);
        }
        // 回転は最初は遅く、徐々に速くする
        const double tau = tp / (double)kPulsarDur;
        const double omega = 0.05 + 0.15 * tau * tau;  // rad/frame
        SnCreateSet(ShotPulsarJet, s_pulsarTheta, 0);
        s_pulsarTheta += omega * kPulsarStep;
        // ジェットが半周するたびに周囲へ小玉リング
        while (s_pulsarTheta >= (s_pulsarHalfTurns + 1) * kPi) {
            s_pulsarHalfTurns++;
            SnPlay(sound_enemyShot_medium);
            SnCreateSet(ShotPulsarRing, s_pulsarTheta, s_pulsarHalfTurns);
        }
    }

    // ---- フィナーレ ----
    if (t == kFinale) {
        SnPlay(sound_enemyShot_heavy);
        SnCreateSet(ShotFinale, 0.0, 0);
    }
}