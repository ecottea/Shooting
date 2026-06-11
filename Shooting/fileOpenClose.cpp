#define _CRT_SECURE_NO_WARNINGS

#include "DxLib.h"
#include "gv.h"
#include "fileOpenClose.h"
#include "stageData.h"
#include <fstream>
#include <string>
#include "json.hpp"

using json = nlohmann::json;

void fileOpen()
{
    // まず全ステージのベストタイムをデフォルト値（59999）で初期化
    for (int i = 0; i < (int)stageData.size(); i++)
    {
        stageData[i].bestTime = 59999;
    }

    std::ifstream ifs("saveData/bestTime.json");
    if (!ifs.is_open())
        return;   // ファイルがなければデフォルト値のまま

    try
    {
        json j;
        ifs >> j;

        // JSONがオブジェクト（キー：ステージID, 値：ベストタイム）の場合のみ処理
        if (j.is_object())
        {
            for (auto it = j.begin(); it != j.end(); ++it)
            {
                std::string key = it.key();
                unsigned int value = it.value();

                // stageData から該当するステージIDを探す
                for (int i = 0; i < (int)stageData.size(); i++)
                {
                    if (key == stageData[i].stageId)
                    {
                        stageData[i].bestTime = value;
                        break;
                    }
                }
            }
        }
        // 配列など想定外の形式は無視（全ステージデフォルト値のまま）
    }
    catch (...)
    {
        // パースエラー時もデフォルト値のまま
    }
}

void fileClose()
{
    json j = json::object();

    for (int i = 0; i < (int)stageData.size(); i++)
    {
        j[stageData[i].stageId] = stageData[i].bestTime;
    }

    std::ofstream ofs("saveData/bestTime.json");
    if (ofs.is_open())
    {
        ofs << j.dump(4);   // インデント付きで出力
    }

    saveCursorPos();   // ★ ここを追加
}

// 追加する読み込み関数
void loadCursorPos()
{
    std::ifstream ifs("saveData/cursor.json");
    if (!ifs.is_open())
        return;   // ファイルがなければデフォルトのまま

    try
    {
        json j;
        ifs >> j;

        if (j.is_object())
        {
            if (j.contains("x")) cursor.x = j["x"];
            if (j.contains("y")) cursor.y = j["y"];
        }
    }
    catch (...)
    {
        // 読み込みエラー時はデフォルト値のまま
    }
}

// 追加する保存関数
void saveCursorPos()
{
    json j;
    j["x"] = cursor.x;
    j["y"] = cursor.y;

    std::ofstream ofs("saveData/cursor.json");
    if (ofs.is_open())
    {
        ofs << j.dump(4);
    }
}
