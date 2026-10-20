// ============================================================
//  銃符「ルナティックガン」
//  (東方紺珠伝のスペルカード風の弾幕パターン)
//  ------------------------------------------------------------
//  【A】ボスを中心に、狭い間隔(18度)と広い間隔(54度)が交互に並んだ
//      10way全方位弾を、10フレーム間隔で常時発射する。
//      1発射ごとに基準角を時計回りへ8度ずつ進めるため、花弁状の
//      配列が画面上を時計回りに回転し続ける。
//
//  【B】ボスの位置から自機へ向かって、逆三角形(▽)を成す密な弾列
//      5本(中央8発/中間5発×2/外側2発×2、計22発)を僅かに時間差
//      (3フレームずつ)を付けて発射する。中央→中間→外側の順に撃ち、
//      遅れて撃つ弾列ほど発射位置を前方へ前倒しすることで、飛行中も
//      三角形の隊列が崩れない(速度は全弾3.45に固定し剛体隊列に)。
//      「荒さ」は1セットごとの照準誤差(±2.8度)で表現する。
//      これを1セットとし、4フレームだけ空けて間髪入れずにもう1セットを
//      撃ち、その後約64フレームの間を空けて繰り返す(周期80フレーム)。
//
//  【素材選定】
//   ・A: 中玉(色5:マゼンタ) … 回転する花弁リングの形を視認させる丸弾
//   ・B: 銃弾(色6:白)      … 「銃符」の名に合わせ、進行方向を向く
//                             細長い銃弾で銃撃感を演出
//   ・発射音: A=light(10フレーム間隔の連射音) / B=heavy(1セットに1発の重い銃声)
//   ・sound_enemyCharge(予告音)は常時反復型の攻撃のため不使用
//   ・大玉/鱗弾/菱形弾/中楕円弾/短レーザーは本弾幕では不使用
//
//  【実装上の前提】
//   ・count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと、
//     画面外に出た弾の消去はメインルーチンで行われる(本ファイルでは行わない)
//   ・GetRand(x) は 0〜x の x+1 種類の整数を返す
//     例: GetRand(8)-4 → -4〜+4 の9通り
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
// A: 回転する10way全方位弾(狭い間隔と広い間隔が交互)
// ------------------------------------------------------------
static void ShotLunaticGunRing(sEnemyShotSet * pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        // 連射感のある軽い発射音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 10way = 2発1組のペアが5組。ペア内の間隔は狭く(18度)、
        // ペア同士の間は広く(54度)空ける。18×5 + 54×5 = 360度。
        for (int i = 0; i < 10; i++) {
            double ang = pEnemyShotSet->muki                  // 今回のリングの基準角
                + (i / 2) * (72.0 * DX_PI / 180.0)     // ペアの位置(72度間隔)
                + (i % 2) * (18.0 * DX_PI / 180.0);    // ペア内の狭い間隔
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = 2.0;
            pEnemyShot->kind = img_enemyShotMediumBall[5];   // マゼンタ中玉
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// B: 自機へ向かって逆三角形を成す密な弾列5本を時間差で発射
//    中央(8発)→中間左右(各5発)→外側左右(各2発)の順に3フレーム間隔で撃つ。
//    遅れて撃つ弾列ほど発射位置を前方へ前倒しする(3フレーム×速度3.45)ので、
//    すべての弾が同一速度で飛んでも逆三角形(▽)の隊列が保たれる。
// ------------------------------------------------------------
static void ShotLunaticGunWedge(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    int c = pEnemyShotSet->count;

    if (c == 0) {
        // 重い銃声(1セット=1回の発射)
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // 照準: 自機方向を基準に少し荒く(±2.8度のランダム誤差)
        // GetRand(8)-4 は -4〜+4 の9通りを返す
        pEnemyShotSet->param_d[0] = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x)
            + (GetRand(8) - 4) * 0.7 * DX_PI / 180.0;
    }

    // 弾列の発射タイミング: c==0 で中央、c==3 で中間左右、c==6 で外側左右
    if (c == 0 || c == 3 || c == 6) {
        int step = c / 3;                       // 0:中央 1:中間 2:外側
        const int colHeight[3] = { 8, 5, 2 };   // 各弾列の弾数(中央が8発=1辺8発分)
        double aim = pEnemyShotSet->param_d[0];
        double ax = cos(aim), ay = sin(aim);    // 自機への進行方向
        double px = -ay, py = ax;               // 進行方向と垂直な方向(弾列の並び)

        // 中央は1列のみ、中間と外側は左右の2列
        for (int s = (step == 0 ? 0 : -1); s <= 1; s += 2) {
            int col = s * step;                              // 弾列番号 -2〜+2
            double lead = 3.45 * 3.0 * step;                 // 発射遅れ3フレーム分を前倒し(速度3.45に一致)
            for (int j = 0; j < colHeight[step]; j++) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x + ax * (8.0 + j * 10.0 + lead) + px * col * 12.0;
                pEnemyShot->y = pEnemyShotSet->y + ay * (8.0 + j * 10.0 + lead) + py * col * 12.0;
                // 全弾が同一方向・同一速度で飛ぶことで隊列は剛体に保たれる
                // (荒さはセット毎の照準誤差で表現)
                pEnemyShot->muki = aim;
                pEnemyShot->speed = 3.45;
                pEnemyShot->kind = img_enemyShotBullet[6];  // 白の銃弾
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン: 銃符「ルナティックガン」
// ------------------------------------------------------------
void EnemyPat_Tmp()
{
    static double ringAngle; // A用: 全方位弾の基準角(発射ごとに時計回りへ進む)
    static int    mukiX;     // ボスの移動方向

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        ringAngle = 0.0;
        mukiX = 1;
    }
    else {
        // ゆっくり左右に揺れる(全ての弾はその瞬間のボス位置を基準に放たれる)
        enemy.x += 0.45 * (double)mukiX;
        if (count % 140 == 0) mukiX *= -1;
    }

    // ---- A: 常時撃ち続ける(10フレーム間隔で1リング、時計回りに回転) ----
    if (count % 10 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLunaticGunRing;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->muki = ringAngle;       // 今回のリングの基準角
        pEnemyShotSet->kind = 0;
        ringAngle += 8.0 * DX_PI / 180.0;      // 次のリングは時計回りに8度回転
        // (画面座標は y 下向きなので +方向が時計回り)
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }

    // ---- B: 1セット撃ったら4フレームだけ空けてもう1セット撃ち、
    //         その後約64フレームの間を空けて繰り返す(周期80フレーム) ----
    // 1セット目: 位相0(弾列は位相0/3/6)  2セット目: 位相10(弾列は位相10/13/16)
    // count>=80 から開始し、最初のサイクルから必ず2セット1組のリズムになるようにする
    int phase = count % 80;
    if (count >= 80 && (phase == 0 || phase == 10)) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotLunaticGunWedge;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = (phase == 0) ? 0 : 1;  // 0:1セット目 1:2セット目
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
