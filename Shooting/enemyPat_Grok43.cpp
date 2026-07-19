// enemyPat_Tmp.cpp
// ボス体当たり弾幕パターン：【紅蓮の突進・螺旋火葬】
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 螺旋弾を発生させながらボスが突進する専用パターン関数
static void ShotSpiralRam(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;
    const int phase = pEnemyShotSet->count;

    // 突進中はボスの現在位置にエミッタを追従（ボス本体から弾を発生）
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 12.0;

    // 効果音（突進開始時と継続的に軽い音）
    if (phase == 0) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
    if (phase % 3 == 0 && phase < 180) {
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
    }

    // 螺旋弾の発生（密度を調整）
    if (phase < 180 && phase % 2 == 0) {  // 突進中は高密度で螺旋を吐き続ける
        for (int i = 0; i < 2; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;

            // 進行方向を軸とした螺旋（2重螺旋）
            double baseAngle = pEnemyShotSet->muki + DX_PI / 2.0 * (phase % 2 == 0 ? 1 : -1);
            double spiralOffset = (phase % 36) * 0.18; // 螺旋の回転
            pEnemyShot->muki = baseAngle + spiralOffset + (GetRand(40) - 20) / 180.0 * DX_PI;

            pEnemyShot->speed = 2.8 + (GetRand(60) / 100.0); // やや速めの螺旋

            // 赤系の炎っぽい弾（小玉と中玉を織り交ぜ）
            if (phase % 4 == 0) {
                pEnemyShot->kind = img_enemyShotMediumBall[0]; // 赤
            }
            else {
                pEnemyShot->kind = img_enemyShotSmallBall[0];  // 赤
            }

            // リストに追加
            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 既存弾の移動（直進）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// 敵本体のパターン
void EnemyPat_Tackle_Grok()
{
    static int phase = 0;        // 0:待機, 1:予備動作, 2:突進, 3:後退
    static int phaseTimer = 0;
    static double targetX = 240.0;
    static int muki = 1;         // 横移動用

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        phase = 0;
        phaseTimer = 90;
        muki = 1;
        return;
    }

    phaseTimer++;

    // 基本的な横揺れ移動（待機時）
    if (phase == 0) {
        enemy.x += 1.2 * muki;
        if (enemy.x > 380) muki = -1;
        if (enemy.x < 100) muki = 1;

        // 定期的に突進フェーズへ移行
        if (phaseTimer % 120 == 0) {
            phase = 1;
            phaseTimer = 0;
            targetX = player.x; // 突進開始時のプレイヤー位置を記憶
        }
    }

    // 予備動作（狙いをつける）
    else if (phase == 1) {
        // プレイヤーに向かって軽く旋回
        double dx = player.x - enemy.x;
        double dy = player.y - enemy.y - 80;
        double targetAngle = atan2(dy, dx);
        enemy.x += cos(targetAngle) * 2.5;
        enemy.y += sin(targetAngle) * 1.2;

        if (phaseTimer >= 45) {  // 約0.75秒予備
            phase = 2;
            phaseTimer = 0;

            // 突進用ShotSet作成
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotSpiralRam;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 12.0;
            pEnemyShotSet->muki = atan2(player.y - enemy.y, player.x - enemy.x); // 初回方向
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

    // 突進中
    else if (phase == 2) {
        double dx = targetX - enemy.x;
        double dy = player.y - enemy.y;
        double dist = sqrt(dx * dx + dy * dy);
        if (dist > 3.0) {
            enemy.x += (dx / dist) * 6.8;  // 高速突進
            enemy.y += (dy / dist) * 4.2;
        }

        // 画面端まで突進したら後退フェーズへ
        if (enemy.y > 420 || phaseTimer > 90) {
            phase = 3;
            phaseTimer = 0;
            targetX = GetRand(300) + 90; // 戻る位置をランダムに
        }
    }

    // 後退（画面上部に戻る）
    else if (phase == 3) {
        enemy.y -= 5.5;
        enemy.x += (targetX - enemy.x) * 0.08;

        if (enemy.y < 70) {
            phase = 0;
            phaseTimer = 0;
        }
    }

    // 突進終了時に爆発的な拡散弾
    if (phase == 3 && phaseTimer == 1) {
        for (int i = 0; i < 24; i++) {
            sEnemyShot* p = new sEnemyShot;
            p->x = enemy.x;
            p->y = enemy.y;
            p->muki = i * DX_PI * 2.0 / 24 + (GetRand(30) - 15) / 180.0 * DX_PI;
            p->speed = 3.5 + GetRand(80) / 100.0;
            p->kind = img_enemyShotMediumBall[0]; // 赤中玉

            p->prev = enemyShotSetHead.prev->pEnemyShotHead->prev; // 簡易追加
            p->next = enemyShotSetHead.prev->pEnemyShotHead;
            enemyShotSetHead.prev->pEnemyShotHead->prev->next = p;
            enemyShotSetHead.prev->pEnemyShotHead->prev = p;
        }
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }
}