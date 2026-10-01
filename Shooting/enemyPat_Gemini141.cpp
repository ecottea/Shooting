#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：罔両「禅寺に棲む妖蝶」風パターン
static void ShotZenji(sEnemyShotSet* pEnemyShotSet)
{
    // 発射位置を常に敵本体に追従させる
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;

    // 1. 卍型の回転ワインダー（光の腕）
    // 毎フレーム発射だと隙間なくレーザー状の壁になる（処理落ちに注意し 2フレームに1回発射）
    if (pEnemyShotSet->count % 2 == 0) {
        if (pEnemyShotSet->count % 8 == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        // 腕の回転角。徐々に回る。
        double base_angle = pEnemyShotSet->count * 0.02;

        for (int i = 0; i < 4; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = base_angle + i * (DX_PI / 2.0); // 90度ずつ4方向
            pEnemyShot->speed = 3.0; // 腕が広がる速度
            pEnemyShot->kind = img_enemyShotMediumOval[5]; // マゼンタの中楕円弾
            pEnemyShot->param_i[1] = 0; // 0: ワインダー弾（直進のみ）

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 2. 渦状に撒かれる青蝶弾（鱗弾）
    // 一定間隔で自機狙い方向を基準に全方位へ発射
    if (pEnemyShotSet->count % 90 == 0) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        // 毎回少しずつ角度をずらして渦巻き感と自機狙いを両立
        double base_muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x) + pEnemyShotSet->count * 0.01;
        int way = 12;
        for (int i = 0; i < way; i++) {
            sEnemyShot* pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = base_muki + i * (DX_PI * 2.0 / way);
            pEnemyShot->speed = 2.5; // 初期速度
            pEnemyShot->kind = img_enemyShotScale[4]; // 青の鱗弾
            pEnemyShot->param_i[1] = 1; // 1: 蝶弾（反射する）
            pEnemyShot->param_i[0] = 0; // 反射フラグ (0: 未反射, 1: 反射済)

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 弾の座標更新と特殊挙動（折り返し）の処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {

        // 蝶弾(青)の特殊挙動：減速しながら画面外へ行き、自機を狙って戻ってくる
        if (pShot->param_i[1] == 1) {
            if (pShot->param_i[0] == 0) { // まだ反射していない場合
                // 外側に向かって徐々に減速
                if (pShot->speed > 1.0) pShot->speed -= 0.01;

                // 画面端判定（メインルーチンの画面外消去に引っかからないよう、少しはみ出た位置で判定）
                if (pShot->x < -5.0 || pShot->x > 485.0 || pShot->y < -5.0 || pShot->y > 485.0) {
                    pShot->param_i[0] = 1; // 反射済みに移行
                    // 戻ってくる方向を自機狙いにする（Phantasmの恐怖感を演出）
                    pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                    pShot->speed = 1.0; // 反射直後の速度
                }
            }
            else { // 反射済みの場合
                // 戻ってくるときは徐々に加速して突っ込んでくる
                if (pShot->speed < 3.0) pShot->speed += 0.02;
            }
        }
        // ※ワインダー弾(param_i[1] == 0)はそのまま等速直線運動

        // 座標更新
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 敵本体のパターン（指定関数名）
void EnemyPat_Zendera_Gemini()
{
    // 初期化処理
    if (count == 1) {
        // 画面中央上部に配置
        enemy.x = 240.0;
        enemy.y = 240.0;
        enemy.maxHp = enemy.hp = 100;
    }

    // スペルカード予告音（30フレーム目に再生）
    if (count == 30) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // スペルカード発動（弾幕セットの登録）
    if (count == 90) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotZenji;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}