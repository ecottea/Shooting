// enemyPat_SakuraComet.cpp
//
// 弾幕：桜と彗星
// - Boss 1 : マゼンタの桜弾幕
// - Boss 2 : シアンの彗星弾幕
// - 自機追従シールド : マゼンタ / シアンの小玉リング
// - 対応色の敵弾だけが同色シールドに触れると消滅する
//
// 使用素材
//   マゼンタ: img_enemyShotSmallBall[5], img_enemyShotMediumBall[5],
//             img_enemyShotScale[5], img_enemyShotDiamond[5]
//   シアン  : img_enemyShotSmallBall[3], img_enemyShotMediumBall[3],
//             img_enemyShotBullet[3], img_enemyShotMediumOval[3]
//   効果音  : sound_enemyCharge, sound_enemyShot_extreme
//
// 注意
//   count / pEnemyShotSet->count / pEnemyShot->count の更新はメインルーチン側。
//   画面外弾の消去もメインルーチン側。
//   shield 用小玉は margin=999.0 とし、画面外判定では消えない。

static constexpr double SHIELD_MARGIN = 999.0;
static constexpr double PI2 = DX_PI * 2.0;

// ------------------------------------------------------------
// 共通：敵弾をリスト末尾へ追加
// ------------------------------------------------------------
static void AddSakuraCometShot(
    sEnemyShotSet* pEnemyShotSet,
    double x,
    double y,
    double muki,
    double speed,
    int kind,
    int param_i0 = 0,
    double param_d0 = 0.0,
    double param_d1 = 0.0)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;

    pEnemyShot->x = x;
    pEnemyShot->y = y;
    pEnemyShot->muki = muki;
    pEnemyShot->speed = speed;
    pEnemyShot->kind = kind;
    pEnemyShot->param_i[0] = param_i0;
    pEnemyShot->param_d[0] = param_d0;
    pEnemyShot->param_d[1] = param_d1;

    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
}

// ------------------------------------------------------------
// 自機追従シールド
// kind : 0 = マゼンタ, 1 = シアン
//
// shield は毎フレーム、自機を中心とした輪として再配置する。
// 位置は count から直接求めるので累積誤差が出ない。
// ------------------------------------------------------------
static void ShotSakuraCometShield(sEnemyShotSet* pEnemyShotSet)
{
    const bool cyan = (pEnemyShotSet->kind != 0);
    const int shieldKind = cyan ? img_enemyShotSmallBall[3] : img_enemyShotSmallBall[5];

    // シールドは輪を構成する小玉群。
    // param_i[0] に shield 判定用フラグ、param_i[1] にインデックスを保存する。
    const int shieldCount = 16;
    const double radius = 42.0;
    const double rotation = pEnemyShotSet->count * 0.035;

    int index = 0;
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        if (pShot->param_i[0] == 1) {
            const double a = rotation + PI2 * index / shieldCount;
            pShot->x = player.x + cos(a) * radius;
            pShot->y = player.y + sin(a) * radius;
            pShot->kind = shieldKind;
            pShot->speed = 0.0;
            pShot->muki = a;
            pShot->margin = SHIELD_MARGIN;
            pShot->param_i[1] = cyan ? 1 : 0;
            ++index;
        }

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 対応色の弾だけをシールドで消す。
// 小玉同士も含め、弾は楕円として判定する。
// ------------------------------------------------------------
static bool IsSakuraCometEnemyShot(sEnemyShot* pShot, bool cyan)
{
    if (pShot->param_i[0] == 1) {
        return false;
    }

    // 画像インデックスを色判定に使う。
    const int colorIndex = cyan ? 3 : 5;

    const int kinds[] = {
        img_enemyShotSmallBall[colorIndex],
        img_enemyShotMediumBall[colorIndex],
        img_enemyShotBullet[colorIndex],
        img_enemyShotScale[colorIndex],
        img_enemyShotDiamond[colorIndex],
        img_enemyShotMediumOval[colorIndex]
    };

    for (int kind : kinds) {
        if (pShot->kind == kind) {
            return true;
        }
    }

    return false;
}

static void UpdateShieldCollision(sEnemyShotSet* pShieldSet, bool cyan)
{
    // まず shield の位置を最新化。
    ShotSakuraCometShield(pShieldSet);

    // shield 半径 : 小玉 2.5
    // 消去対象の弾は種類ごとの長径/短径を楕円半径として扱う。
    // 画面全体の全 EnemyShotSet を走査。
    sEnemyShotSet* pSet = enemyShotSetHead.next;
    while (pSet != &enemyShotSetHead) {
        sEnemyShotSet* pNextSet = pSet->next;

        sEnemyShot* pShot = pSet->pEnemyShotHead->next;
        while (pShot != pSet->pEnemyShotHead) {
            sEnemyShot* pNextShot = pShot->next;

            if (IsSakuraCometEnemyShot(pShot, cyan)) {
                double bulletLong = 2.5;
                double bulletShort = 2.5;

                const int colorIndex = cyan ? 3 : 5;

                if (pShot->kind == img_enemyShotMediumBall[colorIndex]) {
                    bulletLong = bulletShort = 7.0;
                }
                else if (pShot->kind == img_enemyShotLargeBall[colorIndex]) {
                    bulletLong = bulletShort = 20.0;
                }
                else if (pShot->kind == img_enemyShotBullet[colorIndex]) {
                    bulletLong = 5.0;
                    bulletShort = 2.0;
                }
                else if (pShot->kind == img_enemyShotScale[colorIndex]) {
                    bulletLong = 4.0;
                    bulletShort = 3.0;
                }
                else if (pShot->kind == img_enemyShotDiamond[colorIndex]) {
                    bulletLong = 4.5;
                    bulletShort = 2.5;
                }
                else if (pShot->kind == img_enemyShotMediumOval[colorIndex]) {
                    bulletLong = 10.5;
                    bulletShort = 7.0;
                }

                // shield の小玉との衝突を楕円判定で確認。
                sEnemyShot* pShieldShot = pShieldSet->pEnemyShotHead->next;
                while (pShieldShot != pShieldSet->pEnemyShotHead) {
                    if (pShieldShot->param_i[0] == 1) {
                        const double dx = pShot->x - pShieldShot->x;
                        const double dy = pShot->y - pShieldShot->y;

                        // 弾の向きを基準に座標を回転し、
                        // 長径・短径を持つ楕円として判定する。
                        const double c = cos(pShot->muki);
                        const double s = sin(pShot->muki);
                        const double localX = dx * c + dy * s;
                        const double localY = -dx * s + dy * c;

                        const double rx = bulletLong + 2.5 * 2;
                        const double ry = bulletShort + 2.5 * 2;

                        const double nx = localX / rx;
                        const double ny = localY / ry;

                        if (nx * nx + ny * ny <= 1.0) {
                            pShot->prev->next = pShot->next;
                            pShot->next->prev = pShot->prev;
                            delete pShot;
                            break;
                        }
                    }

                    pShieldShot = pShieldShot->next;
                }
            }

            pShot = pNextShot;
        }

        pSet = pNextSet;
    }
}

// ------------------------------------------------------------
// シールド生成
// ------------------------------------------------------------
static void CreateShieldSet()
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->kind = 0;
    pEnemyShotSet->patternFunc = ShotSakuraCometShield;

    pEnemyShotSet->x = player.x;
    pEnemyShotSet->y = player.y;

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    const int shieldCount = 16;
    for (int i = 0; i < shieldCount; ++i) {
        const double a = PI2 * i / shieldCount;

        sEnemyShot* pShot = new sEnemyShot;
        pShot->x = player.x + cos(a) * 42.0;
        pShot->y = player.y + sin(a) * 42.0;
        pShot->muki = a;
        pShot->speed = 0.0;
        pShot->kind = img_enemyShotSmallBall[5];
        pShot->margin = SHIELD_MARGIN;
        pShot->param_i[0] = 1; // shield
        pShot->param_i[1] = 0; // magenta

        pShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pShot;
        pEnemyShotSet->pEnemyShotHead->prev = pShot;
    }

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ------------------------------------------------------------
// ボス1：桜弾幕
//
// 花の中心から、花びらを思わせる湾曲した列を連続発射。
// 外周へ広がる放射と、遅れて現れる菱形弾を重ねる。
// ------------------------------------------------------------
static void ShotSakura(sEnemyShotSet* pEnemyShotSet)
{
    int t = pEnemyShotSet->count;
    const double baseX = pEnemyShotSet->x;
    const double baseY = pEnemyShotSet->y;

    if (t >= 60) t = -1;

    // 中心核から広がる桜色の花弁。
    if (t % 18 == 0) {
        const int petals = 12;
        const double spin = t * 0.055;

        for (int i = 0; i < petals; ++i) {
            const double a = spin + PI2 * i / petals;

            AddSakuraCometShot(
                pEnemyShotSet,
                baseX,
                baseY,
                a,
                1.75,
                img_enemyShotMediumBall[5],
                0
            );

            // 花弁の先端を作る補助弾。
            AddSakuraCometShot(
                pEnemyShotSet,
                baseX,
                baseY,
                a + 0.14,
                2.15,
                img_enemyShotScale[5],
                0
            );
            AddSakuraCometShot(
                pEnemyShotSet,
                baseX,
                baseY,
                a - 0.14,
                2.15,
                img_enemyShotScale[5],
                0
            );
        }
    }

    // 花びらが舞い散るような低速弾。
    if (t % 8 == 0) {
        const int side = (t / 8) & 1;
        const double sway = sin(t * 0.07) * 0.65;
        const double aim = atan2(player.y - baseY, player.x - baseX);

        for (int i = 0; i < 4; ++i) {
            const double a =
                aim
                + (i - 1.5) * 0.10
                + sway
                + (side ? 0.45 : -0.45);

            AddSakuraCometShot(
                pEnemyShotSet,
                baseX + cos(a) * 4.0,
                baseY + sin(a) * 4.0,
                a,
                2.60,
                img_enemyShotDiamond[5],
                0
            );
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// ボス2：彗星弾幕
//
// 狙い方向へ高速の核を飛ばし、その後ろを多数の尾が追従する。
// 尾は同じ色で曲がりながら広がり、横断する帯を形成する。
// ------------------------------------------------------------
static void ShotComet(sEnemyShotSet* pEnemyShotSet)
{
    int t = pEnemyShotSet->count;
    const double baseX = pEnemyShotSet->x;
    const double baseY = pEnemyShotSet->y;

    if (t >= 60) t = -1;

    // 彗星の核。
    if (t % 20 == 0) {
        const double aim = atan2(player.y - baseY, player.x - baseX);
        const double bend = sin(t * 0.09) * 0.35;

        AddSakuraCometShot(
            pEnemyShotSet,
            baseX,
            baseY,
            aim + bend,
            3.9,
            img_enemyShotMediumOval[3],
            0
        );

        // 長い尾。
        for (int i = 0; i < 8; ++i) {
            const double tailA = aim + bend + (i - 3.5) * 0.055;
            AddSakuraCometShot(
                pEnemyShotSet,
                baseX - cos(tailA) * (i + 1) * 7.0,
                baseY - sin(tailA) * (i + 1) * 7.0,
                tailA,
                2.25 + i * 0.035,
                img_enemyShotBullet[3],
                0
            );
        }
    }

    // 離れた場所から横切る高速流星。
    if (t % 32 == 0) {
        const int lane = (t / 32) % 5;
        const double y = 70.0 + lane * 85.0;
        const double direction = (lane & 1) ? DX_PI : 0.0;
        const double x = (lane & 1) ? 500.0 : -20.0;

        AddSakuraCometShot(
            pEnemyShotSet,
            x,
            y,
            direction,
            4.2,
            img_enemyShotBullet[3],
            0
        );

        for (int i = 0; i < 6; ++i) {
            AddSakuraCometShot(
                pEnemyShotSet,
                x - cos(direction) * i * 8.0,
                y - sin(direction) * i * 8.0 + (i - 2.5) * 1.8,
                direction,
                3.2,
                img_enemyShotScale[3],
                0
            );
        }
    }

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// ------------------------------------------------------------
// 2ボス共通の登録
// ------------------------------------------------------------
static void CreateBossShotSet(
    double x,
    double y,
    sEnemyShotSet::PatternFunc func)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = func;
    pEnemyShotSet->x = x;
    pEnemyShotSet->y = y;
    pEnemyShotSet->muki = atan2(player.y - y, player.x - x);

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// ------------------------------------------------------------
// 敵本体のパターン
// ------------------------------------------------------------
void EnemyPat_miComet_ChatGPT()
{
    static int muki = 1;

    if (count == 1) {
        enemy.x = 120.0;
        enemy.y = 55.0;
        enemy.x2 = 360.0;
        enemy.y2 = 55.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;

        // 色シールド生成。
        CreateShieldSet();

        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ボス移動。
    enemy.x += 0.75 * muki;
    enemy.x2 += 0.75 * muki;

    if (enemy.x < 85.0 || enemy.x2 > 395.0) {
        muki *= -1;
    }

    // --------------------------------------------------------
    // シールド色のタイミング
    //
    // 0秒  : charge
    // 1秒  : extreme + マゼンタ
    // 3秒  : charge
    // 4秒  : extreme + シアン
    // 以降 : 3秒おきに charge、その1秒後に色変更
    // --------------------------------------------------------
    if (count == 1 || (count >= 181 && count % 180 == 1)) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // 初回はマゼンタのまま、その後は3秒周期の途中で色を反転。
    if (count == 60 || (count >= 240 && count % 180 == 60)) {
        if (CheckSoundMem(sound_enemyShot_extreme)) StopSoundMem(sound_enemyShot_extreme);
        PlaySoundMem(sound_enemyShot_extreme, DX_PLAYTYPE_BACK);

        sEnemyShotSet* pSet = enemyShotSetHead.next;
        while (pSet != &enemyShotSetHead) {
            if (pSet->patternFunc == ShotSakuraCometShield) {
                // 初回は既にマゼンタ。以後はマゼンタ⇔シアンを反転する。
                if (count != 60) {
                    pSet->kind = (pSet->kind == 0) ? 1 : 0;
                }

                const int newKind =
                    (pSet->kind == 1)
                    ? img_enemyShotSmallBall[3]
                    : img_enemyShotSmallBall[5];

                sEnemyShot* pShot = pSet->pEnemyShotHead->next;
                while (pShot != pSet->pEnemyShotHead) {
                    if (pShot->param_i[0] == 1) {
                        pShot->kind = newKind;
                        pShot->margin = SHIELD_MARGIN;
                        pShot->param_i[1] = pSet->kind;
                    }
                    pShot = pShot->next;
                }
            }

            pSet = pSet->next;
        }
    }

    // --------------------------------------------------------
    // 桜（ボス1）
    // --------------------------------------------------------
    if (count % 60 == 30) {
        CreateBossShotSet(enemy.x, enemy.y + 12.0, ShotSakura);
    }

    // --------------------------------------------------------
    // 彗星（ボス2）
    // --------------------------------------------------------
    if (count % 60 == 30) {
        CreateBossShotSet(enemy.x2, enemy.y2 + 12.0, ShotComet);
    }

    // --------------------------------------------------------
    // 現在のシールド色を取得して対応弾を消す。
    // shield set は1つだけ生成している想定。
    // --------------------------------------------------------
    sEnemyShotSet* pShieldSet = enemyShotSetHead.next;
    while (pShieldSet != &enemyShotSetHead) {
        if (pShieldSet->patternFunc == ShotSakuraCometShield) {
            const bool cyan = (pShieldSet->kind != 0);
            UpdateShieldCollision(pShieldSet, cyan);
            break;
        }
        pShieldSet = pShieldSet->next;
    }
}
