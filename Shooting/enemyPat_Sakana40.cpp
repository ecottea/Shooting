// enemyPat_tmp.cpp
// 二重振り子をモチーフにした弾幕パターン（共鳴弾）

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 外側・内側の弾源を管理するためのパラメータインデックス
enum {
    PARAM_OUTER_ANGLE = 0, // 外側振り子の角度
    PARAM_INNER_ANGLE,     // 内側振り子の角度
    PARAM_OUTER_PHASE,     // 外側の位相（時間経過）
    PARAM_INNER_PHASE,     // 内側の位相（時間経過）
    PARAM_RESONANCE,       // 共鳴度（プレイヤー位置に応じて増減）
};

// 弾幕：二重振り子の共鳴弾
static void ShotDoublePendulum(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // プレイヤーが中央に近いほど共鳴度を上げる
    double dx = player.x - 240.0; // 画面中央からの横方向距離
    double resonance = 1.0 - fabs(dx) / 240.0; // 0.0〜1.0
    pEnemyShotSet->param_d[PARAM_RESONANCE] = resonance;

    // 外側振り子の位相を進める（共鳴度が高いほど速く）
    pEnemyShotSet->param_d[PARAM_OUTER_PHASE] += 0.05 + resonance * 0.02;

    // 内側振り子の位相（外側より少し遅れる＋共鳴で追いつく）
    pEnemyShotSet->param_d[PARAM_INNER_PHASE] += 0.04 + resonance * 0.03;

    // 外側振り子の角度（sin波で左右に振れる）
    pEnemyShotSet->param_d[PARAM_OUTER_ANGLE] =
        sin(pEnemyShotSet->param_d[PARAM_OUTER_PHASE]) * DX_PI * 0.6;

    // 内側振り子の角度（外側の角度＋独自の振動）
    pEnemyShotSet->param_d[PARAM_INNER_ANGLE] =
        pEnemyShotSet->param_d[PARAM_OUTER_ANGLE] +
        sin(pEnemyShotSet->param_d[PARAM_INNER_PHASE]) * DX_PI * 0.4;

    // 外側弾源の位置（画面中央上から左右に振れる）
    double outerX = pEnemyShotSet->x + 240.0 * sin(pEnemyShotSet->param_d[PARAM_OUTER_ANGLE]);
    double outerY = pEnemyShotSet->y + 20.0;

    // 内側弾源の位置（外側弾源の先端にぶら下がる）
    double innerX = outerX + 80.0 * sin(pEnemyShotSet->param_d[PARAM_INNER_ANGLE]);
    double innerY = outerY + 80.0 * (1.0 - cos(pEnemyShotSet->param_d[PARAM_INNER_ANGLE]));

    // 外側弾源の発射（左右の頂点付近で）
    if (fabs(sin(pEnemyShotSet->param_d[PARAM_OUTER_PHASE])) > 0.6) {
        // 効果音（中くらいの弾音）
        if (!CheckSoundMem(sound_enemyShot_medium))
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 扇状に 7 発
        for (int i = 0; i < 7; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = outerX;
            pEnemyShot->y = outerY;

            // 扇状に広がる
            double baseAngle = DX_PI * 0.5; // 下向き
            double spread = DX_PI * 0.3;    // ±約54度
            pEnemyShot->muki = baseAngle + (i - 3) * spread / 6.0;

            // 共鳴度が高いほど速く
            pEnemyShot->speed = 2.0 + resonance * 1.0;

            // 弾の種類：中玉（緑系）
            pEnemyShot->kind = img_enemyShotMediumBall[2];

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 内側弾源の発射（外側と逆のタイミングで）
    if (fabs(cos(pEnemyShotSet->param_d[PARAM_INNER_PHASE])) > 0.6) {
        // 効果音（軽めの弾音）
        if (!CheckSoundMem(sound_enemyShot_light))
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 直線弾を 3 発
        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;

            pEnemyShot->x = innerX;
            pEnemyShot->y = innerY;

            // プレイヤー方向に少しずらす
            double baseAngle = atan2(player.y - innerY, player.x - innerX);
            double offset = (i - 1) * DX_PI * 0.1; // 左右に少し広げる
            pEnemyShot->muki = baseAngle + offset;

            // 共鳴度が高いほど速く
            pEnemyShot->speed = 2.5 + resonance * 0.5;

            // 弾の種類：銃弾（赤系）
            pEnemyShot->kind = img_enemyShotBullet[0];

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動（メインルーチンで count は自動インクリメントされる）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン（二重振り子の共鳴弾）
void EnemyPat_DoublePendulum_Sakana()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 180.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        shot_count = 0;
    }
    else {
        // 敵はゆっくり左右に動く
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔で弾幕セットを生成
    if (count == 30) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDoublePendulum;
        pEnemyShotSet->x = 0.0; // 画面左端基準（外側振り子の支点）
        pEnemyShotSet->y = 0.0; // 画面上端基準
        pEnemyShotSet->muki = 0.0; // 未使用（弾幕関数内で計算）

        // 振り子パラメータ初期化
        pEnemyShotSet->param_d[PARAM_OUTER_ANGLE] = 0.0;
        pEnemyShotSet->param_d[PARAM_INNER_ANGLE] = 0.0;
        pEnemyShotSet->param_d[PARAM_OUTER_PHASE] = GetRand(628) / 100.0; // 0〜2π相当
        pEnemyShotSet->param_d[PARAM_INNER_PHASE] = GetRand(628) / 100.0;
        pEnemyShotSet->param_d[PARAM_RESONANCE] = 0.0;

        pEnemyShotSet->kind = shot_count++;

        // 弾リストのダミーヘッド
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // 弾幕セットをグローバルリストに追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}