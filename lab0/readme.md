# DCS lab0 SystemC 環境設置

## 什麼是 SystemC
**SystemC 是一個用 C++ 編寫的一個函式庫(調用方式 `#inlcude "systemc.h"`)，支援系統級別的設計和驗證， 讓我們使用 C++ 也能實現硬體功能，不需要任何特殊的EDA工具才能使用，只需要安裝一個 C++ 編譯器與 SystemC 函式庫即可**，並且支援與 RTL 設計的協同驗證。與更詳細的 RTL 等級相比， SystemC 這種更高層次的抽象可以實現更快、更有效率的架構權衡分析、設計和重新設計。此外，在系統架構和其他系統級屬性的驗證中， SystemC 比起 RT 級驗證，同樣的 pin 腳精確度、時序精確度快上幾個數量級。

## 資料夾結構
```
lab0                       # 頂層目錄
	├── hello                # 具有Makefile的hello示例，不可被HLS工具合成
	├── counter              # 具有Makefile的counter示例，不可被HLS工具合成
  ├── .cshrc               # tcsh的啟動檔案，登入伺服器時會載入該檔案，可修改prompt
```

### 執行步驟
1. 一開始登入server，請不要選擇任何關鍵字 (`yes`,`coding`,`layout`)，否則EDA tool的環境會與 `systemc.h` 發生衝突。
2. 從 TA 資料夾複製並解壓縮檔案
```
tar -xvf ~mlchipTA01/lab0.tar
```
3. 複製 `.cshrc` 啟動檔案到個人使用者的家目錄，登入伺服器時會載入該檔案，可自由修改喜歡的terminal prompt
```
cp lab0/.cshrc .cshrc
```
4. 進入 hello 示例的目錄
```
cd lab0/hello
```
5. 編譯並執行 `hello.cpp` (如未更改檔案，要重新編譯，請執行第7點)
```
make hello
```
6. 預期運行結果
```
        SystemC 2.3.3-Accellera --- Mar  1 2024 22:24:05
        Copyright (c) 1996-2018 by all Contributors,
        ALL RIGHTS RESERVED

constructor called
method is called
destructor called
```
7. 清除executable二進位檔
```
make clean
```
8. 進入 counter 示例的目錄
```
cd ../counter
```
9. 編譯並執行 `counter.cpp` 和`counter_tb.cpp` (如未更改檔案，要重新編譯，請執行第12點)
```
make counter
```
10. 預期運行結果
```
        SystemC 2.3.3-Accellera --- Mar  1 2024 22:24:05
        Copyright (c) 1996-2018 by all Contributors,
        ALL RIGHTS RESERVED
Executing new

Info: (I702) default timescale unit used for tracing: 1 ps (counter.vcd)
@11 ns Asserting reset
@31 ns De-Asserting reset
@41 ns Asserting Enable
@42 ns :: Incremented Counter 0
@44 ns :: Incremented Counter 1
@46 ns :: Incremented Counter 2
@48 ns :: Incremented Counter 3
@50 ns :: Incremented Counter 4
@52 ns :: Incremented Counter 5
@54 ns :: Incremented Counter 6
@56 ns :: Incremented Counter 7
@58 ns :: Incremented Counter 8
@60 ns :: Incremented Counter 9
@62 ns :: Incremented Counter 10
@64 ns :: Incremented Counter 11
@66 ns :: Incremented Counter 12
@68 ns :: Incremented Counter 13
@70 ns :: Incremented Counter 14
@72 ns :: Incremented Counter 15
@74 ns :: Incremented Counter 0
@76 ns :: Incremented Counter 1
@78 ns :: Incremented Counter 2
@80 ns :: Incremented Counter 3
@81 ns De-Asserting Enable
@81 ns Terminating simulation
```
11. 清除executable二進位檔和`*.vcd`波型檔
```
make clean
```

## 引入的關鍵字和一些基本概念
## 連接接口
SystemC中的連接端口類似於Verilog中的接口，可以是輸入、輸出或雙向接口。
- `sc_in` - 輸入接口
- `sc_out` - 輸出接口
- `sc_inout` - 雙向接口

有用於時鐘的接口，如 `sc_in_clk`，但建議即使是時鐘也使用常規接口。例如 `sc_in<bool> clock` 只是一個具有布林特性的輸入端口。它不是高電平就是低電平(當考慮時鐘時) - 1或0。

## sc_main
`sc_main` 是主函數。在基於 SystemC 構建系統時，`sc_main` 是整個系統的主函數。可以構建多個函數，但必須存在 `sc_main`。如以下示例
```c++
#include <systemc.h>
// module named hello
SC_MODULE (hello) {
    // Constructor (module is created)
    SC_CTOR (hello) {
		// do something
    }

    void say_hello() {
      std::cout << "Hello World!" << std::endl;
    }
};

int sc_main(int argc, char* argv[]) {
  hello h("hello");
  h.say_hello();
  return 0;
}
```
我們有兩個函數。`say_hello` 負責輸出文本，而 `sc_main` 則傳遞 `say_hello` 函數並返回 0（成功）。

## SC_MODULE
`SC_MODULE` 旨在聲明完整的模塊/部分。它的目的與 Verilog 中的 `module` 相同，但是以 SystemC 風格。

## SC_CTOR
`SC_CTOR` 是 SystemC 的構造函數。它執行幾件事

- 聲明敏感列表。
在 SystemC 中，敏感列表是構造函數的一部分，它聲明了哪些信號最敏感。例如：
```c++
sensitive << clk.pos(); 
```
這告訴模塊設計對時鐘敏感，這種情況下是正邊緣。
- 將每個函數註冊為在模塊中發生的進程。
- 創建設計層次結構，如果包含了幾個模塊，則給整個設計提供模塊使用的意義。

 ## 線程
線程是一个被設計成像硬件进程一样運行的函数。具有以下特點
 - 並行運行 - 可以同時啟動多個進程(注意 : SystemC 中的每個函數都是一個進程)
 - 它們對信號敏感。
 - 它們不由用戶調用，而是始終處於活動狀態。
 - 有三種類型的線程：`SC_METHOD`、`SC_THREAD`、`SC_CTHREAD`。

### `SC_METHOD`
- 僅限於單個時鐘周期。適用於簡單的順序邏輯
- 它們在每個敏感事件之後執行一次
- 它們持續運行
- 可合成
- 類似於 Verilog 中的 `always @` 區塊

### `SC_THREAD`
- 開始仿真時運行一次，然後在完成後暫停自身。
- 可以包含無限循環
- 不可合成
- 在測試構建中使用，以描述時鐘
- 類似於 Verilog 中的 `@ initial` 區塊

### `SC_CTHREAD` - 時鐘線程
- 可合成
- 不限於一個周期
- 可以包含連續循環
- 可以包含大塊控制代碼或帶有操作的代碼
- 用於行為合成
- 持續運行
- 可以需要更多時鐘周期來執行單個迭代
- 用於 99％ 的 SystemC 設計
- 類似於 Verilog 中的 `always @ (pos/negedge clock)`


## 參考資料
- [SystemC-tutorial](https://github.com/AleksandarKostovic/SystemC-tutorial/tree/master)