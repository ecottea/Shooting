// ============================================================
//  飛鉢「伝説の飛空円盤」(東方星蓮船 6面ボス 聖白蓮 / Hard・Lunaticのスペルカード)
//
//  【弾幕の構成】
//   ・1ウェーブ = 数個の「円盤」(札弾を円周上に並べたリング)が画面内を飛ぶ
//   ・円盤は生成直後に半径0から kDiscRadius まで広がり(予告を兼ねる)、
//     その後は円ごと直進しつつ、円周上の札がゆっくり自転する
//   ・ウェーブは「固定 → 自機狙い → 自機狙い → 固定 …」の順で繰り返す
//       固定      : 自機位置に依存しない配置・進行方向
//       自機狙い  : 生成した瞬間の自機位置へ向けて円の中心が進む(以後は固定)
//   ・ボスの残りHPに応じて札の色が 青→シアン→黄→マゼンタ→赤 と変化し、
//     発射間隔は短く、円の数・1円あたりの札の数・速度は増える
//   ・札弾の素材は無いので、近い形の鱗弾で代用している
//
//  【エンジン仕様に対する前提】
//   ・count / pEnemyShotSet->count / pEnemyShot->count の加算と
//     画面外の弾の消去はメインルーチン側で行う
//   ・弾の位置は円盤の中心位置から毎フレーム解析的に求めるので、
//     弾自身の x,y を積算することはしない
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
//  調整用定数(難易度や見た目はここを触る)
// ------------------------------------------------------------
static const double kDiscFieldSize = 480.0;  // ゲーム画面は 480x480
static const double kDiscRadius = 72.0;   // 円盤の最終半径
static const int    kDiscExpandFrame = 40;     // 半径0 → kDiscRadius に広がるまでのフレーム数
static const double kDiscSpawnMargin = 40.0;   // 円盤の中心を生成する位置の画面端からの余白
// (画面外消去で生成直後に弾が欠ける場合は大きくする)
static const int    kDiscFirstDelay = 90;     // スペル開始から最初のウェーブまでのフレーム数
static const int    kDiscChargeFrame = 30;     // 予告音を鳴らす、ウェーブの何フレーム前か
static const double kDiscAimedDist = 220.0;  // 自機狙い: 自機から生成位置までの距離
static const double kDiscAimedMinDist = 150.0; // 自機狙い: 生成位置と自機の最低距離
static const double kDiscAimedSpacing = 96.0;  // 自機狙い: 並べる円盤同士の横方向の間隔

// HP段階(0〜4)ごとの札の色: 青(4) → シアン(3) → 黄(1) → マゼンタ(5) → 赤(0)
static const int kDiscStageColor[5] = { 4, 3, 1, 5, 0 };

// ------------------------------------------------------------
//  補助関数
// ------------------------------------------------------------
static double DiscClamp(double v, double lo, double hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// 残りHPから攻撃の段階(0:満タン 〜 4:瀕死)を返す
static int DiscGetStage()
{
    if (enemy.maxHp <= 0) return 0;
    double lost = 1.0 - (double)enemy.hp / (double)enemy.maxHp;
    int stage = (int)(lost * 5.0);
    if (stage < 0) stage = 0;
    if (stage > 4) stage = 4;
    return stage;
}

// ------------------------------------------------------------
//  弾幕：円盤(ShotSet 1個 = 円盤1枚)
//   ShotSet の param_d[0]=生成時の中心x  [1]=中心y  [2]=速度x  [3]=速度y
//                    [4]=最終半径  [5]=自転の初期位相  [6]=自転速度(rad/フレーム)
//            param_i[0]=札の数  [1]=色  [2]=1なら生成時に効果音を鳴らす
//   Shot    の param_d[0]=円周上の基準角(0〜2π)
// ------------------------------------------------------------
static void ShotFlyingDisc(sEnemyShotSet* pEnemyShotSet)
{
    const int    num = pEnemyShotSet->param_i[0];
    const int    color = pEnemyShotSet->param_i[1];
    const double startX = pEnemyShotSet->param_d[0];
    const double startY = pEnemyShotSet->param_d[1];
    const double velX = pEnemyShotSet->param_d[2];
    const double velY = pEnemyShotSet->param_d[3];
    const double radius = pEnemyShotSet->param_d[4];
    const double phase = pEnemyShotSet->param_d[5];
    const double spin = pEnemyShotSet->param_d[6];
    const int    t = pEnemyShotSet->count;

    if (t == 0) {
        if (pEnemyShotSet->param_i[2]) {
            // 使える効果音: sound_enemyShot_light/medium/heavy/extreme, sound_enemyCharge(予告音)
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
        for (int i = 0; i < num; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = startX;
            pEnemyShot->y = startY;
            pEnemyShot->muki = 0.0;   // 直後のループで毎フレーム設定する
            pEnemyShot->speed = 0.0;
            // 札弾の素材は無いので、長方形に近い鱗弾(4.0x3.0)で代用
            pEnemyShot->kind = img_enemyShotScale[color];
            pEnemyShot->param_d[0] = 2.0 * DX_PI * (double)i / (double)num;
            pEnemyShot->margin = 240;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 円盤の中心・半径・自転角を t から求める(半径は ease-out で広がる)
    double u = (double)t / (double)kDiscExpandFrame;
    if (u > 1.0) u = 1.0;
    const double r = radius * u * (2.0 - u);
    const double cx = startX + velX * (double)t;
    const double cy = startY + velY * (double)t;
    const double rot = phase + spin * (double)t;
    const double spd = sqrt(velX * velX + velY * velY);
    pEnemyShotSet->x = cx;
    pEnemyShotSet->y = cy;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        const double ang = pShot->param_d[0] + rot;
        pShot->x = cx + r * cos(ang);
        pShot->y = cy + r * sin(ang);
        // 見た目の向きは円周の接線方向(円のラインに札が連なって見える)。
        // 実際の移動は円盤全体の直進+自転なので、向きと進行方向は一致しない。
        pShot->muki = ang + DX_PI / 2.0;
        pShot->speed = spd;
        pShot = pShot->next;
    }
}

// 円盤を1枚生成する
static void SpawnDisc(double x, double y, double velX, double velY,
    double spin, double phase, int num, int color, int withSound)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotFlyingDisc;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = atan2(velY, velX);
    pEnemyShotSet->kind = color;
    pEnemyShotSet->param_i[0] = num;
    pEnemyShotSet->param_i[1] = color;
    pEnemyShotSet->param_i[2] = withSound;
    pEnemyShotSet->param_d[0] = x;
    pEnemyShotSet->param_d[1] = y;
    pEnemyShotSet->param_d[2] = velX;
    pEnemyShotSet->param_d[3] = velY;
    pEnemyShotSet->param_d[4] = kDiscRadius;
    pEnemyShotSet->param_d[5] = phase;
    pEnemyShotSet->param_d[6] = spin;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// 固定ウェーブ: 自機位置に依存しない配置と進行方向
//   fixedIdx が偶数: 画面上部から扇状に下へ  /  奇数: 左右の画面端から交互に斜め下へ
static void SpawnFixedWave(int fixedIdx, int stage, int rings, int num, double spd)
{
    const int    color = kDiscStageColor[stage];
    const double spinBase = 0.016 + 0.003 * (double)stage;
    for (int k = 0; k < rings; k++) {
        const double c = (double)k - (double)(rings - 1) * 0.5;
        double x, y, ang;
        if (fixedIdx % 2 == 0) {
            x = kDiscFieldSize * 0.5 + c * (380.0 / (double)rings);
            y = kDiscSpawnMargin;
            ang = DX_PI / 2.0 + c * 0.20;
        }
        else if (k % 2 == 0) {
            x = kDiscSpawnMargin;
            y = kDiscSpawnMargin + 34.0 * (double)k;
            ang = 0.45 + 0.05 * (double)(k / 2);
        }
        else {
            x = kDiscFieldSize - kDiscSpawnMargin;
            y = kDiscSpawnMargin + 34.0 * (double)k;
            ang = DX_PI - (0.45 + 0.05 * (double)(k / 2));
        }
        const double spin = (k % 2 == 0 ? 1.0 : -1.0) * spinBase;
        SpawnDisc(x, y, cos(ang) * spd, sin(ang) * spd, spin, 0.7 * (double)k, num, color, k == 0);
    }
}

// 自機狙いウェーブ: 自機の周囲の一方向から、横に並べた円盤を自機へ向けて飛ばす
//   各円盤は生成した瞬間の自機位置(を横にずらした点)へ向かい、以後は真っ直ぐ進む。
static void SpawnAimedWave(int stage, int rings, int num, double spd)
{
    const int    color = kDiscStageColor[stage];
    const double spinBase = 0.016 + 0.003 * (double)stage;
    const double lo = kDiscSpawnMargin;
    const double hi = kDiscFieldSize - kDiscSpawnMargin;

    // 生成方向: 自機から遠くに置ける方向を選ぶ(画面端で自機に近づきすぎる場合は反対側)
    // GetRand(359) は 0〜359 の360通りなので度数法の角度としてそのまま使える
    const double dir0 = (double)GetRand(359) * DX_PI / 180.0;
    double bestAng = dir0, bestDist = -1.0;
    for (int a = 0; a < 2; a++) {
        const double ang = dir0 + (double)a * DX_PI;
        const double bx = DiscClamp(player.x + kDiscAimedDist * cos(ang), lo, hi);
        const double by = DiscClamp(player.y + kDiscAimedDist * sin(ang), lo, hi);
        const double d = sqrt((bx - player.x) * (bx - player.x) + (by - player.y) * (by - player.y));
        if (d > bestDist) { bestDist = d; bestAng = ang; }
        if (d >= kDiscAimedMinDist) break;
    }
    const double baseX = DiscClamp(player.x + kDiscAimedDist * cos(bestAng), lo, hi);
    const double baseY = DiscClamp(player.y + kDiscAimedDist * sin(bestAng), lo, hi);
    const double perpX = cos(bestAng + DX_PI / 2.0);
    const double perpY = sin(bestAng + DX_PI / 2.0);

    for (int k = 0; k < rings; k++) {
        const double off = ((double)k - (double)(rings - 1) * 0.5) * kDiscAimedSpacing;
        const double sx = DiscClamp(baseX + perpX * off, lo, hi);
        const double sy = DiscClamp(baseY + perpY * off, lo, hi);
        // 狙う点は自機を中心に、横ずれの半分だけ散らす(円盤が並んで自機の周りを通過する)
        const double tx = player.x + perpX * off * 0.5;
        const double ty = player.y + perpY * off * 0.5;
        const double ang = atan2(ty - sy, tx - sx);
        const double spin = (k % 2 == 0 ? 1.0 : -1.0) * spinBase;
        SpawnDisc(sx, sy, cos(ang) * spd, sin(ang) * spd, spin, 0.7 * (double)k, num, color, k == 0);
    }
}

// ------------------------------------------------------------
//  敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_HikouEnban_Claude()
{
    static int  wave;        // 何ウェーブ目か
    static int  fixedCount;  // 固定ウェーブを撃った回数
    static int  timer;       // 前回のウェーブからの経過フレーム
    static bool charged;     // 今回のウェーブの予告音を鳴らしたか

    if (count == 1) {
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        wave = 0;
        fixedCount = 0;
        charged = false;
        timer = 150 - kDiscFirstDelay; // 段階0の発射間隔(150)から逆算して最初のウェーブを遅らせる
    }

    // ボスは画面上部でゆっくり左右に揺れる
    enemy.x = kDiscFieldSize * 0.5 + 30.0 * sin((double)count / 80.0);
    enemy.y = 90.0;

    // 攻撃の段階に応じた強さ
    const int    stage = DiscGetStage();
    const int    interval = 150 - 14 * stage;      // 発射間隔: 150 → 94 フレーム
    const int    rings = 3 + (stage + 1) / 2;   // 1ウェーブの円盤の数: 3 → 5
    const int    num = 22 + 3 * stage;        // 1円盤の札の数: 22 → 34
    const double spd = 1.5 + 0.2 * (double)stage; // 円盤の移動速度: 1.5 → 2.3

    timer++;
    if (!charged && timer >= interval - kDiscChargeFrame) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
        charged = true;
    }
    if (timer >= interval) {
        timer = 0;
        charged = false;
        // 固定 → 自機狙い → 自機狙い → 固定 の4ウェーブ周期
        const int phase = wave % 4;
        if (phase == 0 || phase == 3) {
            SpawnFixedWave(fixedCount++, stage, rings, num, spd);
        }
        else {
            SpawnAimedWave(stage, rings, num, spd);
        }
        wave++;
    }
}