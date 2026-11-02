#include "DxLib.h"
#include "gv.h"
#include "imgSoundLoad.h"
#include <math.h>

// =============================================================================
//  弾幕パターン:ヘリンボーン・ウィーブ(矢羽根弾幕)
// -----------------------------------------------------------------------------
//  画面を縦の帯(列)に分割し、各列に短い斜線(=ステッチ)を織り下ろして
//  画面幅いっぱいのヘリンボーン模様を作る。
//
//   ＼　／　＼　／　＼　…   偶数列=「＼」/ 奇数列=「／」で向きを反転
//   　　＼　／　＼　／      奇数列は半ピッチ遅れて発射(端と端を半分ずつ噛み合わせる)
//   ＼　／　＼　／　＼      全弾が同速度で真下に落ちるので、模様は剛体の布のように流れる
//
//  実装上の注意:
//   - 模様の規則性が生命線なので GetRand はあえて一切使わない
//     (発射位置や位相をランダムに揺らすと噛み合わせが崩れて模様が消えるため)
//   - 織り台は画面固定。セットのx/yには敵位置を記録するが、発射には使わない
//   - count / pEnemyShotSet->count / pEnemyShot->count の加算と、画面外に
//     出た弾の消去はメインルーチンが行う仕様。このファイルでは一切触らない
//   - 発射が終わって空になったセットはメインルーチンが破棄する前提(サンプルと同じ)
//
//  使用素材(使える素材一覧から抜粋):
//   - 小弾(2.5x2.5)    斜線の本体。細い「糸目」の見た目 + 高密度でも判定が優しい
//   - 中玉(7.0x7.0)    各斜線の先端1発。縫い目の「玉止め」兼、斜線の先端予告
//   - 0:赤 / 4:青      列ごとに交互に塗り分けて織りの縦糸/横糸を表現
//   - sound_enemyCharge     パス開始の予告音
//   - sound_enemyShot_light 織りのビート音(medium以上は布の質感に重いので不採用)
//
//  難度調整の目安(下の定数を変更):
//   - Easy : STROKE_N=3 / COL_W=56 / 敵本体の速度上限を2.0で固定
//   - Hard : STROKE_N=5 / COL_W=40 / STEP=14 / 速度上限を2.6に
//   - 注意: (STROKE_N-1)*STEP*0.7 <= COL_W を満たさないと斜線が隣列に食い込む
// =============================================================================

// 弾幕:ヘリンボーン・ウィーブ本体
static void ShotHerringboneWeave(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // ---- 設計定数(難度調整はここ) -------------------------------------------
    const double COL_W = 48.0;                                 // 列の幅
    const int    N_COL = (int)(480.0 / COL_W);                 // 列数(=10)
    const int    STROKE_N = 4;                                    // 斜線1本の弾数
    const double STEP = 18.0;                                 // 斜線上の弾の間隔
    const double ANGLE = DX_PI / 4.0;                          // 斜線の角度(±45°)
    const double DXS = STEP * cos(ANGLE);                    // 弾1個の横ずれ(約12.7)
    const double DYS = STEP * sin(ANGLE);                    // 弾1個の縦ずれ(約12.7)
    const double INSET = (COL_W - (STROKE_N - 1) * DXS) / 2.0; // 列内の左右の余白
    const double PITCH = 96.0;                                 // 同じ列の隣の斜線までの縦ピッチ
    const double SPAWN_Y = -12.0;                                // 発射位置(上端の外・消去マージン20の内側)
    const int    START = 60;                                   // セット生成→発射開始までの猶予
    const int    SPAN = 384;                                  // 発射を続けるフレーム数
    const double SPEED = (pEnemyShotSet->param_d[0] > 0.0) ? pEnemyShotSet->param_d[0] : 2.0; // 落下速度(px/フレーム)
    // 同じ列で次の斜線を撃つ間隔(フレーム)。「間隔×速度=PITCH」なので、
    // どのフレームから発射を始めても斜線の縦の目が常に PITCH ずつ整う
    const int    EMIT_IV = (int)(PITCH / SPEED);

    const int t = pEnemyShotSet->count - START;

    // セット生成フレーム:予告音
    if (pEnemyShotSet->count == 0) {
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
    }

    // ---- 発射 -----------------------------------------------------------------
    if (t >= 0 && t < SPAN) {

        // 織りのビート音(半ピッチごと=シャトルが端に着くイメージ。不要ならこのブロックを削除)
        if (t % (EMIT_IV / 2) == 0) {
            if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
            PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);
        }

        for (int i = 0; i < N_COL; i++) {
            // 奇数列は半ピッチ遅らせて発射(噛み合わせ用)
            if ((t + (i % 2) * (EMIT_IV / 2)) % EMIT_IV != 0) continue;

            // 列ごとに斜線の向きを反転(＼と／が交互)。パス番号(kind)の偶奇で全体を左右反転
            const int    dir = ((i + pEnemyShotSet->kind) % 2 == 0) ? +1 : -1;
            const double base_x = (double)i * COL_W;
            const int    color = (i % 2 == 0) ? 0 : 4; // 0:赤 / 4:青

            for (int k = 0; k < STROKE_N; k++) {
                pEnemyShot = new sEnemyShot;
                if (dir > 0) pEnemyShot->x = base_x + INSET + k * DXS;         // 「＼」は左上から
                else         pEnemyShot->x = base_x + COL_W - INSET - k * DXS; // 「／」は右上から
                pEnemyShot->y = SPAWN_Y + k * DYS;
                pEnemyShot->muki = DX_PI / 2.0; // 全弾まっすぐ下へ(模様が崩れず剛体で流れる)
                pEnemyShot->speed = SPEED;
                // 斜線の先端(最も下)だけ中玉で「玉止め」
                if (k == STROKE_N - 1) pEnemyShot->kind = img_enemyShotMediumBall[color];
                else                   pEnemyShot->kind = img_enemyShotSmallBall[color];

                // 連結リストの末尾に追加(サンプルと同じ手順)
                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    // ---- 移動(count加算と画面外消去はメインルーチン担当) ----------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// =============================================================================
//  敵本体:織機のシャトルのように画面上部を左右に往復し、12秒ごとに
//  1往復ぶんの「織りのパス」を画面全体に仕掛ける。パスが替わるたびに
//  斜線の向きが反転して谷の位置が移動する。パス間には約1秒の小休止がある。
// =============================================================================
void EnemyPat_Herringbone_Zai()
{
    static int muki;       // シャトルの往復方向
    static int shot_count; // パス番号(偶数=「＼」開始/奇数=「／」開始)

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 40.0;
        enemy.maxHp = enemy.hp = 200; // 200から変更しないこと
        muki = 1;
        shot_count = 0;
    }
    else {
        // シャトルの往復(x は約179〜299を行き来する)
        enemy.x += 0.8 * (double)muki;
        if (count % 150 == 75) muki *= -1;
    }

    // 12秒(720フレーム)ごとに新しい織りのパスを開始
    // 発射期間(60+384フレーム)と最終弾が流れ切る時間(約256フレーム)を
    // 足し合わせると約700フレームなので、前のパスの弾が消えてから
    // 次のパスが始まる(約1秒の小休止=陣形を立て直す時間)設計
    if (count % 222 == 1) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotHerringboneWeave;
        // 織り台は画面固定のため、発射にセットのx/yは使わない(記録として残すのみ)
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 10.0;
        pEnemyShotSet->muki = DX_PI / 2.0;
        // 落下速度:パスごとに0.08ずつ上げる(上限2.4)
        pEnemyShotSet->param_d[0] = 2.0 + 0.08 * shot_count;
        if (pEnemyShotSet->param_d[0] > 2.4) pEnemyShotSet->param_d[0] = 2.4;
        // パス番号を保存してから1進める(偶数パス=「＼」開始)
        pEnemyShotSet->kind = shot_count++;
        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}