// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
//  弾幕パターン：逆位相干渉鞭（フェイズ・キャンセリング・ウィップ）
// ============================================================
// 概要:
// 2本の鞭が互いに逆位相でうねりながら飛来し、一定間隔で交差する。
// sEnemyShotSet を「鞭の根元」として長寿命化させ、
// 毎フレーム弾を1発ずつ生成し続けることで、連続した軌道（鞭）を形成する。
//
// 使用素材:
// - 弾画像: img_enemyShotBullet (銃弾) ... 細長い形状が鞭のしなりに適している
// - 弾の色: 3:シアン / 5:マゼンタ ... 対比させることで交差点を視認しやすくする
// - 効果音: sound_enemyShot_light ... 連射時の耳障りさを抑えるため
// ============================================================

static void ShotWinder(sEnemyShotSet* pSet)
{
    // pSet->count はメインルーチンで毎フレーム +1 される
    int t = pSet->count;

    // 位相フラグ (0 or 1) によって sin の符号を反転させ、逆位相を実現する
    int phase_flag = pSet->param_i[0];
    double phase_offset = (phase_flag == 0) ? 0.0 : DX_PI;

    // --- うねりの計算 ---
    // 角周波数 0.15 rad/frame, 振幅 0.6 rad (約34度)
    double freq = 0.15;
    double amp_angle = 0.6;

    // 発射座標のズレ (鞭の根元がしなるイメージ)
    // 角周波数 0.15 rad/frame, 振幅 30.0 px
    double amp_pos = 30.0;

    double wave = sin(t * freq + phase_offset);

    // 発射角度のオフセット
    double angle_offset = wave * amp_angle;

    // 発射座標のオフセット (鞭の進行方向に対して垂直方向にずらす)
    // ※muki は下向き(PI/2)を想定
    double pos_offset = wave * amp_pos;

    // --- 弾の生成 (毎フレーム1発) ---
    sEnemyShot* pShot = new sEnemyShot;

    // 発射座標をずらす
    // 進行方向に対して垂直な成分 (muki + PI/2) にオフセットをかける
    pShot->x = pSet->x + cos(pSet->muki + DX_PI / 2) * pos_offset;
    pShot->y = pSet->y + sin(pSet->muki + DX_PI / 2) * pos_offset;

    // 角度をずらす
    pShot->muki = pSet->muki + angle_offset;

    // 速度と種類
    pShot->speed = 3.5; // 少し遅めにし、軌道の視認性を高める

    // 弾画像: 銃弾 (Bullet)
    // 色: 位相0ならシアン(3)、位相1ならマゼンタ(5)
    int color_id = (phase_flag == 0) ? 3 : 5;
    pShot->kind = img_enemyShotBullet[color_id];

    // リンクリストに追加
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;

    // --- 効果音 ---
    // 連射音として、数フレームに1回だけ鳴らす
    if (t % 4 == 0) {
        if (CheckSoundMem(sound_enemyShot_light) == 0) {
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }
    }

    sEnemyShot* pEnemyShot = pSet->pEnemyShotHead->next;
    while (pEnemyShot != pSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);

        pEnemyShot = pEnemyShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Winder_Qwen()
{
    static int muki;

    if (count == 1) {
        // 初期化
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // ----------------------------------------
        // 2本の鞭 (sEnemyShotSet) を生成
        // ----------------------------------------
        for (int i = 0; i < 2; i++) {
            sEnemyShotSet* pSet = new sEnemyShotSet;
            pSet->count = 0;
            pSet->patternFunc = ShotWinder;

            // 発射位置 (敵の中央下)
            pSet->x = enemy.x;
            pSet->y = enemy.y - 20.0;

            // 発射角度 (下向き)
            pSet->muki = DX_PI / 2.0;

            // 位相フラグ設定 (0: 1本目, 1: 2本目)
            pSet->param_i[0] = i;

            // 弾リストの初期化
            pSet->pEnemyShotHead = new sEnemyShot;
            pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

            // 敵の弾幕セットリストに追加
            pSet->prev = enemyShotSetHead.prev;
            pSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pSet;
            enemyShotSetHead.prev = pSet;
        }
    }
    else {
        // 敵本体の動き (左右にゆっくり移動)
        enemy.x += 1.0 * (double)muki;
        if (enemy.x > 440 || enemy.x < 40) muki *= -1;

        // 鞭の発射座標も敵に合わせて更新する
        // ※sEnemyShotSet は毎フレーム ShotWinder が呼ばれる際、
        //   pSet->x/y を参照して弾を生成するため、ここで更新すると
        //   敵の移動に従って鞭の根元も動くようになる
        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            pSet->x = enemy.x;
            pSet->y = enemy.y - 20.0;
            pSet = pSet->next;
        }
    }
}