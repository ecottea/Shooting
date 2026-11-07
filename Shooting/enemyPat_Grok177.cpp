
// 三平方の定理の証明をモチーフにした弾幕パターン
// 辺a+bの大正方形内に4つの合同な直角三角形を配置し、中央に辺cの正方形が現れる図形を弾で描画。
// その後三角形群をスライドさせて再配置し、最後に弾を解放する。
// 大きさを2倍にし、パターンを反復、反復ごとに全体の角度を変化させる。
// 使用素材: 中玉・小玉（色分けでa=赤, b=青, c=黄）、効果音は charge / medium / heavy

#include "gv.h"

// 弾追加ヘルパー
static void AddEnemyShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind, int group)
{
    sEnemyShot* p = new sEnemyShot;
    p->x = x;
    p->y = y;
    p->muki = muki;
    p->speed = speed;
    p->kind = kind;
    p->count = 0;
    p->param_i[0] = group;          // 0-3: 三角形グループ, -1: その他
    p->param_d[0] = 0.0;
    p->param_d[1] = 0.0;
    p->prev = pSet->pEnemyShotHead->prev;
    p->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = p;
    pSet->pEnemyShotHead->prev = p;
}

// 点を中心周りに回転
static void RotatePoint(double& x, double& y, double cx, double cy, double ang)
{
    double dx = x - cx;
    double dy = y - cy;
    x = cx + dx * cos(ang) - dy * sin(ang);
    y = cy + dx * sin(ang) + dy * cos(ang);
}

// 2点間に弾を等間隔配置（両端含む）。必要なら回転を適用
static void AddLineShots(sEnemyShotSet* pSet, double x1, double y1, double x2, double y2, int num, int colorKind, int group, double cx, double cy, double ang)
{
    if (num < 2) num = 2;
    RotatePoint(x1, y1, cx, cy, ang);
    RotatePoint(x2, y2, cx, cy, ang);
    for (int i = 0; i < num; i++) {
        double t = (double)i / (num - 1);
        double x = x1 + (x2 - x1) * t;
        double y = y1 + (y2 - y1) * t;
        int kind = img_enemyShotMediumBall[colorKind];
        if (i == 0 || i == num - 1) {
            kind = img_enemyShotSmallBall[colorKind];
        }
        AddEnemyShot(pSet, x, y, 0.0, 0.0, kind, group);
    }
}

// メイン弾幕パターン関数
static void ShotPythagoras(sEnemyShotSet* pEnemyShotSet)
{
    // 幾何パラメータ（3:4:5 を2倍スケール）
    const double A = 120.0;
    const double B = 160.0;
    const double S = A + B;          // 280
    const double CX = 240.0;
    const double CY = 240.0;
    const double LEFT = CX - S * 0.5;
    const double TOP = CY - S * 0.5;
    const double RIGHT = LEFT + S;
    const double BOTTOM = TOP + S;

    // 中点（各辺上の脚の接合点）
    const double M_BOTTOM_X = LEFT + A;
    const double M_LEFT_Y = BOTTOM - B;
    const double M_RIGHT_Y = TOP + B;
    const double M_TOP_X = RIGHT - A;

    // サイクル長とフェーズ
    const int CYCLE = 480-250;
    const int PHASE_HOLD_END = 150-30;
    const int PHASE_REARR_END = 320-150;

    int cycleIndex = pEnemyShotSet->count / CYCLE;
    int phase = pEnemyShotSet->count % CYCLE;

    // 反復ごとに角度を変化（36度ずつ）
    double ang = cycleIndex * (DX_PI / 5.0);

    // 新しいサイクルの開始時に図形を生成
    if (phase == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // group 0: 左下三角形
        AddLineShots(pEnemyShotSet, LEFT, BOTTOM, M_BOTTOM_X, BOTTOM, 15, 0, 0, CX, CY, ang); // a
        AddLineShots(pEnemyShotSet, LEFT, BOTTOM, LEFT, M_LEFT_Y, 17, 4, 0, CX, CY, ang);     // b
        AddLineShots(pEnemyShotSet, M_BOTTOM_X, BOTTOM, LEFT, M_LEFT_Y, 18, 1, 0, CX, CY, ang); // c

        // group 1: 右下三角形
        AddLineShots(pEnemyShotSet, RIGHT, BOTTOM, M_BOTTOM_X, BOTTOM, 17, 4, 1, CX, CY, ang); // b
        AddLineShots(pEnemyShotSet, RIGHT, BOTTOM, RIGHT, BOTTOM - A, 15, 0, 1, CX, CY, ang);  // a
        AddLineShots(pEnemyShotSet, M_BOTTOM_X, BOTTOM, RIGHT, BOTTOM - A, 18, 1, 1, CX, CY, ang); // c

        // group 2: 右上三角形
        AddLineShots(pEnemyShotSet, RIGHT, TOP, M_TOP_X, TOP, 15, 0, 2, CX, CY, ang);          // a
        AddLineShots(pEnemyShotSet, RIGHT, TOP, RIGHT, M_RIGHT_Y, 17, 4, 2, CX, CY, ang);      // b
        AddLineShots(pEnemyShotSet, M_TOP_X, TOP, RIGHT, M_RIGHT_Y, 18, 1, 2, CX, CY, ang);    // c

        // group 3: 左上三角形
        AddLineShots(pEnemyShotSet, LEFT, TOP, M_TOP_X, TOP, 17, 4, 3, CX, CY, ang);           // b
        AddLineShots(pEnemyShotSet, LEFT, TOP, LEFT, M_LEFT_Y, 15, 0, 3, CX, CY, ang);         // a
        AddLineShots(pEnemyShotSet, M_TOP_X, TOP, LEFT, M_LEFT_Y, 18, 1, 3, CX, CY, ang);      // c

        // 内側正方形の頂点付近の印（回転適用）
        double px, py;
        px = M_BOTTOM_X; py = BOTTOM - 12.0; RotatePoint(px, py, CX, CY, ang);
        AddEnemyShot(pEnemyShotSet, px, py, 0.0, 0.0, img_enemyShotSmallBall[1], -1);
        px = RIGHT - 12.0; py = BOTTOM - A; RotatePoint(px, py, CX, CY, ang);
        AddEnemyShot(pEnemyShotSet, px, py, 0.0, 0.0, img_enemyShotSmallBall[1], -1);
        px = M_TOP_X; py = TOP + 12.0; RotatePoint(px, py, CX, CY, ang);
        AddEnemyShot(pEnemyShotSet, px, py, 0.0, 0.0, img_enemyShotSmallBall[1], -1);
        px = LEFT + 12.0; py = M_LEFT_Y; RotatePoint(px, py, CX, CY, ang);
        AddEnemyShot(pEnemyShotSet, px, py, 0.0, 0.0, img_enemyShotSmallBall[1], -1);
    }

    // 弾の更新（全弾を対象。古いサイクルの弾も動き続ける）
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        int group = pShot->param_i[0];

        // この弾が属するサイクルのphaseを厳密に追うのは難しいので、
        // 速度がまだ0に近いものは「形成中/再配置中」とみなし、既に速度が付いているものは解放済みとして通常移動
        if (pShot->speed < 0.05) {
            // まだ解放されていない弾 → 現在のphaseに従って動かす
            if (phase < PHASE_HOLD_END) {
                pShot->speed = 0.0;
            }
            else if (phase < PHASE_REARR_END) {
                // 再配置：スライド方向も現在のangで回転させる
                double vx = 0.0, vy = 0.0;
                double t = (phase - PHASE_HOLD_END) / (double)(PHASE_REARR_END - PHASE_HOLD_END);
                double spd = 1.1 + 0.7 * t;

                switch (group) {
                case 0: // 左下方向ベース
                    vx = -spd * 0.85;
                    vy = spd * 0.55;
                    break;
                case 1:
                    vx = spd * 0.85;
                    vy = spd * 0.55;
                    break;
                case 2:
                    vx = spd * 0.85;
                    vy = -spd * 0.55;
                    break;
                case 3:
                    vx = -spd * 0.85;
                    vy = -spd * 0.55;
                    break;
                default:
                    vx = (pShot->x - CX) * 0.01;
                    vy = (pShot->y - CY) * 0.01;
                    break;
                }
                // スライドベクトルをangで回転
                double rvx = vx * cos(ang) - vy * sin(ang);
                double rvy = vx * sin(ang) + vy * cos(ang);
                pShot->x += rvx;
                pShot->y += rvy;
                pShot->speed = 0.0;
            }
            else {
                // 解放開始：外向き速度を付与
                double dx = pShot->x - CX;
                double dy = pShot->y - CY;
                double len = sqrt(dx * dx + dy * dy);
                if (len < 1.0) len = 1.0;
                pShot->muki = atan2(dy, dx) + (GetRand(40) - 20) / 180.0 * DX_PI;
                pShot->speed = 2.0 + GetRand(140) / 100.0;
            }
        }
        else {
            // 既に解放済み：通常の等速直線運動
            pShot->x += pShot->speed * cos(pShot->muki);
            pShot->y += pShot->speed * sin(pShot->muki);
        }

        pShot = pShot->next;
    }

    // 解放開始時の追加ばら撒き（各サイクルで）
    if (phase == PHASE_REARR_END) {
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 20*3; i++) {
            double baseAng = (i / 20.0/3) * 2.0 * DX_PI + ang;
            double shotAng = baseAng + (GetRand(30) - 15) / 180.0 * DX_PI;
            double spd = 2.4 + GetRand(120) / 100.0;
            int col = (i % 3 == 0) ? 0 : (i % 3 == 1) ? 4 : 1;
            AddEnemyShot(pEnemyShotSet,
                CX + GetRand(40) - 20,
                CY + GetRand(40) - 20,
                shotAng, spd,
                img_enemyShotMediumBall[col], -1);
        }
    }

    // 解放中の追加ばら撒き
    if (phase > PHASE_REARR_END && (phase % 28) == 6) {
        if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
        PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        for (int i = 0; i < 7*3; i++) {
            double shotAng = GetRand(360) / 180.0 * DX_PI;
            AddEnemyShot(pEnemyShotSet,
                CX, CY,
                shotAng, 1.8 + GetRand(100) / 100.0,
                img_enemyShotSmallBall[GetRand(2) == 0 ? 0 : 4], -1);
        }
    }
}

// 敵本体
void EnemyPat_PythagoreanTheorem_Grok()
{
    static int initialized = 0;

    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200;
        initialized = 0;
    }

    // 敵は上部で小さく揺らす
    if (count > 1) {
        enemy.x = 240.0 + 10.0 * sin(count * 0.025);
        enemy.y = 60.0 + 4.0 * sin(count * 0.04);
    }

    // パターン開始：最初の1回だけ長寿命のショットセットを生成
    if (count == 30 && initialized == 0) {
        initialized = 1;

        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotPythagoras;
        pEnemyShotSet->x = 240.0;
        pEnemyShotSet->y = 240.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->alive = 99999;       // ほぼ永久に生存させて反復させる
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}