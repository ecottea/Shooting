// enemyPat_prism.cpp
// 弾幕：白色分散(ホワイト・リフラクション) -- Hard仕様
// 敵が撃った白色大弾(入射光)が、プリズム領域(画面上側の帯)に達した瞬間に
// 7色の小弾(スペクトル)に分光して扇状に放出される弾幕。
// 分光した色弾は左右の壁で1回だけ反射し、画面が虹色の弾幕で埋まっていく。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---- プリズム弾幕用の定数 ----
// スペクトルの色並び(赤→橙→黄→緑→シアン→青→紫)
// 弾の色: 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
// ※紫の素材がないためマゼンタ(5)を紫の代わりに使用する
static const int PRISM_COLOR[7] = { 0, 8, 1, 2, 3, 4, 5 };

static const double SPLIT_HALF = 36.0 * DX_PI / 180.0; // 扇の半分の角度(拡大:24→36度)
static const double SPLIT_STEP = 12.0 * DX_PI / 180.0; // 色ごとの角度差(全体で72度)
static const double PRISM_LINE_Y = 180.0;                // プリズム領域の下端y座標(深く:150→180)
static const int    BOUNCE_MAX = 1;                    // 色弾の壁反射回数

// プリズム弾幕のショットパターン
static void ShotPrism(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        // 分光でリストから除去する場合に備え、次の弾を先に保存
        sEnemyShot* pNext = pShot->next;

        // 弾の移動
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // param_i[0] == 1 は「未分光の白色大弾(入射光)」のマーカー
        if (pShot->param_i[0] == 1 &&
            (pShot->y >= PRISM_LINE_Y || pShot->count >= 240)) {

            // 分散基準点と進行方向を記録
            double bx = pShot->x;
            double by = pShot->y;
            double baseMuki = pShot->muki;
            // 分散方向のばらつき: GetRand(72) は 0〜72 の73通りを返すので
            // -36〜+36 度にするには 72 を引く
            baseMuki += (GetRand(72) - 36) / 180.0 * DX_PI;

            // 分光音
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

            // 7色の小弾を生成
            // 紫が最速・赤が最遅となるよう速度差をつけ、
            // 本物のスペクトルのように扇が歪んで広がる見た目にする
            for (int i = 0; i < 7; i++) {
                sEnemyShot* pNew = new sEnemyShot;
                pNew->x = bx;
                pNew->y = by;
                pNew->muki = baseMuki - SPLIT_HALF + SPLIT_STEP * i;
                pNew->speed = 4.0 + i * (3.0 / 6.0); // 赤:4.0 〜 紫:7.0(高速化)
                pNew->kind = img_enemyShotMediumOval[PRISM_COLOR[i]];
                pNew->param_i[0] = 0; // 分光済みの弾(再分光しない)
                pNew->param_i[1] = BOUNCE_MAX; // 残り反射回数

                // リストの末尾に追加
                pNew->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pNew->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pNew;
                pEnemyShotSet->pEnemyShotHead->prev = pNew;
            }

            // 7色の生成が済んだので白色弾をリストから除去
            // (画面外消去はメインルーチンの担当だが、分光による消滅はここで行う)
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }
        // 分光済みの色弾は左右の壁で反射する
        else if (pShot->param_i[0] == 0 && pShot->param_i[1] > 0) {
            if (pShot->x < 0.0 || pShot->x > 480.0) {
                pShot->muki = -pShot->muki; // 左右反射
                pShot->param_i[1]--;
                // 壁の内側へ押し戻しておく(再反射の連鎖防止)
                if (pShot->x < 0.0) pShot->x = 0.0;
                else                pShot->x = 480.0;
            }
        }

        pShot = pNext;
    }
}

// 敵本体のパターン
void EnemyPat_Prism_Zai()
{
    static int muki;
    static int shot_count;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
    }
    else {
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
    }

    // 一定間隔で白色大弾(入射光)を2連装で発射
    // 2発の進路をずらすことで、分光タイミングと分散方向がずれ、
    // プレイヤーは2つの扇を同時に読むことになる
    if (count % 40 == 20) {
        for (int j = 0; j < 2; j++) {
            sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
            pEnemyShotSet->count = 0;
            pEnemyShotSet->patternFunc = ShotPrism;
            pEnemyShotSet->x = enemy.x;
            pEnemyShotSet->y = enemy.y + 10.0;
            pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x)
                + (j * 2 - 1) * 10.0 * DX_PI / 180.0; // 左右に10度ずつ振る
            pEnemyShotSet->kind = shot_count++;
            pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

            // 白色大弾(入射光)を1発だけ含むショットセット
            sEnemyShot* pShot = new sEnemyShot;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->muki = pEnemyShotSet->muki;
            pShot->speed = 3.0; // 速度を上げて分散地点の予測を難しくする
            pShot->kind = img_enemyShotLargeBall[6]; // 白色の大玉
            pShot->param_i[0] = 1; // 入射光マーカー(分光フラグ)

            pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
            pEnemyShotSet->pEnemyShotHead->prev = pShot;

            pEnemyShotSet->prev = enemyShotSetHead.prev;
            pEnemyShotSet->next = &enemyShotSetHead;
            enemyShotSetHead.prev->next = pEnemyShotSet;
            enemyShotSetHead.prev = pEnemyShotSet;
        }

        // 発射音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }
}