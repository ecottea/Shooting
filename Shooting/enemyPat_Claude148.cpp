#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  四天王奥義「三歩必殺」(東方地霊殿 3面ボス 星熊勇儀)
//
//  三段構成の静止弾を勇儀の周囲に敷き詰め、最後に全弾を外向きに放つ。
//    1歩目: 勇儀の近くに高密度の静止弾(菱形弾)
//    2歩目: 1歩目より外側に、やや疎なリング状の静止弾(菱形弾)。隙間が避難場所
//    3歩目: 溜め(予告音)の後、2歩目より外側の全域を埋める弾(中玉/大玉)
//    放出 : 全弾が勇儀を中心に外向きへ加速しながら飛ぶ
//  1セットごとに弾の色・種類が変わる(奇数セット: 青系+中玉 / 偶数セット: 赤系+大玉)。
//  難易度は Lunatic 相当(配置ランダム、3歩目は自機周辺にも発生)。
//  Hard以下の「3歩目は自機周辺に出ない」挙動にしたい場合は kSanpoLayer3SafeRadius を正の値にする。
// ============================================================

// ------------------------------------------------------------
//  調整用の定数
// ------------------------------------------------------------
// 敵本体の位置(ゲーム画面は 480x480)
static const double kSanpoBossX = 240.0;
static const double kSanpoBossY = 120.0;

// 1セットの流れ(フレーム)
static const int kSanpoCycleFrames = 360; // 1セットの長さ
static const int kSanpoStomp1Frame = 40;  // 1歩目
static const int kSanpoStomp2Frame = 80;  // 2歩目
static const int kSanpoChargeFrame = 130; // 溜め(予告音)
static const int kSanpoStomp3Frame = 170; // 3歩目
static const int kSanpoReleaseFrame = 200; // 全弾放出

// 1歩目: 勇儀に近い高密度リング
static const double kSanpoLayer1RMin = 24.0;
static const double kSanpoLayer1RMax = 80.0+10;
static const double kSanpoLayer1RStep = 8.0;
static const double kSanpoLayer1Spacing = 13.0; // 同一リング上の弾の間隔(px)

// 2歩目: やや疎なリング(隙間 = 避難場所)
static const double kSanpoLayer2RMin = 100.0-10;
static const double kSanpoLayer2RMax = 172.0+10;
static const double kSanpoLayer2RStep = 12.0;
static const double kSanpoLayer2Spacing = 26.0*0.75;
static const int    kSanpoLayer2GateNum = 1;    // 弾を抜く方向(ゲート)の数
static const double kSanpoLayer2GateHalf = 0.16; // ゲートの半幅(ラジアン)

// 3歩目: 外側全域
static const double kSanpoLayer3InnerR = 192.0-10; // これより内側には出さない
static const double kSanpoLayer3SpacingMid = 20.0;  // 中玉の格子間隔
static const double kSanpoLayer3SpacingLarge = 34.0;  // 大玉の格子間隔
static const double kSanpoLayer3SafeRadius = 0.0;   // 自機の周囲この半径内には出さない(0で無効 = Lunatic)

// 放出後の速度(静止から加速して上限へ)
static const double kSanpoSpeedStart = 0.3;
static const double kSanpoSpeedAccel = 0.05;
static const double kSanpoSpeedMax = 2.6;

// ------------------------------------------------------------
//  補助関数
// ------------------------------------------------------------
// 0.0 以上 1.0 未満の乱数(GetRand を使うのでリプレイでも再現される)
static double SanpoRand01()
{
    return GetRand(9999) / 10000.0;
}

// 2つの角度の差の絶対値(0〜π)
static double SanpoAngleDiff(double a, double b)
{
    double d = fmod(a - b, 2.0 * DX_PI);
    if (d > DX_PI) d -= 2.0 * DX_PI;
    if (d < -DX_PI) d += 2.0 * DX_PI;
    return fabs(d);
}

// 効果音の再生(鳴っていたら止めて頭から)
static void SanpoPlaySe(int soundHandle)
{
    if (CheckSoundMem(soundHandle)) StopSoundMem(soundHandle);
    PlaySoundMem(soundHandle, DX_PLAYTYPE_BACK);
}

// 弾を1つ追加する(速度0の静止弾。muki は放出時に飛ぶ向き = 勇儀から見て外向き)
static void SanpoAddShot(sEnemyShotSet* pEnemyShotSet, double x, double y, double muki, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = 0.0;
    pEnemyShot->kind = kind;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// 同心円状に弾を並べる。gates 方向の弾は抜く(隙間を作る)
static void SanpoSpawnRings(sEnemyShotSet* pEnemyShotSet, double rMin, double rMax, double rStep,
    double spacing, int kind, const double* gates, int gateNum, double gateHalfWidth)
{
    for (int row = 0; ; row++) {
        double r = rMin + rStep * row;
        if (r > rMax + 1.0e-6) break;
        int n = (int)(2.0 * DX_PI * r / spacing);
        if (n < 6) n = 6;
        double step = 2.0 * DX_PI / n;
        double base = SanpoRand01() * 2.0 * DX_PI; // リングごとに位相をずらす
        for (int k = 0; k < n; k++) {
            double a = base + step * k + (SanpoRand01() - 0.5) * step * 0.3;
            bool inGate = false;
            for (int g = 0; g < gateNum; g++) {
                if (SanpoAngleDiff(a, gates[g]) < gateHalfWidth) {
                    inGate = true;
                    break;
                }
            }
            if (inGate) continue;
            SanpoAddShot(pEnemyShotSet,
                pEnemyShotSet->x + r * cos(a),
                pEnemyShotSet->y + r * sin(a),
                a, kind);
        }
    }
}

// 放出後の動き: 勇儀を中心に外向きへ加速
// param_i[0] = 放出までのフレーム数(ShotSet 生成からの経過)
static void SanpoMoveOutward(sEnemyShotSet* pEnemyShotSet)
{
    const int t = pEnemyShotSet->count - pEnemyShotSet->param_i[0];
    if (t < 0) return; // まだ静止
    double speed = kSanpoSpeedStart + kSanpoSpeedAccel * t;
    if (speed > kSanpoSpeedMax) speed = kSanpoSpeedMax;
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->speed = speed;
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
//  弾幕: 1歩目 / 2歩目 / 3歩目
//  pEnemyShotSet->kind : 奇数セット=0, 偶数セット=1
// ------------------------------------------------------------
// 1歩目: 勇儀の周囲に高密度の静止弾
static void ShotSanpoLayer1(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        SanpoPlaySe(sound_enemyShot_heavy);
        // 弾の色一覧: 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
        const int kind = (pEnemyShotSet->kind % 2 == 0) ? img_enemyShotDiamond[4]  // 青
            : img_enemyShotDiamond[0]; // 赤
        SanpoSpawnRings(pEnemyShotSet, kSanpoLayer1RMin, kSanpoLayer1RMax, kSanpoLayer1RStep,
            kSanpoLayer1Spacing, kind, nullptr, 0, 0.0);
    }
    SanpoMoveOutward(pEnemyShotSet);
}

// 2歩目: 1歩目より外側に、ゲート(隙間)つきのリング
static void ShotSanpoLayer2(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        SanpoPlaySe(sound_enemyShot_heavy);
        const int kind = (pEnemyShotSet->kind % 2 == 0) ? img_enemyShotDiamond[3]  // シアン
            : img_enemyShotDiamond[5]; // マゼンタ
        // 隙間の方向をランダムに決める(ほぼ等間隔に散らす)
        double gates[kSanpoLayer2GateNum];
        const double gateBase = SanpoRand01() * 2.0 * DX_PI;
        for (int g = 0; g < kSanpoLayer2GateNum; g++) {
            gates[g] = gateBase + 2.0 * DX_PI * g / kSanpoLayer2GateNum + (SanpoRand01() - 0.5) * 1.0;
        }
        SanpoSpawnRings(pEnemyShotSet, kSanpoLayer2RMin, kSanpoLayer2RMax, kSanpoLayer2RStep,
            kSanpoLayer2Spacing, kind, gates, kSanpoLayer2GateNum, kSanpoLayer2GateHalf);
    }
    SanpoMoveOutward(pEnemyShotSet);
}

// 3歩目: 2歩目より外側の画面全域を埋める
static void ShotSanpoLayer3(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        SanpoPlaySe(sound_enemyShot_extreme);
        const bool large = (pEnemyShotSet->kind % 2 == 1);
        const int kind = large ? img_enemyShotLargeBall[0]   // 赤の大玉
            : img_enemyShotMediumBall[0]; // 赤の中玉
        const double spacing = large ? kSanpoLayer3SpacingLarge : kSanpoLayer3SpacingMid;
        const double cx = pEnemyShotSet->x;
        const double cy = pEnemyShotSet->y;

        // 画面全体に千鳥格子状に並べ、内側(2歩目の領域)は除く
        for (int iy = 0; iy * spacing <= 480.0; iy++) {
            const double rowOffset = (iy % 2 == 1) ? spacing * 0.5 : 0.0;
            for (int ix = 0; ix * spacing + rowOffset <= 480.0; ix++) {
                const double x = ix * spacing + rowOffset + (SanpoRand01() - 0.5) * spacing * 0.3;
                const double y = iy * spacing + (SanpoRand01() - 0.5) * spacing * 0.3;
                const double dx = x - cx;
                const double dy = y - cy;
                if (sqrt(dx * dx + dy * dy) < kSanpoLayer3InnerR) continue;
                if (kSanpoLayer3SafeRadius > 0.0) {
                    const double px = x - player.x;
                    const double py = y - player.y;
                    if (sqrt(px * px + py * py) < kSanpoLayer3SafeRadius) continue;
                }
                SanpoAddShot(pEnemyShotSet, x, y, atan2(dy, dx), kind);
            }
        }
    }
    SanpoMoveOutward(pEnemyShotSet);
}

// ShotSet を生成してリストに登録する
// releaseDelay: 生成から全弾放出までのフレーム数
static void SanpoCreateShotSet(sEnemyShotSet::PatternFunc func, int kind, int releaseDelay)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;
    pEnemyShotSet->muki = 0.0;
    pEnemyShotSet->kind = kind;
    pEnemyShotSet->param_i[0] = releaseDelay;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ------------------------------------------------------------
//  敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_SanpoHissatsu_Claude()
{
    static int cycle; // 何セット目か(0始まり。偶奇で弾の色・種類が変わる)
    if (count == 1) {
        enemy.x = kSanpoBossX;
        enemy.y = kSanpoBossY;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        cycle = 0;
    }

    const int t = (count - 1) % kSanpoCycleFrames;
    const int kind = cycle % 2;
    if (t == kSanpoStomp1Frame) {
        SanpoCreateShotSet(ShotSanpoLayer1, kind, kSanpoReleaseFrame - kSanpoStomp1Frame);
    }
    else if (t == kSanpoStomp2Frame) {
        SanpoCreateShotSet(ShotSanpoLayer2, kind, kSanpoReleaseFrame - kSanpoStomp2Frame);
    }
    else if (t == kSanpoChargeFrame) {
        SanpoPlaySe(sound_enemyCharge); // 3歩目の予告音
    }
    else if (t == kSanpoStomp3Frame) {
        SanpoCreateShotSet(ShotSanpoLayer3, kind, kSanpoReleaseFrame - kSanpoStomp3Frame);
    }
    else if (t == kSanpoReleaseFrame) {
        SanpoPlaySe(sound_enemyShot_medium); // 全弾放出
    }

    if (t == kSanpoCycleFrames - 1) cycle++;
}