// ============================================================
//  綺星楼「黄金螺旋の咲崩」 ― TAS想定・超高難度アート弾幕
//  構成: 全1スペル 60秒(count 1-3599) / 3相
//    Phase A (count    1- 899) 星形檻  : {16/k} 星形多角形の弦波。
//               弦の群れの包絡線が半径 230·cos(kπ/16) の同心円として
//               「弾の無い空白」として現れ、k=3→5→7→8 で 191→128→45→0 へ縮小。
//               弦を走り終えた弾は自機の現在位置へ再照準(誘導弾の「牧畜」が成立)。
//    Phase B (count  900-2339) 黄金咲き: 黄金角 137.50776° のフィロタクシス。
//               種弾が Vogel 配列 r=c·√i に静止して「黄金のひまわり」を描き、
//               全種静止後に開花(21本腕へ種が飛び出し、34本腕へ子弾を残す)。
//               開花後の飛行弾は凍結ストロボ 9/6→7/6→5/6 で移動(凍結中も判定有効)。
//    Phase C (count 2340-3599) 終薔薇 : マウラー・ローズ(n=5, d=71°)の弦を
//               白いインク弾で描き、240f 後に結晶化(白→赤)。網完成後に
//               全弦が放射方向へ飛び去り(崩壊)、黄金角の一斉放射で幕を下ろす。
//
//  仕様準拠メモ:
//   - count / pEnemyShotSet->count / pEnemyShot->count のインクリメントは
//     メインルーチンが行うため、本ファイルでは読み取り専用として扱う。
//   - 画面外に出た弾の消去もメインルーチンが行うため、本ファイルでは
//     delete / リストからの unlink を一切行わない(生成と更新のみ)。
//   - GetRand は使用しない。全弾道を数式と自機位置だけで決定論化し、
//     TAS によるルート探索・リプレイ再現を可能にしている。
// ============================================================

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ---- 幾何学定数 --------------------------------------------
static const double GOLDEN = DX_PI * (3.0 - sqrt(5.0)); // 黄金角 137.50776°
static const double A_XA = 240.0, A_YA = 240.0;         // Phase A 星形檻の中心(画面中央)
static const double A_XB = 240.0, A_YB = 210.0;         // Phase B 黄金咲きの中心
static const double A_XC = 240.0, A_YC = 250.0;         // Phase C 終薔薇の中心

// ---- 使用素材の選定 ----------------------------------------
//  [採用弾種]
//   菱形弾  img_enemyShotDiamond   (4.5x2.5) : A相 弦弾。直線の稜線が星形多角形と調和
//   中玉    img_enemyShotMediumBall(7.0x7.0) : B相 種弾(花の主役) / C相 自機狙い3-way
//   鱗弾    img_enemyShotScale     (4.0x3.0) : B相 開花弾。米粒がフィボナッチ腕を流れる
//   銃弾    img_enemyShotBullet    (5.0x2.0) : C相 インク弾。細長い針が「描く」動きに合う
//   小玉    img_enemyShotSmallBall (2.5x2.5) : C相 薔薇網(隙間を縫える唯一のサイズ)/最終放射
//  [不採用] 大玉・中楕円弾(判定が大きく精密幾何を壊す) / 短レーザー(静止弦の表現に不向き) / 黒(背景に埋もれる)
//  [色設計] A: 青(4)→シアン(3)→白(6)→マゼンタ(5) で包絡円の縮小段階を示す
//           B: 種=黄(1) / 開花弾=橙(8)
//           C: インク=白(6) → 結晶化で赤(0)。自機狙い=マゼンタ(5)
//  [効果音] heavy=開幕・崩壊突入 / medium=B相開始 / light=C相 網描画 /
//           extreme=開花・網解体 / charge=C相 予告
//  ※ 橙(8)の画像が未ロードの環境なら黄(1)やマゼンタ(5)へ差し替えてください。

// ---- 効果音再生ヘルパ(鳴っていたら止めてから再生) --------
static void PlayShotSound(int sound)
{
    if (CheckSoundMem(sound)) StopSoundMem(sound);
    PlaySoundMem(sound, DX_PLAYTYPE_BACK);
}

// ---- 弾を1発追加するヘルパ ---------------------------------
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, double speed, int kind)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = speed;
    pShot->kind = kind;
    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// ---- 弾セットを1個追加するヘルパ ---------------------------
static sEnemyShotSet* AddShotSet(double x, double y, sEnemyShotSet::PatternFunc func, int kind)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->patternFunc = func;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = 0.0;
    pSet->kind = kind;
    pSet->pEnemyShotHead = new sEnemyShot;
    pSet->pEnemyShotHead->prev = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->next = pSet->pEnemyShotHead;
    pSet->prev = enemyShotSetHead.prev;
    pSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pSet;
    enemyShotSetHead.prev = pSet;
    return pSet;
}

// ============================================================
//  Phase A: 星形檻(スターポリゴン・ギロチン)   set count 0-899
//  正16角形の頂点同士を結ぶ「弦」に沿って菱形弾が走る。
//  k=3,5,7 は奇数なので弦は16本すべて異なる経路。k=8(直径)は
//  同一直径上を双方向に流れ、密度が跳ね上がる「崩壊」を演出する。
// ============================================================
static void ShotPat_StarCage(sEnemyShotSet* pSet)
{
    const int    t = pSet->count;
    const double R = 230.0;

    // --- 波生成(42フレーム間隔 × 21波) ---
    if (t <= 840 && t % 42 == 0) {
        int w = t / 42;
        int k, color;
        if (w < 7) { k = 3; color = 4; } // {16/3} 青
        else if (w < 14) { k = 5; color = 3; } // {16/5} シアン
        else if (w < 18) { k = 7; color = 6; } // {16/7} 白
        else { k = 8; color = 5; } // 直径波 マゼンタ(包絡円が0へ崩壊)
        if (w == 18) PlayShotSound(sound_enemyShot_heavy); // 崩壊突入

        double phi = t * 0.35 * DX_PI / 180.0;                    // 格子全体の自転
        double chordTime = 2.0 * R * sin(k * DX_PI / 16.0) / 3.2; // 弦走行フレーム数
        for (int j = 0; j < 16; j++) {
            double a1 = phi + j * 2.0 * DX_PI / 16.0;
            for (int s = -1; s <= 1; s += 2) { // ±k の両方向(同じ弦を双方向に流す)
                double a2 = a1 + s * k * 2.0 * DX_PI / 16.0;
                double x1 = A_XA + R * cos(a1);
                double y1 = A_YA + R * sin(a1);
                double mk = atan2(sin(a2) - sin(a1), cos(a2) - cos(a1));
                sEnemyShot* pShot = AddShot(pSet, x1, y1, mk, 3.2, img_enemyShotDiamond[color]);
                pShot->param_i[0] = 0;           // 0: 弦走行中
                pShot->param_d[0] = 3.2;         // 基準速度(未使用だが保持)
                pShot->param_d[1] = chordTime;   // 再照準までのフレーム数
            }
        }
    }

    // --- 弾更新 ---
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 0) {
            // 弦を走り終えた瞬間、自機の「現在位置」へ再照準。
            // TAS はこの位置を選ぶことで未来の弾幕の形を彫刻する(誘導の牧畜)。
            if (pShot->count >= (int)pShot->param_d[1]) {
                pShot->muki = atan2(player.y - pShot->y, player.x - pShot->x);
                pShot->speed = 2.2;
                pShot->param_i[0] = 1;
            }
        }
        else if (pShot->param_i[0] == 1) {
            pShot->speed += 0.04;               // 追尾加速
            if (pShot->speed > 5.0) pShot->speed = 5.0;
        }
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  Phase B: 黄金咲き(ヴォーゲル・ブルーム)     set count 0-1439
//  3サイクル × 480f。各サイクル:
//    local 0-149   種発射(毎フレーム2発 × 150f = 300種)
//    local 150-299 種が Vogel 配列 r=c·√i へ減速飛行(花が育っていく)
//    local 300     開花: 種は21本腕方向へ自ら飛び出し、34本腕方向へ子弾を1発残す
//    local 300-479 開花弾が凍結ストロボで移動
//  種は静止後も回転する目標に吸着し続けるため、花は静止したまま自転する。
//  ストロボ位相はセット全体で一括制御(全弾が完全同期する)。
// ============================================================
static void ShotPat_GoldenBloom(sEnemyShotSet* pSet)
{
    static double seedX[300], seedY[300]; // 開花時に種の座標を退避
    const int t = pSet->count;
    const int rawCycle = t / 480;                                   // 生のサイクル番号
    const int cycle = (rawCycle < 3) ? rawCycle : 2;                // 丸め(以後は飛行弾の管理のみ)
    const int local = t % 480;
    const double c = (cycle == 0) ? 10.0 : (cycle == 1) ? 13.0 : 16.0; // 配列係数(花の成長)
    const int actLen = (cycle == 0) ? 9 : (cycle == 1) ? 7 : 5;     // ストロボ稼働長(凍結は6f固定)
    const bool active = (t % (actLen + 6)) < actLen;                // ストロボ位相

    // --- 種の発射 ---
    if (local < 150 && rawCycle < 3) {
        for (int q = 0; q < 2; q++) {
            int i = local * 2 + q;
            if (i >= 300) break;
            sEnemyShot* pShot = AddShot(pSet, pSet->x, pSet->y, 0.0, 2.5, img_enemyShotMediumBall[1]);
            pShot->param_i[0] = 0;       // 0: 成長中(目標へ飛行)
            pShot->param_i[1] = i;       // 種番号
            pShot->param_d[0] = c;       // このサイクルの配列係数
        }
    }

    // --- 開花(各サイクル local==300 の1フレームで一斉処理) ---
    if (local == 300 && rawCycle < 3) {
        PlayShotSound(sound_enemyShot_extreme);

        // (1) 全種の現在座標を退避
        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0 || pShot->param_i[0] == 1) {
                int i = pShot->param_i[1];
                seedX[i] = pShot->x;
                seedY[i] = pShot->y;
            }
            pShot = pShot->next;
        }
        // (2) 種自身は 21 本腕方向へ飛び出し(変身)、34 本腕方向へ子弾を1発残す
        pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 0 || pShot->param_i[0] == 1) {
                int i = pShot->param_i[1];
                int j21 = i - 21;
                int j34 = i - 34;
                // 隣接種(21/34個前の種)への方向 = フィボナッチ反葉旋傍の接線方向
                double mk21 = (j21 >= 0) ? atan2(seedY[i] - seedY[j21], seedX[i] - seedX[j21])
                    : atan2(seedY[i] - pSet->y, seedX[i] - pSet->x);
                double mk34 = (j34 >= 0) ? atan2(seedY[i] - seedY[j34], seedX[i] - seedX[j34])
                    : atan2(seedY[i] - pSet->y, seedX[i] - pSet->x);
                pShot->muki = mk21;
                pShot->speed = 2.4;
                pShot->param_i[0] = 2;               // 2: 飛行(種)
                pShot->param_d[0] = 2.4;             // ストロボ基準速度
                pShot->kind = img_enemyShotScale[8]; // 中玉黄 → 鱗弾橙に変身
                sEnemyShot* pChild = AddShot(pSet, pShot->x, pShot->y, mk34, 2.4, img_enemyShotScale[8]);
                pChild->param_i[0] = 3;              // 3: 飛行(子弾)
                pChild->param_d[0] = 2.4;
            }
            pShot = pShot->next;
        }
    }

    // --- 弾更新 ---
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        int st = pShot->param_i[0];
        if (st == 0 || st == 1) {
            // 種: 回転し続ける目標座標へ比例追従 → 花は静止したままゆっくり自転する
            int i = pShot->param_i[1];
            double phi = t * 0.20 * DX_PI / 180.0 * ((cycle % 2 == 0) ? 1.0 : -1.0); // サイクル毎に反転
            double rT = pShot->param_d[0] * sqrt((double)i);
            double thT = i * GOLDEN + phi;
            double dx = pSet->x + rT * cos(thT) - pShot->x;
            double dy = pSet->y + rT * sin(thT) - pShot->y;
            double dist = sqrt(dx * dx + dy * dy);
            if (dist < 2.5) { // 到達: 目標に吸着(以後、目標の回転に同期)
                pShot->x += dx;
                pShot->y += dy;
                pShot->speed = 0.0;
                pShot->param_i[0] = 1;
            }
            else {
                double spd = dist * 0.12; // 比例減速
                if (spd > 5.0) spd = 5.0;
                if (spd < 2.5) spd = 2.5;
                pShot->muki = atan2(dy, dx);
                pShot->speed = spd;
            }
        }
        else if (st == 2 || st == 3) {
            // 開花後の飛行: 凍結ストロボ。
            // 凍結中も当たり判定は有効 → TAS は 6f の凍結窓で静止弾列をピクセル精度で縫う。
            pShot->speed = active ? pShot->param_d[0] : 0.0;
        }
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  Phase C: 幾何崩壊(マウラーの終薔薇)         set count 0-1259
//  薔薇曲線 r=130cos(5θ) 上を舞う本体に対し、マウラー・ローズ
//  (n=5, d=71°, R=170)の弦を白いインク弾が走って「描いていく」。
//  インクは弦の端に達すると白小玉となって静止し、240f 後に結晶化(赤化)。
//  網が完成した後も自機狙い 3-way で圧をかけ続け、t==1080 で全弦が
//  中心から放射方向へ飛び去り(薔薇の崩壊)、t==1140 から黄金角の一斉放射。
// ============================================================
static void MaurerPoint(int m, double cx, double cy, double* px, double* py)
{
    // マウラー・ローズの点: 極角 = 71°·m、動径 = 170·cos(5·極角)
    double phi = m * 71.0 * DX_PI / 180.0;
    double r = 170.0 * cos(5.0 * phi);
    *px = cx + r * cos(phi); // r<0 は自動的に反対側へ配置される
    *py = cy + r * sin(phi);
}

static void ShotPat_MaurerRose(sEnemyShotSet* pSet)
{
    const int t = pSet->count;

    // --- 薔薇網の描画(弦 j を j·7f から開始、5f ごとに 30px 進んだ位置へインクを発射) ---
    //  発射位置を「インクが進んでいるべき距離」に一致させることで、
    //  1本のインクが弦を走り、その軌跡に弾が残っていく「描く」演出になる。
    if (t == 0) PlayShotSound(sound_enemyShot_light);
    if (t < 720) {
        for (int j = 0; j <= 71; j++) {
            int s = t - j * 7;
            if (s < 0 || s % 5 != 0) continue;
            double x1, y1, x2, y2;
            MaurerPoint(j, pSet->x, pSet->y, &x1, &y1);
            MaurerPoint(j + 1, pSet->x, pSet->y, &x2, &y2);
            double L = sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
            double d = (s / 5) * 30.0; // 発射時点でインクが進んでいるべき距離
            if (d > L) continue;
            double mk = atan2(y2 - y1, x2 - x1);
            sEnemyShot* pShot = AddShot(pSet, x1 + d * cos(mk), y1 + d * sin(mk), mk, 6.0, img_enemyShotBullet[6]);
            pShot->param_i[0] = 4;   // 4: インク飛行
            pShot->param_d[0] = L;   // 弦長(静止位置の判定)
            pShot->param_d[1] = 0.0; // 進行距離の累積
        }
    }

    // --- 自機狙い 3-way(網の隙間を縫いながらの回避を強制する圧) ---
    if (t >= 300 && t < 1080 && t % 60 == 30) {
        double base = atan2(player.y - enemy.y, player.x - enemy.x);
        for (int q = -1; q <= 1; q++) {
            sEnemyShot* pShot = AddShot(pSet, enemy.x, enemy.y, base + q * 7.0 * DX_PI / 180.0, 4.5, img_enemyShotMediumBall[5]);
            pShot->param_i[0] = 99; // 99: 単純直進(更新不要)
        }
    }

    // --- 網の解体(t==1080)と最終放射(t 1140-1199) ---
    if (t == 1080) PlayShotSound(sound_enemyShot_extreme);
    if (t >= 1140 && t < 1200) {
        for (int q = 0; q < 10; q++) {
            int i = (t - 1140) * 10 + q;            // 0-599 の 600発
            double mk = i * GOLDEN - DX_PI / 2.0;   // 黄金角ステップの全周放射
            double spd = 1.2 + (i % 20) * 0.18;     // 20段の速度差 → 同心の花模様が広がる
            sEnemyShot* pShot = AddShot(pSet, pSet->x, pSet->y, mk, spd, img_enemyShotSmallBall[0]);
            pShot->param_i[0] = 99;
        }
    }

    // --- 弾更新 ---
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        int st = pShot->param_i[0];
        if (st == 4) {
            // インク: 弦の端に達したら白小玉となって静止
            pShot->param_d[1] += pShot->speed;
            if (pShot->param_d[1] >= pShot->param_d[0]) {
                double over = pShot->param_d[1] - pShot->param_d[0];
                pShot->x -= over * cos(pShot->muki); // 端点に正確に揃える
                pShot->y -= over * sin(pShot->muki);
                pShot->speed = 0.0;
                pShot->param_i[0] = 5;               // 5: 未硬化(白)
                pShot->param_i[2] = 0;               // 硬化タイマ
                pShot->kind = img_enemyShotSmallBall[6];
            }
        }
        else if (st == 5) {
            // 未硬化 → 240f 後に結晶化(白→赤)
            pShot->param_i[2]++;
            if (pShot->param_i[2] >= 240) {
                pShot->param_i[0] = 6;               // 6: 硬化(赤)
                pShot->kind = img_enemyShotSmallBall[0];
            }
        }
        else if (st == 6 && t == 1080) {
            // 崩壊: 硬化済みの全弦が中心から放射方向へ飛び去る
            pShot->muki = atan2(pShot->y - pSet->y, pShot->x - pSet->x);
            pShot->speed = 7.0;
            pShot->param_i[0] = 7;                   // 7: 崩壊飛行
        }
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// ============================================================
//  敵本体のパターン: 綺星楼「黄金螺旋の咲崩」
//  count 1    : 初期化 + Phase A セット生成(開幕音)
//  count 900  : Phase B セット生成
//  count 2340 : Phase C セット生成(予告音)
//  count 3600 : スペル終了(以後はパターン側は飛行中の弾の管理のみ)
// ============================================================
void EnemyPat_GeometryTAS_Zai()
{
    if (count == 1) {
        enemy.x = 240.0;
        enemy.y = 90.0;
        enemy.maxHp = enemy.hp = 300; // 200から変更しないこと
        PlayShotSound(sound_enemyShot_heavy);
        AddShotSet(A_XA, A_YA, ShotPat_StarCage, 0);
    }
    else {
        // ---- 本体の動き ----
        if (count < 900) {
            // Phase A: 上空をゆっくり漂う(幾何学の中心とは独立)
            enemy.x = 240.0 + sin(count * 0.020) * 40.0;
            enemy.y = 90.0;
        }
        else if (count < 2340) {
            // Phase B: 黄金の花の中心を小さく公転(追従移動で相境界も滑らかに)
            int tB = count - 900;
            double tx = A_XB + 30.0 * cos(tB * 2.0 * DX_PI / 90.0);
            double ty = A_YB + 30.0 * sin(tB * 2.0 * DX_PI / 90.0);
            enemy.x += (tx - enemy.x) * 0.10;
            enemy.y += (ty - enemy.y) * 0.10;
        }
        else {
            // Phase C: 薔薇曲線 r=130cos(5θ) を舞い、網完成後に網の中心へ戻る
            int tC = count - 2340;
            if (tC < 720) {
                double phi = tC * 0.5 * DX_PI / 180.0;
                double r = 130.0 * cos(5.0 * phi);
                double tx = A_XC + r * cos(phi);
                double ty = A_YC + r * sin(phi);
                enemy.x += (tx - enemy.x) * 0.20;
                enemy.y += (ty - enemy.y) * 0.20;
            }
            else {
                enemy.x += (A_XC - enemy.x) * 0.05;
                enemy.y += (A_YC - enemy.y) * 0.05;
            }
        }

        // ---- 相遷移 ----
        if (count == 900) {
            PlayShotSound(sound_enemyShot_medium);
            AddShotSet(A_XB, A_YB, ShotPat_GoldenBloom, 1);
        }
        if (count == 2340) {
            PlayShotSound(sound_enemyCharge); // 薔薇網の予告音
            AddShotSet(A_XC, A_YC, ShotPat_MaurerRose, 2);
        }
    }
}