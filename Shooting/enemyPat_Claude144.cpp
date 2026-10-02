// 弾幕：凪の螺旋(なぎのらせん)
//
// 全ての弾を 1px/frame 以下の低速弾で構成し、反射神経ではなく「隙間の読み」で避けさせる弾幕。
//   A層(遅い輪): 0.5px/frame, 24方向, 8フレームごと, 輪ごとに5度ずつ回転。240fごとに回転方向を反転。
//   B層(速い輪): 1.0px/frame, 12方向, 24フレームごと, 輪ごとに-9度ずつ回転。A層を追い越していく。
//   480f以降:    B層と同じタイミングで自機狙い弾(1.0px/frame)を1発追加。
//
// 使用素材:
//   A層     小玉(2.5x2.5)   青(4) / シアン(3) ... 回転方向で色を変え、旧渦と新渦を見分けやすくする
//   B層     中玉(7.0x7.0)   黄(1)               ... A層と大きさ・色で明確に区別
//   自機狙い 菱形弾(4.5x2.5) 赤(0)               ... 向きが分かる形で「圧」を表現
//   効果音  sound_enemyShot_light(A層) / sound_enemyShot_medium(B層) / sound_enemyCharge(反転の予告)

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ------------------------------------------------------------
//  パラメータ
// ------------------------------------------------------------
static const double NAGI_DEG = DX_PI / 180.0;   // 1度あたりのラジアン

// A層(遅い輪)
static const int    NAGI_A_WAY = 24;       // 1輪の弾数(15度間隔)
static const int    NAGI_A_INTERVAL = 8;        // 発射間隔[f]
static const double NAGI_A_SPEED = 0.5;      // 弾速[px/f]
static const double NAGI_A_STEP = 5.1 * NAGI_DEG;   // 輪ごとの回転角
static const int    NAGI_A_COLOR_CW = 4;       // 時計回り中の色(青)
static const int    NAGI_A_COLOR_CCW = 3;       // 反時計回り中の色(シアン)

// B層(速い輪)
static const int    NAGI_B_WAY = 12;       // 1輪の弾数(30度間隔)
static const int    NAGI_B_INTERVAL = 24;       // 発射間隔[f]
static const double NAGI_B_SPEED = 1.0;      // 弾速[px/f]
static const double NAGI_B_STEP = -9.1 * NAGI_DEG;  // 輪ごとの回転角(A層の初期回転と逆)
static const int    NAGI_B_COLOR = 1;        // 黄

// 自機狙い弾
static const int    NAGI_AIM_START = 480;      // 追加される時刻[f]
static const double NAGI_AIM_SPEED = 1.0;      // 弾速[px/f]
static const int    NAGI_AIM_COLOR = 0;        // 赤

// フェーズ
static const int    NAGI_PHASE_LEN = 240;      // A層の回転方向を反転する周期[f]
static const int    NAGI_WARN_TIME = 210;      // 周期内のこの時刻に予告音を鳴らす(反転の30f前)

// ------------------------------------------------------------
//  弾幕：全方位リング(A層・B層・自機狙い弾で共用)
//    pEnemyShotSet->kind       : 弾の画像ハンドル
//    pEnemyShotSet->muki       : 1発目の向き
//    pEnemyShotSet->param_i[0] : 弾数(way)
//    pEnemyShotSet->param_i[1] : 発射時に鳴らす効果音ハンドル(-1なら鳴らさない)
//    pEnemyShotSet->param_d[0] : 弾速
// ------------------------------------------------------------
static void ShotNagiRing(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        const int sound = pEnemyShotSet->param_i[1];
        if (sound != -1) {
            if (CheckSoundMem(sound)) StopSoundMem(sound);
            PlaySoundMem(sound, DX_PLAYTYPE_BACK);
        }
        const int    way = pEnemyShotSet->param_i[0];
        const double speed = pEnemyShotSet->param_d[0];
        for (int i = 0; i < way; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            // 進行方向と見た目の向きを一致させる(菱形弾など向きのある弾のため)
            pEnemyShot->muki = pEnemyShotSet->muki + 2.0 * DX_PI * i / way;
            pEnemyShot->speed = speed;
            pEnemyShot->kind = pEnemyShotSet->kind;
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }
    // 等速直進
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// リング1つ分のShotSetを生成してリストに登録する
static void SpawnNagiRing(double x, double y, double muki, int way, double speed, int image, int sound)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotNagiRing;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = muki;
    pEnemyShotSet->kind = image;
    pEnemyShotSet->param_i[0] = way;
    pEnemyShotSet->param_i[1] = sound;
    pEnemyShotSet->param_d[0] = speed;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ------------------------------------------------------------
//  敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_SlowBullet_Claude()
{
    static double aAngle;   // A層：次に発射する輪の開始角
    static int    aDir;     // A層：回転方向(+1:時計回り, -1:反時計回り)
    static double bAngle;   // B層：次に発射する輪の開始角

    if (count == 1) {
        // ゲーム画面は 480x480。敵は画面上部中央に固定
        enemy.x = 240.0;
        enemy.y = 90.0;
        enemy.maxHp = enemy.hp = 300;
        aAngle = 0.0;
        aDir = 1;
        bAngle = 0.0;
    }

    const int t = count - 1;    // 0始まりの経過フレーム

    // --- フェーズ管理：240fごとにA層の回転方向を反転 ---
    if (t > 0 && t % NAGI_PHASE_LEN == 0) {
        aDir = -aDir;
    }
    // 反転の少し前に予告音
    if (t % NAGI_PHASE_LEN == NAGI_WARN_TIME) {
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // --- A層：遅い輪 ---
    if (t % NAGI_A_INTERVAL == 0) {
        const int color = (aDir > 0) ? NAGI_A_COLOR_CW : NAGI_A_COLOR_CCW;
        SpawnNagiRing(enemy.x, enemy.y, aAngle, NAGI_A_WAY, NAGI_A_SPEED,
            img_enemyShotSmallBall[color], sound_enemyShot_light);
        aAngle += NAGI_A_STEP * (double)aDir;
    }

    // --- B層：速い輪(+ 480f以降は自機狙い弾) ---
    if (t % NAGI_B_INTERVAL == 0) {
        SpawnNagiRing(enemy.x, enemy.y, bAngle, NAGI_B_WAY, NAGI_B_SPEED,
            img_enemyShotMediumBall[NAGI_B_COLOR], sound_enemyShot_medium);
        bAngle += NAGI_B_STEP;

        if (t >= NAGI_AIM_START) {
            // 自機狙いの1発(効果音はB層と重なるので鳴らさない)
            double aim = atan2(player.y - enemy.y, player.x - enemy.x);
            SpawnNagiRing(enemy.x, enemy.y, aim, 1, NAGI_AIM_SPEED,
                img_enemyShotDiamond[NAGI_AIM_COLOR], -1);
        }
    }
}