// ============================================================
//  enemyPat_Tmp.cpp
//  弾幕：ミュラーリヤー・シャフト
// ------------------------------------------------------------
//  同じ全長の「棒」(小玉の横列)を左右交互に落下させる。
//   A型: 両端の翼が外側に開く「>—<」 → 実際より短く見える(シアン)
//   B型: 両端の翼が内側に被る「<—>」 → 実際より長く見える(マゼンタ)
//  A型の先端は画面下1/3ラインに達すると、外側へ豆弾3連射。
//  「見た目より攻撃範囲が長い」= 錯視の裏切りをギミック化。
//
//  ※ count / pEnemyShotSet->count / pEnemyShot->count のインクリメント、
//    画面外の弾の消去はメインルーチンで行う前提。
// ============================================================
#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

static constexpr double BAR_HALF = 120.0;  // 棒の半長(全長240でA/B共通)
static constexpr int    BAR_NUM = 17;     // 本体玉の数
static constexpr double BAR_PITCH = 15.0;   // 本体玉の間隔
static constexpr double WING_OFF = 10.0;   // 翼玉のオフセット(斜め45°)
static constexpr double FIRE_LINE = 360.0;  // 隠し射撃ライン(画面480の下1/3)

// 玉1個をセットの弾リストへ追加
static void AddShot(sEnemyShotSet* pSet, double x, double y,
    double muki, double speed, int color,
    int tipType, int dir)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = img_enemyShotSmallBall[color];
    p->param_i[0] = tipType; // 0:本体/翼 1:A型先端(隠し射撃あり) 2:B型先端
    p->param_i[1] = 0;       // 発射済み弾数
    p->param_i[2] = 0;       // 連射用タイマー
    p->param_i[3] = dir;     // +1:右端 -1:左端(本体/翼では未使用)
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// 弾幕本体：横一列の「棒」を生成し、落下させながらA型の隠し射撃を処理する
static void ShotMullerLyer(sEnemyShotSet* pSet)
{
    if (pSet->count == 0) {
        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium,
        //                   sound_enemyShot_heavy, sound_enemyShot_extreme,
        //                   sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        const int    type = pSet->kind % 2;    // 0:A型(短く見える) 1:B型(長く見える)
        const double speed = pSet->param_d[0];  // 落下速度(外側から受け取る)
        const double cx = pSet->x;
        const double cy = pSet->y;

        // 棒本体(白): 等間隔の小玉列 ※A/Bで全長・速度とも完全に同一
        for (int i = 0; i < BAR_NUM; i++) {
            AddShot(pSet, cx - BAR_HALF + BAR_PITCH * i, cy,
                DX_PI / 2.0, speed, 6, 0, 0);
        }

        // 両端の装飾(先端玉+斜め45°の翼玉2個ずつ)
        for (int dir = -1; dir <= 1; dir += 2) {
            const double ex = cx + dir * BAR_HALF;
            // 先端玉(色と役割がA/Bで違うだけ。当たり判定・位置は同一)
            AddShot(pSet, ex, cy, DX_PI / 2.0, speed,
                (type == 0) ? 3 : 5, (type == 0) ? 1 : 2, dir);
            // 翼: A型は外側へ開く(短く見える) / B型は内側へ被る(長く見える)
            for (int k = 1; k <= 2; k++) {
                const double wingX = (type == 0) ? ex + dir * WING_OFF * k   // 外側
                    : ex - dir * WING_OFF * k;  // 内側
                AddShot(pSet, wingX, cy - WING_OFF * k, DX_PI / 2.0, speed,
                    (type == 0) ? 3 : 5, 0, 0);
                AddShot(pSet, wingX, cy + WING_OFF * k, DX_PI / 2.0, speed,
                    (type == 0) ? 3 : 5, 0, 0);
            }
        }
    }

    // 毎フレーム: 全弾を移動 & A型先端の隠し射撃
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // A型先端(翼が外向き=短く見える棒)が発射ラインに達したら外向き3連射
        if (pShot->param_i[0] == 1 && pShot->param_i[1] < 3
            && pShot->y >= FIRE_LINE) {
            if (pShot->param_i[2] == 0) {
                // 予告音(錯視で読み違えていたプレイヤーへの警告)
                if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
                PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
            }
            if (pShot->param_i[2] % 8 == 0) {  // 8フレーム間隔で3発
                AddShot(pSet,
                    pShot->x + 6.0 * pShot->param_i[3], pShot->y,
                    (pShot->param_i[3] > 0) ? 0.0 : DX_PI,  // 真横外向き
                    3.5, 0, 0, 0);
                pShot->param_i[1]++;
            }
            pShot->param_i[2]++;
        }
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Tmp()
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

    // フェーズ別の間隔と落下速度
    //  前半: ゆっくり交互に降り、錯視を学習させる
    //  中盤: 速度アップ。A→B→Aの連続で幅の読み違えを誘発
    //  後半: さらに高速+A型の隠し射撃が実質的な判定拡張として機能
    int    interval;
    double fallSpeed;
    if (count <= 300) {
        interval = 150-20; fallSpeed = 1.2;
    }
    else if (count <= 600) {
        interval = 90-20; fallSpeed = 1.8;
    }
    else {
        interval = 65-20; fallSpeed = 2.4;
    }

    if (count % interval == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotMullerLyer;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 20.0;
        pEnemyShotSet->muki = DX_PI / 2.0; // 落下方向(各弾のmukiに反映)
        pEnemyShotSet->kind = shot_count++;          // 偶数:A型 奇数:B型
        pEnemyShotSet->param_d[0] = fallSpeed;       // 棒の落下速度
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}