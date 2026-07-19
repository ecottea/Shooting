// enemyPat_Tmp.cpp
// 絶対に回避できない弾幕パターン (高密度・多層・予測困難な動き)
// 新規ファイルとしてまとめたもの。EnemyPat_Unavoidable_Grok() を使用してください。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ======================
// 補助パターン関数群
// ======================

// 高速狙い散弾 (密度高め)
static void ShotDenseAimed(sEnemyShotSet* pEnemyShotSet) {
    sEnemyShot* pEnemyShot;
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        int num = 12 + (pEnemyShotSet->kind % 5); // 12〜16発
        double baseAngle = pEnemyShotSet->muki;
        for (int i = 0; i < num; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            double spread = (i - num / 2.0) * 0.12;
            pEnemyShot->muki = baseAngle + spread;
            pEnemyShot->speed = 4.2 + GetRand(80) / 100.0;
            // 種類選択
            switch (pEnemyShotSet->kind % 6) {
            case 0: pEnemyShot->kind = img_enemyShotSmallBall[GetRand(8)]; break;
            case 1: pEnemyShot->kind = img_enemyShotMediumBall[GetRand(8)]; break;
            case 2: pEnemyShot->kind = img_enemyShotLargeBall[GetRand(8)]; break;
            case 3: pEnemyShot->kind = img_enemyShotBullet[GetRand(8)]; break;
            case 4: pEnemyShot->kind = img_enemyShotScale[GetRand(8)]; break;
            default: pEnemyShot->kind = img_enemyShotDiamond[GetRand(8)]; break;
            }
            pEnemyShot->param_i[0] = GetRand(30) - 15; // 微小角度変化用
            // リンク挿入
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 更新
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        // 軽いカーブ
        pEnemyShot->muki += pEnemyShot->param_i[0] / 800.0;
        pEnemyShot = pEnemyShot->next;
    }
}

// 回転リング + 加速 (予測困難)
static void ShotRotRing(sEnemyShotSet* pEnemyShotSet) {
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        int num = 18;
        for (int i = 0; i < num; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x;
            p->y = pEnemyShotSet->y;
            p->muki = pEnemyShotSet->muki + i * (DX_PI * 2 / num);
            p->speed = 2.8;
            p->kind = img_enemyShotMediumBall[GetRand(8)];
            p->param_d[0] = 0.035; // 回転速度
            p->param_i[0] = i;     // 位相

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        p->muki += p->param_d[0];
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        // 徐々に加速
        if (pEnemyShotSet->count > 40) p->speed += 0.012;
        p = p->next;
    }
}

// ランダムばら撒き + ホーミング要素
static void ShotChaosScatter(sEnemyShotSet* pEnemyShotSet) {
    if (pEnemyShotSet->count == 0) {
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 22; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = pEnemyShotSet->x + GetRand(60) - 30;
            p->y = pEnemyShotSet->y + GetRand(40) - 20;
            p->muki = GetRand(628) / 100.0; // ほぼ全方向
            p->speed = (180 + GetRand(220)) / 100.0;
            p->kind = img_enemyShotScale[GetRand(8)];
            p->param_d[0] = player.x; // 初期ホーミングターゲット
            p->param_d[1] = player.y;

            p->prev = pEnemyShotSet->pEnemyShotHead->prev;
            p->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = p;
            pEnemyShotSet->pEnemyShotHead->prev = p;
        }
    }

    sEnemyShot* p = pEnemyShotSet->pEnemyShotHead->next;
    while (p != pEnemyShotSet->pEnemyShotHead) {
        // 緩やかなホーミング
        double dx = player.x - p->x;
        double dy = player.y - p->y;
        double target = atan2(dy, dx);
        p->muki = p->muki * 0.94 + target * 0.06;
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p->speed = min(6.5, p->speed + 0.008);
        p = p->next;
    }
}

// ======================
// 敵本体パターン
// ======================
void EnemyPat_Unavoidable_Grok() {
    static int phase = 0;
    static int muki = 1;
    static int timer = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 100;
        muki = 1;
        phase = 0;
        timer = 0;
    }

    // 敵の動き: 左右往復 + 微妙な上下
    enemy.x += 1.35 * muki;
    enemy.y = 60 + sin(count / 30.0) * 25;
    if (count % 110 == 55) muki *= -1;

    timer++;

    // フェーズ切替 (高密度攻撃の連鎖)
    if (timer % 180 == 0) {
        phase = (phase + 1) % 3;
    }

    // 定期的な弾幕生成
    if (count % 6 == 0) {
        sEnemyShotSet* set = new sEnemyShotSet;
        set->count = 0;
        set->x = enemy.x;
        set->y = enemy.y + 18.0;
        set->muki = atan2(player.y - set->y, player.x - set->x);
        set->kind = timer / 30; // バリエーション用
        set->pEnemyShotHead = new sEnemyShot;
        set->pEnemyShotHead->prev = set->pEnemyShotHead;
        set->pEnemyShotHead->next = set->pEnemyShotHead;

        // パターン選択
        switch (phase) {
        case 0:
            set->patternFunc = ShotDenseAimed;
            break;
        case 1:
            set->patternFunc = ShotRotRing;
            set->muki += (GetRand(80) - 40) / 180.0 * DX_PI; // 回転開始角度ランダム
            break;
        default:
            set->patternFunc = ShotChaosScatter;
            break;
        }

        // リスト連結
        set->prev = enemyShotSetHead.prev;
        set->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set;
        enemyShotSetHead.prev = set;
    }

    // 追加の超密度波 (さらに回避困難に)
    if (count % 47 == 0 && phase == 2) {
        sEnemyShotSet* set2 = new sEnemyShotSet;
        set2->count = 0;
        set2->x = enemy.x + GetRand(80) - 40;
        set2->y = enemy.y + 10;
        set2->muki = DX_PI / 2 + (GetRand(60) - 30) / 180.0 * DX_PI; // 下方向中心
        set2->kind = 99;
        set2->patternFunc = ShotDenseAimed;
        set2->pEnemyShotHead = new sEnemyShot;
        set2->pEnemyShotHead->prev = set2->pEnemyShotHead;
        set2->pEnemyShotHead->next = set2->pEnemyShotHead;

        set2->prev = enemyShotSetHead.prev;
        set2->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = set2;
        enemyShotSetHead.prev = set2;
    }
}