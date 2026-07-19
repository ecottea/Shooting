// enemyPat_Tmp.cpp
//
// モチーフ: 双曲線 (Hyperbola)
//
// 【数学的背景】
//   直角双曲線  Y^2 - X^2 = v^2  の媒介変数表示:
//     X(t) = v * sinh(t),   vx = v * sinh(t)
//     Y(t) = v * cosh(t),   vy = v * cosh(t)
//
//   各弾の速度ベクトルを (vx, vy) = (v*sinh(t), v*cosh(t)) と設定すると、
//   T フレーム後の変位は (T*v*sinh(t), T*v*cosh(t)) となる。
//   これは恒等式  cosh^2 - sinh^2 = 1  より  (Tv)^2 * (cosh^2 - sinh^2) = (Tv)^2
//   すなわち Y^2 - X^2 = (Tv)^2 を満たし、パラメータ t が異なる弾群が
//   時刻 T における双曲線の一方の腕を描く。
//   vx の符号を逆転 (-sinh) すれば反対の腕が得られる。
//
// 【角度の分布】
//   t 小（頂点付近）: speed ≈ v（遅い）, angle ≈ π/2（軸方向）
//   t 大（漸近線付近）: speed ≈ v*sqrt(2)*e^t/2（速い）, angle → π/4（対角方向）
//   ⇒ 頂点側の弾はゆっくり軸方向に、漸近線側の弾は素早く斜めに広がる。
//
// 【弾幕構成】
//   ・右腕（シアン 鱗弾）: t ∈ [T_MIN, T_MAX] で sinh > 0  → 軸右側へ展開
//   ・左腕（マゼンタ 鱗弾）: t ∈ [T_MIN, T_MAX] で sinh < 0  → 軸左側へ展開
//   ・中心ギャップ（赤 中玉）: 両腕の間（約 ±16°）に狙い撃ち弾を配置
//   全体をプレイヤー方向 muki を軸として回転 (rot = muki - π/2) して発射。
//
// 【利用可能な弾種】
//   img_enemyShotSmallBall[color]   小玉
//   img_enemyShotMediumBall[color]  中玉
//   img_enemyShotLargeBall[color]   大玉
//   img_enemyShotBullet[color]      銃弾
//   img_enemyShotScale[color]       鱗弾 ← 使用（方向感のある形状）
//   img_enemyShotDiamond[color]     菱形弾
//   color: 0=赤 1=黄 2=緑 3=シアン 4=青 5=マゼンタ 6=白 7=黒
//
// 【仕様メモ】
//   ・count / pEnemyShotSet->count / pEnemyShot->count のインクリメントは
//     メインルーチン側で行われる。この関数では一切変更しない。
//   ・画面外弾の消去もメインルーチン側。
//   ・GetRand(x) は 0 以上 x 以下の整数を返す（このパターンでは未使用）。

#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// ──────────────────────────────────────────────────────────────
// 弾幕関数: ShotHyperbola
//   双曲線の両腕をシアン/マゼンタ鱗弾で展開し、
//   中心ギャップ（プレイヤー方向）を赤中玉で封じる。
// ──────────────────────────────────────────────────────────────
static void ShotHyperbola(sEnemyShotSet* pEnemyShotSet)
{
    const int    N_ARM = 10;   // 片腕の弾数
    const double BASE_V = 2.0;  // 頂点付近 (t→0) の速度 [px/frame]
    const double T_MIN = 0.30; // 双曲線パラメータ最小（頂点寄り・ギャップ端）
    const double T_MAX = 1.50; // 双曲線パラメータ最大（漸近線寄り）
    // T_MIN=0.30 で内側弾は約 ±16° オフセット（ギャップ半幅）
    // T_MAX=1.50 で外側弾は約 ±42° オフセット（漸近線方向）
    // 速度範囲: BASE_V*sqrt(cosh(0.6)) ≈ 2.18 〜 BASE_V*sqrt(cosh(3.0)) ≈ 6.34 [px/frame]

    sEnemyShot* pEnemyShot;

    if (pEnemyShotSet->count == 0) {
        // ── SE ─────────────────────────────────────────────
        if (CheckSoundMem(sound_enemyShot_heavy) == 1)
            StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);

        // プレイヤー方向 muki を双曲線軸にする回転角
        // 未回転座標系の軸方向は π/2（スクリーン下向き）なので
        // rot = muki - π/2 だけ回転すれば軸が muki に一致する
        double rot = pEnemyShotSet->muki - DX_PI / 2.0;

        // ── 双曲線の両腕 ────────────────────────────────────
        for (int branch = 0; branch < 2; branch++) {
            // branch 0: 右腕 (sinh > 0), branch 1: 左腕 (sinh < 0)
            double sign = (branch == 0) ? 1.0 : -1.0;

            for (int i = 0; i < N_ARM; i++) {
                // 双曲線パラメータ t: 均等分割
                double t = T_MIN + (T_MAX - T_MIN) * i / (N_ARM - 1);
                double sh = sinh(t);
                double ch = cosh(t);

                // 未回転速度ベクトル（軸方向 = 下向き, t→0 では純粋に下方向）
                //   vx_raw = BASE_V * sign * sinh(t)   （横方向・腕の開き方向）
                //   vy_raw = BASE_V * cosh(t)           （軸方向・スクリーン下）
                double vx_raw = BASE_V * sign * sh;
                double vy_raw = BASE_V * ch;

                // rot だけ 2D 回転してプレイヤー方向へ向ける
                double vx = vx_raw * cos(rot) - vy_raw * sin(rot);
                double vy = vx_raw * sin(rot) + vy_raw * cos(rot);

                // speed = |v| = BASE_V * sqrt(sinh^2 + cosh^2) = BASE_V * sqrt(cosh(2t))
                // （回転は大きさを変えない）
                double speed = sqrt(vx * vx + vy * vy);
                double angle = atan2(vy, vx);

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->x;
                pEnemyShot->y = pEnemyShotSet->y;
                pEnemyShot->speed = speed;
                pEnemyShot->muki = angle;
                pEnemyShot->kind = (branch == 0)
                    ? img_enemyShotScale[3]  // 右腕: シアン
                    : img_enemyShotScale[5]; // 左腕: マゼンタ

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }

        // ── 中心ギャップへの狙い撃ち弾（赤 中玉）───────────────
        // 両腕の間には頂点付近（t=T_MIN）の弾が作る約 ±16° のギャップがある。
        // この中心（= プレイヤー方向）に赤玉を置いてすり抜けを封じる。
        // 両腕の内縁の弾が約 ±16° オフセットなので、
        // 赤玉との間に残る隙間は片側約 16° 程度。
        pEnemyShot = new sEnemyShot;
        pEnemyShot->x = pEnemyShotSet->x;
        pEnemyShot->y = pEnemyShotSet->y;
        pEnemyShot->muki = pEnemyShotSet->muki; // プレイヤー方向へ直行
        pEnemyShot->speed = 2.5;
        pEnemyShot->kind = img_enemyShotLargeBall[0]; // 赤

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // ── 全弾: 直線移動 ──────────────────────────────────────
    // count==0 で生成した直後から移動（メインルーチンが count を +1 するより先に
    // この関数が呼ばれるため、生成フレームから即座に動き出す）
    pEnemyShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pEnemyShot != pEnemyShotSet->pEnemyShotHead) {
        pEnemyShot->x += pEnemyShot->speed * cos(pEnemyShot->muki);
        pEnemyShot->y += pEnemyShot->speed * sin(pEnemyShot->muki);
        pEnemyShot = pEnemyShot->next;
    }
}

// ──────────────────────────────────────────────────────────────
// 敵本体パターン: EnemyPat_Hyperbola_Claude
//   正弦波で画面上部を左右にゆったり移動しながら
//   40 フレームに 1 回、双曲線弾幕を発射する。
//
//   弾幕はプレイヤー方向を軸に毎回追尾するため、
//   敵の移動により軸方向が変化し弾道が変化する。
// ──────────────────────────────────────────────────────────────
void EnemyPat_Hyperbola_Claude()
{
    if (count == 1) {
        // ── 初期化 ────────────────────────────────────────
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
    }
    else {
        // ── 敵移動: 2 周期の正弦波リサージュ ─────────────
        // X: 周期 360 フレーム（約 6 秒）, 振幅 ±140px → X 範囲 [100, 380]
        // Y: 周期 240 フレーム（約 4 秒）, 振幅 ± 12px → Y 範囲 [ 38,  62]
        // 異なる周期の組み合わせで軌跡が一定周期で繰り返さず変化に富む
        enemy.x = 240.0 + sin(count * DX_PI / 180.0) * 140.0;
        enemy.y = 50.0 + sin(count * DX_PI / 120.0) * 12.0;
    }

    // ── 双曲線弾幕の発射: 40 フレームに 1 回 ─────────────
    // count%40==1 とすることで count==1（初回）から即発射し、
    // 以降は count==41, 81, 121, ... に発射する。
    if (count % 20 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHyperbola;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y,
            player.x - pEnemyShotSet->x);

        // センチネルノード（自己循環）で双方向循環リストを初期化
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        // グローバルリストの末尾に挿入
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}