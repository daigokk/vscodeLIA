#include "Pipe.h"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <regex>

// ============================================================
// ユーティリティ
// ============================================================
namespace utils {
    inline std::vector<std::string> split(const std::string& str, char delimiter = ':') {
        std::vector<std::string> tokens;
        if (str.empty()) return tokens;
        std::string token;
        std::istringstream tokenStream(str);
        while (std::getline(tokenStream, token, delimiter)) {
            if (!token.empty()) {
                tokens.push_back(token);
            }
        }
        return tokens;
    }

    inline void toLower(std::string& str) {
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    }

    inline std::pair<std::string, int> parseIndex(const std::string& s) {
        std::regex re("([a-z]+)([0-9]+)");
        std::smatch match;
        if (std::regex_match(s, match, re)) {
            return { match[1].str(), std::stoi(match[2].str()) };
        }
        return { s, -1 };
    }
}

void Pipe::start(Config* pCfg) {
    thread_ = std::jthread([this, pCfg](std::stop_token st) {
        run(st, pCfg);
    });
}

void Pipe::stop() {
    thread_.request_stop();
    if (thread_.joinable()) {
        thread_.join();
    }
}

void Pipe::commandShaping(std::string line, std::vector<std::string>& tokens, std::string& argument) {
    if (!line.empty() && line.front() == ':') {
        // コロンで始まる場合、先頭のコロンを削除
        line.erase(0, 1);
    }
    utils::toLower(line); // コマンドを小文字に変換
    std::istringstream iss(line); // 入力行をストリームに変換
    // コロンで分割してトークンを取得
    std::string commandPart;
    iss >> commandPart;
    tokens = utils::split(commandPart, ':');
    
    iss >> argument; // 引数部分を取得
    float value = 0.0f;
    try {
        if (!argument.empty()) {
            // 引数が存在する場合、floatに変換
            value = std::stof(argument);
        }
    }
    catch (...) {
        // 引数がfloatに変換できない場合は0.0fに設定
        value = 0.0f;
    }
}

bool Pipe::commandDispach(std::vector<std::string>& tokens, std::string& argument, Config* pCfg) {
    bool isCommandRecognized = true;
    if (tokens[0] == "pause" || tokens[0] == "stop" || tokens[0] == "halt") {
        pCfg->buttonPause();
    }
    else if (tokens[0] == "resume" || tokens[0] == "unpause" || tokens[0] == "continue" || tokens[0] == "play" || tokens[0] == "start" || tokens[0] == "run") {
        pCfg->buttonRun();
    }
    else if (tokens[0] == "data" && tokens.size() > 1) {
        if(tokens[1] == "xy?") {
            // 現在の測定値を複素平面上の座標として出力
            const size_t idx = pCfg->ringBuffer.meaBuffer.idxCurrent;
            const auto& rb = pCfg->ringBuffer.meaBuffer;
            std::cout << std::format("{:e},{:e}", rb.chs[0].xs[idx], rb.chs[0].ys[idx]);
            for (int ch = 1; ch < pCfg->ringBuffer.meaBuffer.chs.size(); ++ch) {
                std::cout << std::format(",{:e},{:e}", rb.chs[ch].xs[idx], rb.chs[ch].ys[idx]);
            }
            std::cout << "\n";
        }
    }
    else if (tokens[0] == "post" && tokens.size() > 1) {
        if (tokens[1] == "offset" && tokens.size() > 2) {
            if (tokens[2] == "auto") {
                pCfg->buttonOffsetAutoOnce();
            }
            else if (tokens[2] == "off") {
                pCfg->buttonOffsetOff();
            }
            else {
                isCommandRecognized = false;
            }
        }
        else {
            isCommandRecognized = false;
        }
    }
    else {
        isCommandRecognized = false;
    }
    return isCommandRecognized;
}

void Pipe::run(std::stop_token st, Config* pCfg) {
    static std::string lastErrorCmd; // 最後にエラーが発生したコマンドを保持する変数
    while (!st.stop_requested()) {
        std::string line;
        if (!std::getline(std::cin, line)) {
            // 標準入力が閉じられた場合、ループを終了
            break;
        }
        std::vector<std::string> tokens;
        std::string argument;
        commandShaping(line, tokens, argument);
        if(tokens.empty()) {
            lastErrorCmd = "Empty command"; // エラーが発生したコマンドを記録
            continue; // トークンが空の場合は次のループへ
        }
        if (tokens[0] == "end" || tokens[0] == "exit" || tokens[0] == "quit" || tokens[0] == "close") {
            break;
        }
        else if(tokens[0] == "error?") {
            std::cout << std::format("Error: '{}'\n", lastErrorCmd);
            lastErrorCmd.clear();
            continue;
        }
        if(!commandDispach(tokens, argument, pCfg)){
            lastErrorCmd = line; // エラーが発生したコマンドを記録
        }
    }
    endCommandStatus = true;
}
