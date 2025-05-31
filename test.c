#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>  // 若需在 Windows CMD 顯示 UTF-8 中文，可保留，否則可移除

// ============================================================================
// 1. 結構定義（SudokuProblem、SudokuDataHeader）
// ----------------------------------------------------------------------------
// 描述：用於二進位檔案存取的資料結構。
// ----------------------------------------------------------------------------
typedef struct {
    int id;
    int data[9][9];
} SudokuProblem;

typedef struct {
    int numbers;   // 檔案中包含的題目數量
    int datasize;  // 每個 SudokuProblem 占用的位元組大小
} SudokuDataHeader;


// ============================================================================
// 2. 全域變數宣告
// ----------------------------------------------------------------------------
// 描述：遊戲所需的全域變數：
//       - board：初始題目盤面（動態自檔案載入）
//       - original_board：複製一份不允許修改的原始盤面
//       - player_board：玩家目前的盤面
//       - answer_board：完整解答盤面
//       - error_count：記錄玩家錯誤次數
//       - move_history[]、move_count：可擴充的落子／回復記錄
// ----------------------------------------------------------------------------

int board[9][9];           // 由檔案讀入的當前題目
int original_board[9][9];  // 原始盤面，不能被玩家改動
int player_board[9][9];    // 玩家目前的盤面
int answer_board[9][9];    // 完整解答盤面

int error_count = 0;       // 記錄玩家錯誤次數
int move_history[81][3];   // 落子歷史：每筆記錄 {row, col, oldValue}
int move_count = 0;        // 歷史記錄筆數


// ============================================================================
// 3. 函式宣告（Prototype）
// ----------------------------------------------------------------------------
// 描述：在使用之前先宣告所有自訂函式。
// ----------------------------------------------------------------------------

// 檔案讀取與寫入
int read_from_binary_file(int board[][9], const char* filename, int problem_index);
void save_to_binary_file(int board[][9], int problem_id, const char* filename, int is_append);

// 解題器（Backtracking Solver）
int solve(int puzzle[][9], int pos);
int solve_sudoku(int board[][9]);
int is_safe_place(int board[][9], int row, int col, int num);
int exists_in_row(int board[][9], int row, int num);
int exists_in_col(int board[][9], int col, int num);
int exists_in_box(int board[][9], int start_row, int start_col, int num);

// 盤面顯示
void print_board(int board[][9]);
void print_board_beauty(int board[][9]);

// 遊戲邏輯
void init_game(void);
int handle_input(void);
int is_complete(void);
int is_valid_solution(int puzzle[][9]);
int count_empty_cells(void);

// 產生隨機盤面（可選）
void generate_random_sudoku(int board[][9], int difficulty);


// ============================================================================
// 4. 主程式
// ----------------------------------------------------------------------------
// 描述：程式一啟動時，先詢問玩家要載入哪一個盤面（1~18），然後才進行遊戲。
// ----------------------------------------------------------------------------
int main() {
    // 如果需要在 Windows CMD 下顯示中文，就保留以下兩行；不需要就可以移除
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    int choice;
    char restart;

    do {
        // ---------- 4.1 在程式一開始詢問「要載入哪一個盤面 (1~18)」 ----------
        while (1) {
            printf("請選擇要載入的題目 (1~18)：");
            if (scanf("%d", &choice) != 1) {
                // 清空錯誤輸入
                while (getchar() != '\n');
                printf("輸入錯誤，請輸入 1~18 的整數。\n");
                continue;
            }
            if (choice < 1 || choice > 18) {
                printf("請輸入 1~18 之間的數字。\n");
                continue;
            }
            break;
        }

        // ---------- 4.2 從 sudoku.dat 讀取玩家所選的題目 (索引 = choice-1) ----------
        if (!read_from_binary_file(board, "sudoku.dat", choice - 1)) {
            printf("無法讀取 sudoku.dat 或指定題目不存在，請確認檔案與編號。\n");
            return 1;
        }

        // ---------- 4.3 初始化遊戲（複製盤面、求解答案、重置計數） ----------
        init_game();

        // ---------- 4.4 遊戲主迴圈：顯示剩餘空格、處理輸入、檢查勝負 ----------
        while (error_count < 5) {
            // 4.4.1 印出玩家目前盤面
            printf("\n當前盤面：\n");
            print_board(player_board);

            // 4.4.2 印出剩餘空格數
            int remaining = count_empty_cells();
            printf("剩餘空格數：%d\n", remaining);

            // 4.4.3 處理玩家輸入
            int result = handle_input();
            if (result == -1) {
                // 玩家主動輸入 0 0 0 → 結束本局
                printf("遊戲結束。\n");
                break;
            }
            else if (result == 1) {
                // 玩家合法填了一個數字（不論正確與否都算一次「已處理」）
                // 檢查是否已完成
                if (is_complete()) {
                    if (is_valid_solution(player_board)) {
                        print_board(player_board);
                        printf("🎉 恭喜！你完成了數獨！\n");
                    } else {
                        print_board(player_board);
                        printf("盤面已填滿，但答案不正確，請檢查！\n");
                    }
                    break;
                }
                // 若尚未完成，迴圈繼續下一輪
            }
            // 如果 result == 0 → 無效輸入，已在 handle_input() 中提示，直接繼續迴圈
        }

        // ---------- 4.5 若錯誤次數達 5 次，自動結束並顯示正確答案 ----------
        if (error_count >= 5) {
            printf("\n💥 錯誤太多次，遊戲結束！\n");
            printf("正確答案：\n");
            print_board(answer_board);
        }

        // ---------- 4.6 詢問玩家是否要重新開始（Y/N） ----------
        while (1) {
            printf("\n是否要重新開始？(Y/N)：");
            // 把緩存換行符清掉，才不會直接讀到上一次的換行
            while (getchar() != '\n');
            restart = getchar();
            if (restart == 'Y' || restart == 'y') {
                break;  // 由外層 do-while 重新開始
            }
            else if (restart == 'N' || restart == 'n') {
                printf("感謝遊玩，再見！\n");
                return 0;  // 結束整個程式
            }
            else {
                printf("請輸入 Y 或 N。\n");
                // 若不是 Y/N，繼續詢問
            }
        }

    } while (1);

    return 0;
}


// ============================================================================
// 5. 讀取二進位檔函式
// ----------------------------------------------------------------------------
// 描述：檔案格式為：
//
// [SudokuDataHeader][SudokuProblem #0][SudokuProblem #1]...
//
// 讀出 header 後，可跳到指定題目索引位置進行 fread。
// ----------------------------------------------------------------------------
int read_from_binary_file(int board[][9], const char* filename, int problem_index) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("無法開啟檔案 %s 進行讀取\n", filename);
        return 0;
    }

    // 讀取檔案標頭
    SudokuDataHeader header;
    fread(&header, sizeof(header), 1, fp);
    if (header.numbers <= 0) {
        printf("檔案 %s 格式錯誤或沒有題目。\n", filename);
        fclose(fp);
        return 0;
    }
    printf("檔案中共有 %d 個數獨題目。\n", header.numbers);

    // 檢查索引是否有效
    if (problem_index < 0 || problem_index >= header.numbers) {
        printf("題目索引 %d 超出範圍 (0 ~ %d)\n",
               problem_index, header.numbers - 1);
        fclose(fp);
        return 0;
    }

    // 跳到指定題目位置：header 後面 + offset
    fseek(fp, sizeof(header) + problem_index * header.datasize, SEEK_SET);

    // 讀取一筆 SudokuProblem
    SudokuProblem problem;
    fread(&problem, sizeof(problem), 1, fp);

    // 複製到 board
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            board[i][j] = problem.data[i][j];
        }
    }

    printf("已讀取題目 ID: %d\n", problem.id);
    fclose(fp);
    return 1;
}


// ============================================================================
// 6. 解題器：回溯法求解完整盤面
// ----------------------------------------------------------------------------
// solve()：從第 pos 格 (0~80) 開始，若填完則回傳 1；否則對 1～9 試填，
//           並遞迴。is_safe_place() 等輔助函式判斷是否合法。
// ----------------------------------------------------------------------------
int solve(int puzzle[][9], int pos) {
    if (pos == 81) {
        return 1;  // 全部填完 → 成功
    }

    int row = pos / 9;
    int col = pos % 9;

    // 如果此格已有數字符號，就直接進到下一格
    if (puzzle[row][col] != 0) {
        return solve(puzzle, pos + 1);
    }

    // 嘗試填入 1～9
    for (int num = 1; num <= 9; num++) {
        if (is_safe_place(puzzle, row, col, num)) {
            puzzle[row][col] = num;
            if (solve(puzzle, pos + 1)) {
                return 1;
            }
            // 回溯
            puzzle[row][col] = 0;
        }
    }

    // 無法填入任何數字 → 回溯失敗
    return 0;
}


// ============================================================================
// 7. solve_sudoku(): 隨機解出一個完整盤面（用於隨機產題）
// ----------------------------------------------------------------------------
// 描述：先找第一個空格，再隨機排列 1~9 的順序依序嘗試，直到整盤填滿。
//       通常搭配 generate_random_sudoku() 使用。
// ----------------------------------------------------------------------------
int solve_sudoku(int board[][9]) {
    int row, col;
    int found_empty = 0;

    for (row = 0; row < 9; row++) {
        for (col = 0; col < 9; col++) {
            if (board[row][col] == 0) {
                found_empty = 1;
                break;
            }
        }
        if (found_empty) break;
    }
    // 全盤填滿
    if (!found_empty) {
        return 1;
    }

    // 產生 1~9 的隨機順序
    int nums[9] = {1,2,3,4,5,6,7,8,9};
    for (int i = 0; i < 9; i++) {
        int j = i + rand() % (9 - i);
        int tmp = nums[i];
        nums[i] = nums[j];
        nums[j] = tmp;
    }

    // 嘗試填入
    for (int i = 0; i < 9; i++) {
        int num = nums[i];
        if (is_safe_place(board, row, col, num)) {
            board[row][col] = num;
            if (solve_sudoku(board)) {
                return 1;
            }
            board[row][col] = 0;  // 回溯
        }
    }
    return 0;
}


// ============================================================================
// 8. 檢查某一位置填入 num 是否安全（不衝突）
// ----------------------------------------------------------------------------
// exists_in_row/col/box(): 檢查同一行、同一列、同一 3×3 宏方格內是否已有 num。
// ----------------------------------------------------------------------------
int is_safe_place(int board[][9], int row, int col, int num) {
    return !exists_in_row(board, row, num)
        && !exists_in_col(board, col, num)
        && !exists_in_box(board, row - row % 3, col - col % 3, num);
}

int exists_in_row(int board[][9], int row, int num) {
    for (int c = 0; c < 9; c++) {
        if (board[row][c] == num) return 1;
    }
    return 0;
}

int exists_in_col(int board[][9], int col, int num) {
    for (int r = 0; r < 9; r++) {
        if (board[r][col] == num) return 1;
    }
    return 0;
}

int exists_in_box(int board[][9], int start_row, int start_col, int num) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (board[start_row + i][start_col + j] == num) {
                return 1;
            }
        }
    }
    return 0;
}


// ============================================================================
// 9. 產生隨機數獨題目（可選功能）
// ----------------------------------------------------------------------------
// 描述：清空整盤，先隨機放 5 個數字解出完整盤面，再隨機挖洞。
// difficulty: 1 (30~39 格洞), 2 (40~49 格洞), 3 (50~59 格洞)。
// ----------------------------------------------------------------------------
void generate_random_sudoku(int board[][9], int difficulty) {
    // 先清空盤面
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            board[i][j] = 0;
        }
    }

    srand((unsigned int)time(NULL));

    // 隨機放 5 個數字
    int placed = 0;
    while (placed < 5) {
        int r = rand() % 9;
        int c = rand() % 9;
        int num = 1 + rand() % 9;
        if (board[r][c] == 0 && is_safe_place(board, r, c, num)) {
            board[r][c] = num;
            placed++;
        }
    }

    // 解出完整盤面
    solve_sudoku(board);

    // 根據難度決定要挖掉多少格
    int cells_to_remove;
    switch (difficulty) {
        case 1: cells_to_remove = 30 + rand() % 10; break;
        case 2: cells_to_remove = 40 + rand() % 10; break;
        case 3: cells_to_remove = 50 + rand() % 10; break;
        default: cells_to_remove = 40; break;
    }

    while (cells_to_remove > 0) {
        int r = rand() % 9;
        int c = rand() % 9;
        if (board[r][c] != 0) {
            board[r][c] = 0;
            cells_to_remove--;
        }
    }
}


// ============================================================================
// 10. 印出盤面：簡易版
// ----------------------------------------------------------------------------
// 描述：以「+—+—+—+」形式顯示 9×9 盤面，空格用 '_' 表示。
// ----------------------------------------------------------------------------
void print_board(int board[][9]) {
    printf("\n +-------+-------+-------+\n");
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (j % 3 == 0) printf(" | ");
            else printf(" ");
            if (board[i][j] == 0) {
                printf("_");
            } else {
                printf("%d", board[i][j]);
            }
        }
        printf(" |\n");
        if (i % 3 == 2) {
            printf(" +-------+-------+-------+\n");
        }
    }
}


// ============================================================================
// 11. 印出盤面：美化版（Unicode 框線）
// ----------------------------------------------------------------------------
// 描述：使用 ╔ ╦ ═ ║ ╠ ╬ ╚ ╩ ╩ 等符號讓盤面看起來更漂亮。
// ----------------------------------------------------------------------------
void print_board_beauty(int board[][9]) {
    printf("\n  ╔═══════╦═══════╦═══════╗\n");
    for (int i = 0; i < 9; i++) {
        printf("  ║");
        for (int j = 0; j < 9; j++) {
            if (board[i][j] == 0) {
                printf(" · ");
            } else {
                printf(" %d ", board[i][j]);
            }
            if (j % 3 == 2 && j < 8) {
                printf("║");
            }
        }
        printf("║\n");
        if (i % 3 == 2 && i < 8) {
            printf("  ╠═══════╬═══════╬═══════╣\n");
        }
    }
    printf("  ╚═══════╩═══════╩═══════╝\n");
}


// ============================================================================
// 12. 新增遊戲初始化函式：init_game()
// ----------------------------------------------------------------------------
// 描述：複製 board → original_board / player_board / answer_board，
//       呼叫解題器算出完整答案，並重置 error_count、move_count。
// ----------------------------------------------------------------------------
void init_game(void) {
    // 複製初始題目到各種盤面
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            original_board[i][j] = board[i][j];
            player_board[i][j] = board[i][j];
            answer_board[i][j] = board[i][j];
        }
    }

    // 計算完整答案
    solve(answer_board, 0);

    // 重置錯誤次數與動作歷史
    error_count = 0;
    move_count = 0;

    printf("遊戲初始化完成！\n");
}


// ============================================================================
// 13. 新增輸入處理函式：handle_input()
// ----------------------------------------------------------------------------
// 描述：讀取玩家輸入 (row, col, num)，若輸入 (0,0,0) 則結束；
//       否則檢查輸入是否合法、是否填在原始格子、是否已填過、
//       再比較答案，若正確填入 player_board 中，否則 error_count++。
// ----------------------------------------------------------------------------
int handle_input(void) {
    int row, col, num;

    printf("請輸入 行 列 數字 (1-9)，或輸入 0 0 0 結束本局: ");
    if (scanf("%d %d %d", &row, &col, &num) != 3) {
        // 清空錯誤緩存
        while (getchar() != '\n');
        printf("輸入格式錯誤，請輸入三個整數。\n");
        return 0;
    }

    // 判斷是否結束
    if (row == 0 && col == 0 && num == 0) {
        return -1;  // 玩家選擇主動結束本局
    }

    // 輸入範圍檢查
    if (row < 1 || row > 9 || col < 1 || col > 9 || num < 1 || num > 9) {
        printf("輸入超出範圍！請輸入 1 ~ 9 之間的整數。\n");
        return 0;   // 無效輸入，不算錯誤
    }

    // 轉換為 0-based 索引
    row--;
    col--;

    // 檢查是否為原始題目格子（不可修改）
    if (original_board[row][col] != 0) {
        printf("該位置為原始數字，不能修改！\n");
        return 0;
    }

    // 檢查是否已填過
    if (player_board[row][col] != 0) {
        printf("該位置已經填過數字！\n");
        return 0;
    }

    // 檢查答案是否正確
    if (answer_board[row][col] == num) {
        player_board[row][col] = num;
        printf("正確！\n");
    } else {
        error_count++;
        printf("錯誤！錯誤次數：%d\n", error_count);
    }

    return 1;  // 表示一次有效處理
}


// ============================================================================
// 14. 新增遊戲完成檢查函式：is_complete()
// ----------------------------------------------------------------------------
// 描述：只要 player_board 中有任何一個位置為 0，即代表尚未完成，回傳 0。
//       若全盤皆非 0，則回傳 1（完成）。
// ----------------------------------------------------------------------------
int is_complete(void) {
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (player_board[i][j] == 0) {
                return 0;  // 有空格
            }
        }
    }
    return 1;  // 全盤填滿
}


// ============================================================================
// 15. 檢查盤面合法性：is_valid_solution()
// ----------------------------------------------------------------------------
// 描述：針對已填滿的 puzzle，檢查每一行、每一列、每一 3×3 小方格是否都
//       含有 1~9 各一次，若皆符合則回傳 1，否則回傳 0。
// ----------------------------------------------------------------------------
int is_valid_solution(int puzzle[][9]) {
    int check[10];

    // 檢查每一行
    for (int i = 0; i < 9; i++) {
        for (int k = 1; k <= 9; k++) check[k] = 0;
        for (int j = 0; j < 9; j++) {
            int v = puzzle[i][j];
            if (v < 1 || v > 9 || check[v]) {
                return 0;
            }
            check[v] = 1;
        }
    }

    // 檢查每一列
    for (int j = 0; j < 9; j++) {
        for (int k = 1; k <= 9; k++) check[k] = 0;
        for (int i = 0; i < 9; i++) {
            int v = puzzle[i][j];
            if (v < 1 || v > 9 || check[v]) {
                return 0;
            }
            check[v] = 1;
        }
    }

    // 檢查每個 3×3 小方格
    for (int boxRow = 0; boxRow < 3; boxRow++) {
        for (int boxCol = 0; boxCol < 3; boxCol++) {
            for (int k = 1; k <= 9; k++) check[k] = 0;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    int v = puzzle[boxRow * 3 + i][boxCol * 3 + j];
                    if (v < 1 || v > 9 || check[v]) {
                        return 0;
                    }
                    check[v] = 1;
                }
            }
        }
    }

    return 1;
}


// ============================================================================
// 16. 計算剩餘空格數：count_empty_cells()
// ----------------------------------------------------------------------------
// 描述：統計 player_board 中值為 0 的格子數量，並回傳。
// ----------------------------------------------------------------------------
int count_empty_cells(void) {
    int count = 0;
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (player_board[i][j] == 0) {
                count++;
            }
        }
    }
    return count;
}
