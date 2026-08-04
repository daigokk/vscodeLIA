# Makefile for vscodeLIA

# コンパイラと実行ファイルの設定
CXX      := g++
TARGET   := vscodeLIA.exe

# ディレクトリ設定
# 必要に応じてパスを書き換えてください（相対パスまたは絶対パス）
WORKSPACE_DIR := $(CURDIR)
IMGUI_DIR     := $(WORKSPACE_DIR)/external/include/IMGUI

# コンパイルオプション
CXXFLAGS := -O2 -std=c++20 -g -fdiagnostics-color=always
DEPFLAGS := -MMD -MP
INCLUDES := -I$(WORKSPACE_DIR)/external/include

# ライブラリとリンク設定
LDFLAGS  := -L$(WORKSPACE_DIR)/external/lib
LIBS     := C:/Windows/System32/dwf.dll -lglfw3 -lopengl32 -lgdi32 -luser32

# ソースファイルの自動取得
# カレントディレクトリの .cpp ファイルと ImGui の .cpp ファイルを取得
SRCS     := $(wildcard *.cpp) $(wildcard $(IMGUI_DIR)/*.cpp)
OBJS     := $(SRCS:.cpp=.o)
DEPS     := $(OBJS:.o=.d)

# デフォルトターゲット
all: $(TARGET)

# 実行ファイルの生成ルール
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(TARGET) $(LDFLAGS) $(LIBS)

# オブジェクトファイルの生成ルール
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

-include $(DEPS)

# クリーンアップコマンド
clean:
	del /Q *.o $(IMGUI_DIR)\*.o $(TARGET) 2>nul || rm -f *.o $(IMGUI_DIR)/*.o $(TARGET)

.PHONY: all clean