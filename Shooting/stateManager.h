// stateManager.h
#pragma once

struct sCursor {
    int x = 0, y = 0;
    int page = 0;
};

// 状態を表す列挙型（StateManager とともに定義）
enum class Joutai {
    Menu,
    Win,
    Lose,
    Game,
    Replay,
};

// 状態遷移を一括管理するクラス
class StateManager {
public:
    // 現在の状態を返す（読み取り専用）
    static Joutai GetState();

    // 状態を変更する（唯一の変更手段）
    // 戻り値: 遷移が成功したら true、Replay 開始失敗時などは false
    static bool ChangeState(Joutai newState);

private:
    StateManager() = default; // インスタンス化禁止
    static Joutai currentState;
};

extern int     stageNum;
extern sCursor cursor;
