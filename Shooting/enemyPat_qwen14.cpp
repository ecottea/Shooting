// enemyPat_Tmp.cpp

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：毒龍の息吹と腐乱の華 (Venomous Dragon's Breath and Blooming Rot)
// ・フェーズ1: 自機を狙う毒の鱗弾が、うねりながら減速・加速する
// ・フェーズ2: 5つの毒胞子が降りてきて停止する
// ・フェーズ3: 胞子から魔弾の花が咲き、回転しながら収束する
// ・フェーズ4: 胞子が弾け、無数の毒の欠片が渦を巻いて飛び散る
// ============================================================
static void ShotVenomousDragon(sEnemyShotSet* p)
{
    int c = p->count;

    // --------------------------------------------------------
    // フェーズ1：毒龍の息吹 (0 - 119)
    // --------------------------------------------------------
    if (c < 120) {
        if (c % 1 == 0) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = p->x;
            shot->y = p->y;
            // 自機を狙いつつ、サインカーブで大きく角度を振る
            double base_angle = atan2(player.y - p->y, player.x - p->x);
            shot->muki = base_angle + sin(c * 0.15) * 1.2;
            shot->speed = 2.5;
            shot->kind = img_enemyShotScale[3]; // 3:シアン (毒々しい鱗弾)

            // リストに追加
            shot->prev = p->pEnemyShotHead->prev;
            shot->next = p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next = shot;
            p->pEnemyShotHead->prev = shot;
        }
    }

    // --------------------------------------------------------
    // フェーズ2：毒胞子の降下 (120)
    // --------------------------------------------------------
    if (c == 120) {
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 5; i++) {
            sEnemyShot* shot = new sEnemyShot;
            shot->x = p->x - 80.0 + i * 40.0;
            shot->y = p->y;
            shot->muki = DX_PI / 2.0; // 真下へ
            shot->speed = 1.5;
            shot->kind = img_enemyShotLargeBall[2]; // 2:緑 (毒胞子)

            shot->prev = p->pEnemyShotHead->prev;
            shot->next = p->pEnemyShotHead;
            p->pEnemyShotHead->prev->next = shot;
            p->pEnemyShotHead->prev = shot;
        }
    }

    // --------------------------------------------------------
    // フェーズ3：腐乱の華 (180 - 299)
    // --------------------------------------------------------
    if (c >= 180 && c < 300) {
        if ((c - 180) % 20 == 0) {
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

            // リストの更新中に新規追加要素を巡回しないよう、末尾を記録しておく
            sEnemyShot* old_tail = p->pEnemyShotHead->prev;
            sEnemyShot* current = p->pEnemyShotHead->next;

            while (true) {
                // 緑の大玉 (毒胞子) からだけ花を咲かせる
                if (current->kind == img_enemyShotLargeBall[2]) {
                    for (int i = 0; i < 16; i++) {
                        sEnemyShot* spore = new sEnemyShot;
                        spore->x = current->x;
                        spore->y = current->y;
                        spore->muki = (DX_PI * 2.0 / 16.0) * i + (c * 0.1); // 時間とともに回転
                        spore->speed = 1.2;
                        spore->kind = img_enemyShotSmallBall[5]; // 5:マゼンタ (魔弾)

                        spore->prev = p->pEnemyShotHead->prev;
                        spore->next = p->pEnemyShotHead;
                        p->pEnemyShotHead->prev->next = spore;
                        p->pEnemyShotHead->prev = spore;
                    }
                }
                if (current == old_tail) break;
                current = current->next;
            }
        }
    }

    // --------------------------------------------------------
    // フェーズ4：毒の爆散 (300)
    // --------------------------------------------------------
    if (c == 300) {
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        sEnemyShot* old_tail = p->pEnemyShotHead->prev;
        sEnemyShot* current = p->pEnemyShotHead->next;

        while (true) {
            if (current->kind == img_enemyShotLargeBall[2]) {
                for (int i = 0; i < 48; i++) {
                    sEnemyShot* shard = new sEnemyShot;
                    shard->x = current->x;
                    shard->y = current->y;
                    shard->muki = (DX_PI * 2.0 / 48.0) * i;
                    shard->speed = 2.5;
                    shard->kind = img_enemyShotDiamond[5]; // 5:マゼンタ (毒の欠片)

                    shard->prev = p->pEnemyShotHead->prev;
                    shard->next = p->pEnemyShotHead;
                    p->pEnemyShotHead->prev->next = shard;
                    p->pEnemyShotHead->prev = shard;
                }
                // 爆発済みの胞子は速度を止め、色を変えて残骸にする
                current->speed = 0.0;
                current->kind = img_enemyShotLargeBall[5];
            }
            if (current == old_tail) break;
            current = current->next;
        }
    }

    // --------------------------------------------------------
    // 弾の移動・挙動更新
    // --------------------------------------------------------
    sEnemyShot* shot = p->pEnemyShotHead->next;
    while (shot != p->pEnemyShotHead) {
        int sc = shot->count;

        // シアンの鱗弾 (毒龍の息吹)
        if (shot->kind == img_enemyShotScale[3]) {
            shot->muki += sin(sc * 0.15) * 0.08 * 0.5; // うねり
            if (sc < 40) {
                shot->speed *= 0.96; // 減速して密集させる
            }
            else {
                shot->speed += 0.05; // 加速して突き刺さる
            }
        }

        // 緑の大玉 (毒胞子)
        if (shot->kind == img_enemyShotLargeBall[2]) {
            if (sc < 40) {
                shot->speed *= 0.95; // 減速して停止
                shot->x += sin(sc * 0.2) * 0.5; // ゆらめき
            }
            else {
                shot->speed = 0.0; // 完全に停止
            }
        }

        // マゼンタの小玉 (腐乱の華)
        if (shot->kind == img_enemyShotSmallBall[5]) {
            if (sc < 60) {
                shot->speed += 0.05; // 外側へ広がる
                shot->muki += 0.05;  // 右回転
            }
            else {
                shot->speed *= 0.98; // 減速
                shot->muki -= 0.08;  // 左回転（内側へ収束）
            }
        }

        // マゼンタの菱形 (毒の欠片)
        if (shot->kind == img_enemyShotDiamond[5]) {
            shot->muki += sin(sc * 0.2) * 0.15 * 0.5; // うねり
            shot->speed += 0.05;                 // 加速
        }

        // 座標更新
        shot->x += shot->speed * cos(shot->muki);
        shot->y += shot->speed * sin(shot->muki);

        if (shot->count >= 600) {
            shot->x = 9999;
        }

        shot = shot->next;
    }
}

// ============================================================
// 敵本体のパターン
// ============================================================
void EnemyPat_Poison_Qwen()
{
    static sEnemyShotSet* pMyShotSet = nullptr;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
    }

    // 敵の移動 (不気味な浮遊感)
    enemy.x = 240.0 + sin(count * 0.015) * 120.0;
    enemy.y = 60.0 + sin(count * 0.03) * 40.0;

    //// 発射座標を敵に追従させる
    //if (pMyShotSet) {
    //    pMyShotSet->x = enemy.x;
    //    pMyShotSet->y = enemy.y + 20.0;
    //}

    if (count % 300 == 30) {
        // 弾幕セットの初期化
        pMyShotSet = new sEnemyShotSet;
        pMyShotSet->count = 0;
        pMyShotSet->patternFunc = ShotVenomousDragon;
        pMyShotSet->x = enemy.x;
        pMyShotSet->y = enemy.y + 20.0;
        pMyShotSet->muki = 0.0;

        pMyShotSet->pEnemyShotHead = new sEnemyShot;
        pMyShotSet->pEnemyShotHead->prev = pMyShotSet->pEnemyShotHead;
        pMyShotSet->pEnemyShotHead->next = pMyShotSet->pEnemyShotHead;

        // メインのリストに接続
        pMyShotSet->prev = enemyShotSetHead.prev;
        pMyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pMyShotSet;
        enemyShotSetHead.prev = pMyShotSet;
    }
}