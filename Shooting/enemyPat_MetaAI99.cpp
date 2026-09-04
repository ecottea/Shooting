// EnemyPat_Tmp.cpp
// ドップラー効果モチーフ「ドップラー・サイレン」
// 仕様: ボスが高速で左右往復しながら、等間隔で全方位リングを置いていく。
//       置かれたリングは等速で広がるだけだが、発生源が動いているため
//       進行方向前方ではリング同士が圧縮されて高密度(青)、後方では疎(赤)になる。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 使える素材一覧 (enemyPat_sampleForAI.cppより)
// 効果音: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge
// 弾画像: img_enemyShotSmallBall[9], img_enemyShotMediumBall[9], img_enemyShotLargeBall[9], img_enemyShotBullet[9], img_enemyShotScale[9], img_enemyShotDiamond[9], img_enemyShotMediumOval[9], img_enemyShotLaser[9]
// 色インデックス: 0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙
//
// 素材選定理由:
// - SE: sound_enemyShot_light ... 12F毎の連射なので軽い音が最適。重い音だと騒音になる。
// - 弾形状: 小玉(2.5x2.5) = 青方偏移=波長が短い=小さい, 中玉(7.0x7.0) = 赤方偏移=波長が長い=大きい
//          見た目で波長の変化を表現するため2種類を使い分け。大きすぎる大玉やレーザーはリングの美しさを損なうため不採用。
// - 色: 4:青 = 前方最密部、3:シアン = 前方、8:橙 = 後方、0:赤 = 後方最疎部。物理の青方偏移/赤方偏移をそのまま色で表現。

static void ShotDopplerWave(sEnemyShotSet* pEnemyShotSet)
{
    // pEnemyShotSet->count, pEnemyShot->count のインクリメントと画面外削除はメイン側で行われる

    if (pEnemyShotSet->count == 0) {
        // 発生時に軽いショット音
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        double bossVx = pEnemyShotSet->param_d[0]; // EnemyPat_Tmpから渡されるボスのVx

        const int WAY = 32;          // 1リングの弾数。32で円が綺麗に見える
        const double baseSpeed = 2.0; // リングの広がる速さ。遅いほどドップラー圧縮が分かりやすい

        for (int i = 0; i < WAY; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;

            double ang = 2.0 * DX_PI * (double)i / (double)WAY;

            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = ang;
            pEnemyShot->speed = baseSpeed;

            // 進行方向判定: 弾の進行方向(cos)とボスの進行方向(Vx)の内積
            // 正なら前方(圧縮)、負なら後方(伸長)
            double forward = cos(ang) * bossVx;

            int col;
            if (forward > 2.0) {
                col = 4; // 青 - 最前方 高密度
            }
            else if (forward > 0.2) {
                col = 3; // シアン - 前方
            }
            else if (forward > -2.0) {
                col = 8; // 橙 - 後方
            }
            else {
                col = 0; // 赤 - 最後方 低密度
            }

            // 青方偏移は小さく、赤方偏移は大きく
            if (forward > 0) {
                pEnemyShot->kind = img_enemyShotSmallBall[col];
            }
            else {
                pEnemyShot->kind = img_enemyShotMediumBall[col];
            }

            // 双方向リストへ追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 移動 - 全て等速直進。発生源が動くことで自然にドップラー圧縮が起きる
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

void EnemyPat_Doppler_MetaAI()
{
    static int dir;        // 1:右 -1:左
    static double baseVx;  // 基本速度

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 100;
        dir = 1;
        baseVx = 7.5; // 弾速2.0に対して3.75倍。ドップラーが最も綺麗に見える比率
    }
    else {
        // 高速左右往復
        enemy.x += baseVx * (double)dir;

        // 端で反転
        if (enemy.x < 70.0) {
            enemy.x = 70.0;
            dir = 1;
        }
        if (enemy.x > 410.0) {
            enemy.x = 410.0;
            dir = -1;
        }
    }

    // 12F毎に1リングを生成
    if (count % 12 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotDopplerWave;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        // ボスの速度を弾側に渡す。色分けに使用
        pEnemyShotSet->param_d[0] = baseVx * (double)dir;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // グローバルリストへ追加
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
