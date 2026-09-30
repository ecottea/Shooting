// enemyPat_typhoon.cpp
// 台風モチーフ弾幕「台風の眼」
// フェーズ1:接近(0〜15秒) / フェーズ2:最盛期(15〜40秒)
// フェーズ3:眼の通過(40〜55秒) / フェーズ4:通過後の強風(55〜60秒)
// ※ count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと
//   画面外弾の消去はメインルーチンで行われる前提。
// ※ 空になった弾セットはメインルーチン側で破棄される前提。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static const double TAU = DX_PI * 2.0;

// ---- サイクル定義(60fps想定 / 全60秒でループ) ----
static const int CYCLE = 2000; // 1ループ = 60秒
static const int T_PHASE1_END = 600;  // 15秒 接近終了
static const int T_PHASE2_END = 1200; // 40秒 最盛期終了
static const int T_PHASE3_END = 1800; // 55秒 眼の通過終了(以降は強風)

// ---- 素材(色index: 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙) ----
static int KIND_RAIN_SMALL = img_enemyShotSmallBall[3];   // 雨(小玉・シアン)
static int KIND_RAIN_MEDIUM = img_enemyShotMediumOval[4];  // 風に流される雨(中楕円・青)
static int KIND_EYE_WALL = img_enemyShotLargeBall[6];   // 眼の壁(大玉・白)
static int KIND_BURST = img_enemyShotMediumOval[5];  // 壁の破裂(中楕円・マゼンタ)
static int KIND_GUST = img_enemyShotLaser[6];       // 突風(短レーザー・白)

// ---- 効果音再生ヘルパ ----
static void PlayShotSound(int soundHandle)
{
    if (CheckSoundMem(soundHandle)) StopSoundMem(soundHandle);
    PlaySoundMem(soundHandle, DX_PLAYTYPE_BACK);
}

// ---- 弾セット生成ヘルパ ----
static sEnemyShotSet* CreateShotSet(double x, double y, double muki, int kind,
    void (*patternFunc)(sEnemyShotSet*))
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = patternFunc;
    pSet->x = x;
    pSet->y = y;
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

// ---- 弾生成ヘルパ(生成した弾のポインタを返す) ----
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// ---- 共通移動処理(フェーズ4の強風で全弾が同一方向へ流される) ----
static void MoveShotsCommon(sEnemyShotSet* pSet)
{
    bool wind = (((count - 1) % CYCLE) >= T_PHASE3_END);
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        if (wind) { pShot->x += 2.2; pShot->y -= 0.2; } // 強風で横に流される
        pShot = pShot->next;
    }
}

// ============================================================
// フェーズ1:接近 — 仮想渦の中心かららせん状に雨(小玉)が流入
// param_d[0]/[1]: 渦の中心の移動速度  param_d[2]: 渦の回転速度
// ============================================================
static void ShotSpiralRain(sEnemyShotSet* pSet)
{
    int t = (count - 1) % CYCLE;
    if (t < T_PHASE1_END) {
        // 渦の中心(台風本体)を画面中央へゆっくり接近させる
        pSet->x += pSet->param_d[0];
        pSet->y += pSet->param_d[1];
        if (pSet->count % 3 == 0) {
            double base = pSet->count * pSet->param_d[2];
            for (int i = 0; i < 2; i++) {
                // 角度に揺らぎを入れて「横殴りの雨」感を出す
                double ang = base + i * DX_PI + (GetRand(10) - 5) / 180.0 * DX_PI;
                double sp = 1.4 + GetRand(60) / 100.0;
                AddShot(pSet, pSet->x + cos(ang) * 30.0, pSet->y + sin(ang) * 30.0, ang, sp, KIND_RAIN_SMALL);
            }
        }
    }
    MoveShotsCommon(pSet);
}

// ============================================================
// フェーズ2:最盛期 — 螺旋腕(中楕円弾)が回転しながら広がる
// param_i[0]: 腕の本数  param_d[0]: 腕の基準角  param_d[1]: 回転速度
// ============================================================
static void ShotSpiralArms(sEnemyShotSet* pSet)
{
    int t = (count - 1) % CYCLE;
    if (t >= T_PHASE1_END && t < T_PHASE2_END) {
        // フェーズ2の途中で回転方向を反転(台風の不規則な揺らぎ)
        double dir = (t < 1650) ? 1.0 : -1.0;
        pSet->param_d[0] += pSet->param_d[1] * dir;
        if (pSet->count % 4 == 0) {
            int arms = pSet->param_i[0];
            for (int i = 0; i < arms; i++) {
                double ang = pSet->param_d[0] + i * (TAU / arms);
                AddShot(pSet, enemy.x + cos(ang) * 20.0, enemy.y + sin(ang) * 20.0, ang, 2.2, KIND_RAIN_MEDIUM);
            }
        }
    }
    MoveShotsCommon(pSet);
}

// ============================================================
// 突風 — 短レーザーがプレイヤーめがけて直線的に突き抜ける
// ============================================================
static void ShotGust(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        double ang = atan2(player.y - pSet->y, player.x - pSet->x)
            + (GetRand(20) - 10) / 180.0 * DX_PI; // 狙いに揺らぎ
        AddShot(pSet, pSet->x, pSet->y, ang, 9.0, KIND_GUST);
        PlayShotSound(sound_enemyShot_light);
    }
    MoveShotsCommon(pSet);
}

// ============================================================
// フェーズ3:眼の通過 — 大玉リングが収縮→眼の内部から雨が噴出→破裂
// param_i[0]: stage(0:収縮 1:停止+噴出 2:破裂済み)
// param_i[1]: 壁の弾数
// param_d[0]: 現在半径  param_d[1]/[2]: 中心x/y
// 各リング弾の param_d[0]: 自分の円周上の角度 / param_i[0]: 1=リング以外の弾
// ============================================================
static void ShotEyeRing(sEnemyShotSet* pSet)
{
    int stage = pSet->param_i[0];

    if (stage == 0) {
        // ゆっくり収縮
        pSet->param_d[0] -= 0.3;
        if (pSet->param_d[0] <= 120.0) {
            pSet->param_d[0] = 120.0;
            pSet->param_i[0] = 1;
        }
    }
    else if (stage == 1) {
        // 眼の中から不規則な雨(小玉)が噴出
        if (pSet->count % 6 == 0) {
            int n = 1 + GetRand(1); // 1〜2発
            for (int i = 0; i < n; i++) {
                double ang = GetRand(359) / 180.0 * DX_PI;
                double sp = 1.5 + GetRand(200) / 100.0;
                double r = GetRand(50);
                sEnemyShot* pShot = AddShot(pSet,
                    pSet->param_d[1] + cos(ang) * r,
                    pSet->param_d[2] + sin(ang) * r, ang, sp, KIND_RAIN_SMALL);
                pShot->param_i[0] = 1; // リング弾ではない印
            }
        }
        // 破裂タイミング
        if (pSet->count >= 650) {
            pSet->param_i[0] = 2;
            // 先にリング弾の情報を退避してから新しい弾を追加する
            // (リストへ追加しながら走査すると新規弾まで処理してしまうため)
            const int NMAX = 32;
            double bx[NMAX], by[NMAX], ba[NMAX];
            int n = 0;
            sEnemyShot* pShot = pSet->pEnemyShotHead->next;
            while (pShot != pSet->pEnemyShotHead && n < NMAX) {
                bx[n] = pShot->x;
                by[n] = pShot->y;
                ba[n] = pShot->param_d[0];
                pShot->speed = 5.5;              // 壁も弾けて外へ飛ぶ
                pShot->muki = pShot->param_d[0];
                n++;
                pShot = pShot->next;
            }
            for (int i = 0; i < n; i++) {
                for (int k = -1; k <= 1; k++) {  // 最も危険な一撃
                    AddShot(pSet, bx[i], by[i], ba[i] + k * 0.22, 4.5, KIND_BURST);
                }
            }
            PlayShotSound(sound_enemyShot_extreme);
        }
    }

    if (pSet->param_i[0] < 2) {
        // stage 0/1: リング弾は円周上に直接配置、それ以外は通常移動
        double r = pSet->param_d[0];
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0) {
                pShot->x = pSet->param_d[1] + r * cos(pShot->param_d[0]);
                pShot->y = pSet->param_d[2] + r * sin(pShot->param_d[0]);
            }
            else {
                pShot->x += pShot->speed * cos(pShot->muki);
                pShot->y += pShot->speed * sin(pShot->muki);
            }
            pShot = pShot->next;
        }
    }
    else {
        // stage 2: 破裂後は全弾通常移動(リング弾は画面外へ抜けて自動消去)
        MoveShotsCommon(pSet);
    }
}

// ============================================================
// フェーズ3後半:通過後の残り雨 — 上からぽつぽつ降る
// ============================================================
static void ShotResidueRain(sEnemyShotSet* pSet)
{
    int t = (count - 1) % CYCLE;
    if (t >= 3120 && t < T_PHASE3_END && pSet->count % 18 == 0) {
        for (int i = 0; i < 3; i++) {
            double x = GetRand(440) + 20;
            double ang = DX_PI / 2.0 + (GetRand(20) - 10) / 180.0 * DX_PI;
            double sp = 2.0 + GetRand(100) / 100.0;
            AddShot(pSet, x, -10.0, ang, sp, KIND_RAIN_SMALL);
        }
    }
    MoveShotsCommon(pSet);
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Tmp()
{
    int t = (count - 1) % CYCLE; // サイクル内の経過フレーム(0開始)

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        KIND_RAIN_SMALL = img_enemyShotSmallBall[3];   // 雨(小玉・シアン)
        KIND_RAIN_MEDIUM = img_enemyShotMediumOval[4];  // 風に流される雨(中楕円・青)
        KIND_EYE_WALL = img_enemyShotLargeBall[6];   // 眼の壁(大玉・白)
        KIND_BURST = img_enemyShotMediumOval[5];  // 壁の破裂(中楕円・マゼンタ)
        KIND_GUST = img_enemyShotLaser[6];       // 突風(短レーザー・白)
    }

    // ---- 敵の移動 ----
    if (t < T_PHASE2_END) {
        // 上部を滑らかに左右に揺れる
        enemy.x = 240.0 + 120.0 * sin((count % 240) / 240.0 * TAU);
        enemy.y = 40.0;
    }
    else {
        // 眼のフェーズは中央上部へ落ち着く
        enemy.x += (240.0 - enemy.x) * 0.05;
    }

    // ---- フェーズごとの弾幕セット生成 ----
    if (t == 0) {
        // フェーズ1:左右から渦を巻いて接近する雨
        PlayShotSound(sound_enemyShot_light);
        sEnemyShotSet* pL = CreateShotSet(80.0, 120.0, 0, 0, ShotSpiralRain);
        pL->param_d[0] = (240.0 - 80.0) / T_PHASE1_END;   // 中心の移動速度x
        pL->param_d[1] = (240.0 - 120.0) / T_PHASE1_END;  // 中心の移動速度y
        pL->param_d[2] = 0.12;                            // 渦の回転速度
        sEnemyShotSet* pR = CreateShotSet(400.0, 120.0, 0, 0, ShotSpiralRain);
        pR->param_d[0] = (240.0 - 400.0) / T_PHASE1_END;
        pR->param_d[1] = (240.0 - 120.0) / T_PHASE1_END;
        pR->param_d[2] = -0.12;                           // 反対回転
    }
    if (t == T_PHASE1_END) {
        // フェーズ2:螺旋腕(4本)
        PlayShotSound(sound_enemyShot_heavy);
        sEnemyShotSet* pArms = CreateShotSet(0, 0, 0, 0, ShotSpiralArms);
        pArms->param_i[0] = 4;    // 腕の本数(3=易 / 4=普通 / 5〜7=激)
        pArms->param_d[1] = 0.05; // 回転速度
    }
    if (t >= T_PHASE1_END + 60 && t < T_PHASE2_END && (t - (T_PHASE1_END + 60)) % 90 == 0) {
        // フェーズ2:突風(レーザー)を断続的に撃つ
        CreateShotSet(enemy.x, enemy.y + 10.0, 0, 0, ShotGust);
    }
    if (t == T_PHASE2_END) {
        // フェーズ3:台風の眼
        PlayShotSound(sound_enemyCharge); // 予告音
        sEnemyShotSet* pEye = CreateShotSet(0, 0, 0, 0, ShotEyeRing);
        pEye->param_d[0] = 230.0; // 初期半径
        pEye->param_d[1] = 240.0; // 中心x
        pEye->param_d[2] = 240.0; // 中心y
        pEye->param_i[0] = 0;     // stage
        pEye->param_i[1] = 24;    // 壁の弾数
        for (int i = 0; i < 24; i++) {
            sEnemyShot* pShot = AddShot(pEye, 0, 0, 0, 0.0, KIND_EYE_WALL);
            pShot->param_d[0] = i * TAU / 24.0; // 自分の円周上の角度
        }
    }
    if (t == T_PHASE3_END - 180) {
        // フェーズ3後半:残り雨
        CreateShotSet(0, 0, 0, 0, ShotResidueRain);
    }
}