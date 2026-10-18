// =====================================================================
// 銃符「ルナティックガン」再現弾幕  (enemyPat_Tmp.cpp)
// ---------------------------------------------------------------------
// 【原作データ】 東方紺珠伝 ～ Legacy of Lunatic Kingdom.
//   使用者 : 1面ボス 清蘭(せいらん) … 浅葱色のEagle Rabbit(月の兎)
//   難易度 : Easy / Normal / Hard / Lunatic 全難易度の1面2番目の符
//   能力   : 「異次元から弾丸を飛ばす程度の能力」
//
// 【原作の挙動(調査結果)】
//   1. 清蘭が画面上部を左右に移動しながら、自機を狙った高速の銃弾を
//      バースト連射してくる(ランダムブレ付き自機狙い)。
//   2. 能力の演出として、画面内のランダムな位置に縦一列に連なった
//      銃弾が突然出現し、非常に速い弾速で画面下へ突き抜けていく。
//   3. 弾速が速いため下側での回避は危険で、自機狙い弾を左右に大きく
//      引きながら、縦列弾との交差の谷間を縫って避ける弾幕。
//
// 【使用素材の選定】
//   弾 : img_enemyShotBullet(銃弾 5.0x2.0) のみを使用。
//        - img_enemyShotBullet[4](青) … 清蘭本体の自機狙い銃撃
//        - img_enemyShotBullet[1](黄) … 異次元から出現する縦列銃弾
//        ※原作が「銃撃戦」であるため、玉弾やレーザー等は不使用。
//          2色に分けて自機狙い弾と縦列弾を視認しやすくしている。
//   音 : sound_enemyShot_extreme … 銃撃(バースト)の発射音
//        sound_enemyCharge      … 縦列弾出現の予告音
//        sound_enemyShot_heavy  … 縦列弾出現音
// =====================================================================

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---------------------------------------------------------------------
// 弾セット(sEnemyShotSet)を生成してグローバルリストに登録するヘルパー
// ---------------------------------------------------------------------
static sEnemyShotSet* CreateShotSet(sEnemyShotSet::PatternFunc patternFunc,
    double x, double y, double muki, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = patternFunc;
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

// ---------------------------------------------------------------------
// 自機狙いバースト銃撃
//   4フレームおきに左右2発ずつ(2丁拳銃のイメージ)、計6回=12発。
//   1発ごとに ±5° のランダムブレが入り、弾速は 8.00〜9.00 と速い。
// ---------------------------------------------------------------------
static void ShotLunaticAim(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 銃撃音(重ならないよう鳴っていたら止めてから再生)
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
    }

    // count 0,4,8,12,16,20 の6回、毎回2発 = 12発のバースト
    if (pEnemyShotSet->count % 4 == 0 && pEnemyShotSet->count < 24) {
        for (int i = 0; i < 2; i++) {
            pEnemyShot = new sEnemyShot;
            // 銃口を左右に分けて発射
            pEnemyShot->x = pEnemyShotSet->x + (i * 2 - 1) * 10.0;
            pEnemyShot->y = pEnemyShotSet->y;
            // 自機狙い + ランダムブレ ±5° (GetRand(10)は0〜10なので-5で補正)
            pEnemyShot->muki = pEnemyShotSet->muki + (GetRand(10) - 5) / 180.0 * DX_PI;
            // 弾速 8.00〜9.00
            pEnemyShot->speed = 8.0 + GetRand(100) / 100.0;
            pEnemyShot->kind = img_enemyShotBullet[4]; // 青銃弾
            // リンクリストの末尾に追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ---------------------------------------------------------------------
// 異次元縦列銃弾
//   15フレームおきに計3列出現。各列は画面上端付近のランダムX位置に
//   縦4連で出現し、真下(弾速11.0)へ高速で突き抜ける。
//   50%の確率で自機の近く(±80px)に出現する(異次元からの狙撃イメージ)。
// ---------------------------------------------------------------------
static void ShotLunaticColumn(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // 出現の予告音
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // count 1,16,31 の3回、毎回1列(4発) = 計12発
    if (pEnemyShotSet->count % 15 == 1 && pEnemyShotSet->count <= 31) {
        double gx;
        if (GetRand(1) == 0) {
            gx = 40.0 + GetRand(400);              // 画面内 40〜440 のランダム位置
        }
        else {
            gx = player.x + GetRand(160) - 80;     // 自機X -80〜+80
        }
        // ゲート位置が画面外に暴れないようクランプ
        if (gx < 20.0) gx = 20.0;
        if (gx > 460.0) gx = 460.0;

        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int j = 0; j < 4; j++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = gx;
            pEnemyShot->y = -12.0 - j * 24.0;      // 画面上端の外に縦4連で並べる
            pEnemyShot->muki = DX_PI / 2.0;        // 真下向き
            pEnemyShot->speed = 11.0;
            pEnemyShot->kind = img_enemyShotBullet[1]; // 黄銃弾
            pEnemyShot->margin = 240;
            // リンクリストの末尾に追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// =====================================================================
// 敵本体のパターン: 銃符「ルナティックガン」
// =====================================================================
void EnemyPat_Tmp()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
    }
    else {
        // 清蘭は画面上部を左右に往復しながら、上下にもわずかに揺れる
        enemy.x += 0.8 * (double)muki;
        if (count % 120 == 60) muki *= -1;
        enemy.y = 40.0 + 12.0 * sin(count / 40.0 * DX_PI);
    }

    // ---- 攻撃サイクル: 140フレーム周期 ----
    // 自機狙いバースト銃撃 ×2 (周期の前半・中盤)
    if (count % 140 == 10 || count % 140 == 60) {
        double mx = enemy.x;
        double my = enemy.y + 10.0;
        double am = atan2(player.y - my, player.x - mx); // 発射時の自機狙い
        CreateShotSet(ShotLunaticAim, mx, my, am, shot_count++);
    }
    // 異次元縦列銃弾 (周期の後半、バースト後の一杯おき)
    if (count % 140 == 90) {
        CreateShotSet(ShotLunaticColumn, enemy.x, enemy.y, DX_PI / 2.0, shot_count++);
    }
}
