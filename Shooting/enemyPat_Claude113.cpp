// enemyPat_SakuraComet.cpp
// D:/workspace/Visual_Cpp/Shooting/Shooting/enemyPat_SakuraComet.cpp
//
// ボス1（enemy.x, enemy.y）：桜モチーフ／マゼンタ弾
// ボス2（enemy.x2, enemy.y2）：彗星モチーフ／シアン弾
//
// 自機を取り囲む小玉シールドが、同色の敵弾に触れると消滅させる（斑鳩の吸収ギミック）。
// シールドの色はマゼンタ⇔シアンで3秒おきに切り替わる。
//   - t=0s, t=3s, t=6s, ... : sound_enemyCharge（予告音）
//   - t=1s, t=4s, t=7s, ... : sound_enemyShot_extreme と共にシールドの色が切り替わる
//     （最初はマゼンタ5個で出現し、以降マゼンタ⇔シアン(3個)を交互に繰り返す）
//
// img_enemyShotSmallBall[5] / img_enemyShotSmallBall[3] / sound_enemyCharge /
// sound_enemyShot_extreme は、このシールド演出以外では使用しない。
//
// 弾幕本体は「桜だけなら躱せる／彗星だけなら躱せる／両方同時だと躱しきれない」密度を狙い、
// count（各弾の経過フレーム数）のみから毎フレーム絶対位置を導出する数式駆動で実装している
// （速度の積算は行わない）。密度は1画面あたり数百発を目安にしてあるが、最終的な強度調整は
// プレイテストで詰めてほしいので、密度・速度系の定数は全てファイル先頭付近にまとめてある。
//
// img_enemyShotSmallBall / img_enemyShotMediumBall / img_enemyShotLargeBall /
// img_enemyShotBullet / img_enemyShotScale / img_enemyShotDiamond /
// img_enemyShotMediumOval / img_enemyShotLaser の各画像ハンドル配列、および
// sound_enemyShot_light / medium / heavy / extreme / sound_enemyCharge は
// enemyPat_sampleForAI.cpp と同様、既存のリソースヘッダで宣言済みという前提で使用する。

// ============================================================
//  色・形状の定義
// ============================================================

// 弾の色番号（プロジェクト全体の色定義に準拠。0:赤 1:黄 2:緑 3:シアン 4:青 5:マゼンタ 6:白 7:黒 8:橙）
static constexpr int kColorCyan = 3;
static constexpr int kColorMagenta = 5;

// 弾の形状（enum classでスコープを切り、短い名前でもWindows SDK等のマクロと衝突しないようにする）
enum class EShotShape { Small, Medium, Large, Bullet, Scale, Diamond, Oval, Laser, Max };

struct sShapeSize { double a, b; }; // a:長径（muki方向） b:短径

// 弾の種類と半径一覧（お題の仕様書どおり）
static const sShapeSize kShapeSize[static_cast<int>(EShotShape::Max)] = {
    { 2.5,  2.5 }, // Small
    { 7.0,  7.0 }, // Medium
    { 20.0, 20.0}, // Large
    { 5.0,  2.0 }, // Bullet
    { 4.0,  3.0 }, // Scale
    { 4.5,  2.5 }, // Diamond
    { 10.5, 7.0 }, // Oval
    { 64.0, 4.0 }, // Laser
};

static int ShapeToImg(EShotShape shape, int color)
{
    switch (shape) {
    case EShotShape::Small:  return img_enemyShotSmallBall[color];
    case EShotShape::Medium: return img_enemyShotMediumBall[color];
    case EShotShape::Large:  return img_enemyShotLargeBall[color];
    case EShotShape::Bullet: return img_enemyShotBullet[color];
    case EShotShape::Scale:  return img_enemyShotScale[color];
    case EShotShape::Diamond:return img_enemyShotDiamond[color];
    case EShotShape::Oval:   return img_enemyShotMediumOval[color];
    case EShotShape::Laser:  return img_enemyShotLaser[color];
    default:                 return img_enemyShotSmallBall[color];
    }
}

// param_i[] の用途（このファイル内だけのローカルな取り決め）
enum EParamIndex { PI_SHAPE = 0, PI_COLOR = 1 };

// 弾を1発生成して指定の弾セットへ連結する共通処理。
// このファイルの弾は全てcountからの数式で位置を導出するため、pShot->speedは使用しない（0.0固定）。
static sEnemyShot* AddShot(sEnemyShotSet* pSet, double x, double y, double muki, EShotShape shape, int color)
{
    sEnemyShot* pShot = new sEnemyShot;
    pShot->x = x;
    pShot->y = y;
    pShot->muki = muki;
    pShot->speed = 0.0;
    pShot->kind = ShapeToImg(shape, color);
    pShot->param_i[PI_SHAPE] = static_cast<int>(shape);
    pShot->param_i[PI_COLOR] = color;

    pShot->prev = pSet->pEnemyShotHead->prev;
    pShot->next = pSet->pEnemyShotHead;
    pSet->pEnemyShotHead->prev->next = pShot;
    pSet->pEnemyShotHead->prev = pShot;
    return pShot;
}

// 弾セットを1つ新規作成し、全体リストへ連結する共通処理
static sEnemyShotSet* CreateShotSet(double x, double y, double muki, sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* pSet = new sEnemyShotSet;
    pSet->count = 0;
    pSet->x = x;
    pSet->y = y;
    pSet->muki = muki;
    pSet->patternFunc = func;
    pSet->alive = 99999;

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
//  楕円弾（向きmukiに長径aを合わせた楕円） vs 円（半径r）の当たり判定
//  ※ 円側の半径ぶん楕円を膨らませた近似判定（斜め方向にわずかに寛容になるが、
//     シールドの当たり判定としては十分な精度）
// ============================================================
static bool CheckHitEllipseCircle(double ex, double ey, double muki, double a, double b, double cx, double cy, double r)
{
    double dx = cx - ex;
    double dy = cy - ey;
    double c = cos(-muki), s = sin(-muki);
    double lx = dx * c - dy * s;
    double ly = dx * s + dy * c;
    double aa = a + r;
    double bb = b + r;
    if (aa <= 0.0 || bb <= 0.0) return false;
    double nx = lx / aa;
    double ny = ly / bb;
    return (nx * nx + ny * ny) <= 1.0;
}

// ============================================================
//  桜弾幕（ボス1 / マゼンタ）
//  黄金角ずつ回転しながら等間隔で発射され続ける五弁花。
//  各花びらは「外側へ広がりながら回転し、ゆるやかに落ちていく」螺旋を
//  count（自弾の経過フレーム）だけから毎フレーム計算する（速度積算はしない）。
//  連続発射により、画面内には常に数百発規模の花びらが重なって存在する。
// ============================================================
static constexpr double kSakuraGoldenAngle = 2.399963; // 黄金角(rad)。少しずつ位相をずらして咲かせ続ける
static constexpr double kSakuraExpandRate = 0.6;      // 半径方向に広がる速さ
static constexpr double kSakuraSpiralRate = 0.03;     // 回転の速さ
static constexpr double kSakuraFallDrift = 0.1;      // 落下していく速さ
static constexpr int    kSakuraPetals = 5;        // 桜は五弁花
static constexpr int    kSakuraEmitInterval = 8;        // 発射間隔（フレーム）

static void ShotSakura(sEnemyShotSet* pSet)
{
    if (pSet->count % kSakuraEmitInterval == 0) {
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        int emissionIndex = pSet->count / kSakuraEmitInterval;
        double globalTheta = emissionIndex * kSakuraGoldenAngle;

        for (int i = 0; i < kSakuraPetals; i++) {
            double theta0 = globalTheta + DX_PI * 2.0 / kSakuraPetals * i;

            sEnemyShot* pShot = AddShot(pSet, pSet->x, pSet->y, theta0, EShotShape::Diamond, kColorMagenta);
            pShot->param_d[0] = 3.0 ;                          // r0（初期半径）
            pShot->param_d[1] = kSakuraExpandRate;     // expandRate
            pShot->param_d[2] = kSakuraSpiralRate * (GetRand(1) == 0 ? 1.0 : -1.0); // spiralRate（左右ランダム）
            pShot->param_d[3] = theta0;                                   // theta0
            pShot->param_d[4] = kSakuraFallDrift;                         // fallDrift
            pShot->param_d[5] = pSet->x;                                  // originX
            pShot->param_d[6] = pSet->y;                                  // originY
            pShot->margin = 480;
        }
    }

    for (sEnemyShot* pShot = pSet->pEnemyShotHead->next; pShot != pSet->pEnemyShotHead; pShot = pShot->next) {
        double t = pShot->count;
        double r0 = pShot->param_d[0];
        double expandRate = pShot->param_d[1];
        double spiralRate = pShot->param_d[2];
        double theta0 = pShot->param_d[3];
        double fallDrift = pShot->param_d[4];
        double originX = pShot->param_d[5];
        double originY = pShot->param_d[6];

        double r = r0 + expandRate * t;
        double theta = theta0 + spiralRate * t;

        pShot->x = originX + r * cos(theta);
        pShot->y = originY + r * sin(theta) + fallDrift * t;

        // 螺旋の接線方向を弾の向きとする
        double dx = expandRate * cos(theta) - r * spiralRate * sin(theta);
        double dy = expandRate * sin(theta) + r * spiralRate * cos(theta) + fallDrift;
        pShot->muki = atan2(dy, dx);
    }
}

// ============================================================
//  彗星弾幕（ボス2 / シアン）
//  自機を狙って弾をまとめ撃ちし続ける。同時発射の中で速度に差をつけることで、
//  噴射そのものが彗星の尾のように伸びる（先頭が大玉＝彗星核、後方が中玉＝尾）。
//  こちらもcountだけから毎フレーム位置を導出する（速度積算なし）。
// ============================================================
static constexpr int    kCometGroupSize = 3;   // 1回の噴射でまとめて撃つ弾数
static constexpr int    kCometEmitInterval = 4;   // 発射間隔（フレーム）
static constexpr double kCometSpeedBase = 1.8; // 最も遅い（尾側）弾の速さ
static constexpr double kCometSpeedRange = 1.6; // 速度差（最速の弾は base+range）
static constexpr double kCometSpread = 0.1; // まとめ撃ちの広がり（rad）
static constexpr double kCometWobbleAmp = 2.5; // 尾のきらめき（進行方向に垂直な揺れ幅）
static constexpr double kCometWobbleFreq = 0.12;// きらめきの速さ

static void ShotComet(sEnemyShotSet* pSet)
{
    if (pSet->count % kCometEmitInterval == 0) {
        if (pSet->count % (kCometEmitInterval * 3) == 0) {
            if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
            PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
        }

        double aimMuki = atan2(player.y - pSet->y, player.x - pSet->x);

        for (int i = 0; i < kCometGroupSize; i++) {
            double spreadRatio = i - (kCometGroupSize - 1) / 2.0;
            double muki0 = aimMuki + kCometSpread * spreadRatio;
            double speedRatio = (kCometGroupSize > 1) ? (double)i / (kCometGroupSize - 1) : 1.0;
            double speed = kCometSpeedBase + kCometSpeedRange * speedRatio;

            // 一番速い弾（先頭）だけ大玉＝彗星核、それ以外は中玉＝尾
            EShotShape shape = (i == kCometGroupSize - 1) ? EShotShape::Large : EShotShape::Medium;

            sEnemyShot* pShot = AddShot(pSet, pSet->x, pSet->y, muki0, shape, kColorCyan);
            pShot->param_d[0] = speed;
            pShot->param_d[1] = muki0;
            pShot->param_d[2] = kCometWobbleAmp;
            pShot->param_d[3] = kCometWobbleFreq;
            pShot->param_d[4] = GetRand(628) / 100.0; // wobblePhase（およそ0〜2π）
            pShot->param_d[5] = pSet->x;               // originX
            pShot->param_d[6] = pSet->y;               // originY
        }
    }

    for (sEnemyShot* pShot = pSet->pEnemyShotHead->next; pShot != pSet->pEnemyShotHead; pShot = pShot->next) {
        double t = pShot->count;
        double speed = pShot->param_d[0];
        double muki0 = pShot->param_d[1];
        double wobbleAmp = pShot->param_d[2];
        double wobbleFreq = pShot->param_d[3];
        double wobblePhase = pShot->param_d[4];
        double originX = pShot->param_d[5];
        double originY = pShot->param_d[6];

        double baseX = originX + speed * t * cos(muki0);
        double baseY = originY + speed * t * sin(muki0);
        double wobble = wobbleAmp * sin(wobbleFreq * t + wobblePhase);

        pShot->x = baseX + wobble * cos(muki0 + DX_PI / 2.0);
        pShot->y = baseY + wobble * sin(muki0 + DX_PI / 2.0);
        pShot->muki = muki0; // 全体としての進行方向を弾の向きとする
    }
}

// ============================================================
//  自機シールド（小玉が自機を取り囲んで追従し、同色の敵弾に触れると消滅させる）
// ============================================================
static constexpr double kShieldRadius = 42.0; // 自機からの距離
static constexpr double kShieldRotSpeed = 0.02; // 回転の速さ（rad/frame）

// シールド弾セットに現在乗っている全ての小玉を削除する
static void ClearShieldBalls(sEnemyShotSet* pSet)
{
    sEnemyShot* pShot = pSet->pEnemyShotHead->next;
    while (pShot != pSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next;
        pShot->prev->next = pShot->next;
        pShot->next->prev = pShot->prev;
        delete pShot;
        pShot = pNext;
    }
    pSet->param_i[1] = 0; // ballCount
}

// シールドを指定した色・個数で（既存の玉を消してから）生成し直す
static void SpawnShieldBalls(sEnemyShotSet* pSet, int color, int ballCount)
{
    ClearShieldBalls(pSet);
    pSet->param_i[0] = color;     // shieldColor
    pSet->param_i[1] = ballCount; // ballCount

    for (int i = 0; i < ballCount; i++) {
        double angle = DX_PI * 2.0 / ballCount * i;
        double x = player.x + kShieldRadius * cos(angle);
        double y = player.y + kShieldRadius * sin(angle);
        sEnemyShot* pShot = AddShot(pSet, x, y, angle, EShotShape::Small, color);
        pShot->margin = 999.0; // 画面外判定で消去されないようにする
    }
}

static void ShotPlayerShield(sEnemyShotSet* pSet)
{
    int ballCount = pSet->param_i[1];
    if (ballCount <= 0) return; // まだ玉が出ていない

    int shieldColor = pSet->param_i[0];
    double baseAngle = pSet->count * kShieldRotSpeed;

    // 自機を中心に等間隔で追従（毎フレーム回転しながら位置を再計算）
    int i = 0;
    for (sEnemyShot* pShot = pSet->pEnemyShotHead->next; pShot != pSet->pEnemyShotHead; pShot = pShot->next, i++) {
        double angle = baseAngle + DX_PI * 2.0 / ballCount * i;
        pShot->x = player.x + kShieldRadius * cos(angle);
        pShot->y = player.y + kShieldRadius * sin(angle);
        pShot->muki = angle; // 外向きを弾の向きとする
    }

    double shieldBallRadius = kShapeSize[static_cast<int>(EShotShape::Small)].a * 2; // 2.5

    // 全ての敵弾セットを走査し、シールドと同色の敵弾を消していく
    for (sEnemyShotSet* pOtherSet = enemyShotSetHead.next; pOtherSet != &enemyShotSetHead; pOtherSet = pOtherSet->next) {
        if (pOtherSet == pSet) continue;

        sEnemyShot* pOther = pOtherSet->pEnemyShotHead->next;
        while (pOther != pOtherSet->pEnemyShotHead) {
            sEnemyShot* pNext = pOther->next; // 削除後も辿れるよう先に控えておく

            if (pOther->param_i[PI_COLOR] == shieldColor) {
                int shapeIndex = pOther->param_i[PI_SHAPE];
                double a = kShapeSize[shapeIndex].a;
                double b = kShapeSize[shapeIndex].b;

                bool hit = false;
                for (sEnemyShot* pShield = pSet->pEnemyShotHead->next; pShield != pSet->pEnemyShotHead; pShield = pShield->next) {
                    if (CheckHitEllipseCircle(pOther->x, pOther->y, pOther->muki, a, b, pShield->x, pShield->y, shieldBallRadius)) {
                        hit = true;
                        break;
                    }
                }

                if (hit) {
                    pOther->prev->next = pOther->next;
                    pOther->next->prev = pOther->prev;
                    delete pOther;
                }
            }

            pOther = pNext;
        }
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_miComet_Claude()
{
    static sEnemyShotSet* pSakuraSet = nullptr;
    static sEnemyShotSet* pCometSet = nullptr;
    // シールドの弾セットは、最初の玉が出る1秒後（=フレーム61）まで生成しない。
    // 空の弾セットをフレームをまたいで放置しない設計にすることで、
    // 「フェーズ境界で空セットが誤って片付けられる」問題を回避している。
    static sEnemyShotSet* pShieldSet = nullptr;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 120.0;  // ボス1（桜・マゼンタ）
        enemy.y = 60.0;
        enemy.x2 = 360.0; // ボス2（彗星・シアン）
        enemy.y2 = 60.0;
        enemy.maxHp = enemy.hp = 200; // 2体で共用

        pSakuraSet = CreateShotSet(enemy.x, enemy.y, 0.0, ShotSakura);
        pCometSet = CreateShotSet(enemy.x2, enemy.y2, 0.0, ShotComet);
        pShieldSet = nullptr;
    }

    // ボス1（桜）は木がそよぐようにその場で小さく揺れる
    enemy.x = 120.0 + sin(count * 0.006) * 25.0;
    enemy.y = 80.0 + sin(count * 0.012) * 10.0;

    // ボス2（彗星）は空を横切るように大きく弧を描いて移動する
    enemy.x2 = 300.0 + sin(count * 0.008) * 55.0;
    enemy.y2 = 80.0 + sin(count * 0.016) * 20.0;

    pSakuraSet->x = enemy.x;
    pSakuraSet->y = enemy.y;
    pCometSet->x = enemy.x2;
    pCometSet->y = enemy.y2;

    // ---- シールドの出現・色変化スケジュール ----
    // 0秒後、3秒後、6秒後、…：予告音（sound_enemyCharge）
    if ((count - 1) % 180 == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 1秒後、4秒後、7秒後、…：sound_enemyShot_extreme と共にシールドが出現／色変化
    if (count >= 61 && (count - 61) % 180 == 0) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        if (pShieldSet == nullptr) {
            pShieldSet = CreateShotSet(player.x, player.y, 0.0, ShotPlayerShield);
        }

        bool spawnMagenta = (pShieldSet->param_i[1] == 0) || (pShieldSet->param_i[0] == kColorCyan);
        if (spawnMagenta) {
            SpawnShieldBalls(pShieldSet, kColorMagenta, 25);
        }
        else {
            SpawnShieldBalls(pShieldSet, kColorCyan, 25);
        }
    }
}