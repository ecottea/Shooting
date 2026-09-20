// ============================================================
//  弾幕:カルマン渦列 〜後流の道〜
//
//  画面上部から白小玉の平行流(主流)が流れ、ボス(擬似円柱)に
//  当たると消える。ボスの下流からは左右交互に渦を巻く弾を放出。
//  各弾は基準方向に対してサイン波状に蛇行して進むため、
//  交互に交差する波形の弾幕(カルマン渦列)が形成される。
//
//  一定時間ごとに「乱流フェーズ」へ移行し、放出周期と
//  振幅が不規則になってリズムが崩れる。
// ============================================================

// 白小玉の平行流:上部のランダムな位置から発射し、ボスに当たると消える
static void ShotFlow(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 絶え間なく発射し続ける(2フレームに1発で珠状の流線にする)
    if (pEnemyShotSet->count % 1 == 0) {
        pEnemyShot = new sEnemyShot;

        // 画面上部のランダムな位置から
        pEnemyShot->x = GetRand(478) + 1.0;   // 1〜479
        pEnemyShot->y = -10.0;

        // ボス→自機の方向と平行に撃つ
        pEnemyShot->muki = atan2(player.y - enemy.y, player.x - enemy.x);
        pEnemyShot->speed = 2.5;

        // 弾の色一覧: 0:赤、1:黄、2:緑、3:シアン、4:青、5:マゼンタ、6:白、7:黒、8:橙
        pEnemyShot->kind = img_enemyShotSmallBall[6]; // 白小玉

        pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
        pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
        pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
        pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
    }

    // 移動 + ボス(擬似円柱)との当たり判定
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        sEnemyShot* pNext = pShot->next; // 消去に備えて先に保存

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        // ボスの円領域に当たったら消える
        double dx = pShot->x - enemy.x;
        double dy = pShot->y - enemy.y;
        if (dx * dx + dy * dy < 40.0 * 40.0) {
            pShot->prev->next = pShot->next;
            pShot->next->prev = pShot->prev;
            delete pShot;
        }

        pShot = pNext;
    }
}

// カルマン渦弾:サイン波で蛇行しながら進む
static void ShotKarmanVortex(sEnemyShotSet* pEnemyShotSet)
{
    sEnemyShot* pEnemyShot;

    // 最初のフレームで渦弾を1組(3発)放出する
    if (pEnemyShotSet->count == 0) {
        int side = pEnemyShotSet->kind % 2; // 0:左渦(反時計回り) 1:右渦(時計回り)

        // 使える効果音一覧: sound_enemyShot_light, sound_enemyShot_medium, sound_enemyShot_heavy, sound_enemyShot_extreme, sound_enemyCharge(予告音)
        if (CheckSoundMem(sound_enemyShot_light)) StopSoundMem(sound_enemyShot_light);
        PlaySoundMem(sound_enemyShot_light, DX_PLAYTYPE_BACK);

        for (int i = 0; i < 3; i++) {
            pEnemyShot = new sEnemyShot;

            // 放出位置を左右に振り分ける(交互渦の発生源)
            pEnemyShot->x = pEnemyShotSet->x + (side == 0 ? -20.0 : 20.0);
            pEnemyShot->y = pEnemyShotSet->y;
            pEnemyShot->speed = 2.2;

            // 左渦=青 / 右渦=赤 で交互放出が視覚的に分かるようにする
            pEnemyShot->kind = img_enemyShotMediumBall[side == 0 ? 4 : 0];

            // param_d[0]: 基準方向
            // param_d[1]: 位相(左右で反転させて渦が交互に屈くように)
            // param_d[2]: 蛇行の振幅
            // param_d[3]: 角振動数(3発で微妙に変え、三つ編み状にする)
            pEnemyShot->param_d[0] = pEnemyShotSet->muki;
            pEnemyShot->param_d[1] = (side == 0 ? 0.0 : DX_PI);
            pEnemyShot->param_d[2] = pEnemyShotSet->param_d[0];
            pEnemyShot->param_d[3] = 0.09 + i * 0.02;

            pEnemyShot->prev = pEnemyShotSet->pEnemyShotHead->prev;
            pEnemyShot->next = pEnemyShotSet->pEnemyShotHead;
            pEnemyShotSet->pEnemyShotHead->prev->next = pEnemyShot;
            pEnemyShotSet->pEnemyShotHead->prev = pEnemyShot;
        }
    }

    // 蛇行移動: 基準方向に対する向きをサイン波で揺らす
    sEnemyShot* pShot = pEnemyShotSet->pEnemyShotHead->next;
    while (pShot != pEnemyShotSet->pEnemyShotHead) {
        pShot->muki = pShot->param_d[0]
            + sin(pShot->count * pShot->param_d[3] + pShot->param_d[1]) * pShot->param_d[2];

        pShot->x += pShot->speed * cos(pShot->muki);
        pShot->y += pShot->speed * sin(pShot->muki);

        pShot = pShot->next;
    }
}

// 渦弾1組(3発)を放出する
static void EmitVortex(int side, double amp, int turbulence)
{
    sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
    pEnemyShotSet->count = 0;
    pEnemyShotSet->patternFunc = ShotKarmanVortex;
    pEnemyShotSet->x = enemy.x;
    pEnemyShotSet->y = enemy.y + 10.0;

    // 基準方向は自機へ向ける(乱流時はブレさせる)
    pEnemyShotSet->muki = atan2(player.y - pEnemyShotSet->y, player.x - pEnemyShotSet->x);
    if (turbulence) {
        pEnemyShotSet->muki += (GetRand(40) - 20) / 180.0 * DX_PI;
    }

    pEnemyShotSet->kind = side;          // 0:左渦 1:右渦
    pEnemyShotSet->param_d[0] = amp;     // 蛇行の振幅

    pEnemyShotSet->pEnemyShotHead = new sEnemyShot;
    pEnemyShotSet->pEnemyShotHead->prev = pEnemyShotSet->pEnemyShotHead;
    pEnemyShotSet->pEnemyShotHead->next = pEnemyShotSet->pEnemyShotHead;

    pEnemyShotSet->prev = enemyShotSetHead.prev;
    pEnemyShotSet->next = &enemyShotSetHead;
    enemyShotSetHead.prev->next = pEnemyShotSet;
    enemyShotSetHead.prev = pEnemyShotSet;
}

// 敵本体のパターン
void EnemyPat_KarmanVortex_Zai()
{
    static int side;        // 次に渦を放出する側
    static int phase;       // 0:規則的(層流) 1:不規則(乱流)
    static int phaseTimer;  // 現在のフェーズの残りフレーム数
    static int nextEmit;    // 乱流時の次の放出までのカウンタ

    if (count == 1) {
        // ゲーム画面は 480x480
        enemy.x = 240.0;
        enemy.y = 100.0;
        enemy.maxHp = enemy.hp = 200; // 200で固定
        side = 0;
        phase = 0;
        phaseTimer = 600/2;
        nextEmit = 0;

        // 白小玉の平行流(常時稼働する弾セットを1つ作成)
        sEnemyShotSet* pEnemyShotSet = new sEnemyShotSet;
        pEnemyShotSet->count = 0;
        pEnemyShotSet->patternFunc = ShotFlow;
        pEnemyShotSet->x = 0.0;
        pEnemyShotSet->y = 0.0;
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
        // 擬似円柱の振動:ボス本体がゆっくり左右に揺れる
        // 乱流フェーズでは振動が速くなり、渦の放出も乱れるイメージ
        double swaySpeed = (phase == 0) ? 0.01 : 0.035;
        enemy.x = 240.0 + sin(count * swaySpeed) * 60.0;

        // フェーズ管理: 600F規則的 → 300F乱流 → 以下ループ
        phaseTimer--;
        if (phaseTimer <= 0) {
            phase = 1 - phase;
            if (phase == 1) {
                phaseTimer = 300;
                // 乱流フェーズの予告音
                if (CheckSoundMem(sound_enemyCharge)) StopSoundMem(sound_enemyCharge);
                PlaySoundMem(sound_enemyCharge, DX_PLAYTYPE_BACK);
            }
            else {
                phaseTimer = 600/2;
            }
        }

        if (phase == 0) {
            // 層流:一定周期で左右交互に放出
            if (count % 24 == 0) {
                EmitVortex(side, 0.55, 0);
                side ^= 1;
            }
        }
        else {
            // 乱流:放出周期が不規則(GetRandは0〜xのx+1種を返すので8〜36F)
            // 振幅も大きくなり、基準方向もブレて波形が崩れる
            nextEmit--;
            if (nextEmit <= 0) {
                EmitVortex(side, 0.85, 1);
                side ^= 1;
                nextEmit = 8 + GetRand(28);
            }
        }
    }
}