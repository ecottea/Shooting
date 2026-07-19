// enemyPat_shuriken.cpp
// 弾幕パターン：「手裏剣乱舞 ～影縫い～」
//
// ■ 3フェーズ構成
//   Phase 1 (f0  ~ f89 ) : 4腕の手裏剣が回転しながら拡散（CW/CCW 交互）
//   Phase 2 (f65        ) : 各腕の先端から黒い影弾が逆方向へ扇展開
//   Phase 3 (f90 ~      ) : 本体弾が発射元に向かってUターン加速
//
// ■ 使用素材
//   img_enemyShotDiamond[8] 橙菱形  … 手裏剣腕(偶数腕)
//   img_enemyShotDiamond[3] シアン菱形 … 手裏剣腕(奇数腕)
//   img_enemyShotSmallBall[7] 黒小玉 … 影弾
//   sound_enemyShot_heavy   … 投擲 SE
//   sound_enemyShot_medium  … 影分離 SE

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//============================================================
// 定数
//============================================================
static const int    SHURIKEN_ARMS = 4;    // 腕の本数
static const int    SHOTS_PER_ARM = 3;    // 1腕あたりの弾数
static const int    SHADOW_SPAWN_FRAME = 65;   // 影生成タイミング
static const int    BOOMERANG_FRAME = 90;   // Uターン開始タイミング
static const double ROT_SPEED = 0.022; // 手裏剣の回転速度 [rad/frame]
static const double TURN_SPEED = 0.07;  // Uターン向き補正速度 [rad/frame]
static const double BOOMERANG_MAX_SPD = 5.5;   // Uターン時の最大速度
static const double BOOMERANG_ACCEL = 0.05;  // Uターン時の加速量 [/frame]

//============================================================
// ShotShurikenShadow
//   各腕の先端位置から逆向きに3発扇展開する影弾パターン
//
//   pSet->muki      : 影の展開中心方向（元弾の進行逆向き）
//   pSet->param_d[0]: 影弾の基本速度
//============================================================
static void ShotShurikenShadow(sEnemyShotSet* pSet)
{
    sEnemyShot* pS;

    if (pSet->count == 0) {
        const double spread = DX_PI / 6.0; // 扇の開き（片側 30°）
        const double spd = pSet->param_d[0];

        for (int i = -10; i <= 10; i++) {
            pS = new sEnemyShot;
            pS->x = pSet->x;
            pS->y = pSet->y;
            pS->muki = pSet->muki + i * spread / 18;
            pS->speed = spd + i * 0.15; // 左右に速度差をつけて扇に広がらせる
            pS->kind = img_enemyShotSmallBall[7]; // 黒小玉

            pS->margin = 480;

            pS->prev = pSet->pEnemyShotHead->prev;
            pS->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pS;
            pSet->pEnemyShotHead->prev = pS;
        }
    }

    // 毎フレーム: 直進
    pS = pSet->pEnemyShotHead->next;
    while (pS != pSet->pEnemyShotHead) {
        pS->x += pS->speed * cos(pS->muki);
        pS->y += pS->speed * sin(pS->muki);
        pS = pS->next;
    }
}

//============================================================
// ShotShuriken
//   手裏剣本体の弾幕パターン（Phase 1→2→3 を管理）
//
//   pSet->param_i[0]: 回転方向 (+1=CW, -1=CCW)
//   pSet->param_i[1]: 影生成済みフラグ (0=未, 1=済)
//   pSet->param_d[0]: 発射元 x（Uターン目標座標）
//   pSet->param_d[1]: 発射元 y（Uターン目標座標）
//
//   sEnemyShot->param_d[0]: 現在の進行角度（毎フレーム回転）
//   sEnemyShot->param_d[1]: 現在の速さ
//   sEnemyShot->param_i[0]: フェーズ (0=展開, 1=Uターン)
//   sEnemyShot->param_i[1]: 腕先端フラグ (1=先端弾・影生成の基点)
//============================================================
static void ShotShuriken(sEnemyShotSet* pSet)
{
    const int    rotDir = pSet->param_i[0];
    const double originX = pSet->param_d[0];
    const double originY = pSet->param_d[1];

    //----------------------------------------------------------
    // count == 0: 12弾（4腕 × 3弾）を手裏剣形状に生成
    //----------------------------------------------------------
    if (pSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy) == 1)
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const double speeds[SHOTS_PER_ARM] = { 1.8, 2.6, 3.5 };
        const double base = pSet->muki; // プレイヤー方向を0腕の基準角とする

        for (int arm = 0; arm < SHURIKEN_ARMS; arm++) {
            // 腕を 90° 刻みで配置 → 手裏剣の4腕形状
            double armAngle = base + arm * (DX_PI * 0.5);

            for (int b = 0; b < SHOTS_PER_ARM; b++) {
                sEnemyShot* pS = new sEnemyShot;
                pS->x = pSet->x;
                pS->y = pSet->y;
                pS->muki = armAngle;
                pS->speed = speeds[b];

                // 偶数腕=橙、奇数腕=シアン（菱形弾で刃の形を表現）
                pS->kind = (arm % 2 == 0)
                    ? img_enemyShotDiamond[8]   // 橙
                    : img_enemyShotDiamond[3];  // シアン

                pS->param_d[0] = armAngle;   // 進行角度（回転で書き換わる）
                pS->param_d[1] = speeds[b];  // 速さ（Uターンで書き換わる）
                pS->param_i[0] = 0;          // Phase 0: 展開中
                // b が最大＝最速弾＝腕の先端。影生成の基点に使う
                pS->param_i[1] = (b == SHOTS_PER_ARM - 1) ? 1 : 0;

                pS->margin = 480;

                pS->prev = pSet->pEnemyShotHead->prev;
                pS->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = pS;
                pSet->pEnemyShotHead->prev = pS;
            }
        }
    }

    //----------------------------------------------------------
    // count == SHADOW_SPAWN_FRAME: 各腕先端から影ShotSetを生成
    //   「抜けた！」の瞬間に逆から来る驚きを演出
    //----------------------------------------------------------
    if (pSet->count == SHADOW_SPAWN_FRAME && pSet->param_i[1] == 0) {
        pSet->param_i[1] = 1; // 二重生成防止

        if (CheckSoundMem(sound_enemyShot_medium) == 1)
            StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShot* pS = pSet->pEnemyShotHead->next;
        while (pS != pSet->pEnemyShotHead) {
            if (pS->param_i[1] == 1) { // 先端弾のみ基点にする
                sEnemyShotSet* pShadow = new sEnemyShotSet;
                pShadow->count = 0;
                pShadow->patternFunc = ShotShurikenShadow;
                pShadow->x = pS->x;
                pShadow->y = pS->y;
                pShadow->muki = pS->param_d[0] + DX_PI; // 進行逆向き
                pShadow->param_d[0] = 2.0;                     // 影の弾速

                pShadow->pEnemyShotHead = new sEnemyShot;
                pShadow->pEnemyShotHead->prev = pShadow->pEnemyShotHead;
                pShadow->pEnemyShotHead->next = pShadow->pEnemyShotHead;

                pShadow->prev = enemyShotSetHead.prev;
                pShadow->next = &enemyShotSetHead;
                enemyShotSetHead.prev->next = pShadow;
                enemyShotSetHead.prev = pShadow;
            }
            pS = pS->next;
        }
    }

    //----------------------------------------------------------
    // count == BOOMERANG_FRAME: 全弾を Phase 1 (Uターン) に切替
    //----------------------------------------------------------
    if (pSet->count == BOOMERANG_FRAME) {
        sEnemyShot* pS = pSet->pEnemyShotHead->next;
        while (pS != pSet->pEnemyShotHead) {
            pS->param_i[0] = 1;
            pS = pS->next;
        }
    }

    //----------------------------------------------------------
    // 毎フレーム移動
    //----------------------------------------------------------
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {

        if (pShot->param_i[0] == 0) {
            //---- Phase 0: 回転しながら展開 -------------------------
            // 向きをフレームごとに回転させることで手裏剣全体が
            // スピンしながら広がるように見える
            pShot->param_d[0] += rotDir * ROT_SPEED;
            pShot->x += pShot->param_d[1] * cos(pShot->param_d[0]);
            pShot->y += pShot->param_d[1] * sin(pShot->param_d[0]);
        }
        else {
            //---- Phase 1: Uターン（発射元へ向き補正 + 加速） ------
            double dx = originX - pShot->x;
            double dy = originY - pShot->y;
            double tgt = atan2(dy, dx);
            double diff = tgt - pShot->param_d[0];

            // -PI ~ PI に正規化して最短回転方向を選ぶ
            while (diff > DX_PI) diff -= 2.0 * DX_PI;
            while (diff < -DX_PI) diff += 2.0 * DX_PI;

            pShot->param_d[0] += (diff > 0.0 ? 1.0 : -1.0) * TURN_SPEED;

            // 加速（戻る弾が追い打ち感を出す）
            if (pShot->param_d[1] < BOOMERANG_MAX_SPD)
                pShot->param_d[1] += BOOMERANG_ACCEL;

            pShot->x += pShot->param_d[1] * cos(pShot->param_d[0]);
            pShot->y += pShot->param_d[1] * sin(pShot->param_d[0]);
        }

        pShot = pShot->next;
    }
}

//============================================================
// EnemyPat_Shuriken_Claude
//   左右往復しながら 50フレームごとに手裏剣を1セット投擲
//   CW/CCW を交互に切り替えて螺旋の向きを反転させる
//============================================================
void EnemyPat_Shuriken_Claude()
{
    static int muki;
    static int wave; // 投擲回数 (CW/CCW 判定用)

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 150.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        wave = 0;
    }
    else {
        // 左右往復（100フレーム周期）
        enemy.x += 1.2 * (double)muki;
        if (count % 100 == 50) muki *= -1;
    }

    // 50フレームごとに手裏剣1セット投擲
    if (count > 20 && count % 50 == 0) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = ShotShuriken;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = atan2(player.y - pSet->y, player.x - pSet->x);
        pSet->param_i[0] = (wave % 2 == 0) ? 1 : -1; // CW/CCW 交互
        pSet->param_i[1] = 0;                          // 影未生成
        pSet->param_d[0] = pSet->x; // Uターン目標 x
        pSet->param_d[1] = pSet->y; // Uターン目標 y

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;

        wave++;
    }
}