#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  銃符「ルナティックガン」
//  清蘭（Stage 1 ボス）のスペルカード
//  ------------------------------------------------------------
//  ・青弾：清蘭の周囲から画面全体へ拡散する青い座薬弾
//  ・赤弾：自機を狙って逆三角形に整列・乱射される赤い座薬弾
//  ------------------------------------------------------------
//  使用素材：
//    ・img_enemyShotMediumOval[4]（青い座薬弾：中楕円弾）
//    ・img_enemyShotMediumOval[0]（赤い座薬弾：中楕円弾）
//    ・sound_enemyShot_light / sound_enemyShot_medium
// ============================================================

// --- 弾をセットに追加するヘルパー（sample のリンクリスト操作を関数化） ---
static void AddShotToList(sEnemyShotSet* pEnemyShotSet, sEnemyShot* pEnemyShot)
{
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// --- 弾を等速直線運動させるヘルパー ---
static void MoveShotsStraight(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// --- 新しい弾幕セットを生成するヘルパー ---
static sEnemyShotSet* CreateEnemyShotSet(
    sEnemyShotSet::PatternFunc func,
    double x, double y, double muki, int kind)
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
    return pEnemyShotSet;
}

// ============================================================
//  青弾：画面全体への拡散
//  清蘭の周囲から放射状に展開する青い座薬弾
// ============================================================
static void ShotLunaticBlue(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 使える効果音: sound_enemyShot_light / medium / heavy / extreme / sound_enemyCharge
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int numPerRing = pEnemyShotSet->param_i[0];  // 1環あたりの弾数
        int rings = pEnemyShotSet->param_i[1];       // 環の数
        double baseAng = pEnemyShotSet->param_d[0];  // 基準角度

        for (int r = 0; r < rings; r++) {
            // 環ごとに角度をずらして弾幕の隙間を埋める
            double ringAng = baseAng + (double)r * DX_PI / numPerRing;
            for (int i = 0; i < numPerRing; i++) {
                double ang = ringAng + (double)i / numPerRing * 2.0 * DX_PI;
                // GetRand(x) は 0 以上 x 以下の x+1 種類の整数を返す点に注意
                ang += (GetRand(80) - 40) * 0.001; // ±0.04 rad のばらつき

                sEnemyShot* pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = ang;
                pEnemyShot->speed = 1.0 + (GetRand(100) / 100.0) * 1.0; // 1.0～2.0
                pEnemyShot->kind = img_enemyShotMediumOval[4]; // 青い座薬弾
                AddShotToList(pEnemyShotSet, pEnemyShot);
            }
        }
    }
    MoveShotsStraight(pEnemyShotSet);
}

// ============================================================
//  赤弾：自機狙いの逆三角形乱射
//  逆三角形に整列した赤い座薬弾を自機の周辺へ乱射
// ============================================================
static void ShotLunaticRed(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        int rows = pEnemyShotSet->param_i[0]; // 段数（逆三角形の段数）
        int perRow = pEnemyShotSet->param_i[1]; // 1段あたりの最大弾数
        double rowGap = pEnemyShotSet->param_d[0]; // 段の間隔（自機方向）
        double colGap = pEnemyShotSet->param_d[1]; // 弾の間隔（垂直方向）
        double speed = pEnemyShotSet->param_d[2]; // 速さ

        // 自機方向
        double aimAng = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        double dirX = cos(aimAng), dirY = sin(aimAng);
        double perpX = -dirY, perpY = dirX;

        for (int r = 0; r < rows; r++) {
            int num = perRow - r; // 逆三角形：先端ほど弾が少ない
            if (num < 1) num = 1;
            for (int i = 0; i < num; i++) {
                double offset = (i - (num - 1) * 0.5) * colGap;

                sEnemyShot* pEnemyShot = new sEnemyShot;
                // 自機方向に段をずらし、垂直方向に並べる（逆三角形）
                pEnemyShot->x = pEnemyShotSet->x + perpX * offset + dirX * (rowGap * r);
                pEnemyShot->y = pEnemyShotSet->y + perpY * offset + dirY * (rowGap * r);
                // 自機を狙う（±0.03 rad のばらつき）
                pEnemyShot->muki = aimAng + (GetRand(60) - 30) * 0.001;
                pEnemyShot->speed = speed + (GetRand(40) / 100.0); // 2.0～2.4
                pEnemyShot->kind = img_enemyShotMediumOval[0]; // 赤い座薬弾
                AddShotToList(pEnemyShotSet, pEnemyShot);
            }
        }
    }
    MoveShotsStraight(pEnemyShotSet);
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_LunaticGun_DeepSeek()
{
    static int muki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
    }
    else {
        // 左右に往復移動
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // ----------------------------------------------------------
    //  青弾：画面全体に拡散する青い座薬弾
    // ----------------------------------------------------------
    if (count % 50 == 1) {
        sEnemyShotSet* pEnemyShotSet = CreateEnemyShotSet(
            ShotLunaticBlue,
            enemy.x,
            enemy.y,
            0.0,
            0
        );
        pEnemyShotSet->param_i[0] = 10;   // 1環あたりの弾数
        pEnemyShotSet->param_i[1] = 2*2;    // 環の数
        pEnemyShotSet->param_d[0] = (GetRand(1000) / 1000.0) * 2.0 * DX_PI; // 基準角度をランダムに
    }

    // ----------------------------------------------------------
    //  赤弾：逆三角形に整列して自機を狙う赤い座薬弾
    // ----------------------------------------------------------
    if (count % 60 == 30) {
        sEnemyShotSet* pEnemyShotSet = CreateEnemyShotSet(
            ShotLunaticRed,
            enemy.x,
            enemy.y + 10.0,
            0.0,
            1
        );
        pEnemyShotSet->param_i[0] = 3+7;    // 段数（逆三角形の段数）
        pEnemyShotSet->param_i[1] = 5+5;    // 1段あたり最大弾数
        pEnemyShotSet->param_d[0] = 8.0;  // 段の間隔（自機方向）
        pEnemyShotSet->param_d[1] = 10.0; // 弾の間隔（垂直方向）
        pEnemyShotSet->param_d[2] = 2.0;  // 速さ
    }
}