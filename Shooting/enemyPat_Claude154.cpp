// ============================================================
//  enemyPat_sumo.cpp
//  スペルカード「相撲符『大一番・寄り切り』」
//
//  ・画面いっぱいの土俵を、俵を模した敵弾の輪で表現する(自転・縮小・破壊と復活あり)
//  ・ボスが予告線のあと自機へ突進し、かわされると俵に激突してスタンする
//  ・ボスと自機が近いときだけ斥力が働く(自機は軽く、ボスは重い)
//  ・ボスのHPと激突回数で難易度が3段階に上がる(仕切り〜土俵際〜決まり手)
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include "player.h"
#include <math.h>

// ------------------------------------------------------------
//  定数 (CX/CY など Windows SDK と衝突しそうな短い名前は避ける)
// ------------------------------------------------------------
static constexpr double kFieldCenterX = 240.0;   // 土俵の中心
static constexpr double kFieldCenterY = 240.0;
static constexpr int    kTawaraNum = 112;        // 俵の数
static constexpr double kRingRadiusMax = 232.0;  // 土俵の最大半径(画面いっぱい)
static constexpr double kRingRadiusMin = 150.0;  // 土俵の最小半径
static constexpr double kRingShrinkStep = 12.0;  // 激突1回ごとに縮む量
static constexpr double kBossBodyRadius = 18.0;  // ボスが俵に激突したとみなす余白
static constexpr int    kTawaraRespawnFrame = 150; // 砕けた俵が復活するまで
static constexpr int    kStunFrame = 150;        // 激突後のスタン時間
static constexpr int    kYoriFrame = 252;        // 寄りフェーズの長さ(最終段階)

// 難易度(0:ぶつかり稽古, 1:土俵際, 2:決まり手)ごとのパラメータ
static const double kForceRadius[3] = { 60.0, 78.0, 95.0 };  // 斥力が働く距離
static const int    kTeleLen[3] = { 75, 70, 55 };             // 予告線の長さ(フレーム)
static const double kChargeMax[3] = { 6.0, 6.8, 7.4 };        // 突進の最高速度

// ボスの状態
enum {
    SUMO_ENTER,      // 入場
    SUMO_SHIKIRI,    // 仕切り(塩まき)
    SUMO_TELEGRAPH,  // 予告線
    SUMO_CHARGE,     // 突進
    SUMO_STUN,       // 激突後のスタン
    SUMO_RETURN,     // 土俵中央へ戻る
    SUMO_YORI        // 寄り(決まり手/最終段階のみ)
};

// ------------------------------------------------------------
//  ファイルスコープの状態
// ------------------------------------------------------------
static int    s_state;
static int    s_stateCount;
static int    s_level;
static int    s_crashNum;           // 俵への激突回数
static int    s_waveIdx;            // 掛け声の波の回数
static double s_ringRadius;         // 現在の土俵の半径
static double s_ringTargetRadius;   // 縮小の目標半径
static double s_ringRotate;         // 俵の自転角(時計回り)
static double s_ringShake;          // 激突時の揺れの大きさ
static double s_ringCx, s_ringCy;   // 揺れを含む現在の土俵中心
static int    s_tawaraTimer[kTawaraNum]; // -1:存在する, 0以上:復活までの残りフレーム
static sEnemyShotSet* s_pTawaraSet; // 俵の弾セット(常に弾を持つので消されない)
static double s_chargeAngle;        // 突進の向き
static double s_feintAngle;         // フェイントの向き
static double s_chargeSpeed;        // 突進の現在速度
static double s_repelF;             // 直前フレームの斥力の大きさ(ボスの減速に使う)

// ------------------------------------------------------------
//  小物ユーティリティ
// ------------------------------------------------------------
static double AngleToPlayer(double x, double y)
{
    return atan2(player.y - y, player.x - x);
}

// 低速移動(踏ん張り)中か。自機の低速キーに合わせて書き換えること。
static bool IsPlayerFocusing()
{
    return key[KEY_INPUT_LSHIFT] > 0 || key[KEY_INPUT_RSHIFT] > 0;
}

static void PlaySe(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

static void ChangeState(int next)
{
    s_state = next;
    s_stateCount = 0; // 次フレームの先頭でインクリメントされ、新状態の最初のフレームが1になる
}

// 土俵中心からlimit以内に収める
static void ClampInRing(double* pX, double* pY, double limit)
{
    double dx = *pX - s_ringCx;
    double dy = *pY - s_ringCy;
    double d = sqrt(dx * dx + dy * dy);
    if (d > limit && d > 0.001) {
        *pX = s_ringCx + dx * limit / d;
        *pY = s_ringCy + dy * limit / d;
    }
}

// ------------------------------------------------------------
//  弾セット・弾の生成と削除
// ------------------------------------------------------------
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc func, double x, double y, double muki, int kind)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
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

// 弾をリストから外して解放する(ループ中は事前にnextを退避しておくこと)
static void RemoveShot(sEnemyShot* pShot)
{
    pShot->prev->next = pShot->next;
    pShot->next->prev = pShot->prev;
    delete pShot;
}

// ------------------------------------------------------------
//  弾幕：直進(張り手・掛け声・砕けた俵・予告線で共用)
// ------------------------------------------------------------
static void ShotLinear(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 弾幕：塩まき(放物線を描いて落ち、着地したら消える)
//   param_d[0]:x速度, param_d[1]:y速度, param_d[2]:重力, param_i[0]:着地までのフレーム数
static void ShotSalt(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;
        pShot->param_d[1] += pShot->param_d[2];
        pShot->x += pShot->param_d[0];
        pShot->y += pShot->param_d[1];
        pShot->muki = atan2(pShot->param_d[1], pShot->param_d[0]);
        if (pShot->count >= pShot->param_i[0]) {
            RemoveShot(pShot);
        }
        pShot = pNext;
    }
}

// ------------------------------------------------------------
//  弾幕：土俵(俵の輪)
//   param_i[0]:俵のスロット番号
// ------------------------------------------------------------
static void GetTawaraPos(int slot, double* pAngle, double* pX, double* pY)
{
    double a = s_ringRotate + (double)slot * (2.0 * DX_PI / kTawaraNum);
    *pAngle = a;
    *pX = s_ringCx + s_ringRadius * cos(a);
    *pY = s_ringCy + s_ringRadius * sin(a);
}

static void SpawnTawara(sEnemyShotSet* pEnemyShotSet, int slot)
{
    double a, x, y;
    GetTawaraPos(slot, &a, &x, &y);
    // 8個に1個だけ黄色にして縄目っぽく見せる
    int kind = (slot % 8 == 0) ? img_enemyShotMediumOval[1] : img_enemyShotMediumOval[8];
    sEnemyShot* pShot = AddShot(pEnemyShotSet, x, y, a + DX_PI * 0.5, 0.0, kind); // 向きは円周の接線方向
    pShot->param_i[0] = slot;
}

static void ShotTawara(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        for (int i = 0; i < kTawaraNum; i++) {
            s_tawaraTimer[i] = -1;
            SpawnTawara(pEnemyShotSet, i);
        }
    }
    // 砕けた俵の復活(自機が近すぎるときは重なって即被弾しないよう待つ)
    for (int i = 0; i < kTawaraNum; i++) {
        if (s_tawaraTimer[i] < 0) continue;
        if (s_tawaraTimer[i] > 0) s_tawaraTimer[i]--;
        if (s_tawaraTimer[i] == 0) {
            double a, x, y;
            GetTawaraPos(i, &a, &x, &y);
            double dx = player.x - x;
            double dy = player.y - y;
            if (dx * dx + dy * dy > 30.0 * 30.0) {
                SpawnTawara(pEnemyShotSet, i);
                s_tawaraTimer[i] = -1;
            }
        }
    }
    // 位置更新(自転・縮小・揺れを反映)
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        double a, x, y;
        GetTawaraPos(pShot->param_i[0], &a, &x, &y);
        pShot->x = x;
        pShot->y = y;
        pShot->muki = a + DX_PI * 0.5;
        pShot = pShot->next;
    }
}

// 激突地点の周辺の俵を砕き、内側へ破片を飛ばす
static void BreakTawara(int slotCenter, int half)
{
    if (s_pTawaraSet == nullptr) return;
    sEnemyShotSet* pBurst = CreateShotSet(ShotLinear, s_ringCx, s_ringCy, 0.0, 0);
    sEnemyShot* pShot = s_pTawaraSet->pEnemyShotHead->next;
    while (pShot != s_pTawaraSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;
        int slot = pShot->param_i[0];
        int diff = slot - slotCenter;
        if (diff > kTawaraNum / 2) diff -= kTawaraNum;
        if (diff < -kTawaraNum / 2) diff += kTawaraNum;
        if (diff <= half && diff >= -half) {
            double inward = atan2(s_ringCy - pShot->y, s_ringCx - pShot->x);
            for (int j = 0; j < 6; j++) {
                double a = inward + ((double)j - 2.5) * 0.24;
                double sp = 1.3 + GetRand(180) / 100.0; // 1.3〜3.1
                int kind;
                switch (j % 3) {
                case 0:  kind = img_enemyShotMediumOval[8]; break; // 俵のかけら
                case 1:  kind = img_enemyShotScale[1];      break; // 藁くず
                default: kind = img_enemyShotSmallBall[6];  break; // 砂ぼこり
                }
                AddShot(pBurst, pShot->x, pShot->y, a, sp, kind);
            }
            s_tawaraTimer[slot] = kTawaraRespawnFrame;
            RemoveShot(pShot);
        }
        pShot = pNext;
    }
}

// ------------------------------------------------------------
//  攻撃の発射
// ------------------------------------------------------------
// 1発だけ直進する弾(予告線の点列用)
static void EmitLinearOne(double x, double y, double muki, double speed, int kind)
{
    sEnemyShotSet* pSet = CreateShotSet(ShotLinear, x, y, muki, 0);
    AddShot(pSet, x, y, muki, speed, kind);
}

// 塩まき：自機の周囲へ放物線で降らせる
static void ThrowSalt()
{
    sEnemyShotSet* pSet = CreateShotSet(ShotSalt, enemy.x, enemy.y, 0.0, 0);
    const double g = 0.04;
    for (int i = 0; i < 9; i++) {
        int flightFrame = 60 + GetRand(30);               // 60〜90フレームで着地
        double ta = GetRand(359) * DX_PI / 180.0;
        double tr = 25.0 + GetRand(90);                   // 自機から25〜115離れた地点を狙う
        double tx = player.x + tr * cos(ta);
        double ty = player.y + tr * sin(ta);
        ClampInRing(&tx, &ty, s_ringRadius - 20.0);        // 土俵の外には落とさない
        sEnemyShot* pShot = AddShot(pSet, enemy.x, enemy.y, -DX_PI * 0.5, 0.0, img_enemyShotSmallBall[6]);
        pShot->param_d[0] = (tx - enemy.x) / flightFrame;
        pShot->param_d[1] = (ty - enemy.y - g * flightFrame * (flightFrame + 1) * 0.5) / flightFrame;
        pShot->param_d[2] = g;
        pShot->param_i[0] = flightFrame;
    }
}

// 張り手：突進するボスの背後へ扇状の高速弾
static void EmitHariteFan()
{
    sEnemyShotSet* pSet = CreateShotSet(ShotLinear, enemy.x, enemy.y, s_chargeAngle + DX_PI, 0);
    int n = 9 + s_level * 2;
    double spread = DX_PI * (0.50 + 0.08 * s_level);
    for (int j = 0; j < n; j++) {
        double a = s_chargeAngle + DX_PI + ((double)j - (n - 1) * 0.5) * (spread / (n - 1));
        double sp = 3.2 + (j % 3) * 0.7;
        int kind = (j % 2 == 0) ? img_enemyShotScale[0] : img_enemyShotScale[8];
        AddShot(pSet, enemy.x, enemy.y, a, sp, kind);
    }
}

// 掛け声：ボスを中心に広がる同心円の波(自機側からずれた位置に隙間)
static void EmitKakegoe()
{
    double gapCenter = AngleToPlayer(enemy.x, enemy.y) + ((s_waveIdx % 2 == 0) ? 1.0 : -1.0) * 1.0;
    int color = (s_waveIdx % 2 == 0) ? 8 : 1;
    const int n = 60/4;
    for (int layer = 0; layer < 2; layer++) {
        sEnemyShotSet* pSet = CreateShotSet(ShotLinear, enemy.x, enemy.y, 0.0, 0);
        double speed = (layer == 0) ? 2.4 : 1.8;
        for (int i = 0; i < n; i++) {
            double a = 2.0 * DX_PI * i / n + layer * DX_PI / n; // 層ごとに半ピッチずらす
            double diff = a - gapCenter;
            while (diff > DX_PI) diff -= 2.0 * DX_PI;
            while (diff < -DX_PI) diff += 2.0 * DX_PI;
            if (diff < 0.32 && diff > -0.32) continue;          // 隙間
            int kind = (layer == 0) ? img_enemyShotMediumBall[color] : img_enemyShotSmallBall[color];
            AddShot(pSet, enemy.x, enemy.y, a, speed, kind);
        }
    }
    s_waveIdx++;
}

// ------------------------------------------------------------
//  俵への激突
// ------------------------------------------------------------
static void OnCrash(double phi)
{
    // ボスを土俵際ぎりぎりに置く
    double rr = s_ringRadius - kBossBodyRadius;
    enemy.x = s_ringCx + rr * cos(phi);
    enemy.y = s_ringCy + rr * sin(phi);

    PlaySe(sound_enemyShot_extreme);

    // 土俵を揺らし、次の目標半径を決める
    s_ringShake = 6.0;
    s_crashNum++;
    s_ringTargetRadius = kRingRadiusMax - kRingShrinkStep * s_crashNum;
    if (s_ringTargetRadius < kRingRadiusMin) s_ringTargetRadius = kRingRadiusMin;

    // 激突地点の俵を砕く
    const double step = 2.0 * DX_PI / kTawaraNum;
    int slot = (int)floor((phi - s_ringRotate) / step + 0.5);
    slot = ((slot % kTawaraNum) + kTawaraNum) % kTawaraNum;
    BreakTawara(slot, 6);

    // 激突地点から内側へ向かう扇状の衝撃弾(3層＋大玉)
    double inward = phi + DX_PI;
    sEnemyShotSet* pFan = CreateShotSet(ShotLinear, enemy.x, enemy.y, inward, 0);
    const int n = 25;
    for (int layer = 0; layer < 3; layer++) {
        double speed = 1.8 + layer * 0.8;
        int color = (layer == 0) ? 0 : ((layer == 1) ? 8 : 1);
        for (int j = 0; j < n; j++) {
            double a = inward + ((double)j / (n - 1) - 0.5) * 1.8 + (layer % 2) * 0.036;
            AddShot(pFan, enemy.x, enemy.y, a, speed, img_enemyShotMediumBall[color]);
        }
    }
    for (int j = -1; j <= 1; j++) {
        AddShot(pFan, enemy.x, enemy.y, inward + j * 0.35, 1.4, img_enemyShotLargeBall[8]);
    }

    ChangeState(SUMO_STUN);
}

// ------------------------------------------------------------
//  土俵・難易度の更新
// ------------------------------------------------------------
static void UpdateLevel()
{
    int lvHp = (enemy.hp > 130) ? 0 : ((enemy.hp > 65) ? 1 : 2);
    int lvCrash = s_crashNum / 3;
    if (lvCrash > 2) lvCrash = 2;
    s_level = (lvHp > lvCrash) ? lvHp : lvCrash;
}

static void UpdateDohyo()
{
    s_ringRotate += 0.004; // ゆっくり時計回りに自転
    if (s_ringRadius > s_ringTargetRadius) {
        s_ringRadius -= 0.15;
        if (s_ringRadius < s_ringTargetRadius) s_ringRadius = s_ringTargetRadius;
    }
    if (s_ringShake > 0.05) {
        s_ringShake *= 0.90;
        s_ringCx = kFieldCenterX + (GetRand(200) - 100) / 100.0 * s_ringShake;
        s_ringCy = kFieldCenterY + (GetRand(200) - 100) / 100.0 * s_ringShake;
    }
    else {
        s_ringShake = 0.0;
        s_ringCx = kFieldCenterX;
        s_ringCy = kFieldCenterY;
    }
}

// ------------------------------------------------------------
//  斥力：ボスと自機が近いときだけ働く
//    F = k * (1 - d / R)  自機は軽く弾かれ、ボスは重いので減速するだけ
// ------------------------------------------------------------
static void ApplyRepulsion()
{
    const double kForceK = 6.0;
    s_repelF = 0.0;

    double dx = player.x - enemy.x;
    double dy = player.y - enemy.y;
    double d = sqrt(dx * dx + dy * dy);
    double forceR = kForceRadius[s_level];
    if (d >= forceR) return;

    double f = kForceK * (1.0 - d / forceR);

    double pullSpeed; // 引き寄せの強さ
    double angle; // 引き寄せの向き
    double ratio = 3.0; // 比例定数
    angle = (d > 0.001) ? atan2(dy, dx) : DX_PI * 0.5; // ボスから見た自機の向き(=自機が押される向き)
    pullSpeed = f * (IsPlayerFocusing() ? 0.5 : 1.0);  // 低速移動(踏ん張り)で斥力が半減

    player.x += pullSpeed * cos(angle);
    player.y += pullSpeed * sin(angle);
    spawnForceParticles(player.x, player.y, pullSpeed * cos(angle) * ratio, pullSpeed * sin(angle) * ratio);

    // ボス側の反作用(重いので速度が落ちるだけ)
    s_repelF = pullSpeed;
    if (s_state == SUMO_CHARGE) {
        s_chargeSpeed -= pullSpeed * 0.45;
        if (s_chargeSpeed < 1.2) s_chargeSpeed = 1.2;
    }
}

// 自機は土俵の外に出られない(俵の位置までは行けるので、押し込まれると被弾する)
static void ClampPlayerToDohyo()
{
    double limit = s_ringRadius + 2.0;
    if (count < 90) limit = s_ringRadius - 16.0; // 開始直後は確実に土俵の内側へ入れる
    ClampInRing(&player.x, &player.y, limit);
}

// ------------------------------------------------------------
//  初期化
// ------------------------------------------------------------
static void InitSumo()
{
    enemy.x = 240.0;
    enemy.y = 40.0;
    enemy.maxHp = enemy.hp = 200; // 200から変更しないこと

    s_state = SUMO_ENTER;
    s_stateCount = 0;
    s_level = 0;
    s_crashNum = 0;
    s_waveIdx = 0;
    s_ringRadius = s_ringTargetRadius = kRingRadiusMax;
    s_ringRotate = 0.0;
    s_ringShake = 0.0;
    s_ringCx = kFieldCenterX;
    s_ringCy = kFieldCenterY;
    s_chargeAngle = s_feintAngle = 0.0;
    s_chargeSpeed = 0.0;
    s_repelF = 0.0;
    for (int i = 0; i < kTawaraNum; i++) s_tawaraTimer[i] = -1;

    // 土俵(俵の輪)。弾は最初のフレームにShotTawara側で生成される
    s_pTawaraSet = CreateShotSet(ShotTawara, kFieldCenterX, kFieldCenterY, 0.0, 0);
}

// ------------------------------------------------------------
//  敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_Sumou_Claude()
{
    if (count == 1) {
        InitSumo();
    }

    s_stateCount++;
    UpdateLevel();
    UpdateDohyo();

    switch (s_state) {
    case SUMO_ENTER: {
        // 画面上部から土俵中央へ入場
        double t = s_stateCount / 60.0;
        if (t > 1.0) t = 1.0;
        enemy.x = kFieldCenterX;
        enemy.y = 40.0 + (kFieldCenterY - 40.0) * sin(t * DX_PI * 0.5);
        if (s_stateCount >= 60) ChangeState(SUMO_SHIKIRI);
        break;
    }
    case SUMO_SHIKIRI: {
        // 蹲踞して塩まき。終盤は静かになり、「はっけよい」の気配だけ鳴る
        enemy.x = kFieldCenterX + 3.0 * sin(s_stateCount * 0.35);
        enemy.y = kFieldCenterY;
        if (s_stateCount >= 20 && s_stateCount <= 220 && s_stateCount % 6 == 0) ThrowSalt();
        if (s_stateCount == 250) PlaySe(sound_enemyCharge);
        if (s_stateCount >= 300) {
            enemy.x = kFieldCenterX;
            ChangeState(s_level >= 2 ? SUMO_YORI : SUMO_TELEGRAPH);
        }
        break;
    }
    case SUMO_TELEGRAPH: {
        // 白い塩の点列が突進の進路を示す。難易度1以降はフェイント(白)→本命(赤)の2本
        const int teleLen = kTeleLen[s_level];
        if (s_stateCount == 1) {
            PlaySe(sound_enemyCharge);
            s_chargeAngle = AngleToPlayer(enemy.x, enemy.y);
            s_feintAngle = s_chargeAngle;
        }
        if (s_level >= 1 && s_stateCount < teleLen / 2) {
            if (s_stateCount % 3 == 0) {
                EmitLinearOne(enemy.x, enemy.y, s_feintAngle, 4.5, img_enemyShotSmallBall[6]);
            }
            enemy.x += 0.5 * cos(s_feintAngle); // フェイントの方へじわっと動く
            enemy.y += 0.5 * sin(s_feintAngle);
        }
        else {
            if (s_level >= 1 && s_stateCount == teleLen / 2) {
                s_chargeAngle = AngleToPlayer(enemy.x, enemy.y); // 本命の進路を自機に再照準
                PlaySe(sound_enemyCharge);
            }
            if (s_stateCount % 3 == 0) {
                EmitLinearOne(enemy.x, enemy.y, s_chargeAngle, 4.5, img_enemyShotSmallBall[0]);
            }
        }
        if (s_stateCount >= teleLen) ChangeState(SUMO_CHARGE);
        break;
    }
    case SUMO_CHARGE: {
        if (s_stateCount == 1) {
            PlaySe(sound_enemyShot_heavy);
            s_chargeSpeed = 1.5;
        }
        s_chargeSpeed += 0.35;
        if (s_chargeSpeed > kChargeMax[s_level]) s_chargeSpeed = kChargeMax[s_level];
        enemy.x += s_chargeSpeed * cos(s_chargeAngle);
        enemy.y += s_chargeSpeed * sin(s_chargeAngle);

        // 張り手
        if (s_stateCount % 5 == 0) EmitHariteFan();

        // 俵への激突判定
        double dx = enemy.x - s_ringCx;
        double dy = enemy.y - s_ringCy;
        double d = sqrt(dx * dx + dy * dy);
        if (d >= s_ringRadius - kBossBodyRadius) {
            OnCrash(atan2(dy, dx));
        }
        break;
    }
    case SUMO_STUN: {
        // 激突後の大きな隙(うっちゃりの反撃チャンス)
        if (s_stateCount >= kStunFrame) ChangeState(SUMO_RETURN);
        break;
    }
    case SUMO_RETURN: {
        double dx = kFieldCenterX - enemy.x;
        double dy = kFieldCenterY - enemy.y;
        double d = sqrt(dx * dx + dy * dy);
        if (d <= 3.0) {
            enemy.x = kFieldCenterX;
            enemy.y = kFieldCenterY;
            ChangeState(s_level >= 2 ? SUMO_YORI : SUMO_TELEGRAPH);
        }
        else {
            double sp = (d < 2.4) ? d : 2.4;
            enemy.x += sp * dx / d;
            enemy.y += sp * dy / d;
        }
        break;
    }
    case SUMO_YORI: {
        // 決まり手：自機を追って押し込みながら、全方位に同心円の掛け声を放つ
        if (s_stateCount == 1) PlaySe(sound_enemyCharge);
        double ang = AngleToPlayer(enemy.x, enemy.y);
        double sp = 2.0 - s_repelF * 0.25;
        if (sp < 0.3) sp = 0.3;
        enemy.x += sp * cos(ang);
        enemy.y += sp * sin(ang);
        ClampInRing(&enemy.x, &enemy.y, s_ringRadius - 40.0);
        if (s_stateCount % 42 == 1) {
            PlaySe(sound_enemyShot_medium);
            EmitKakegoe();
        }
        if (s_stateCount >= kYoriFrame) ChangeState(SUMO_TELEGRAPH);
        break;
    }
    }

    ApplyRepulsion();
    ClampPlayerToDohyo();
}