// enemyPat_SoapBubble.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// 弾幕：ソープバブル・トラップ（泡沫の連鎖）
static void SoapBubbleTrap(sEnemyShotSet* pSet)
{
    // ==========================================
    // フェーズ①：シャボン玉のふんわり生成
    // ==========================================
    // 45フレームに1回、新しいシャボン玉を生成
    if (pSet->count % 6 == 0) {
        if (!CheckSoundMem(sound_enemyShot_medium)) PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        sEnemyShot* bubble = new sEnemyShot;
        bubble->x = enemy.x;
        bubble->y = enemy.y;

        // シャボン玉らしい爽やかな色をランダム選択 (3:シアン, 4:青, 5:マゼンタ, 6:白)
        int color_choices[] = { 3, 4, 5, 6 };
        int c = color_choices[GetRand(3)];

        bubble->kind = img_enemyShotLargeBall[c];
        bubble->param_i[0] = 0; // 0: シャボン玉状態, 1: 破裂後の針弾
        bubble->param_i[1] = c; // 破裂時の針弾に引き継ぐ色
        bubble->param_d[0] = GetRand(628) / 100.0; // 揺れの位相 (0.00 ~ 6.28)

        bubble->prev = pSet->pEnemyShotHead->prev;
        bubble->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = bubble;
        pSet->pEnemyShotHead->prev = bubble;
    }

    // ==========================================
    // 弾の更新処理（移動・当たり判定・破裂）
    // ==========================================
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        sEnemyShot* next_p = p->next; // 削除に備えて次のポインタを退避

        if (p->param_i[0] == 0) { // --- シャボン玉の場合 ---
            // ゆっくり下降 ＋ サイン波でX軸にゆらゆら
            p->y += 0.8;
            p->x += sin(p->count * 0.04 + p->param_d[0]) * 1.5;

            bool isBurst = false;

            // 破裂条件1: 寿命（300フレーム経過で自然破裂）
            if (p->count > 300) {
                //isBurst = true;
            }

            // フェーズ②：誘発する破裂（プレイヤーのショットとの当たり判定）
            if (!isBurst) {
                sPlayerShot* ps = playerShotHead.next;
                while (ps != &playerShotHead) {
                    double dx = p->x - ps->x;
                    double dy = p->y - ps->y;
                    // 大玉の半径を約20.0として円形当たり判定
                    if (dx * dx + dy * dy < 20.0 * 20.0) {
                        isBurst = true;

                        // プレイヤーの弾を消去（貫通を防ぎ、連射のリスクを高める）
                        sPlayerShot* del_ps = ps;
                        ps = ps->next;
                        del_ps->prev->next = del_ps->next;
                        del_ps->next->prev = del_ps->prev;
                        delete del_ps;
                        break;
                    }
                    else {
                        ps = ps->next;
                    }
                }
            }

            // フェーズ③：鋭利な拡散（破裂処理）
            if (isBurst) {
                if (CheckSoundMem(sound_enemyShot_heavy) == 1) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                int needleCount = 12; // 12方位に拡散
                double baseAngle = atan2(player.y - p->y, player.x - p->x); // プレイヤー方向を基準にする
                double burstSpeed = 3.5 + (GetRand(10) / 10.0);             // 破裂の勢い（3.5 ~ 4.5）

                for (int i = 0; i < needleCount; i++) {
                    sEnemyShot* needle = new sEnemyShot;
                    needle->x = p->x;
                    needle->y = p->y;
                    needle->kind = img_enemyShotBullet[p->param_i[1]]; // 同じ色の銃弾(針弾)
                    needle->param_i[0] = 1; // 状態を「針弾」に変更
                    needle->muki = baseAngle + (i * DX_PI * 2.0 / needleCount);
                    needle->speed = burstSpeed;

                    needle->prev = pSet->pEnemyShotHead->prev;
                    needle->next = pSet->pEnemyShotHead;
                    pSet->pEnemyShotHead->prev->next = needle;
                    pSet->pEnemyShotHead->prev = needle;
                }

                // シャボン玉自体は消滅させる
                p->prev->next = p->next;
                p->next->prev = p->prev;
                delete p;
            }
        }
        else if (p->param_i[0] == 1) { // --- 針弾の場合 ---
            // 直線的な高速移動
            p->x += p->speed * cos(p->muki);
            p->y += p->speed * sin(p->muki);
        }

        p = next_p; // 次の弾へ
    }
}

// 敵本体のパターン（メインから呼ばれる関数）
void EnemyPat_SoapBubbles_Gemini()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 80.0;
        enemy.maxHp = enemy.hp = 200; // ボス想定でHPを高めに設定

        // 弾幕管理用セットを1つだけ生成（これが全てのシャボン玉と針弾を管理し続ける）
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = SoapBubbleTrap;
        pSet->x = enemy.x;
        pSet->y = enemy.y;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }

    // 敵本体は画面上部で、サイン波を描きながら滑らかに左右に動く
    enemy.x = 240.0 + sin(count * 0.015) * 180.0;
    enemy.y = 80.0 + sin(count * 0.030) * 15.0;
}