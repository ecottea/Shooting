// enemyPat_Tmp.cpp
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 隕石弾幕パターン「メテオフォール」
static void MeteorFall(sEnemyShotSet* pSet)
{
    int t = pSet->count;

    // 初期化（最初のフレーム）
    if (t == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // 隕石本体（黒い大玉）
        sEnemyShot* core = new sEnemyShot;
        core->x = pSet->x;
        core->y = pSet->y;
        core->muki = pSet->muki;
        core->speed = 1.5;
        core->kind = img_enemyShotLargeBall[7]; // 黒
        core->param_i[0] = 1; // 役割: コア
        core->margin = 99999;
        core->prev = pSet->pEnemyShotHead->prev;
        core->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = core;
        pSet->pEnemyShotHead->prev = core;

        // 周囲を公転する小型破片（オレンジ小玉）
        for (int i = 0; i < 6 * 2; i++) {
            sEnemyShot* orb = new sEnemyShot;
            orb->x = core->x;
            orb->y = core->y;
            orb->muki = 0.0;
            orb->speed = 0.0;
            orb->kind = img_enemyShotSmallBall[8]; // オレンジ
            orb->param_i[0] = 2; // 役割: オービター
            orb->param_d[0] = i * (2.0 * DX_PI / 6.0 / 2); // 初期角度
            orb->param_d[1] = 20.0; // 公転半径
            orb->param_d[2] = 0.03;  // 角速度
            orb->margin = 120;
            orb->prev = pSet->pEnemyShotHead->prev;
            orb->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = orb;
            pSet->pEnemyShotHead->prev = orb;
        }
        return;
    }

    // コアを探す
    sEnemyShot* core = nullptr;
    for (sEnemyShot* p = pSet->pEnemyShotHead->next; p != pSet->pEnemyShotHead; p = p->next) {
        if (p->param_i[0] == 1) {
            core = p;
            break;
        }
    }
    if (!core) return;

    // コアの移動フェーズ制御
    if (t < 120) {
        // 飛来フェーズ：初期方向のまま
    }
    else if (t < 240) {
        // 降下フェーズ：真下へ
        core->muki = DX_PI / 2.0;
        core->speed = 1.2;
    }
    else {
        // 爆散フェーズ（t==240で爆発、以降はコアを画面外へ）
        if (t == 240) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

            // 中型破片（赤中玉）を全方位に放出
            for (int i = 0; i < 16 * 3; i++) {
                sEnemyShot* frag = new sEnemyShot;
                frag->x = core->x;
                frag->y = core->y;
                frag->muki = i * (2.0 * DX_PI / 16.0 / 3) + (GetRand(100) - 50) / 100.0 * 0.2;
                frag->speed = 3.0;
                frag->kind = img_enemyShotMediumBall[0]; // 赤
                frag->param_i[0] = 3; // 役割: 破片
                frag->prev = pSet->pEnemyShotHead->prev;
                frag->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = frag;
                pSet->pEnemyShotHead->prev = frag;
            }
            // 追加の小さな破片（オレンジ小玉）をランダム方向へ
            for (int i = 0; i < 8 * 2; i++) {
                sEnemyShot* frag2 = new sEnemyShot;
                frag2->x = core->x;
                frag2->y = core->y;
                frag2->muki = GetRand(360) / 180.0 * DX_PI;
                frag2->speed = 2.5;
                frag2->kind = img_enemyShotSmallBall[8];
                frag2->param_i[0] = 3;
                frag2->prev = pSet->pEnemyShotHead->prev;
                frag2->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = frag2;
                pSet->pEnemyShotHead->prev = frag2;
            }
            // コアを高速で上へ飛ばして画面外へ（自動消去に任せる）
            core->muki = -DX_PI / 2.0;
            core->speed = 10.0;
        }
    }

    // コアの位置を更新
    core->x += core->speed * cos(core->muki);
    core->y += core->speed * sin(core->muki);

    // 降下フェーズ中は尾を引く（黄色の高速弾を上向きに放出）
    if (t >= 120 && t < 240 && (t % 5 == 0)) {
        sEnemyShot* tail = new sEnemyShot;
        tail->x = core->x;
        tail->y = core->y;
        tail->muki = -DX_PI / 2.0;
        tail->speed = 4.0;
        tail->kind = img_enemyShotBullet[1]; // 黄
        tail->param_i[0] = 4; // 役割: 尾
        tail->prev = pSet->pEnemyShotHead->prev;
        tail->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = tail;
        pSet->pEnemyShotHead->prev = tail;
    }

    // コア以外の弾を更新
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        if (p == core) {
            p = p->next;
            continue;
        }

        if (p->param_i[0] == 2) {
            // オービター：コアの周りを公転、徐々に半径拡大
            p->param_d[0] += p->param_d[2];
            if (t >= 120 && t < 240) {
                p->param_d[1] += 0.1; // 破片が剥がれ落ちる表現
            }
            p->x = core->x + p->param_d[1] * cos(p->param_d[0]);
            p->y = core->y + p->param_d[1] * sin(p->param_d[0]);

            // 爆発時は接線方向へ飛び去る
            if (t == 240) {
                p->muki = p->param_d[0] + DX_PI / 2.0;
                p->speed = 2.0;
                p->param_i[0] = 3;
            }
        }
        else {
            // 破片・尾など：自身の速度と向きで直進
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }
        p = p->next;
    }
}

// 敵本体パターン
void EnemyPat_Meteor_DeepSeek()
{
    static int muki = 1;
    static int shot_count = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.5 * muki;
        if (count % 180 == 90) muki *= -1;
    }

    // 180フレームごとに隕石セットを生成
    if (count % 70 == 1) {
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = MeteorFall;

        // 画面外左右どちらかから出現
        if (GetRand(1) == 0) {
            pSet->x = -30.0;
            pSet->y = 50.0 + GetRand(100);
        }
        else {
            pSet->x = 510.0;
            pSet->y = 50.0 + GetRand(100);
        }
        pSet->muki = atan2(240.0 - pSet->y, 240.0 - pSet->x);
        pSet->kind = shot_count++;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
}