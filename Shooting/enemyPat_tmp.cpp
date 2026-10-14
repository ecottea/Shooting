// ============================================================
//  弾幕パターン:二重スリット実験モチーフ「干渉の波紋」
// ------------------------------------------------------------
//  ・大玉の青弾を並べた「スリット壁」を画面上部に配置(2箇所の隙間=スリット)
//  ・壁の上から白い小玉の「粒子」が降りてくる
//  ・粒子がスリットに到達すると、そのスリットが波源となり
//    干渉角ずつの扇状弾(縞=明線)をプレイヤー方向へ放つ
//    - スリットA由来:シアンの小玉
//    - スリットB由来:マゼンタの小玉
//  ・プレイヤーがスリット間の中央線に近づくと「観測」となり、
//    干渉が消えて各スリットから1発の白い中玉(粒子として振る舞う)が直進する
//  ・壁は正弦波で揺れるため、波源位置と干渉縞も連動して動く
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---- 定数 ----
static const double GAME_CENTER_X = 240.0;  // ゲーム画面中央X(480x480)
static const double WALL_Y_BASE = 140.0;  // スリット壁の基準Y
static const double WALL_SWAY_AMP = 10.0;   // 壁の揺れ振幅
static const double SLIT_A_X = 160.0;  // スリットAの中心X
static const double SLIT_B_X = 320.0;  // スリットBの中心X
static const double SLIT_HALF_WIDTH = 30.0;   // スリットの半幅
static const double OBSERVE_RANGE = 60.0;   // 観測判定:中央線からこの距離以内で観測
static const double INTERFERENCE_ANGLE = 0.13;   // 干渉角(明線同士の角度間隔)
static const double WAVE_SPAWN_OFFSET = 6.0;    // 波を壁の少し下から出すオフセット

// param_i[1] の意味
//   1:粒子(スリットに到達すると波に変わる) / 0:波の弾(判定なし)
static const int PARTICLE_FLAG = 1;

// 位置xにおける壁の現在のY(グローバルcountに同期 → 全セットで共有できる)
static double WallYAt(double x)
{
    return WALL_Y_BASE + WALL_SWAY_AMP * sin(count * 0.02 + (x - GAME_CENTER_X) * 0.012);
}

// 弾をセットのリスト末尾に追加する
static sEnemyShot* AddShot(sEnemyShotSet* pEnemyShotSet, double x, double y, double muki, double speed, int kind)
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

// ---- スリット壁:大玉(青)を横一列に並べ、2箇所の隙間をスリットとする ----
// 弾自体は不動(speed=0)。毎フレーム、壁全体を正弦波で上下に揺らすだけ。
static void ShotWall(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->y = WallYAt(pShot->x);   // xごとに位相のずれた揺れ(生きている壁の表現)
        pShot = pShot->next;
    }
}

// ---- 粒子と干渉:本体パターン ----
static void ShotParticle(sEnemyShotSet* pEnemyShotSet)
{
    // 30フレームごとに、上から白い小玉の「粒子」を1発降らせる
    // スリットA/Bを交互に狙わせることで、2色の波が交互に放たれる
    if (pEnemyShotSet->count % 10 == 0) {
        pEnemyShotSet->param_i[0] ^= 1; // 0↔1をトグル(スリットA/B交互)
        double slitX = (pEnemyShotSet->param_i[0] == 0) ? SLIT_A_X : SLIT_B_X;

        sEnemyShot* pShot = AddShot(pEnemyShotSet,
            slitX + GetRand(40) - 20,   // スリット内にランダムに着弾させる
            -10.0,                      // 画面上端の外から降ってくる
            DX_PI / 2.0,                // 真下へ
            2.5,
            img_enemyShotSmallBall[6]); // 白:どちらのスリットを通るか未知の粒子
        pShot->param_i[0] = pEnemyShotSet->param_i[0]; // 狙ったスリットを記憶
        pShot->param_i[1] = PARTICLE_FLAG;             // 粒子フラグ(★無限ループ修正点)
        pShot->param_d[0] = slitX;                     // スリット中心Xを記憶

        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // 粒子の移動と、スリット到達時の波の放出
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 削除に備えて先に保存

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // ★無限ループ修正点:粒子フラグが立っている弾のみ、
        //   壁の位置(スリット)への到達判定を行う。
        //   これがないと、スリットから出現した波の弾自身が
        //   「スリットに到達した粒子」と誤認識され、波を無限に生成し続けていた。
        if (pShot->param_i[1] == PARTICLE_FLAG && pShot->y >= WallYAt(pShot->param_d[0])) {
            double sx = pShot->param_d[0];           // 波源X(スリット中心)
            double sy = WallYAt(sx) + WAVE_SPAWN_OFFSET; // 波源Y(揺れてる壁の少し下)
            int    slit = pShot->param_i[0];

            // 「観測」:プレイヤーがスリット間の中央線に近いと干渉が消える
            if (fabs(player.x - GAME_CENTER_X) < OBSERVE_RANGE) {
                // 粒子として振る舞う:各スリットから1発の白い中玉がプレイヤーへ直進
                AddShot(pEnemyShotSet,
                    sx, sy,
                    atan2(player.y - sy, player.x - sx),
                    4.5,
                    img_enemyShotMediumBall[6]); // 白:観測された粒子

                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
            }
            else {
                // 干渉:プレイヤー方向を中心に干渉角ずつ角度をずらした縞状の弾を放つ
                // k = -3..3 の7本の「明線」に相当する放射線が描かれる
                double baseAngle = atan2(player.y - sy, player.x - sx);
                int color = (slit == 0) ? 3 : 5;     // シアン:A由来 / マゼンタ:B由来

                for (int k = -3; k <= 3; k++) {
                    double angle = baseAngle + k * INTERFERENCE_ANGLE;
                    // 同角度に2速で発射 → 放射線が「縞」らしく見える
                    for (int j = 0; j < 2; j++) {
                        AddShot(pEnemyShotSet,
                            sx, sy,
                            angle,
                            2.2 - j * 0.8,                       // 2.2 と 1.4
                            img_enemyShotSmallBall[color]);
                    }
                }

                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }

            // 粒子を削除(波に変わった)
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }
        pShot = pNext;
    }
}

// ---- 補助攻撃:敵本体からの3方向鱗弾(定位置に居させないための攪乱) ----
static void ShotAim(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        for (int i = -1; i <= 1; i++) {
            AddShot(pEnemyShotSet,
                pEnemyShotSet->x, pEnemyShotSet->y,
                pEnemyShotSet->muki + i * 0.15,
                3.0,
                img_enemyShotScale[8]); // 橙の鱗弾
        }
    }
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Tmp()
{
    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと

        // 開幕の予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // --- スリット壁セットの生成(一度だけ) ---
        sEnemyShotSet* pWallSet = new sEnemyShotSet;
        pWallSet->count = 0;
        pWallSet->patternFunc = ShotWall;
        pWallSet->x = 0.0;
        pWallSet->y = WALL_Y_BASE;
        pWallSet->muki = 0.0;
        pWallSet->kind = 0;
        pWallSet->pEnemyShotHead = new sEnemyShot;
        pWallSet->pEnemyShotHead->prev = pWallSet->pEnemyShotHead;
        pWallSet->pEnemyShotHead->next = pWallSet->pEnemyShotHead;
        pWallSet->prev = enemyShotSetHead.prev;
        pWallSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pWallSet;
        enemyShotSetHead.prev = pWallSet;

        // 壁本体:大玉(青)を x=40〜440 に20px間隔で並べ、
        // スリットA(x=160)/B(x=320)の周囲だけ隙間を空ける
        for (int i = 0; i <= 20; i++) {
            double x = 40.0 + i * 20.0;
            if (fabs(x - SLIT_A_X) <= SLIT_HALF_WIDTH) continue; // スリットA
            if (fabs(x - SLIT_B_X) <= SLIT_HALF_WIDTH) continue; // スリットB
            AddShot(pWallSet, x, WALL_Y_BASE, 0.0, 0.0, img_enemyShotLargeBall[4]);
        }

        // --- 粒子セットの生成(一度だけ、以降は自分で粒子を降らせ続ける) ---
        sEnemyShotSet* pParticleSet = new sEnemyShotSet;
        pParticleSet->count = 0;
        pParticleSet->patternFunc = ShotParticle;
        pParticleSet->x = 0.0;
        pParticleSet->y = 0.0;
        pParticleSet->muki = 0.0;
        pParticleSet->kind = 0;
        pParticleSet->param_i[0] = 0; // 0:次はスリットA / 1:次はスリットB
        pParticleSet->alive = 9999;
        pParticleSet->pEnemyShotHead = new sEnemyShot;
        pParticleSet->pEnemyShotHead->prev = pParticleSet->pEnemyShotHead;
        pParticleSet->pEnemyShotHead->next = pParticleSet->pEnemyShotHead;
        pParticleSet->prev = enemyShotSetHead.prev;
        pParticleSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pParticleSet;
        enemyShotSetHead.prev = pParticleSet;
    }
    else {
        // 敵本体はゆっくり左右に揺れる(演出用)
        enemy.x = 240.0 + 50.0 * sin((count - 1) * 0.008);
    }

    // 補助攻撃:180フレームごとに3方向の鱗弾
    if (count > 1 && count % 180 == 90) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotAim;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}