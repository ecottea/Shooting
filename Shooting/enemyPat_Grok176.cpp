
// 線香花火弾幕パターン
// 使用素材: 小玉・中玉（色は主に橙=8, 黄=1）
// 効果音: sound_enemyShot_medium / sound_enemyCharge

#include "gv.h"

// 線香花火の弾幕本体
static void ShotSenkoHanabi(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 初期化（1回だけ）
    if (pEnemyShotSet->count == 0) {
        // 予告音
        if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
        PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);

        // パラメータ初期化
        // param_i[0] : フェーズ管理用
        // param_d[0] : 火球中心X
        // param_d[1] : 火球中心Y
        pEnemyShotSet->param_i[0] = 0;
        pEnemyShotSet->param_d[0] = pEnemyShotSet->x;
        pEnemyShotSet->param_d[1] = pEnemyShotSet->y + 48.0;  // 線香の先端位置
        pEnemyShotSet->alive = 280;  // 存在時間を長めに確保
    }

    //--------------------------------------------------
    // フェーズ0：線香本体（軸）の生成
    //--------------------------------------------------
    if (pEnemyShotSet->count == 5) {
        // 縦に並ぶ小玉で線香本体を表現（動かさない）
        for (int i = 0; i < 10; i++) {
            pEnemyShot = new sEnemyShot;
            pEnemyShot->x = pEnemyShotSet->x;
            pEnemyShot->y = pEnemyShotSet->y + 4.0 + i * 4.5;
            pEnemyShot->muki = DX_PI / 2.0;          // 下向き（見た目用）
            pEnemyShot->speed = 0.0;                 // 固定
            pEnemyShot->kind = img_enemyShotSmallBall[1];  // 黄
            pEnemyShot->margin = 40.0;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    //--------------------------------------------------
    // フェーズ1：火球の生成・成長
    //--------------------------------------------------
    if (pEnemyShotSet->count >= 20 && pEnemyShotSet->count <= 90) {
        // 20フレームごとに火球を少しずつ大きくする
        if ((pEnemyShotSet->count - 20) % 14 == 0) {
            int ballNum = 4 + (pEnemyShotSet->count - 20)*14 / 14;  // 徐々に増やす

            for (int i = 0; i < ballNum; i++) {
                double ang = GetRand(360) / 180.0 * DX_PI;
                double dist = 2.0 + GetRand(60) / 10.0;

                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->param_d[0] + cos(ang) * dist;
                pEnemyShot->y = pEnemyShotSet->param_d[1] + sin(ang) * dist;
                pEnemyShot->muki = ang;
                pEnemyShot->speed = 0.0;  // 最初は固定
                // 橙と黄を混ぜる
                pEnemyShot->kind = (GetRand(1) == 0) ? img_enemyShotMediumBall[8] : img_enemyShotMediumBall[1];
                pEnemyShot->margin = 30.0;

                // 火球の弾であることを示すフラグ
                pEnemyShot->param_i[0] = 1;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }

            // 火球成長時の音
            if (CheckSoundMem(sound_enemyShot_medium)) StopSoundMem(sound_enemyShot_medium);
            PlaySoundMem(sound_enemyShot_medium, DX_PLAYTYPE_BACK);
        }
    }

    //--------------------------------------------------
    // フェーズ2：火花を散らす（メイン弾幕）
    //--------------------------------------------------
    if (pEnemyShotSet->count >= 40 && pEnemyShotSet->count <= 200) {
        // 火花の発生頻度を徐々に上げる
        int interval = 8;
        if (pEnemyShotSet->count > 100) interval = 5;
        if (pEnemyShotSet->count > 150) interval = 3;

        if (pEnemyShotSet->count % interval == 0) {
            int sparkNum = 3 + GetRand(4);  // 3〜7発

            for (int i = 0; i < sparkNum; i++) {
                pEnemyShot = new sEnemyShot;

                // 火球中心付近から発射
                double offset = (GetRand(100) - 50) / 10.0;
                pEnemyShot->x = pEnemyShotSet->param_d[0] + offset;
                pEnemyShot->y = pEnemyShotSet->param_d[1] + (GetRand(40) - 20) / 10.0;

                // 下方向に偏った角度（線香花火らしい散り方）
                // 基準を下（π/2）にして ±70度程度の範囲
                double base = DX_PI / 2.0;
                double range = 1.4;  // 約80度
                pEnemyShot->muki = base + (GetRand(200) - 100) / 100.0 * range;

                // 速度にばらつき
                pEnemyShot->speed = 1.4 + GetRand(180) / 100.0;

                // 小玉（橙・黄中心、たまに白）
                int col = GetRand(5);
                if (col <= 2)      pEnemyShot->kind = img_enemyShotSmallBall[8];  // 橙
                else if (col <= 4) pEnemyShot->kind = img_enemyShotSmallBall[1];  // 黄
                else               pEnemyShot->kind = img_enemyShotSmallBall[6];  // 白

                pEnemyShot->margin = 25.0;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }

            // 時々大きめの火花（中玉）を混ぜる
            if (GetRand(4) == 0) {
                pEnemyShot = new sEnemyShot;
                pEnemyShot->x = pEnemyShotSet->param_d[0];
                pEnemyShot->y = pEnemyShotSet->param_d[1];
                pEnemyShot->muki = DX_PI / 2.0 + (GetRand(120) - 60) / 180.0 * DX_PI;
                pEnemyShot->speed = 2.2 + GetRand(100) / 100.0;
                pEnemyShot->kind = img_enemyShotMediumBall[8];  // 橙の中玉
                pEnemyShot->margin = 30.0;

                pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
                pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
                pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
                pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
            }
        }
    }

    //--------------------------------------------------
    // フェーズ3：燃え尽き（終盤）
    //--------------------------------------------------
    if (pEnemyShotSet->count == 210) {
        // 火球を外側へ弾き飛ばす
        sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
        while (pShot != pEnemyShotSet->pEnemyShotHead) {
            if (pShot->param_i[0] == 1) {  // 火球の弾だけ
                pShot->speed = 1.8 + GetRand(120) / 100.0;
                // 放射状に飛ばす
                pShot->muki = atan2(pShot->y - pEnemyShotSet->param_d[1],
                    pShot->x - pEnemyShotSet->param_d[0]);
                if (pShot->muki == 0.0 && pShot->x == pEnemyShotSet->param_d[0]) {
                    pShot->muki = GetRand(360) / 180.0 * DX_PI;
                }
            }
            pShot = pShot->next;
        }

        // 最後の炸裂音
        if (CheckSoundMem(sound_enemyShot_heavy)) StopSoundMem(sound_enemyShot_heavy);
        PlaySoundMem(sound_enemyShot_heavy, DX_PLAYTYPE_BACK);
    }

    //--------------------------------------------------
    // 全弾の移動処理
    //--------------------------------------------------
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);
        pShot = pShot->next;
    }
}

// 敵本体パターン
void EnemyPat_Sparkler_Grok()
{
    static int muki;
    static int phase;

    if (count == 1) {
        // 初期配置（画面上部中央）
        enemy.x = 240.0;
        enemy.y = 50.0;
        enemy.maxHp = enemy.hp = 200;
        muki = 1;
        phase = 0;
    }
    else {
        // ゆっくり左右に揺れる
        enemy.x += 0.6 * (double)muki;
        if (enemy.x < 160.0) muki = 1;
        if (enemy.x > 320.0) muki = -1;
    }

    // 線香花火弾幕を一定間隔で発動
    // （最初は早め、以降は間隔を空ける）
    bool fire = false;
    if (count == 30) fire = true;
    if (count > 30 && (count - 30) % 220 == 0) fire = true;

    if (fire) {
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotSenkoHanabi;
        pEnemyShotSet->x = enemy.x;
        pEnemyShotSet->y = enemy.y + 8.0;
        pEnemyShotSet->muki = DX_PI / 2.0;  // 下向き基準
        pEnemyShotSet->kind = 0;
        pEnemyShotSet->alive = 280;

        pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

        pEnemyShotSet->prev = enemyShotSetHead.prev;
        pEnemyShotSet->next = &enemyShotSetHead;
        enemyShotSetHead.prev->next = pEnemyShotSet;
        enemyShotSetHead.prev = pEnemyShotSet;
    }
}
