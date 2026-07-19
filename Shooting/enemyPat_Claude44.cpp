// enemyPat_Tmp.cpp
// 「回転矢羽根」- 矢印(→)モチーフの回転弾幕
//
// 概要:
//   画面中央を軸に多数の矢クラスターが円周上に等間隔配置され、ゆっくり回転する。
//   一定周期ごとに各スロットが独立して抽選を行い、当たると自機へ向け直進・加速発射される。
//
// 重要な設計上の注意:
//   このゲームのメインルーチンは「sEnemyShotSet が持つ弾が全て画面外へ出て自動削除
//   されると、その sEnemyShotSet 自体も削除する」仕様になっている。そのため、
//   「発射済みの矢クラスターが空になったら補充状態へ…」という情報を、削除される
//   側の sEnemyShotSet 自身(param_i など)に持たせることはできない
//   (削除された時点でそのインスタンスの patternFunc は二度と呼ばれず、
//   ポインタも安全に参照できなくなるため)。
//
//   そこで、各スロットの状態(軌道上/発射中/補充待ち)はこのファイル内の
//   file-scope な静的配列 g_arrowSlots[] で管理する。この配列は特定の
//   sEnemyShotSet インスタンスのライフサイクルに依存せず、EnemyPat_Arrow_Claude() から
//   毎フレーム参照・更新される。
//
//   発射後は「画面外へ完全に出て削除されるまでの猶予フレーム数」を
//   FLIGHT_DURATION_FRAMES として保守的に見積もり、その経過をもって
//   「もう削除されているはず」とみなす。発射した sEnemyShotSet へのポインタは
//   発射を指示した直後に手放し(nullptr にし)、以後は一切参照しない。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

//==============================================================
// 定数
//==============================================================
static const int    NUM_ARROWS = 32;                     // 矢の本数(=回転スロット数)

static const double ORBIT_CENTER_X = 240.0;              // 回転軸(画面中央。画面は480x480)
static const double ORBIT_CENTER_Y = 240.0;
static const double ORBIT_RADIUS = 200.0;                // 回転半径
static const double ROTATION_SPEED = -0.0025;            // 1フレームあたりの回転角[rad](負値=反時計回り)

// 矢の形状(ローカル座標。+X方向を矢の先端方向とする)
static const double ARROW_TIP_X = 30.0;                  // 中心から先端までの距離
static const double ARROW_BARB_BACK = 10.0;              // 先端から羽根までの後退距離
static const double ARROW_BARB_SIDE = 7.0;               // 羽根の左右への開き幅
static const double ARROW_SHAFT_SPACING = 9.0;           // 矢柄の弾間隔
static const int    ARROW_SHAFT_COUNT = 4;               // 矢柄を構成する弾数
static const int    ARROW_SHOT_COUNT = ARROW_SHAFT_COUNT + 2; // 矢1本の総弾数(羽根2+柄4=6)

// 発射(自機狙い撃ち)関連
static const int    LAUNCH_CHECK_INTERVAL = 90;          // 発射抽選の周期[フレーム](60fpsで1.5秒)
static const int    LAUNCH_PROBABILITY_PERCENT = 24;     // 抽選が回ってきた時に発射する確率[%]
static const int    MAX_CONCURRENT_LAUNCHED = 16;        // 同時に発射状態になれる矢の最大数
static const double LAUNCH_INITIAL_SPEED = 3.0;          // 発射直後の速度
static const double LAUNCH_ACCEL = 0.05;                 // 発射後、毎フレーム加算する加速度

// 発射してから画面外に出て自動削除されるまでの猶予フレーム数(保守的な見積もり)。
// 軌道半径200の位置から初速3.0・加速度0.05で480x480画面+マージンを確実に
// 抜けきるのに必要なフレーム数を余裕を持って見積もった値。
static const int    FLIGHT_DURATION_FRAMES = 160;
static const int    REPLENISH_DELAY_FRAMES = 10;         // 消滅想定後、補充までの待機フレーム数

// スロット1つあたりの角度間隔(コンパイル時定数。SlotBaseAngle()での毎フレーム除算を避けるため)
static const double ANGLE_STEP = 2.0 * DX_PI / NUM_ARROWS;

// 矢クラスター(sEnemyShotSet)自身の移動モード(param_i[0] に格納)
enum {
    MOVE_ORBIT = 0,  // 軌道上を回転移動
    MOVE_LAUNCH = 1, // 自機へ向け直進
};

// スロット(回転軌道上の位置)の状態
enum {
    SLOT_ORBIT = 0,    // 矢が軌道上に存在し、発射抽選待ち
    SLOT_LAUNCHED = 1, // 矢を発射済み(sEnemyShotSet はもう参照しない)
    SLOT_WAITING = 2,  // 消滅想定後、補充までのクールダウン中
};

// スロットごとの永続的な管理情報。
// 特定の sEnemyShotSet インスタンスが削除されても、この情報自体は消えない。
struct ArrowSlot {
    int            state;  // SLOT_ORBIT / SLOT_LAUNCHED / SLOT_WAITING
    int            timer;  // 状態ごとに意味が変わる汎用カウントダウン
    sEnemyShotSet* pSet;    // SLOT_ORBIT の間だけ有効。それ以外は nullptr(参照しない)
};

static ArrowSlot g_arrowSlots[NUM_ARROWS];
static int       g_launchedCount = 0; // 現在 SLOT_LAUNCHED 状態にあるスロット数

//==============================================================
// スロット番号から軌道上の基準角度を求める(乗算のみ。除算はコンパイル時に済ませてある)
//==============================================================
static double SlotBaseAngle(int slotIndex)
{
    return slotIndex * ANGLE_STEP;
}

//==============================================================
// 矢1本分の弾(羽根2+柄4=計6発)を生成する
// 呼び出し前に pEnemyShotSet->x, y, muki が確定していること。
//==============================================================
static void SpawnArrowShots(sEnemyShotSet* pEnemyShotSet)
{
    // ローカル座標(矢の先端方向を+Xとする、原点=矢クラスターの中心)。
    // static const にして、呼び出しのたびに再構築されないようにする。
    static const double localOffsets[ARROW_SHOT_COUNT][2] = {
        // 矢尻(先端の羽根)「く」の字型に左右2発
        { ARROW_TIP_X - ARROW_BARB_BACK,  ARROW_BARB_SIDE },
        { ARROW_TIP_X - ARROW_BARB_BACK, -ARROW_BARB_SIDE },
        // 矢柄(先端から後方へ一直線に4発)
        { ARROW_TIP_X - ARROW_SHAFT_SPACING * 1, 0.0 },
        { ARROW_TIP_X - ARROW_SHAFT_SPACING * 2, 0.0 },
        { ARROW_TIP_X - ARROW_SHAFT_SPACING * 3, 0.0 },
        { ARROW_TIP_X - ARROW_SHAFT_SPACING * 4, 0.0 },
    };

    double c = cos(pEnemyShotSet->muki);
    double s = sin(pEnemyShotSet->muki);

    for (int i = 0; i < ARROW_SHOT_COUNT; i++) {
        sEnemyShot* pEnemyShot = new sEnemyShot;

        double lx = localOffsets[i][0];
        double ly = localOffsets[i][1];

        // ローカルオフセットは毎フレーム param_d[] から読み出して再回転させるため保持しておく
        pEnemyShot->param_d[0] = lx;
        pEnemyShot->param_d[1] = ly;

        pEnemyShot->x = pEnemyShotSet->x + lx * c - ly * s;
        pEnemyShot->y = pEnemyShotSet->y + lx * s + ly * c;
        pEnemyShot->muki = pEnemyShotSet->muki; // 弾の向き(描画・当たり判定の回転に使われる)

        // 細い矢を表現するため img_enemyShotBullet (5.0 x 2.0) を使用。
        // img_enemyShotLaser (64.0 x 4.0) は見た目に対して当たり判定が大きすぎるため不使用。
        pEnemyShot->kind = img_enemyShotBullet[3]; // 3:シアン

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }
}

//==============================================================
// 弾幕パターン: 矢クラスター1本分の移動処理
// (状態遷移・発射抽選などのスロット管理は一切行わない。
//  自分がどう動くか = param_i[0] の指示に従うだけの「素直な」関数)
//
// 計算量メモ: muki の cos/sin はこの関数内で1回だけ計算し、
// クラスター中心の移動計算と、配下の弾の座標更新の両方で使い回す
// (以前は同じ角度に対して cos/sin を2回ずつ計算しており無駄があった)。
//==============================================================
static void ShotArrowRotate(sEnemyShotSet* pEnemyShotSet)
{
    if (pEnemyShotSet->param_i[0] == MOVE_ORBIT) {
        // 軌道移動: 毎フレーム、グローバルな経過フレーム数(count)を基準に向きを再計算する
        // (このクラスター固有の pEnemyShotSet->count は補充のたびに0からリセットされるため、
        //  回転の基準には使えない)
        pEnemyShotSet->muki = SlotBaseAngle(pEnemyShotSet->kind) + ROTATION_SPEED * count;
    }
    // MOVE_LAUNCH の場合、muki は発射した瞬間の狙い角度で固定済みなのでここでは変更しない

    double c = cos(pEnemyShotSet->muki);
    double s = sin(pEnemyShotSet->muki);

    if (pEnemyShotSet->param_i[0] == MOVE_ORBIT) {
        pEnemyShotSet->x = ORBIT_CENTER_X + ORBIT_RADIUS * c;
        pEnemyShotSet->y = ORBIT_CENTER_Y + ORBIT_RADIUS * s;
    }
    else {
        // 発射後: 直進+加速
        double speed = pEnemyShotSet->param_d[0];
        pEnemyShotSet->x += speed * c;
        pEnemyShotSet->y += speed * s;
        pEnemyShotSet->param_d[0] += LAUNCH_ACCEL;
    }

    // クラスター内の各弾の絶対座標・向きを更新(ローカルオフセットを muki だけ回転)
    sEnemyShot* pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        double lx = pEnemyShot->param_d[0];
        double ly = pEnemyShot->param_d[1];
        pEnemyShot->x = pEnemyShotSet->x + lx * c - ly * s;
        pEnemyShot->y = pEnemyShotSet->y + lx * s + ly * c;
        pEnemyShot->muki = pEnemyShotSet->muki; // ここが未設定だったため、弾の向きが正しく反映されていなかった

        pEnemyShot = pEnemyShot->next;
    }
}

//==============================================================
// 指定スロットの位置に、軌道上を移動する新しい矢クラスターを生成する
// (初回生成・補充生成の両方から呼ばれる)
//==============================================================
static sEnemyShotSet* CreateArrowShotSet(int slotIndex)
{
    // count(グローバル)を基準に、現在この軌道上にあるべき角度を計算
    double angle = SlotBaseAngle(slotIndex) + ROTATION_SPEED * count;

    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotArrowRotate;
    pEnemyShotSet->kind = slotIndex;
    pEnemyShotSet->muki = angle;
    pEnemyShotSet->x = ORBIT_CENTER_X + ORBIT_RADIUS * cos(angle);
    pEnemyShotSet->y = ORBIT_CENTER_Y + ORBIT_RADIUS * sin(angle);
    pEnemyShotSet->param_i[0] = MOVE_ORBIT;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    SpawnArrowShots(pEnemyShotSet);

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;

    return pEnemyShotSet;
}

//==============================================================
// 敵本体のパターン
//==============================================================
void EnemyPat_Arrow_Claude()
{
    static int enemyMuki;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        enemyMuki = 1;

        g_launchedCount = 0;

        // 全スロット分の矢クラスターを生成し、回転軌道上に等間隔で配置。
        // 発射抽選のタイミングがスロット間でずれるよう、初期タイマーに位相差を付ける。
        for (int i = 0; i < NUM_ARROWS; i++) {
            g_arrowSlots[i].state = SLOT_ORBIT;
            g_arrowSlots[i].timer = LAUNCH_CHECK_INTERVAL + i * (LAUNCH_CHECK_INTERVAL / NUM_ARROWS);
            g_arrowSlots[i].pSet = CreateArrowShotSet(i);
        }
    }
    else {
        // 敵本体は左右に軽く揺れる程度の動き(任意の演出)
        enemy.x += 0.3 * (double)enemyMuki;
        if (count % 240 == 120) enemyMuki *= -1;
    }

    // スロットごとの状態管理(発射抽選・消滅想定・補充)。
    // sEnemyShotSet 自体には状態を持たせず、こちらの静的配列だけで管理する。
    for (int i = 0; i < NUM_ARROWS; i++) {
        ArrowSlot& slot = g_arrowSlots[i];

        switch (slot.state) {
        case SLOT_ORBIT:
            slot.timer--;
            if (slot.timer <= 0) {
                slot.timer = LAUNCH_CHECK_INTERVAL;
                if (g_launchedCount < MAX_CONCURRENT_LAUNCHED && GetRand(99) < LAUNCH_PROBABILITY_PERCENT) {
                    // slot.pSet はまだ生存が保証されている(SLOT_ORBIT の間だけ保持するポインタ)
                    slot.pSet->param_i[0] = MOVE_LAUNCH;
                    slot.pSet->muki = atan2(player.y - slot.pSet->y, player.x - slot.pSet->x);
                    slot.pSet->param_d[0] = LAUNCH_INITIAL_SPEED;

                    slot.pSet = nullptr; // 以後は一切参照しない(消滅はメインルーチン任せ)
                    slot.state = SLOT_LAUNCHED;
                    slot.timer = FLIGHT_DURATION_FRAMES;
                    g_launchedCount++;
                }
            }
            break;

        case SLOT_LAUNCHED:
            // 画面外に出て sEnemyShotSet ごと自動削除されるまでの猶予を
            // フレーム数で見積もるだけで、実体には一切触れない。
            slot.timer--;
            if (slot.timer <= 0) {
                slot.state = SLOT_WAITING;
                slot.timer = REPLENISH_DELAY_FRAMES;
                g_launchedCount--;
            }
            break;

        case SLOT_WAITING:
            slot.timer--;
            if (slot.timer <= 0) {
                // 現在の軌道位置(count基準で計算)に新しい矢クラスターを生成
                slot.pSet = CreateArrowShotSet(i);
                slot.state = SLOT_ORBIT;
                slot.timer = LAUNCH_CHECK_INTERVAL;
            }
            break;
        }
    }
}