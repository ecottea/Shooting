// EnemyPat_LunaticGun.cpp
// 東方紺珠伝：銃符「ルナティックガン」風
// EnemyPat_LunaticGun_DeepSeek() を実装したファイルです。
// count, pEnemyShotSet->count, pEnemyShot->count のインクリメント、
// 画面外弾の削除はメインルーチン側で行う前提です。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾を1つ追加する補助関数
static void AddEnemyShot(
    sEnemyShotSet* pSet,
    double x,
    double y,
    double muki,
    double speed,
    int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;

    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;

    pEnemyShot->prev = pSet->pEnemyShotHead->prev;
    pEnemyShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pEnemyShot;
    pSet->pEnemyShotHead->prev = pEnemyShot;
}

// 弾幕セットを1つ追加する補助関数
static sEnemyShotSet* AddEnemyShotSet(
    double x,
    double y,
    double muki,
    sEnemyShotSet::PatternFunc patternFunc,
    int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;

    pEnemyShotSet->count = 0;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->patternFunc = patternFunc;
    pEnemyShotSet->kind = kind;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;

    return pEnemyShotSet;
}

// A：間隔が狭い箇所と広い箇所が交互の10way全方位弾
// セット生成時に10発出し、その後は直進更新
static void ShotA_LunaticGun(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 10方向、隙間は 狭24° / 広48° の交互。
        // 5*(24+48)=360° で1周する。
        const double narrowGap = 24.0 * DX_PI / 180.0;
        const double wideGap = 48.0 * DX_PI / 180.0;

        double offset = 0.0;

        for (int i = 0; i < 10; i++) {
            AddEnemyShot(
                pEnemyShotSet,
                pEnemyShotSet->x,
                pEnemyShotSet->y,
                pEnemyShotSet->muki + offset,
                2.2,
                img_enemyShotBullet[4] // 青系の銃弾
            );

            // 現在の弾から次の弾までの隙間
            offset += (i % 2 == 0) ? narrowGap : wideGap;
        }
    }

    // このセットに属する弾を直進移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// B：1辺8発の逆三角形を1つ生成
// angle 方向へ向かって、ボス位置から逆三角形の弾列を出す
static void SpawnInvertedTriangle(sEnemyShotSet* pEnemyShotSet, double angle)
{
    const int side = 8;              // 1辺の弾数
    const double lateralStep = 10.0*2; // 横方向の弾間隔
    const double forwardStep = 8.0*2;  // 進行方向の行間隔
    const double startForward = 14.0;// ボスから少し前から開始

    const double fx = cos(angle);
    const double fy = sin(angle);
    const double px = -sin(angle);
    const double py = cos(angle);

    // 逆三角形：手前(ボス側)が8発、先(自機側)が1発
    for (int row = 0; row < side; row++) {
        const int n = side - row;
        const double forward = startForward + row * forwardStep;

        for (int j = 0; j < n; j++) {
            const double lateral = (j - (n - 1) * 0.5) * lateralStep;

            const double x = pEnemyShotSet->x + fx * forward + px * lateral;
            const double y = pEnemyShotSet->y + fy * forward + py * lateral;

            AddEnemyShot(
                pEnemyShotSet,
                x,
                y,
                angle,
                2.6,
                img_enemyShotBullet[5] // マゼンタ系の銃弾
            );
        }
    }
}

// B：逆三角形5個を僅かな時間差で撃ち、間髪空けずにもう1セット撃つ。
// その後は EnemyPat_LunaticGun_DeepSeek 側で間隔を空けて次のBセットを生成する。
static void ShotB_LunaticGun(sEnemyShotSet* pEnemyShotSet)
{
    // 1セット = 5個の逆三角形。
    // 2セット目は1セット目に続けて少しだけ遅らせる。
    static const int delays[10] = {
        0, 2, 4, 6, 8,       // 1セット目
        10, 12, 14, 16, 18   // 2セット目
    };

    for (int i = 0; i < 10; i++) {
        if (pEnemyShotSet->count == delays[i]) {
            // 各三角形の生成時点で自機方向を向く
            const double aim = atan2(
                player.y - pEnemyShotSet->y,
                player.x - pEnemyShotSet->x
            );
            SpawnInvertedTriangle(pEnemyShotSet, aim);
        }
    }

    // このセットに属する弾を直進移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_LunaticGun_DeepSeek()
{
    static int moveDir;
    static double aBaseAngle;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと

        moveDir = 1;
        aBaseAngle = 0.0;
    }
    else {
        // サンプル同様、左右にゆっくり移動
        enemy.x += 0.98 * (double)moveDir;
        if (count % 120 == 60) moveDir *= -1;
    }

    // A：常時、10way全方位弾を撃ち続ける。
    // 8フレームごとに新しいセットを出し、基準角を少しずつ進めることで時計回りに回転。
    if (count % 8 == 1) {
        AddEnemyShotSet(
            enemy.x,
            enemy.y,
            aBaseAngle,
            ShotA_LunaticGun,
            0
        );

        // DxLib座標系は y 下向きなので、角度を増やすと画面上は時計回り。
        aBaseAngle += 0.07;
        if (aBaseAngle >= DX_PI * 2.0) {
            aBaseAngle -= DX_PI * 2.0;
        }
    }

    // B：120フレーム周期で、逆三角形2セットを連続で撃つ。
    // その後、次の周期まで少し間を空ける。
    if (count % 120 == 40) {
        if (CheckSoundMem(sound_enemyShot_heavy)) {
            StopSoundMem(sound_enemyShot_heavy);
        }
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const double aim = atan2(
            player.y - enemy.y,
            player.x - enemy.x
        );

        AddEnemyShotSet(
            enemy.x,
            enemy.y,
            aim,
            ShotB_LunaticGun,
            1
        );
    }
}