#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕：サブタレイニアンローズ
//  ・ボスを中心に、青と赤の N way 全方位弾を T フレーム間隔で交互に発射する。
//  ・基準角は毎回ランダム。
//  ・青は「回転する扇形」の内側だけ大玉、それ以外は中玉。
//    赤は逆回転する扇形の内側だけ大玉、それ以外は中玉。
//  ・HP が 1/4 減るごとに、少し発射を止めて T, V, A, B を切り替える。
//
//  使用素材
//    弾  : img_enemyShotMediumBall[色] / img_enemyShotLargeBall[色]
//          色 0:赤、4:青
//    効果音: sound_enemyShot_heavy (全方位弾の発射時)
//            sound_enemyCharge     (フェーズ切り替え時の予告音)
// ============================================================

namespace {

    constexpr double ROSE_TWO_PI = 2.0 * DX_PI;

    constexpr int    ROSE_WAY = 60-5;     // N way
    constexpr int    ROSE_COLOR_RED = 0;      // 画像配列の色インデックス
    constexpr int    ROSE_COLOR_BLUE = 4;
    constexpr int    ROSE_SET_BLUE = 0;      // sEnemyShotSet::param_i[0] の値
    constexpr int    ROSE_SET_RED = 1;

    constexpr double ROSE_BOSS_X = 240.0;      // ボスの位置(画面は 480x480)
    constexpr double ROSE_BOSS_Y = 100.0;

    constexpr int    ROSE_START_WAIT = 30;     // 開始直後に撃つまでの待ち時間(フレーム)
    constexpr int    ROSE_PHASE_WAIT = 60;     // フェーズ切り替え時に撃つのを止める時間(フレーム)

    // フェーズごとのパラメータ
    struct sRosePhase {
        int    interval;  // T : 発射間隔(フレーム)
        double speed;     // V : 弾速
        double angVel;    // A : 扇形の回転速度(rad/フレーム)。負なら逆回転
        double width;     // B : 扇形の幅(rad)
    };
    constexpr int ROSE_PHASE_NUM = 4;          // HP を 1/4 ずつ削る
    const sRosePhase ROSE_PHASE[ROSE_PHASE_NUM] = {
        { 30, 1.6,  0.020, DX_PI / 6.0 },
        { 24, 2.0, -0.030, DX_PI / 5.0 },
        { 20, 2.4,  0.045, DX_PI / 4.0 },
        { 16, 2.8, -0.065, DX_PI / 3.0 },
    };

    // 扇形の現在状態（EnemyPat_SubterraneanRose_Claude が更新し、弾の関数が参照する）
    // s_wedgeAngle は count*A に相当する値。
    // A が切り替わっても扇形が瞬間移動しないよう、毎フレーム A を足して求める。
    // （A が一定の間は count*A と一致する）
    double s_wedgeAngle = 0.0;
    double s_wedgeWidth = 0.0;                 // B

    // theta が [start, start+width] の扇形内にあるか(角度は周期 2π で判定)
    bool InSector(double theta, double start, double width)
    {
        double d = fmod(theta - start, ROSE_TWO_PI);
        if (d < 0.0) d += ROSE_TWO_PI;
        return d <= width;
    }

    // 弾幕：N way 全方位弾 (1ショットセット = 1回の全方位発射)
    // param_i[0]: 色(ROSE_SET_BLUE / ROSE_SET_RED)
    // param_d[0]: 弾速 V
    void ShotRose(sEnemyShotSet* pEnemyShotSet)
    {
        const bool isRed = (pEnemyShotSet->param_i[0] == ROSE_SET_RED);
        const int  color = isRed ? ROSE_COLOR_RED : ROSE_COLOR_BLUE;

        if (pEnemyShotSet->count == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            for (int i = 0; i < ROSE_WAY; i++) {
                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                // 基準角(ランダム)から等間隔に N 方向。向きは進行方向
                pEnemyShot->muki = pEnemyShotSet->muki + ROSE_TWO_PI * i / ROSE_WAY;
                pEnemyShot->speed = pEnemyShotSet->param_d[0];
                pEnemyShot->kind = img_enemyShotMediumBall[color];
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // 青: [ count*A, count*A + B ]
        // 赤: [ -(count*A + B), -count*A ]  (逆回転)
        const double sectorStart = isRed ? -(s_wedgeAngle + s_wedgeWidth) : s_wedgeAngle;

        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);

            // 弾は中心から直進するので、中心から見た偏角は muki と一致する。
            // （弾を曲げる改造をする場合は atan2(y - 中心y, x - 中心x) に置き換えること）
            if (InSector(pShot->muki, sectorStart, s_wedgeWidth)) {
                pShot->kind = img_enemyShotLargeBall[color];
            }
            else {
                if (s_wedgeWidth < DX_PI / 4.0) {
                    pShot->kind = img_enemyShotMediumBall[color];
                }
                else {
                    pShot->kind = img_enemyShotSmallBall[color];
                }
            }
            pShot = pShot->next;
        }
    }

    // 全方位弾のショットセットを生成する
    void SpawnRose(int setColor, double speed)
    {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotRose;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        // 基準角: 0 〜 2π未満 をランダムに (GetRand(35999) は 0〜35999)
        pEnemyShotSet->muki = GetRand(35999) / 36000.0 * ROSE_TWO_PI;
        pEnemyShotSet->param_i[0] = setColor;
        pEnemyShotSet->param_d[0] = speed;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

} // namespace

// 敵本体のパターン
void EnemyPat_SubterraneanRose_Claude()
{
    static int phase;        // 現在のフェーズ(0〜3)
    static int waitTimer;    // 発射を止めている残りフレーム
    static int shotTimer;    // 次の発射までのフレーム
    static int colorToggle;  // 0:次は青、1:次は赤

    if (count == 1) {
        enemy.x = ROSE_BOSS_X;
        enemy.y = ROSE_BOSS_Y;
        enemy.maxHp = enemy.hp = 200; // 400から変更しないこと
        phase = 0;
        waitTimer = ROSE_START_WAIT;
        shotTimer = 0;
        colorToggle = 0;
        s_wedgeAngle = 0.0;
        s_wedgeWidth = ROSE_PHASE[0].width;
    }

    // HP が 1/4 減るごとにフェーズ切り替え
    int newPhase = (enemy.maxHp - enemy.hp) * ROSE_PHASE_NUM / enemy.maxHp;
    if (newPhase > ROSE_PHASE_NUM - 1) newPhase = ROSE_PHASE_NUM - 1;
    if (newPhase > phase) {
        phase = newPhase;
        waitTimer = ROSE_PHASE_WAIT;
        shotTimer = 0;
        colorToggle = 0;
        s_wedgeWidth = ROSE_PHASE[phase].width; // B を切り替え
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 扇形の回転は発射を止めている間も続ける
    s_wedgeAngle += ROSE_PHASE[phase].angVel;

    if (waitTimer > 0) {
        waitTimer--;
        return;
    }

    // T フレーム間隔で青・赤を交互に発射
    if (shotTimer <= 0) {
        SpawnRose(colorToggle == 0 ? ROSE_SET_BLUE : ROSE_SET_RED, ROSE_PHASE[phase].speed);
        colorToggle ^= 1;
        shotTimer = ROSE_PHASE[phase].interval;
    }
    shotTimer--;
}