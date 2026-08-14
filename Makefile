# Makefile for vscodeLIA

# コンパイラと実行ファイルの設定
CXX      := g++
TARGET   := vscodeLIA.exe

# ディレクトリ設定
# 必要に応じてパスを書き換えてください（相対パスまたは絶対パス）
WORKSPACE_DIR := $(CURDIR)
IMGUI_DIR     := $(WORKSPACE_DIR)/external/include/IMGUI
DAQ_DIR       := $(WORKSPACE_DIR)/external/include/DWF

# コンパイルオプション
CXXFLAGS := -std=c++20 -g -fdiagnostics-color=always
DEPFLAGS := -MMD -MP
INCLUDES := -I$(WORKSPACE_DIR)/external/include

# 並列コンパイル設定 短縮効果はわずか。コア数を増やしても観測できるほどの変化はない
MAKEFLAGS += -j4

# ライブラリとリンク設定
LDFLAGS  := -L$(WORKSPACE_DIR)/external/lib
LIBS     := C:/Windows/System32/dwf.dll -lglfw3 -lopengl32 -lgdi32 -luser32

# ソースファイルの自動取得
# カレントディレクトリ、ImGui、及びDAQの.cppファイルを取得
SRCS     := $(wildcard *.cpp) $(wildcard $(IMGUI_DIR)/*.cpp) $(wildcard $(DAQ_DIR)/*.cpp)
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
.PHONY: all clean

# パスのスラッシュを Windows 用のバックスラッシュに変換するヘルパー関数
FIX_PATH = $(subst /,\,$(1))

clean:
ifeq ($(OS),Windows_NT)
	@if exist *.o del /f /q *.o 2>nul
	@if exist *.d del /f /q *.d 2>nul
	@if exist $(call FIX_PATH,$(IMGUI_DIR))\*.o del /f /q $(call FIX_PATH,$(IMGUI_DIR))\*.o 2>nul
	@if exist $(call FIX_PATH,$(IMGUI_DIR))\*.d del /f /q $(call FIX_PATH,$(IMGUI_DIR))\*.d 2>nul
	@if exist $(call FIX_PATH,$(DAQ_DIR))\*.o del /f /q $(call FIX_PATH,$(DAQ_DIR))\*.o 2>nul
	@if exist $(call FIX_PATH,$(DAQ_DIR))\*.d del /f /q $(call FIX_PATH,$(DAQ_DIR))\*.d 2>nul
	@if exist $(TARGET) del /f /q $(TARGET) 2>nul
else
	rm -f *.o *.d $(IMGUI_DIR)/*.o $(IMGUI_DIR)/*.d $(DAQ_DIR)/*.o $(DAQ_DIR)/*.d $(TARGET)
endif