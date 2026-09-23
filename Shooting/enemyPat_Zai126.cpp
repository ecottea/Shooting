#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ============================================================
// 弾幕：メントスコーラ「メントス投下・コーラ大噴射」
//
// ・橙の大玉＝コーラの液体(落下して画面下に溜まり液面を形成)
// ・白の小玉＝メントス(落下加速して液面に着弾)
// ・橙の鱗弾＝噴き上がるジェット(減速しながら上昇)
// ・シアンの小玉＝炭酸の泡(浮き上がってからゆっくり降下)
//
// count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと
// 画面外の弾の消去はメインルーチンで行われるため、ここでは行わない。
// ============================================================

// ---- 液面(コーラ溜まり)の共有状態 ----
static int g_poolCount = 0;                 // 溜まったコーラ弾の数
static constexpr int POOL_CAP = 24*999;         // 溜め上限(液面の最高到達点)
static constexpr double POOL_STEP = 5.5-3.5;    // 1個ごとの液面上昇量

// 液面のY座標を取得
static double GetSurfaceY()
{
    return 470.0 - g_poolCount * POOL_STEP;
}

// ---- 噴射(ジェット)の管理 ----
struct sJet {
    double x;
    int    timer;
    bool   active;
};
static sJet g_jets[8];

static void ResetJets()
{
    for (int i = 0; i < 8; i++) g_jets[i].active = false;
}

static void AddJet(double x)
{
    for (int i = 0; i < 8; i++) {
        if (!g_jets[i].active) {
            g_jets[i].x = x;
            g_jets[i].timer = 26;   // 約0.4秒噴射し続ける
            g_jets[i].active = true;
            return;
        }
    }
}

// ---- 弾の状態遷移用フラグ(param_i[0]) ----
//  0: コーラ落下中   1: コーラ滞留中(液面)
// 10: メントス落下中
// 20: ジェット上昇中 21: 泡(浮上→降下)

// 新しい弾セットを生成して敵弾リストに繋ぐ
static void SpawnSet(sEnemyShotSet::PatternFunc func, int kind)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y;
    pEnemyShotSet->muki = 0.0;
    pEnemyShotSet->kind = kind;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// 新しい弾を生成してセットに繋ぐ
static void AddShot(sEnemyShotSet* pEnemyShotSet, double x, double y,
    double muki, double speed, int kind, int state)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->param_i[0] = state;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// ------------------------------------------------------------
// コーラ(大玉)の供給と液面の形成
// ------------------------------------------------------------
static void ShotColaPool(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->count == 0) {
        // 使える効果音: sound_enemyShot_light, sound_enemyShot_medium,
        //               sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        // コーラの塊を4つ、時間差で落ちてくるように初期位置をずらして投下
        for (int i = 0; i < 4; i++) {
            AddShot(pEnemyShotSet,
                0.0 + GetRand(480),            // 画面のランダムな位置に落ちる
                -20.0 - i * 40.0,               // 縦にずらして列車のように
                DX_PI / 2.0, 2.5,
                img_enemyShotLargeBall[8],      // 大玉・橙＝コーラ
                0);                             // 落下中
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 落下中 → 液面に到達したら滞留に切り替え
            pShot->y += pShot->speed;
            if (pShot->y >= GetSurfaceY() - 12.0) {
                if (g_poolCount < POOL_CAP) g_poolCount++;
                pShot->y = GetSurfaceY();           // 新しい液面の高さに固定
                pShot->param_i[0] = 1;
            }
        }
        else {
            // 滞留中 → 液面が波打つようにゆっくり揺れる
            pShot->x += sin(pShot->count * 0.05) * 0.3;
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// メントス(白小玉)の投下と大噴射
// ------------------------------------------------------------
static void ShotMentosFlow(sEnemyShotSet* pEnemyShotSet)
{
    // メントスの投下(セット生成直後の1回だけ)
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);   // 投下の予告音

        AddShot(pEnemyShotSet,
            30.0 + GetRand(420),                // 落下地点で読ませる
            -10.0,
            DX_PI / 2.0, 1.0,
            img_enemyShotSmallBall[6],          // 小玉・白＝メントス
            10);                                // メントス落下中
    }

    // アクティブなジェットから鱗弾を毎フレーム2発ずつ噴き出させる
    for (int i = 0; i < 8; i++) {
        if (!g_jets[i].active) continue;
        g_jets[i].timer--;
        if (g_jets[i].timer <= 0) {
            g_jets[i].active = false;
            continue;
        }
        for (int j = 0; j < 2; j++) {
            AddShot(pEnemyShotSet,
                g_jets[i].x + (GetRand(20) - 10) * 0.8,     // 噴射口で揺らぐ
                GetSurfaceY() - 6.0,
                -DX_PI / 2.0 + (GetRand(30) - 15) / 180.0 * DX_PI, // 真上±15度
                8.0 + GetRand(150) / 100.0,
                img_enemyShotScale[8],          // 鱗弾・橙＝コーラのジェット
                20);                            // ジェット上昇中
        }
    }

    // 弾の移動と状態遷移
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {
        case 10: // メントス落下：徐々に加速
            if (pShot->speed < 6.0) pShot->speed += 0.08;
            pShot->y += pShot->speed;

            // 液面に着弾 → 大噴射を開始してメントスは消滅
            if (pShot->y >= GetSurfaceY() - 8.0) {
                AddJet(pShot->x);
                if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
                PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

                sEnemyShot* pDel = pShot;
                pShot = pShot->next;
                pDel->prev->next = pDel->next;
                pDel->next->prev = pDel->prev;
                delete pDel;
                continue;
            }
            break;

        case 20: // ジェット：減速しながら上昇、左右に揺れる
            pShot->speed *= 0.975;
            pShot->x += pShot->speed * cos(pShot->muki) + sin(pShot->count * 0.4) * 0.5;
            pShot->y += pShot->speed * sin(pShot->muki);

            // 失速したら泡(炭酸)に変化
            if (pShot->speed < 1.2) {
                pShot->param_i[0] = 21;
                pShot->kind = img_enemyShotSmallBall[3];   // 小玉・シアン＝泡
                pShot->param_d[0] = -0.6;                  // 泡の初速(上向き)
            }
            break;

        case 21: // 泡：ふわふわ浮き上がり、やがてゆっくり降下
            pShot->param_d[0] += 0.02;
            if (pShot->param_d[0] > 1.0) pShot->param_d[0] = 1.0;
            pShot->y += pShot->param_d[0];
            pShot->x += sin(pShot->count * 0.1) * 0.3;
            break;
        }
        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_MentosCola_Zai()
{
    static int muki;
    static int colaTimer;    // コーラ投下までのカウンタ
    static int mentosTimer;  // メントス投下までのカウンタ

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        muki = 1;
        g_poolCount = 0;
        ResetJets();
        colaTimer = 60;
        mentosTimer = 20;
    }
    else {
        // 左右に揺れ動く
        enemy.x += 0.98 * (double)muki;
        if (count % 120 == 60) muki *= -1;
        if (enemy.x < 40.0)  enemy.x = 40.0;
        if (enemy.x > 440.0) enemy.x = 440.0;
    }

    // コーラの供給(液面が上限に達するまで繰り返す)
    if (g_poolCount < POOL_CAP) {
        colaTimer--;
        if (colaTimer <= 0) {
            colaTimer = 20;
            SpawnSet(ShotColaPool, 0);
        }
    }

    // メントスの投下(液面が出来上がってからも継続し、噴射が連鎖する)
    mentosTimer--;
    if (mentosTimer <= 0) {
        mentosTimer = 110;
        SpawnSet(ShotMentosFlow, 1);
    }
}