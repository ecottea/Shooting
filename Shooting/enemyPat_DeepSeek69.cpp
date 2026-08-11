// enemyPat_Tmp.cpp
// 果実合成「弾けるスイカ連鎖」

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>
#include <vector>
#include <set>
#include <algorithm>

// ---------- 補助関数 ----------
static double GetBulletRadius(int stage) {
    if (stage <= 3) return 2.5;               // チェリー～ブドウ：小玉
    if (stage <= 7) return 7.0;               // デコポン～ナシ ：中玉
    if (stage == 8) return 6.0;               // モモ          ：中楕円弾
    return 20.0;                              // パイナップル～スイカ：大玉
}

static void SetFruitKind(sEnemyShot* p) {
    int stage = p->param_i[0];
    int color = 6; // 白（デフォルト）
    switch (stage) {
    case 1: color = 0; break; // チェリー：赤
    case 2: color = 5; break; // イチゴ　：マゼンタ
    case 3: color = 3; break; // ブドウ　：シアン
    case 4: color = 8; break; // デコポン：橙
    case 5: color = 8; break; // カキ　　：橙
    case 6: color = 0; break; // リンゴ　：赤
    case 7: color = 1; break; // ナシ　　：黄
    case 8: color = 5; break; // モモ　　：マゼンタ
    case 9: color = 1; break; // パイナップル：黄
    case 10: color = 2; break;// メロン　：緑
    case 11: color = 2; break;// スイカ　：緑
    }
    if (stage <= 3)
        p->kind = img_enemyShotSmallBall[color];
    else if (stage <= 7)
        p->kind = img_enemyShotMediumBall[color];
    else if (stage == 8)
        p->kind = img_enemyShotMediumOval[color];
    else
        p->kind = img_enemyShotLargeBall[color];
}

static void SpawnWatermelonExplosion(sEnemyShotSet* pSet, sEnemyShot* pWM) {
    // 4way×2重の環状弾（色混合の小玉）
    int colors[] = { 0,1,2,3,5,8 }; // 赤,黄,緑,シアン,マゼンタ,橙
    double baseAngle[4] = { 0.0, DX_PI / 2.0, DX_PI, DX_PI * 3.0 / 2.0 };
    for (int a = 0; a < 4; ++a) {
        for (int s = 0; s < 2; ++s) {
            double speed = (s == 0) ? 2.0 : 3.5;
            sEnemyShot* p = new sEnemyShot;
            p->x = pWM->x;
            p->y = pWM->y;
            p->kind = img_enemyShotSmallBall[colors[GetRand(5)]];
            p->muki = baseAngle[a];
            p->speed = speed;
            p->param_i[1] = 0; // 通常弾（非果実）

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    // 跳ね返る大粒弾 3個（黄色大玉、重力＋跳ね返り）
    for (int i = 0; i < 3; ++i) {
        sEnemyShot* p = new sEnemyShot;
        p->x = pWM->x;
        p->y = pWM->y;
        p->kind = img_enemyShotLargeBall[1]; // 黄色大玉
        p->param_i[1] = 3; // 跳ね返り非合成弾
        p->param_d[0] = (GetRand(200) - 100) / 50.0 * 3; // vx : -2.0 ～ 2.0
        p->param_d[1] = -(GetRand(200) + 100) / 50.0 * 2; // vy : -6.0 ～ -2.0
        p->muki = 0.0;
        p->speed = 0.0;

        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
    }

    if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
    PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
}

// ---------- 弾幕パターン関数 ----------
static void FruitMerge(sEnemyShotSet* pEnemyShotSet) {
    // 初期化
    if (pEnemyShotSet->count == 0) {
        pEnemyShotSet->param_i[2] = 0;  // スポーンタイマー
        pEnemyShotSet->param_i[3] = 9999; // 果実弾の最大数
        return;
    }

    // 1. スポーンタイマー更新
    int& timer = pEnemyShotSet->param_i[2];
    timer = (timer + 1) % 8; // 48フレーム（0.8秒）周期

    // 2. 全弾の物理演算／移動
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;
        int type = pShot->param_i[1];

        if (type == 1 && pShot->param_i[0] < 11) {
            // 果実弾（スイカ以外）：重力＋跳ね返り
            double& vx = pShot->param_d[0];
            double& vy = pShot->param_d[1];
            vy += 0.1; // 重力
            pShot->x += vx;
            pShot->y += vy;
            double r = GetBulletRadius(pShot->param_i[0]);

            // 画面端で反射 (480x480)
            if (pShot->y + r > 480.0) { pShot->y = 480.0 - r; vy *= -0.6; }
            if (pShot->y - r < 0.0) { pShot->y = r;          vy *= -0.6; }
            if (pShot->x - r < 0.0) { pShot->x = r;          vx *= -0.6; }
            if (pShot->x + r > 480.0) { pShot->x = 480.0 - r;  vx *= -0.6; }
        }
        else if (type == 1 && pShot->param_i[0] == 11) {
            // スイカ：カウントダウン後爆発
            if (--pShot->param_i[2] <= 0) {
                SpawnWatermelonExplosion(pEnemyShotSet, pShot);
                // スイカをリストから削除
                pShot->prev->next = pShot->next;
                pShot->next->prev = pShot->prev;
                delete pShot;
                pShot = pNext;
                continue;
            }
        }
        else if (type == 0) {
            // 通常弾（破片など）：等速直線移動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        else if (type == 3) {
            // 跳ね返り専用弾（爆発の大粒）：重力＋跳ね返り
            double& vx = pShot->param_d[0];
            double& vy = pShot->param_d[1];
            vy += 0.1;
            pShot->x += vx;
            pShot->y += vy;
            double r = 20.0; // 大玉の半径
            if (pShot->y + r > 480.0) { pShot->y = 480.0 - r; vy *= -0.6; }
            if (pShot->y - r < 0.0) { pShot->y = r;          vy *= -0.6; }
            if (pShot->x - r < 0.0) { pShot->x = r;          vx *= -0.6; }
            if (pShot->x + r > 480.0) { pShot->x = 480.0 - r;  vx *= -0.6; }
        }
        pShot = pNext;
    }

    // 3. 果実同士の合成判定
    std::vector<sEnemyShot*> fruits;
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[1] == 1 && pShot->param_i[0] < 11)
            fruits.push_back(pShot);
        pShot = pShot->next;
    }

    struct MergePair { sEnemyShot* a, * b; int newStage; };
    std::vector<MergePair> merges;
    for (size_t i = 0; i < fruits.size(); ++i) {
        for (size_t j = i + 1; j < fruits.size(); ++j) {
            if (fruits[i]->param_i[0] == fruits[j]->param_i[0]) {
                double r = GetBulletRadius(fruits[i]->param_i[0]);
                double dx = fruits[i]->x - fruits[j]->x;
                double dy = fruits[i]->y - fruits[j]->y;
                if (dx * dx + dy * dy <= (r + r) * (r + r)) {
                    merges.push_back({ fruits[i], fruits[j], fruits[i]->param_i[0] + 1 });
                }
            }
        }
    }

    std::set<sEnemyShot*> removed;
    for (auto& m : merges) {
        if (removed.count(m.a) || removed.count(m.b)) continue;
        // 古い2つをリストから外す
        m.a->prev->next = m.a->next;
        m.a->next->prev = m.a->prev;
        m.b->prev->next = m.b->next;
        m.b->next->prev = m.b->prev;
        removed.insert(m.a);
        removed.insert(m.b);

        // 新しい果実を生成
        double midX = (m.a->x + m.b->x) / 2.0;
        double midY = (m.a->y + m.b->y) / 2.0;
        double vx = (m.a->param_d[0] + m.b->param_d[0]) / 2.0;
        double vy = (m.a->param_d[1] + m.b->param_d[1]) / 2.0;

        sEnemyShot* pNew = new sEnemyShot;
        pNew->x = midX;
        pNew->y = midY;
        pNew->param_i[0] = m.newStage;
        pNew->param_i[1] = 1;          // 果実
        pNew->param_d[0] = vx;
        pNew->param_d[1] = vy;
        pNew->muki = 0.0;
        pNew->speed = 0.0;
        pNew->param_i[4] = count;      // 生成フレーム（年齢用）
        SetFruitKind(pNew);

        if (m.newStage == 11) {
            pNew->param_i[2] = 60;     // スイカの寿命（60フレーム）
        }
        else {
            pNew->param_i[2] = 0;
        }

        // リスト末尾に追加
        pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pNew->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
        pEnemyShotSet->pEnemyShotHead->prev = pNew;

        // 破片弾を発生（白小玉）
        int fragNum = m.newStage;
        for (int k = 0; k < fragNum; ++k) {
            double angle = (k * 2.0 * DX_PI) / fragNum;
            sEnemyShot* pFrag = new sEnemyShot;
            pFrag->x = midX;
            pFrag->y = midY;
            pFrag->kind = img_enemyShotSmallBall[6]; // 白
            pFrag->muki = angle;
            pFrag->speed = 1.5;
            pFrag->param_i[1] = 0; // 通常弾
            pFrag->param_i[0] = 0;

            pFrag->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pFrag->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pFrag;
            pEnemyShotSet->pEnemyShotHead->prev = pFrag;
        }

        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);

        delete m.a;
        delete m.b;
    }

    // 4. 果実弾の数が上限を超えたら最古のものを削除
    int fruitCount = 0;
    pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[1] == 1 && pShot->param_i[0] < 11) fruitCount++;
        pShot = pShot->next;
    }

    if (fruitCount > pEnemyShotSet->param_i[3]) {
        sEnemyShot* oldest = nullptr;
        pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[1] == 1 && pShot->param_i[0] < 11) {
                if (!oldest || pShot->param_i[4] < oldest->param_i[4])
                    oldest = pShot;
            }
            pShot = pShot->next;
        }
        if (oldest) {
            oldest->prev->next = oldest->next;
            oldest->next->prev = oldest->prev;
            delete oldest;
            fruitCount--;
        }
    }

    // 5. チェリーを定期的に追加
    if (timer == 0 && fruitCount < pEnemyShotSet->param_i[3]) {
        sEnemyShot* pCherry = new sEnemyShot;
        pCherry->x = GetRand(440) + 20.0; // 20～460
        pCherry->y = -5.0;
        pCherry->param_i[0] = std::clamp((count - 5 * 60) / 70, 1, 9);          // チェリー
        pCherry->param_i[1] = 1;          // 果実
        pCherry->param_d[0] = 0.0;
        pCherry->param_d[1] = 0.0;
        pCherry->muki = 0.0;
        pCherry->speed = 0.0;
        pCherry->param_i[4] = count;      // 年齢
        SetFruitKind(pCherry);

        pCherry->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pCherry->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pCherry;
        pEnemyShotSet->pEnemyShotHead->prev = pCherry;
    }
}

// ---------- 敵本体パターン ----------
void EnemyPat_SuikaGame_DeepSeek()
{
    if (count == 1) {
        // ボス初期化（中央上部で停止）
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;

        // 果実弾幕用のショットセットを1つだけ生成
        sEnemyShotSet* pSet = new sEnemyShotSet;
        pSet->count = 0;
        pSet->patternFunc = FruitMerge;
        pSet->x = enemy.x;
        pSet->y = enemy.y + 10.0;
        pSet->muki = 0.0;
        pSet->kind = 0;

        pSet->pEnemyShotHead = new sEnemyShot;
        pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

        pSet->prev = enemyShotSetHead.prev;
        pSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pSet;
        enemyShotSetHead.prev = pSet;
    }
    // それ以降は特に何もしない（全て FruitMerge が管理）
}