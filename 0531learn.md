## 0531sudoku報告

### 今日進度紀錄（2025/05/31）

1. 新增函式，能夠讀取 sudoku.dat 檔案中的不同數獨表盤，並正確顯示盤面。
2. 學習並實作遞迴（recursion）方式解決數獨問題，並成功用程式自動解出盤面。

### 練習1
```c
int count_empty_cells(int puzzle[][9]) {
    int count = 0;
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (puzzle[i][j] == 0) {
                count++;
            }
        }
    }
    return count;
}
```
### 練習2
```c
int is_valid_solution(int puzzle[][9]) {
    int check[10];
    // 檢查每一行
    for (int i = 0; i < 9; i++) {
        for (int k = 1; k <= 9; k++) check[k] = 0;
        for (int j = 0; j < 9; j++) {
            int v = puzzle[i][j];
            if (v < 1 || v > 9 || check[v]) return 0;
            check[v] = 1;
        }
    }
    // 檢查每一列
    for (int j = 0; j < 9; j++) {
        for (int k = 1; k <= 9; k++) check[k] = 0;
        for (int i = 0; i < 9; i++) {
            int v = puzzle[i][j];
            if (v < 1 || v > 9 || check[v]) return 0;
            check[v] = 1;
        }
    }
    // 檢查每個 3x3 方格
    for (int boxRow = 0; boxRow < 3; boxRow++) {
        for (int boxCol = 0; boxCol < 3; boxCol++) {
            for (int k = 1; k <= 9; k++) check[k] = 0;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    int v = puzzle[boxRow*3 + i][boxCol*3 + j];
                    if (v < 1 || v > 9 || check[v]) return 0;
                    check[v] = 1;
                }
            }
        }
    }
    return 1;
}
```