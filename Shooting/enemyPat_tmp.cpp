// enemyPat_tmp.cpp
// 弾幕：超新星爆発「コア・コラプス」
//
// [フェーズ0] コア崩壊(150フレーム)
//     大玉がボス中心へ渦を巻きながら収束していく(予告音あり)
// [フェーズ1] 衝撃波+残骸の漂流(150〜480フレーム)
//     収束完了の瞬間、全方向へ高速の細弾2リング+低速の大玉12個が放射される
//     大玉は減速しながら不規則に振動(残骸の漂流)、一定間隔で残骸から自機狙い弾
// [フェーズ2] 再重力(480〜660フレーム)
//     残った弾が渦を巻きながらボス中心へ吸引され、中心を通過して画面外へ排出
//     終了後、フェーズ0へ戻ってサイクル継続

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---- タイミング定数 ----
static const int COLLAPSE_FRAMES = 150; // コア崩壊の長さ(=爆発のタイミング)
static const int NOVA_GRAVITY_START = 480; // サイクル内:再重力フェーズ開始
static const int CYCLE_FRAMES = 660; // 1サイクルの長さ
static const int DEBRIS_END = 300; // 爆発セット内:残骸フェーズ終了(以降は再重力)

// ---- 弾セット生成ヘルパー ----
static sEnemyShotSet* CreateShotSet(double x, double y, sEnemyShotSet::PatternFunc func, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = 0.0;
    pEnemyShotSet->kind = kind;
    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
    return pEnemyShotSet;
}

// ---- 弾生成ヘルパー ----
static sEnemyShot* AddShot(sEnemyShotSet* pEnemyShotSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->margin = 480;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// ============================================================
// セットA:コア崩壊 → 残骸化 → 再重力(サイクル全体を担当)
// param_i[0]: 状態 (0:収縮中 / 1:残骸 / 2:再重力吸引)
// param_d[0]: 中心からの半径  param_d[1]: 中心からの角度
// ============================================================
static void ShotCoreCollapse(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // 崩壊するコアの破片(大玉・白)を全方位に配置
        for (int i = 0; i < 20; i++) {
            double ang = GetRand(359) / 180.0 * DX_PI; // GetRand(x)は0〜xのx+1種類を返すので注意
            double r = 180.0 + GetRand(100);
            sEnemyShot* pShot = AddShot(pEnemyShotSet,
                pEnemyShotSet->x + r * cos(ang), pEnemyShotSet->y + r * sin(ang),
                ang, 0.0, img_enemyShotLargeBall[6]); // 6:白
            pShot->param_d[0] = r;
            pShot->param_d[1] = ang;
            pShot->param_i[0] = 0;
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // フェーズ0: 渦を巻きながら中心へ収束
            pShot->param_d[0] *= 0.978;
            pShot->param_d[1] += 0.045;
            pShot->x = pEnemyShotSet->x + pShot->param_d[0] * cos(pShot->param_d[1]);
            pShot->y = pEnemyShotSet->y + pShot->param_d[0] * sin(pShot->param_d[1]);
            if (pShot->count >= COLLAPSE_FRAMES) {
                // 爆発の瞬間: 残骸(大玉 赤/黄)となって散開
                pShot->param_i[0] = 1;
                pShot->param_d[1] += (GetRand(40) - 20) / 180.0 * DX_PI;
                pShot->muki = pShot->param_d[1];
                pShot->speed = 1.5 + GetRand(150) / 100.0;
                pShot->kind = img_enemyShotLargeBall[GetRand(1)]; // 0:赤 or 1:黄
            }
        }
        else if (pShot->param_i[0] == 1) {
            // フェーズ1: 残骸の漂流(減速+不規則な振動)
            pShot->speed *= 0.99;
            double wob = sin(pShot->count * 0.35) * 0.8;
            pShot->x += pShot->speed * cos(pShot->muki) - sin(pShot->muki) * wob;
            pShot->y += pShot->speed * sin(pShot->muki) + cos(pShot->muki) * wob;
            if (pEnemyShotSet->count >= NOVA_GRAVITY_START) {
                // フェーズ2へ移行: 現在位置を極座標に変換
                double dx = pShot->x - pEnemyShotSet->x;
                double dy = pShot->y - pEnemyShotSet->y;
                pShot->param_d[0] = sqrt(dx * dx + dy * dy);
                pShot->param_d[1] = atan2(dy, dx);
                pShot->param_i[0] = 2;
            }
        }
        else {
            // フェーズ2: 再重力(渦を巻いて中心へ吸引)
            // 半径が負になると中心を通過して反対側へ渦状に飛び出し、画面外で自動消去される
            pShot->param_d[0] -= 3.0;
            pShot->param_d[1] += 0.07;
            pShot->x = pEnemyShotSet->x + pShot->param_d[0] * cos(pShot->param_d[1]);
            pShot->y = pEnemyShotSet->y + pShot->param_d[0] * sin(pShot->param_d[1]);
        }
        pShot = pShot->next;
    }
}

// ============================================================
// セットB:衝撃波(爆発の瞬間に生成)
// param_i[0]: 0:通常(直進) / 1:残骸(大玉) / 2:再重力吸引中
// ============================================================
static void ShotSupernova(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        // 高速の細弾 2リング(全36方向)……速度差で抜け道を作る
        for (int i = 0; i < 36; i++) {
            double ang = i * (2.0 * DX_PI / 36.0);
            AddShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y, ang, 5.5, img_enemyShotSmallBall[3]); // 3:シアン
            AddShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y, ang + DX_PI / 36.0, 4.2, img_enemyShotSmallBall[6]); // 6:白
        }
        // 低速の大玉リング(全12方向)……後の残骸になる
        for (int i = 0; i < 12; i++) {
            double ang = (i + 0.5) * (2.0 * DX_PI / 12.0);
            sEnemyShot* pShot = AddShot(pEnemyShotSet, pEnemyShotSet->x, pEnemyShotSet->y, ang, 2.2,
                img_enemyShotLargeBall[GetRand(1)]); // 0:赤 or 1:黄
            pShot->param_i[0] = 1; // 残骸フラグ
        }
    }

    // フェーズ3: 残骸から自機狙いの小弾を3連射(0.8秒間隔)
    if (pEnemyShotSet->count > 0 && pEnemyShotSet->count % 48 == 0 && pEnemyShotSet->count < DEBRIS_END) {
        sEnemyShot* debris[12];
        int n = 0;
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 1 && n < 12) debris[n++] = pShot;
            pShot = pShot->next;
        }
        if (n > 0) {
            sEnemyShot* src = debris[GetRand(n - 1)];
            for (int j = 0; j < 3; j++) {
                double am = atan2(player.y - src->y, player.x - src->x)
                    + (GetRand(20) - 10) / 180.0 * DX_PI;
                AddShot(pEnemyShotSet, src->x, src->y, am, 3.2, img_enemyShotSmallBall[0]); // 0:赤
            }
        }
    }

    // 移動処理
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            // 残骸: 減速しながら振動
            if (pEnemyShotSet->count < DEBRIS_END) {
                pShot->speed *= 0.985;
                double wob = sin(pShot->count * 0.3) * 0.9;
                pShot->x += pShot->speed * cos(pShot->muki) - sin(pShot->muki) * wob;
                pShot->y += pShot->speed * sin(pShot->muki) + cos(pShot->muki) * wob;
            }
            else {
                // フェーズ2へ移行
                double dx = pShot->x - pEnemyShotSet->x;
                double dy = pShot->y - pEnemyShotSet->y;
                pShot->param_d[0] = sqrt(dx * dx + dy * dy);
                pShot->param_d[1] = atan2(dy, dx);
                pShot->param_i[0] = 2;
            }
        }
        else if (pShot->param_i[0] == 2) {
            // 再重力: 中心へ渦状に吸引(半径が負になると反対側へ飛び出し画面外で消える)
            pShot->param_d[0] -= 3.0;
            pShot->param_d[1] += 0.07;
            pShot->x = pEnemyShotSet->x + pShot->param_d[0] * cos(pShot->param_d[1]);
            pShot->y = pEnemyShotSet->y + pShot->param_d[0] * sin(pShot->param_d[1]);
        }
        else {
            // 衝撃波・自機狙い弾: 直進
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }
        pShot = pShot->next;
    }
}

// ============================================================
// 敵本体のパターン: 超新星爆発「コア・コラプス」
// ============================================================
void EnemyPat_Tmp()
{
    static int phase;      // 0:コア崩壊 / 1:残骸 / 2:再重力
    static int t;          // サイクル内フレーム
    static double coreX, coreY; // 爆発中心(セットAの収束中心と統一)

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 170.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        phase = 0;
        t = 0;
        coreX = enemy.x;
        coreY = enemy.y;
        CreateShotSet(coreX, coreY, ShotCoreCollapse, 0);
    }
    else {
        t++;
        // コアの脈動のようにゆっくり漂う
        enemy.x = coreX + 24.0 * sin(t * 0.03);
        enemy.y = coreY + 8.0 * sin(t * 0.07);

        if (phase == 0 && t >= COLLAPSE_FRAMES) {
            // フェーズ1へ: 爆発(衝撃波セットを生成)
            phase = 1;
            CreateShotSet(coreX, coreY, ShotSupernova, 1);
        }
        else if (phase == 1 && t >= NOVA_GRAVITY_START) {
            // フェーズ2へ: 再重力(残骸側の状態遷移は各セットのパターン内で実施)
            phase = 2;
        }
        else if (phase == 2 && t >= CYCLE_FRAMES) {
            // サイクル restart: フェーズ0へ戻る
            phase = 0;
            t = 0;
            CreateShotSet(coreX, coreY, ShotCoreCollapse, 0);
        }
    }
}