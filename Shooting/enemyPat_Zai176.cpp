// ============================================================
//  弾幕パターン「線香花火」
//  敵本体: void EnemyPat_Sparkler_Zai()
// ------------------------------------------------------------
//  【素材の選定と理由】
//   ・大玉(20x20)      … 火球本体 / ぽろりと落ちる火の玉。
//                          ぼんやりと光る塊の表現に最も近い大きさ。
//   ・菱形弾(4.5x2.5)  … 松葉。細く鋭く突き出す火花の形に合う。
//   ・小玉(2.5x2.5)    … 菊の火の粉 / 破裂の火花。粒立ちの細かい飛沫。
//   ・中玉(7.0x7.0)    … 菊で 10 発に 1 発混ぜる「大粒の火の粉」。
//   ・銃弾 / 鱗弾 / 中楕円弾 / 短レーザー … 和風の火花に合わないため不使用。
//   ・色は炎の色に限定: 0:赤 / 1:黄 / 6:白 / 7:黒 / 8:橙。
//     緑・シアン・青・マゼンタは炎の色ではないため使わない。
//   ・音: sound_enemyCharge  = 着火と火の玉落下の予告音
//         sound_enemyShot_light   = 火の玉の着地破裂
//         sound_enemyShot_medium  = 松葉フェーズの始まり
//         sound_enemyShot_heavy   = 菊フェーズの始まり
//         sound_enemyShot_extreme = 不使用(静かで儚い花火なので大音量は合わない)
//
//  【仕様】
//   ・count / pEnemyShotSet->count / pEnemyShot->count のインクリメントと、
//     画面外に出た弾の消去はメインルーチンが行うため、ここでは行わない
//     (各 count は読み取りにのみ使用)。
//   ・寿命が来た弾は「燃え尽きた火花がポトリと落ちる」演出で画面外へ押し出し、
//     消去はメインルーチンに一任する(delete / リストからの削除は行わない)。
//   ・GetRand(x) は 0 以上 x 以下の x+1 種類の整数を返す。
//     すべての乱数はこの範囲で設計している(各箇所にコメント記載)。
//   ・弾の移動は patternFunc 内で行う(サンプル ShotScatter と同じ作法)。
//
//  【弾幕の構成: 1 サイクル = 1560 フレーム(約 26 秒)をループ】
//     t =     0 -  179 : 芽出し … 小玉がポツポツと飛び散る
//     t =   180 -  479 : 松葉   … 菱形弾が放射状に鋭く突き出る
//     t =   480 - 1139 : 菊     … 火の粉が全周に滲み、重力で降り積もる(主体)
//     t =  1140 - 1499 : 散り   … 火の玉がぽろり落ち、画面下端で「ぱん」と弾ける
//     t =  1500 - 1559 : 静寂   … 火球が黒い燃えカスになり、間(ま)を置いて再点火
//   敵本体は線香が燃え落ちるようにゆっくり下降しながら左右に揺れ、
//   サイクルの終わりでまた上まで戻る(新しい線香花火に着火するイメージ)。
// ============================================================

#include "gv.h"
#include <cmath>

// DX_PI が未定義の環境のための保険
#ifndef DX_PI
#define DX_PI 3.1415926535897932384626433832795
#endif

// ------------------------------------------------------------
//  定数
// ------------------------------------------------------------
static const int    CYCLE = 1560-200;  // 1 サイクルのフレーム数
static const int    PH_MATSUBA = 180;   // 松葉フェーズ開始
static const int    PH_KIKU = 480;   // 菊フェーズ開始
static const int    PH_CHIRI = 1140-200;  // 散りフェーズ開始
static const int    PH_QUIET = 1500-200;  // 静寂フェーズ開始

static const double GRAVITY = 0.04;  // 火の粉・火の玉が受ける重力(毎フレーム)
static const double HIBANA_MAX = 2.8;   // 火の粉の終端速度(降る雨の速さの上限)

// ------------------------------------------------------------
//  弾の役割(param_i[0] に格納)
// ------------------------------------------------------------
enum eShotRole {
    ROLE_HIKYU = 0,  // 火球(常駐・敵に追従する静止弾)
    ROLE_MATSUBA = 1,  // 松葉/芽出しの火花(寿命が来ると燃えカスとして落下)
    ROLE_HIBANA = 2,  // 菊の火の粉(重力で降り積もる)
    ROLE_OTCHI = 3,  // ぽろりと落ちる火の玉(画面下端で破裂)
    ROLE_HARETSU = 4,  // 破裂して散る火花(直線移動のみ・画面外で自然消滅)
};

// ------------------------------------------------------------
//  効果音の共通再生処理(サンプルと同じ作法: 二重再生を防ぐ)
// ------------------------------------------------------------
static void PlayShotSound(int handle)
{
    if (CheckSoundMem(handle)) StopSoundMem(handle);
    PlaySoundMem(handle, DX_PLAYTYPE_BACK);
}

// ------------------------------------------------------------
//  弾を 1 個生成して ShotSet のリストの末尾につなぐ共通処理
// ------------------------------------------------------------
static sEnemyShot* CreateShot(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot = new sEnemyShot;
    pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
    pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    return pEnemyShot;
}

// ============================================================
//  ShotSet 側: 全フェーズの弾の発射と全弾の物理を担う
//  (ShotSet はサイクルを通して 1 個のみ。フェーズは count で判定)
// ============================================================
static void ShotSenkoHanabi(sEnemyShotSet* pEnemyShotSet)
{
    // 発射位置は常に敵(火球)に追従させる
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 8.0;

    const int t = pEnemyShotSet->count % CYCLE; // サイクル内の経過フレーム

    // サイクルの境目(静寂 → 芽出し)を検知したら再着火音
    // param_i[15]: 前回フレームの t を記録しておく窓
    if (pEnemyShotSet->param_i[15] >= PH_QUIET && t < PH_MATSUBA) {
        PlayShotSound(sound_enemyCharge);
    }
    pEnemyShotSet->param_i[15] = t;

    // =========================================================
    //  1) フェーズごとの発射処理
    // =========================================================
    if (t < PH_MATSUBA) {
        // ---- 芽出し: 30フレームごとに 4〜7 個の小玉がポツポツ ----
        if (t % 30 == 0) {
            const int n = 4 + GetRand(3); // GetRand(3)=0〜3 の 4 種類 → 4〜7 個
            for (int i = 0; i < n*2; i++) {
                sEnemyShot* pEnemyShot = CreateShot(pEnemyShotSet);
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->muki = GetRand(360) * DX_PI / 180.0;  // 全周ランダム(0〜360 度)
                pEnemyShot->speed = 1.0 + GetRand(100) / 100.0;    // 1.0〜2.0
                pEnemyShot->kind = img_enemyShotSmallBall[1];     // 黄
                pEnemyShot->param_i[0] = ROLE_MATSUBA;
                pEnemyShot->param_i[1] = 60 + GetRand(30);         // 寿命 60〜89 フレーム
            }
        }
    }
    else if (t < PH_KIKU) {
        // ---- 松葉 ----
        if (t == PH_MATSUBA) PlayShotSound(sound_enemyShot_medium);

        // 20フレームごとに 12 方向の菱形弾が鋭く突き出る
        if (t % 20 == 0) {
            const double base = GetRand(360) * DX_PI / 180.0;      // 放射の開始角(0〜360 度)
            for (int i = 0; i < 12*2; i++) {
                sEnemyShot* pEnemyShot = CreateShot(pEnemyShotSet);
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                // 等間隔 12 方向 + 揺らぎ(GetRand(10)-5 → -0.05〜+0.05 ラジアン)
                pEnemyShot->muki = base + i * (2.0 * DX_PI / 12.0/2) + (GetRand(10) - 5) / 100.0;
                pEnemyShot->speed = 4.0 + GetRand(200) / 100.0;    // 4.0〜6.0
                pEnemyShot->kind = img_enemyShotDiamond[GetRand(1) == 0 ? 1 : 6]; // 黄/白
                pEnemyShot->param_i[0] = ROLE_MATSUBA;
                pEnemyShot->param_i[1] = 50 + GetRand(30);         // 寿命 50〜79 フレーム(飛距離≒200〜470px)
            }
        }
    }
    else if (t < PH_CHIRI) {
        // ---- 菊(メイン) ----
        if (t == PH_KIKU) PlayShotSound(sound_enemyShot_heavy);

        // 3フレームごとに火の粉が全周に滲み、重力で降り積もる
        if (t % 3 == 0) {
            const int n = 1 + GetRand(1); // GetRand(1)=0〜1 の 2 種類 → 1〜2 個
            for (int i = 0; i < n; i++) {
                const double m = GetRand(360) * DX_PI / 180.0;    // 全周ランダム(0〜360 度)
                const double sp = 0.5 + GetRand(100) / 100.0;      // 0.5〜1.5(ほぼその場に滲む)

                sEnemyShot* pEnemyShot = CreateShot(pEnemyShotSet);
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                // 重力は速度ベクトル(param_d)で管理し、speed / muki にも常に反映させる
                pEnemyShot->param_d[0] = sp * cos(m);              // vx
                pEnemyShot->param_d[1] = sp * sin(m);              // vy
                pEnemyShot->speed = sp;
                pEnemyShot->muki = m;
                pEnemyShot->param_i[0] = ROLE_HIBANA;

                if (GetRand(9) == 0) {                             // GetRand(9)=0〜9 → 1/10 の確率
                    // 大粒の火の粉(「ぱちっ」と弾ける大きめの粒)
                    pEnemyShot->kind = img_enemyShotMediumBall[8]; // 橙
                }
                else {
                    const int c = GetRand(3);                      // GetRand(3)=0〜3 → 0:黄 / 1:橙 / 2:赤 / 3:黄
                    pEnemyShot->kind = img_enemyShotSmallBall[(c == 1) ? 8 : (c == 2) ? 0 : 1];
                }
            }
        }
    }
    else if (t < PH_QUIET) {
        // ---- 散り: 150フレームごとに火の玉がぽろりと落ちる ----
        if (t % 150 == 0) {
            sEnemyShot* pEnemyShot = CreateShot(pEnemyShotSet);
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->muki = DX_PI / 2.0;                       // 真下へ
            pEnemyShot->speed = 0.5;
            pEnemyShot->param_d[1] = 0.5;                          // vy(以後、重力で加速)
            pEnemyShot->kind = img_enemyShotLargeBall[0];         // 赤
            pEnemyShot->param_i[0] = ROLE_OTCHI;
            PlayShotSound(sound_enemyCharge);                      // 「ぽろり」の予告音
        }
    }
    else if (t == PH_QUIET) {
        // ---- 静寂: 最後の 1 粒だけ白い火の玉を落として火は尽きる ----
        sEnemyShot* pEnemyShot = CreateShot(pEnemyShotSet);
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = DX_PI / 2.0;
        pEnemyShot->speed = 0.5;
        pEnemyShot->param_d[1] = 0.5;
        pEnemyShot->kind = img_enemyShotLargeBall[6];             // 白
        pEnemyShot->param_i[0] = ROLE_OTCHI;
        PlayShotSound(sound_enemyCharge);
    }
    // t > PH_QUIET は発射なし(静寂の間)。火球が黒い燃えカスになるのは下の全弾処理側。

    // =========================================================
    //  2) 全弾の挙動処理と移動
    // =========================================================
    bool hikyuExists = false;

    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        switch (pShot->param_i[0]) {
        case ROLE_HIKYU:
            // 火球: 敵に追従し、8フレーム周期で橙⇔黄と明滅。静寂期は黒い燃えカスに
            hikyuExists = true;
            pShot->x = pEnemyShotSet->x;
            pShot->y = pEnemyShotSet->y;
            pShot->kind = (t >= PH_QUIET)
                ? img_enemyShotLargeBall[7]                        // 黒(燃えカス)
                : (((pEnemyShotSet->count / 8) % 2 == 0)
                    ? img_enemyShotLargeBall[8]                    // 橙
                    : img_enemyShotLargeBall[1]);                  // 黄
            break;

        case ROLE_MATSUBA:
            // 火花: 寿命(pShot->count はメインルーチンが毎フレーム +1 してくれる)を
            // 迎えたら燃え尽きて、ポトリと燃えカスが落ちる。
            // そのまま画面外まで落下し、消去はメインルーチンに委ねる。
            if (pShot->count >= pShot->param_i[1]) {
                pShot->muki = DX_PI / 2.0;
                pShot->speed = 3.0;
            }
            break;

        case ROLE_HIBANA: {
            // 火の粉: 重力で引かれながら降り積もる(終端速度で頭打ち → 静かな火の粉の雨)
            double vx = pShot->param_d[0];
            double vy = pShot->param_d[1] + GRAVITY;
            if (vy > HIBANA_MAX) vy = HIBANA_MAX;
            pShot->param_d[0] = vx;
            pShot->param_d[1] = vy;
            pShot->speed = sqrt(vx * vx + vy * vy);
            pShot->muki = atan2(vy, vx);
            break;
        }

        case ROLE_OTCHI: {
            // 火の玉: 重力で加速して落ち、画面下端で「ぱんっ」と破裂する
            const double vy = pShot->param_d[1] + GRAVITY;
            pShot->param_d[1] = vy;
            pShot->speed = vy;
            pShot->muki = DX_PI / 2.0;

            if (pShot->y >= 460.0) {
                // 破裂: 8 方向の小玉(赤/橙交互)
                for (int i = 0; i < 8; i++) {
                    sEnemyShot* pNew = CreateShot(pEnemyShotSet);
                    pNew->x = pShot->x;
                    pNew->y = pShot->y;
                    pNew->muki = i * DX_PI / 4.0;
                    pNew->speed = 2.5;
                    pNew->kind = img_enemyShotSmallBall[(i % 2 == 0) ? 0 : 8];
                    pNew->param_i[0] = ROLE_HARETSU;
                }
                PlayShotSound(sound_enemyShot_light);

                // 本体は役目を終える: 位置を画面外へ出し、消去はメインルーチンに委ねる
                // (speed = 0 なので以後動かない)
                pShot->x = 1000.0;
                pShot->y = 1000.0;
                pShot->speed = 0.0;
            }
            break;
        }

        case ROLE_HARETSU:
        default:
            // 破裂した火花: 直線移動のみで画面外へ抜けていく(消去はメインルーチン)
            break;
        }

        // 共通の移動処理(火球は speed = 0 のため動かない)
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }

    // =========================================================
    //  3) 火球が存在しなければ生成(初回フレームでの着火)
    //  ※ メインルーチンの count の増分タイミングに依存しないよう、
    //    「火球がまだ居ない」ことをもって着火判定にしている
    // =========================================================
    if (!hikyuExists) {
        sEnemyShot* pEnemyShot = CreateShot(pEnemyShotSet);
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = 0.0;
        pEnemyShot->speed = 0.0;                                   // 静止弾(位置は毎フレーム直接代入)
        pEnemyShot->kind = img_enemyShotLargeBall[8];             // 橙
        pEnemyShot->param_i[0] = ROLE_HIKYU;
        PlayShotSound(sound_enemyCharge);                          // 着火音
    }
}

// ============================================================
//  敵本体のパターン
// ============================================================
void EnemyPat_Sparkler_Zai()
{
    static int startCount;

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 60.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        startCount = count;

        // 線香花火用の ShotSet を 1 個だけ生成(以後すべてのフェーズをこれで賄う)
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSenkoHanabi;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 8.0;
        pEnemyShotSet->muki = 0.0;
        pEnemyShotSet->kind = 0;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
    else {
        const int t = (count - startCount) % CYCLE; // 自分のサイクル内フレーム

        // 線香花火を差し上げているように、左右にゆらゆら揺れる
        // 揺れの周期は 312 フレーム = CYCLE / 5 なので、サイクル境界でも位置が連続する
        // 静寂期には揺れを収束させ、次サイクル(再点火)へ滑らかにつなぐ
        double sway = 1.0;
        if (t > PH_QUIET) sway = 1.0 - (double)(t - PH_QUIET) / (double)(CYCLE - PH_QUIET);

        enemy.x = 240.0 + sin(2.0 * DX_PI * t / 312.0) * 50.0 * sway;

        // 線香が燃え落ちるように、菊フェーズの終わりまでゆっくり下降する
        if (t <= PH_CHIRI) {
            enemy.y = 60.0 + 0.03 * t;                             // 60.0 → 94.2 へ燃え落ちる
        }
        else if (t <= PH_QUIET) {
            enemy.y = 60.0 + 0.03 * PH_CHIRI;                      // 静かに燃え尽きる(94.2 で停留)
        }
        else {
            // 静寂期に 1 秒かけて元の高さへ戻る(新しい線香花火に着替えるイメージ)
            enemy.y = 60.0 + 0.03 * PH_CHIRI
                - 0.03 * PH_CHIRI * (double)(t - PH_QUIET) / (double)(CYCLE - PH_QUIET);
        }
    }
}
