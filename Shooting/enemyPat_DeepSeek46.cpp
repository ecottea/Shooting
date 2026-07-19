// enemyPat_Tmp.cpp
// 砂塵旋風『埋没の大砂丘』

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//----------------------------------------
// 内部パターン関数 (shot set の更新)
//----------------------------------------

// フェーズ1 : 砂壁の接近
static void UpdateSandWall(sEnemyShotSet* pSet)
{
    // 初回のみ初期化不要 (このsetは弾を動的に追加するだけ)
    // 壁を構成する弾の移動
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);
        p = p->next;
    }

    // 一定間隔で新しい列を画面下から生成 (pSet->count は作成時0, 毎フレーム自動+1)
    if (pSet->count % 8 == 0) {
        double time = pSet->count;               // 経過フレーム
        double baseY = 480.0;
        double speed = 1.5;
        double step = 24.0;
        int    cols = 21;                        // 0 ～ 480 まで 24刻みで 21個
        double amplitude = 30.0;
        double freq = 2.0 * DX_PI / 480.0 * 2.0; // 画面内に2波
        double phaseShift = time * 0.03;
        double gapCenter = 240.0 + 180.0 * sin(time * 0.01);
        double halfGap = 30.0;

        for (int i = 0; i < cols; ++i) {
            double baseX = i * step;
            double xAdj = baseX + amplitude * sin(baseX * freq + phaseShift);
            // 隙間部分は作らない
            if (fabs(xAdj - gapCenter) < halfGap) continue;

            sEnemyShot* pNew = new sEnemyShot;
            pNew->x = xAdj;
            pNew->y = baseY;
            pNew->muki = 3.0 * DX_PI / 2.0; // 真上
            pNew->speed = speed;
            pNew->kind = img_enemyShotSmallBall[8]; // 橙色の小玉

            // 双方向リストに追加
            pNew->prev = pSet->pEnemyShotHead->prev;
            pNew->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = pNew;
            pSet->pEnemyShotHead->prev = pNew;
        }
    }
}

// フェーズ2 : 旋風 (左右から渦を巻くレーザー)
static void UpdateWindStreaks(sEnemyShotSet* pSet)
{
    // 初回のみ4本の筋を作成 (左outer,左inner,右outer,右inner)
    if (pSet->count == 0) {
        struct { double delay; double radius; double angle; double rot; double shrink; }
        init[4] = {
            {  0.0, 240.0, DX_PI,  0.04, 0.5 }, // 左 outer
            { 60.0, 240.0, DX_PI, -0.04, 0.5 }, // 左 inner
            {  0.0, 240.0, 0.0,    0.04, 0.5 }, // 右 outer
            { 60.0, 240.0, 0.0,   -0.04, 0.5 }  // 右 inner
        };

        for (int i = 0; i < 4; ++i) {
            sEnemyShot* p = new sEnemyShot;
            p->kind = img_enemyShotLaser[8]; // 橙色レーザー
            p->x = -100.0; p->y = -100.0;
            p->speed = 0.0;
            p->muki = 0.0;
            p->param_i[0] = i;
            p->param_d[0] = init[i].delay;
            p->param_d[1] = init[i].radius;
            p->param_d[2] = init[i].angle;
            p->param_d[3] = init[i].rot;
            p->param_d[4] = init[i].shrink;

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }

    // 毎フレーム更新
    const double cx = 240.0, cy = 240.0; // 渦の中心
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        double delay = p->param_d[0];
        if (pSet->count >= delay) {
            double& r = p->param_d[1];
            double& a = p->param_d[2];
            double  rot = p->param_d[3];
            double  shrink = p->param_d[4];

            r -= shrink;
            if (r < 5.0) {
                // 消滅 (画面外へ)
                p->x = -100.0; p->y = -100.0;
            }
            else {
                a += rot;
                p->x = cx + r * cos(a);
                p->y = cy + r * sin(a);
                // 進行方向に画像を向ける
                double vx = -shrink * cos(a) - r * sin(a) * rot;
                double vy = -shrink * sin(a) + r * cos(a) * rot;
                p->muki = atan2(vy, vx);
            }
        }
        p = p->next;
    }
}

// フェーズ2 : 砂柱 + 炸裂
static void UpdatePillar(sEnemyShotSet* pSet)
{
    // 初回のみ砂柱の先端となる弾を1つ生成
    if (pSet->count == 0) {
        sEnemyShot* p = new sEnemyShot;
        p->x = pSet->x;
        p->y = pSet->y;
        p->muki = pSet->muki;            // -PI/2 (真上)
        p->speed = 2.0;
        p->kind = img_enemyShotMediumOval[8]; // 橙色の中楕円弾
        p->param_i[0] = 0;               // 未炸裂

        p->prev = pSet->pEnemyShotHead->prev;
        p->next = pSet->pEnemyShotHead;
        pSet->pEnemyShotHead->prev->next = p;
        pSet->pEnemyShotHead->prev = p;
    }

    // 全弾の移動 + 炸裂判定
    sEnemyShot* p = pSet->pEnemyShotHead->next;
    while (p != pSet->pEnemyShotHead) {
        p->x += p->speed * cos(p->muki);
        p->y += p->speed * sin(p->muki);

        // 砂柱が一定高度に達したら炸裂 (一度だけ)
        if (p->kind == img_enemyShotMediumOval[8] && p->param_i[0] == 0 && p->y < 80.0) {
            p->param_i[0] = 1; // 炸裂済み
            // 12方向に橙色小玉
            for (int i = 0; i < 12; ++i) {
                sEnemyShot* b = new sEnemyShot;
                b->x = p->x;
                b->y = p->y;
                double ang = i * (2.0 * DX_PI / 12.0);
                b->muki = ang;
                b->speed = 2.0;
                b->kind = img_enemyShotSmallBall[8];

                b->prev = pSet->pEnemyShotHead->prev;
                b->next = pSet->pEnemyShotHead;
                pSet->pEnemyShotHead->prev->next = b;
                pSet->pEnemyShotHead->prev = b;
            }
        }
        p = p->next;
    }
}

// フェーズ3 : 大砂嵐の目 (渦 → 爆発)
static void UpdateVortex(sEnemyShotSet* pSet)
{
    // 爆発後は何もしない
    if (pSet->param_i[0] == 1) return;

    if (pSet->count == 0) {
        // 初期渦 : 半径200の円上に36個の小玉
        double cx = pSet->x, cy = pSet->y;
        int N = 36;
        for (int i = 0; i < N; ++i) {
            double ang = i * (2.0 * DX_PI / N);
            sEnemyShot* p = new sEnemyShot;
            p->x = cx + 200.0 * cos(ang);
            p->y = cy + 200.0 * sin(ang);
            p->speed = 0.0;
            p->muki = 0.0;
            p->kind = img_enemyShotSmallBall[8];
            p->param_d[0] = 200.0; // 現在半径
            p->param_d[1] = ang;   // 現在角度
            p->param_d[2] = 0.03;  // 角速度
            p->param_d[3] = 0.15;  // 縮小速度
            p->param_i[0] = 0;

            p->prev = pSet->pEnemyShotHead->prev;
            p->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = p;
            pSet->pEnemyShotHead->prev = p;
        }
    }
    else if (pSet->count == 800) {
        // 渦収縮完了 → 爆発四散
        // 既存の渦弾を全消去
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            sEnemyShot* next = p->next;
            p->prev->next = p->next;
            p->next->prev = p->prev;
            delete p;
            p = next;
        }
        // 全方位 72発の衝撃波
        double cx = enemy.x, cy = enemy.y;
        for (int i = 0; i < 72; ++i) {
            double ang = i * (2.0 * DX_PI / 72.0);
            sEnemyShot* b = new sEnemyShot;
            b->x = cx;
            b->y = cy;
            b->muki = ang;
            b->speed = 3.0;
            b->kind = img_enemyShotSmallBall[8];

            b->prev = pSet->pEnemyShotHead->prev;
            b->next = pSet->pEnemyShotHead;
            pSet->pEnemyShotHead->prev->next = b;
            pSet->pEnemyShotHead->prev = b;
        }
        pSet->param_i[0] = 1; // 爆発済みフラグ
        return;
    }
    else {
        // 通常の収縮 + 飛散処理
        double cx = pSet->x, cy = pSet->y;
        sEnemyShot* p = pSet->pEnemyShotHead->next;
        while (p != pSet->pEnemyShotHead) {
            double& r = p->param_d[0];
            double& a = p->param_d[1];
            double  av = p->param_d[2];
            double  sh = p->param_d[3];

            r -= sh;
            if (r < 5.0) {
                // 中心に吸い込まれて消滅
                sEnemyShot* next = p->next;
                p->prev->next = p->next;
                p->next->prev = p->prev;
                delete p;
                p = next;
                continue;
            }
            a += av;
            p->x = cx + r * cos(a);
            p->y = cy + r * sin(a);

            // 2% の確率で弾を外側へ不規則に飛ばす
            if (GetRand(99) < 2) {
                r += 30.0;
                a += (GetRand(40) - 20) * DX_PI / 180.0;
            }
            p = p->next;
        }
    }
}

// ShotSet 作成の簡易ヘルパー
static sEnemyShotSet* CreateShotSet(double x, double y, double muki, int kind,
    void (*pattern)(sEnemyShotSet*))
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = pattern;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->kind = kind;

    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;

    // 全体の ShotSet リスト末尾に追加
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;

    return pSet;
}

//============================================================
// 敵本体パターン
//============================================================
void EnemyPat_SandStorm_DeepSeek()
{
    static int  phase;           // 1:砂壁  2:旋風  3:大砂嵐の目  0:終了
    static int  phaseStart;
    static int  pillarTimer;     // 砂柱の生成間隔カウンタ

    // パターン開始
    if (count == 1) {
        // 初期配置
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200;

        phase = 1;
        phaseStart = count;
        pillarTimer = 0;

        // フェーズ1 : 砂壁のセットを作成
        CreateShotSet(0, 0, 0, 0, UpdateSandWall);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
    }
    else {
        int elapsed = count - phaseStart;

        // フェーズ移行
        if (phase == 1 && elapsed >= 900) {   // 20秒経過
            phase = 2;
            phaseStart = count;
            pillarTimer = 0;
            CreateShotSet(0, 0, 0, 0, UpdateWindStreaks);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }
        else if (phase == 2 && elapsed >= 900) { // 次の25秒経過
            phase = 3;
            phaseStart = count;
            enemy.x = 240.0;
            enemy.y = 240.0;                     // 中心へ移動
            CreateShotSet(enemy.x, enemy.y, 0, 0, UpdateVortex);
            PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);
        }
        else if (phase == 3 && elapsed >= 900) {  // さらに15秒で終了
            phase = 0;   // パターン終了
        }

        // フェーズ2中は定期的に砂柱を生成
        if (phase == 2) {
            if (pillarTimer <= 0) {
                double px = 60.0 + GetRand(360);           // X = 60～420
                CreateShotSet(px, 480.0, -DX_PI / 2.0, 0, UpdatePillar);
                pillarTimer = 30;                          // 0.5秒間隔
            }
            else {
                --pillarTimer;
            }
        }
    }
}