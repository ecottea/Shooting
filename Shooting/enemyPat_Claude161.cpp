// 銃符「ルナティックガン」(東方紺珠伝 清蘭)
//
// 調査に基づく構成:
//   1. 銃弾列(固定の鈍足銃弾列で壁を作る)  … ボス体力ゲージ相当の半径の外側から放射状に発射
//      ・通常は放射状の「列」、3回に1回は全方位の「リング」
//      ・列を構成する銃弾は速度にばらつきがあり、時間とともに放射状の線に伸びる
//   2. 自機狙い銃弾塊(逆三角形に並んだ赤い銃弾)… 壁の隙間を抜けようとする自機に上から蓋をする
//
// 使用素材:
//   弾: 銃弾(5.0x2.0) の 青(4) = 壁/リング、赤(0) = 自機狙い塊
//   音: sound_enemyShot_heavy = 壁の発射、sound_enemyShot_medium = 自機狙い塊の発射
//
// count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと画面外の弾の消去はメインルーチン側で行う。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  調整用パラメータ
// ============================================================
// --- 全体の流れ(1サイクル = 敵が移動して、壁を撒き、自機狙いを連射する) ---
static const int    kLgCycle = 120;  // 1サイクルのフレーム数
static const int    kLgWallTiming = 16;   // サイクル内で壁を撒くフレーム
static const int    kLgClusterFirst = 36;   // 最初の自機狙い塊を撃つフレーム
static const int    kLgClusterStep = 20;   // 自機狙い塊の発射間隔
static const int    kLgClusterNum = 5;    // 1サイクルあたりの自機狙い塊の数
static const int    kLgRingPeriod = 3;    // 何回に1回リングにするか

// --- 銃弾列(放射状の壁) ---
static const int    kLgRayNum = 24;   // 列の本数
static const int    kLgRayBullets = 7;    // 1本の列を構成する弾数
static const double kLgWallRadius = 36.0; // 発射位置(敵からの距離。ボスの近くには弾を出さない)
static const double kLgRaySpeedMin = 0.90; // 列の最低速度
static const double kLgRaySpeedStep = 0.13; // 列内の弾ごとの速度差
static const int    kLgRingBullets = 36;   // リング1層の弾数(2層構成)
static const double kLgRingSpeed = 1.15; // リング内側層の速度
static const double kLgRingSpeedAdd = 0.50; // リング外側層の速度加算

// --- 自機狙い銃弾塊(逆三角形) ---
static const int    kLgClusterRows = 7;    // 三角形の段数(弾数 = 1+2+...+段数)
static const double kLgClusterLatSp = 7.0;  // 横方向の弾間隔
static const double kLgClusterBackSp = 8.0;  // 段ごとの間隔(後ろ方向)
static const double kLgClusterDist = 56.0; // 先頭弾の生成位置(敵からの距離)
static const double kLgClusterSpeed0 = 1.4;  // 初速
static const double kLgClusterAccel = 0.06; // 毎フレームの加速度
static const double kLgClusterSpeedMax = 2.7; // 最高速度

// --- 弾の色 ---
static const int    kLgColorRed = 0;
static const int    kLgColorBlue = 4;

// ============================================================
//  ヘルパー
// ============================================================
// 効果音(同じ音の多重再生を避けて鳴らし直す)
static void LgPlaySound(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// ShotSet に弾を1発追加する
static sEnemyShot* LgAddShot(sEnemyShotSet* pEnemyShotSet, double x, double y,
    double muki, double speed, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// ShotSet を生成してリストに登録する
static void LgAddShotSet(sEnemyShotSet::PatternFunc func, double x, double y,
    double muki, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->kind = kind;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ============================================================
//  弾幕：銃弾列(放射状の壁) / 全方位リング
//    kind == 0 : 放射状の銃弾列
//    kind == 1 : 全方位リング(2層)
//    muki      : 基準角(ランダム)
// ============================================================
static void ShotLunaticWall(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        LgPlaySound(sound_enemyShot_heavy);

        if (pEnemyShotSet->kind == 0) {
            // 放射状の列: 同じ向きに速度違いの銃弾を並べ、時間とともに線状に伸ばす
            for (int i = 0; i < kLgRayNum; i++) {
                double ang = pEnemyShotSet->muki + DX_PI * 2.0 / kLgRayNum * i;
                for (int k = 0; k < kLgRayBullets; k++) {
                    double speed = kLgRaySpeedMin + kLgRaySpeedStep * k + GetRand(15) / 100.0;
                    LgAddShot(pEnemyShotSet,
                        pEnemyShotSet->x + cos(ang) * kLgWallRadius,
                        pEnemyShotSet->y + sin(ang) * kLgWallRadius,
                        ang, speed, img_enemyShotBullet[kLgColorBlue]);
                }
            }
        }
        else {
            // 全方位リング: 内側層と外側層を半ピッチずらして重ねる
            for (int layer = 0; layer < 2; layer++) {
                for (int i = 0; i < kLgRingBullets; i++) {
                    double ang = pEnemyShotSet->muki + DX_PI * 2.0 / kLgRingBullets * (i + 0.5 * layer);
                    double speed = kLgRingSpeed + kLgRingSpeedAdd * layer;
                    LgAddShot(pEnemyShotSet,
                        pEnemyShotSet->x + cos(ang) * kLgWallRadius,
                        pEnemyShotSet->y + sin(ang) * kLgWallRadius,
                        ang, speed, img_enemyShotBullet[kLgColorBlue]);
                }
            }
        }
    }

    // 壁は固定の鈍足弾: 等速直進のみ
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  弾幕：自機狙い銃弾塊(先頭が自機を向く三角形の赤い銃弾)
//    muki : 発射時の自機方向
// ============================================================
static void ShotLunaticCluster(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        LgPlaySound(sound_enemyShot_medium);

        double cosM = cos(pEnemyShotSet->muki);
        double sinM = sin(pEnemyShotSet->muki);
        for (int r = 0; r < kLgClusterRows; r++) {
            for (int j = 0; j <= r; j++) {
                double lat = (j - r * 0.5) * kLgClusterLatSp; // 進行方向に対して横のずれ
                double back = r * kLgClusterBackSp;           // 先頭から後ろへのずれ
                sEnemyShot* pEnemyShot = LgAddShot(
                    pEnemyShotSet,
                    pEnemyShotSet->x - cosM * back - sinM * lat,
                    pEnemyShotSet->y - sinM * back + cosM * lat,
                    pEnemyShotSet->muki, kLgClusterSpeed0,
                    img_enemyShotBullet[kLgColorRed]);
                pEnemyShot->param_d[0] = kLgClusterSpeedMax;
            }
        }
    }

    // 加速しながら直進(全弾が同じ加速度なので三角形の形は保たれる)
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->speed < pShot->param_d[0]) {
            pShot->speed += kLgClusterAccel;
            if (pShot->speed > pShot->param_d[0]) pShot->speed = pShot->param_d[0];
        }
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_LunaticGun_Claude()
{
    static int    wall_count;          // 壁を撒いた回数
    static double target_x, target_y;  // 敵の移動目標

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        wall_count = 0;
        target_x = enemy.x;
        target_y = enemy.y;
    }

    int t = (count - 1) % kLgCycle; // サイクル内フレーム

    // サイクルの頭で次の移動先を決める(画面端寄りなら内側へ、中央付近ならランダムな側へ)
    if (t == 0) {
        int dir;
        if (enemy.x < 150.0)      dir = 1;
        else if (enemy.x > 330.0) dir = -1;
        else                      dir = GetRand(1) ? 1 : -1;
        target_x = enemy.x + dir * (90.0 + GetRand(90));
        if (target_x < 60.0)  target_x = 60.0;
        if (target_x > 420.0) target_x = 420.0;
        target_y = 40.0 + GetRand(60);
    }

    // 目標へ減速しながら移動
    enemy.x += (target_x - enemy.x) * 0.07;
    enemy.y += (target_y - enemy.y) * 0.07;

    // 銃弾列(壁): 通常は放射状の列、kLgRingPeriod回に1回は全方位リング
    if (t == kLgWallTiming) {
        int kind = (wall_count % kLgRingPeriod == kLgRingPeriod - 1) ? 1 : 0;
        double baseAngle = GetRand(359) / 180.0 * DX_PI;
        LgAddShotSet(ShotLunaticWall, enemy.x, enemy.y, baseAngle, kind);
        wall_count++;
    }

    // 自機狙い銃弾塊: 壁の隙間を抜けようとする自機に連続で撃ち込む
    if (t >= kLgClusterFirst && (t - kLgClusterFirst) % kLgClusterStep == 0
        && (t - kLgClusterFirst) / kLgClusterStep < kLgClusterNum) {
        double aim = atan2(player.y - enemy.y, player.x - enemy.x);
        LgAddShotSet(ShotLunaticCluster,
            enemy.x + cos(aim) * kLgClusterDist,
            enemy.y + sin(aim) * kLgClusterDist,
            aim, 0);
    }
}