#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <conio.h> // 用於捕捉方向鍵輸入
#include <windows.h>

// ANSI escape codes for colors
#define COLOR_RESET "\033[0m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_WHITE "\033[37m"

// 全域顏色選擇
char* BOARD_COLOR;
char* CURSOR_COLOR;
char* NUMBER_COLOR;

//--- 函式原型宣告（prototype） -----------------------------------
void print_board(int board[][9]);
void print_board_beauty(int board[][9]);
void play_game(int difficulty);
int solve(int puzzle[][9], int pos);
int handle_input(int difficulty);
int is_complete(void);
int is_valid_solution(int puzzle[][9]);

int exists_in_row(int board[][9], int row, int num);
int exists_in_col(int board[][9], int col, int num);
int exists_in_box(int board[][9], int start_row, int start_col, int num);
int is_safe_place(int board[][9], int row, int col, int num);
int solve_sudoku(int board[][9]);
void generate_random_sudoku(int board[][9], int difficulty);

void save_to_text_file(int board[][9], const char* filename);
void save_to_binary_file(int board[][9], int problem_id, const char* filename, int is_append);

int read_from_text_file(int board[][9], const char* filename);
int read_from_binary_file(int board[][9], const char* filename, int problem_index);
void print_board_with_cursor(int board[][9], int cursor_row, int cursor_col);

//--- 全域變數宣告 ---------------------------------------------------
int board[9][9] = {
    {0,0,0,0,0,0,0,9,0},
    {1,9,0,4,7,0,6,0,8},
    {0,5,2,8,1,9,4,0,7},
    {2,0,0,0,4,8,0,0,0},
    {0,0,9,0,0,0,5,0,0},
    {0,0,0,7,5,0,0,0,9},
    {9,0,7,3,6,4,1,8,0},
    {5,0,6,0,8,1,0,7,4},
    {0,8,0,0,0,0,0,0,0}
};
int original_board[9][9];
int player_board[9][9];
int answer_board[9][9];
int error_count = 0;
int move_history[81][3];
int move_count = 0;

// 新增光標位置全域變數
int cursor_row = 0;
int cursor_col = 0;

// 新增計時功能變數
time_t start_time;
time_t end_time;

// 定義 SudokuProblem 結構
typedef struct {
    int id;
    int data[9][9];
} SudokuProblem;

//=== 第一段：檔案讀取、遊戲初始化、主程式 =================================

int main() {
    SetConsoleOutputCP(65001); // 設定輸出為 UTF-8

    int choice, problem_index, color_choice;
    int difficulty = 0;
    
    printf("=== 數獨遊戲 ===\n");
    
    // 顏色選擇
    printf("請選擇遊戲界面顏色：\n");
    printf("1. 紅色\n");
    printf("2. 綠色 (預設)\n");
    printf("3. 藍色\n");
    printf("4. 黃色\n");
    printf("5. 洋紅色\n");
    printf("6. 青色\n");
    printf("請輸入選擇 (1-6): ");
    scanf("%d", &color_choice);
    
    // 設置顏色
    switch (color_choice) {
        case 1:
            BOARD_COLOR = COLOR_RED;
            CURSOR_COLOR = COLOR_YELLOW;
            NUMBER_COLOR = COLOR_RED;
            break;
        case 3:
            BOARD_COLOR = COLOR_BLUE;
            CURSOR_COLOR = COLOR_YELLOW;
            NUMBER_COLOR = COLOR_BLUE;
            break;
        case 4:
            BOARD_COLOR = COLOR_YELLOW;
            CURSOR_COLOR = COLOR_RED;
            NUMBER_COLOR = COLOR_YELLOW;
            break;
        case 5:
            BOARD_COLOR = COLOR_MAGENTA;
            CURSOR_COLOR = COLOR_YELLOW;
            NUMBER_COLOR = COLOR_MAGENTA;
            break;
        case 6:
            BOARD_COLOR = COLOR_CYAN;
            CURSOR_COLOR = COLOR_RED;
            NUMBER_COLOR = COLOR_CYAN;
            break;
        case 2:
        default:
            BOARD_COLOR = COLOR_GREEN;
            CURSOR_COLOR = COLOR_YELLOW;
            NUMBER_COLOR = COLOR_GREEN;
            break;
    }
    
    printf("=== 數獨遊戲 ===\n");
    printf("請選擇盤面來源：\n");
    printf("1. 從 sudoku.dat 選擇預設盤面 (1-18)\n");
    printf("2. 隨機生成新盤面\n");
    printf("請輸入選擇 (1 或 2): ");
    scanf("%d", &choice);
      if (choice == 1) {
        printf("請選擇盤面編號 (1-18): ");
        scanf("%d", &problem_index);
        if (problem_index < 1 || problem_index > 18) {
            printf("無效的盤面編號，將使用預設盤面 (編號 1)。\n");
            problem_index = 1;
        }
        
        if (read_from_binary_file(board, "sudoku.dat", problem_index - 1)) {
            // 從檔案讀取的盤面，難度設為預設2（中等）
            difficulty = 2;
            play_game(difficulty);
        } else {
            printf("\n讀取失敗！將使用預設盤面。\n");
            play_game(difficulty); // 使用預設盤面
        }
    } else if (choice == 2) {
        printf("請選擇難度 (1-簡單, 2-中等, 3-困難): ");
        scanf("%d", &difficulty);
        if (difficulty < 1 || difficulty > 3) {
            printf("無效的難度選擇，將使用中等難度。\n");
            difficulty = 2;
        }
        generate_random_sudoku(board, difficulty);
        printf("已生成隨機數獨盤面 (難度: %d)\n", difficulty);
        play_game(difficulty);
    } else {
        printf("無效的選擇，將使用預設盤面。\n");
        // 嘗試從 sudoku.dat 讀第1題 (index=0)
        difficulty = 2; // 預設難度
        if (read_from_binary_file(board, "sudoku.dat", 0)) {
            play_game(difficulty);
        } else {
            printf("\n讀取失敗！將使用內建預設盤面。\n");
            play_game(difficulty); // 使用預設盤面
        }
    }
    
    return 0;
}

// 讀取文字檔
int read_from_text_file(int board[][9], const char* filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("無法開啟檔案 %s 進行讀取\n", filename);
        return 0;
    }
    char line[64];
    int row = 0;
    while (row < 9 && fgets(line, sizeof(line), fp)) {
        for (int col = 0; col < 9; col++) {
            if (line[col] == '.') board[row][col] = 0;
            else if (line[col] >= '1' && line[col] <= '9')
                board[row][col] = line[col] - '0';
            else
                continue;
        }
        row++;
    }
    fclose(fp);
    if (row < 9) {
        printf("警告：檔案格式不正確或檔案不完整\n");
        return 0;
    }
    printf("已成功從 %s 讀取數獨盤面\n", filename);
    return 1;
}

// 讀取二進位檔，載入指定問題
int read_from_binary_file(int board[][9], const char* filename, int problem_index) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        printf("無法開啟檔案 %s 進行讀取\n", filename);
        return 0;
    }
    // 讀 header
    typedef struct {
        int numbers;
        int datasize;
    } SudokuDataHeader;

    SudokuDataHeader header;
    fread(&header, sizeof(header), 1, fp);
    printf("檔案中有 %d 個數獨問題\n", header.numbers);
    if (problem_index < 0 || problem_index >= header.numbers) {
        printf("問題編號 %d 超出範圍 (0-%d)\n", problem_index, header.numbers - 1);
        fclose(fp);
        return 0;
    }
    // 跳到指定問題位置
    fseek(fp, sizeof(header) + problem_index * header.datasize, SEEK_SET);
    SudokuProblem problem;
    fread(&problem, sizeof(problem), 1, fp);
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            board[i][j] = problem.data[i][j];
    printf("已讀取問題 ID: %d\n", problem.id);
    fclose(fp);
    return 1;
}

// 遊戲主流程
void play_game(int difficulty) {
    printf("=== 數獨遊戲 ===\n");
    // 顯示難度
    printf("難度：");
    switch (difficulty) {
        case 1:
            printf("簡單\n");
            break;
        case 2:
            printf("中等\n");
            break;
        case 3:
            printf("困難\n");
            break;
        default:
            printf("未知\n");
    }
    printf("規則：\n");
    printf("- 使用方向鍵移動光標\n");
    printf("- 按 Enter 填寫數字\n");
    printf("- 按 U 撤銷上一步操作\n");
    printf("- 按 H 獲得提示\n");
    printf("- 按 Q 結束遊戲\n");
    printf("錯誤5次遊戲結束\n\n");    // 初始化遊戲：「複製盤面」+「求解答案」+「錯誤歸零」+「動作歷史歸零」
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++) {
            original_board[i][j] = board[i][j];
            player_board[i][j] = board[i][j];
            answer_board[i][j] = board[i][j];
        }
    solve(answer_board, 0); // 求解答案盤面，但不顯示
    error_count = 0;
    move_count = 0;
    cursor_row = 0; // 初始化光標位置
    cursor_col = 0;
    printf("遊戲初始化完成！\n");
    printf("初始盤面：\n");
    print_board_with_cursor(player_board, cursor_row, cursor_col); // 顯示初始盤面
    
    // 開始計時
    start_time = time(NULL);    // 互動迴圈
    while (error_count < 5) {
        int result = handle_input(difficulty);
        if (result == -1) {
            printf("遊戲結束！\n");
            break;
        }
        if (result == 1) {
            printf("\n當前盤面：\n");
            print_board_with_cursor(player_board, cursor_row, cursor_col);
            if (is_complete()) {
                if (is_valid_solution(player_board)) {
                    // 記錄結束時間並計算經過的時間
                    end_time = time(NULL);
                    int elapsed_time = (int)difftime(end_time, start_time);
                    int hours = elapsed_time / 3600;
                    int minutes = (elapsed_time % 3600) / 60;
                    int seconds = elapsed_time % 60;
                    printf("🎉 恭喜！你完成了數獨！\n");
                    printf("您的完成時間：%02d:%02d:%02d\n", hours, minutes, seconds);
                } else {
                    printf("盤面已填滿，但答案不正確，請檢查！\n");
                }
                break;
            }
        }
    }
    if (error_count >= 5) {
        printf("💥 錯誤太多次，遊戲結束！\n");
        printf("正確答案：\n");
        print_board(answer_board); // 遊戲結束後才顯示答案
    }
}

// 處理玩家輸入
int handle_input(int difficulty) {
    printf("使用方向鍵移動光標，按 Enter 填寫數字，Q 結束遊戲，U 撤銷，H 提示\n");
    // 顯示已用時間和難度
    time_t current_time = time(NULL);
    int elapsed_time = (int)difftime(current_time, start_time);
    int hours = elapsed_time / 3600;
    int minutes = (elapsed_time % 3600) / 60;
    int seconds = elapsed_time % 60;
    
    // 顯示難度
    printf("難度：");
    switch (difficulty) {
        case 1:
            printf("簡單 | ");
            break;
        case 2:
            printf("中等 | ");
            break;
        case 3:
            printf("困難 | ");
            break;
        default:
            printf("未知 | ");
    }
    printf("已用時間：%02d:%02d:%02d\n", hours, minutes, seconds);
    
    while (1) {
        print_board_with_cursor(player_board, cursor_row, cursor_col);
        char key = getch(); // 捕捉按鍵輸入
        if (key == 'q' || key == 'Q') {
            return -1; // 結束遊戲
        } else if (key == 'u' || key == 'U') {
            undo_last_move();
            continue;
        } else if (key == 'h' || key == 'H') {
            provide_hint();
            return 1;
        } else if (key == 13) { // Enter 鍵
            int num;
            printf("請輸入數字 (1-9): ");
            scanf("%d", &num);
            if (num < 1 || num > 9) {
                printf(COLOR_RED "輸入超出範圍！請輸入 1-9 之間的數字。\n" COLOR_RESET);
                continue;
            }
            if (original_board[cursor_row][cursor_col] != 0) {
                printf(COLOR_YELLOW "該位置是原始數字，不能修改！\n" COLOR_RESET);
                continue;
            }
            if (player_board[cursor_row][cursor_col] != 0) {
                printf(COLOR_YELLOW "該位置已經填過數字了！\n" COLOR_RESET);
                continue;
            }
            if (answer_board[cursor_row][cursor_col] == num) {
                player_board[cursor_row][cursor_col] = num;
                move_history[move_count][0] = cursor_row;
                move_history[move_count][1] = cursor_col;
                move_history[move_count][2] = num;
                move_count++;
                printf(COLOR_GREEN "正確！\n" COLOR_RESET);
                return 1;
            } else {
                error_count++;
                printf(COLOR_RED "錯誤！錯誤次數：%d\n" COLOR_RESET, error_count);
                return 0;
            }
        } else if (key == -32) { // 方向鍵
            key = getch(); // 捕捉方向鍵的具體值
            switch (key) {
                case 72: // 上
                    if (cursor_row > 0) cursor_row--;
                    break;
                case 80: // 下
                    if (cursor_row < 8) cursor_row++;
                    break;
                case 75: // 左
                    if (cursor_col > 0) cursor_col--;
                    break;
                case 77: // 右
                    if (cursor_col < 8) cursor_col++;
                    break;
            }
        }
    }
}

// 新增函式以顯示盤面並突出光標位置
void print_board_with_cursor(int board[][9], int cursor_row, int cursor_col) {
    printf("\n +-------+-------+-------+\n");
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (j % 3 == 0) printf(" | ");
            else printf(" ");
            if (i == cursor_row && j == cursor_col) {
                printf("%s[%d]%s", CURSOR_COLOR, board[i][j] == 0 ? 0 : board[i][j], COLOR_RESET);
            } else {
                if (board[i][j] == 0) printf("_");
                else printf("%s%d%s", NUMBER_COLOR, board[i][j], COLOR_RESET);
            }
        }
        printf(" |\n");
        if (i % 3 == 2) printf(" +-------+-------+-------+\n");
    }
}

// 新增撤銷功能
void undo_last_move() {
    if (move_count == 0) {
        printf("無法撤銷，沒有任何動作記錄！\n");
        return;
    }
    int last_row = move_history[move_count - 1][0];
    int last_col = move_history[move_count - 1][1];
    player_board[last_row][last_col] = 0;
    move_count--;
    printf("已撤銷上一步操作。\n");
}

// 新增提示功能
void provide_hint() {
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (player_board[i][j] == 0) {
                player_board[i][j] = answer_board[i][j];
                printf("提示：位置 (%d, %d) 的正確答案是 %d\n", i + 1, j + 1, answer_board[i][j]);
                return;
            }
        }
    }
    printf("盤面已填滿，無法提供提示！\n");
}

// 檢查是否填滿
int is_complete(void) {
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            if (player_board[i][j] == 0) return 0;
    return 1;
}

// 檢查盤面是否符合 Sudoku 規則
int is_valid_solution(int puzzle[][9]) {
    int check[10];
    // 每行
    for (int i = 0; i < 9; i++) {
        for (int k = 1; k <= 9; k++) check[k] = 0;
        for (int j = 0; j < 9; j++) {
            int v = puzzle[i][j];
            if (v < 1 || v > 9 || check[v]) return 0;
            check[v] = 1;
        }
    }
    // 每列
    for (int j = 0; j < 9; j++) {
        for (int k = 1; k <= 9; k++) check[k] = 0;
        for (int i = 0; i < 9; i++) {
            int v = puzzle[i][j];
            if (v < 1 || v > 9 || check[v]) return 0;
            check[v] = 1;
        }
    }
    // 每 3x3 方塊
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

//=== 第二段：解題器（Backtracking Solver）============================

// 檢查指定數字在 row, col, 3x3 方格中是否安全
int exists_in_row(int board[][9], int row, int num) {
    for (int col = 0; col < 9; col++)
        if (board[row][col] == num) return 1;
    return 0;
}
int exists_in_col(int board[][9], int col, int num) {
    for (int row = 0; row < 9; row++)
        if (board[row][col] == num) return 1;
    return 0;
}
int exists_in_box(int board[][9], int start_row, int start_col, int num) {
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (board[start_row+i][start_col+j] == num)
                return 1;
    return 0;
}
int is_safe_place(int board[][9], int row, int col, int num) {
    return !exists_in_row(board, row, num) &&
           !exists_in_col(board, col, num) &&
           !exists_in_box(board, row-row%3, col-col%3, num);
}

// 回溯法解 Sudoku
int solve(int puzzle[][9], int pos) {
    if (pos == 81) return 1;
    int row = pos / 9;
    int col = pos % 9;
    if (puzzle[row][col] != 0) {
        return solve(puzzle, pos + 1);
    }
    for (int num = 1; num <= 9; num++) {
        if (is_safe_place(puzzle, row, col, num)) {
            puzzle[row][col] = num;
            if (solve(puzzle, pos + 1)) return 1;
            puzzle[row][col] = 0;
        }
    }
    return 0;
}

// 美化版顯示
void print_board(int board[][9]) {
    printf("\n +-------+-------+-------+\n");
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (j % 3 == 0) printf(" | ");
            else printf(" ");
            if (board[i][j] == 0) printf(COLOR_YELLOW "_" COLOR_RESET);
            else printf(COLOR_GREEN "%d" COLOR_RESET, board[i][j]);
        }
        printf(" |\n");
        if (i % 3 == 2) printf(" +-------+-------+-------+\n");
    }
}

// 另種美化顯示（Unicode 框線）
void print_board_beauty(int board[][9]) {
    printf("\n  ╔═══════╦═══════╦═══════╗\n");
    for (int i = 0; i < 9; i++) {
        printf("  ║");
        for (int j = 0; j < 9; j++) {
            if (board[i][j] == 0) printf(" · ");
            else printf(" %d ", board[i][j]);
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

//=== 第三段：使用者可選擇產生隨機題目（可擴充） =========================

// 回溯法隨機解題
int solve_sudoku(int board[][9]) {
    int row, col, is_empty = 0;
    for (row = 0; row < 9; row++) {
        for (col = 0; col < 9; col++) {
            if (board[row][col] == 0) {
                is_empty = 1;
                break;
            }
        }
        if (is_empty) break;
    }
    if (!is_empty) return 1;
    int nums[9] = {1,2,3,4,5,6,7,8,9};
    for (int i = 0; i < 9; i++) {
        int j = i + rand() % (9 - i);
        int tmp = nums[i]; nums[i] = nums[j]; nums[j] = tmp;
    }
    for (int i = 0; i < 9; i++) {
        int num = nums[i];
        if (is_safe_place(board, row, col, num)) {
            board[row][col] = num;
            if (solve_sudoku(board)) return 1;
            board[row][col] = 0;
        }
    }
    return 0;
}

// 產生隨機數獨（難度 1~3）
void generate_random_sudoku(int board[][9], int difficulty) {
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            board[i][j] = 0;
    srand(time(NULL));
    // 隨機塞 5 題
    for (int i = 0; i < 5; i++) {
        int r = rand() % 9;
        int c = rand() % 9;
        int num = 1 + rand() % 9;
        if (is_safe_place(board, r, c, num)) {
            board[r][c] = num;
        }
    }
    solve_sudoku(board);
    int cells_to_remove;
    switch (difficulty) {
        case 1: cells_to_remove = 30 + rand() % 10; break;
        case 2: cells_to_remove = 40 + rand() % 10; break;
        case 3: cells_to_remove = 50 + rand() % 10; break;
        default: cells_to_remove = 40;
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

//=== 第四段：檔案儲存功能（另作參考） ================================

void save_to_text_file(int board[][9], const char* filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        printf("無法開啟檔案 %s 進行寫入\n", filename);
        return;
    }
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (board[i][j] == 0) fprintf(fp, ".");
            else fprintf(fp, "%d", board[i][j]);
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
    printf("已成功儲存到 %s\n", filename);
}

void save_to_binary_file(int board[][9], int problem_id, const char* filename, int is_append) {
    FILE *fp;
    if (is_append) {
        fp = fopen(filename, "rb+");
        if (!fp) {
            fp = fopen(filename, "wb+");
            if (!fp) {
                printf("無法建立檔案 %s\n", filename);
                return;
            }
            typedef struct {
                int numbers;
                int datasize;
            } SudokuDataHeader;
            SudokuDataHeader header = {1, sizeof(SudokuProblem)};
            fwrite(&header, sizeof(header), 1, fp);
        } else {
            typedef struct {
                int numbers;
                int datasize;
            } SudokuDataHeader;
            SudokuDataHeader header;
            fread(&header, sizeof(header), 1, fp);
            header.numbers++;
            fseek(fp, 0, SEEK_SET);
            fwrite(&header, sizeof(header), 1, fp);
            fseek(fp, 0, SEEK_END);
        }
    } else {
        fp = fopen(filename, "wb");
        if (!fp) {
            printf("無法開啟檔案 %s 進行寫入\n", filename);
            return;
        }
        typedef struct {
            int numbers;
            int datasize;
        } SudokuDataHeader;
        SudokuDataHeader header = {1, sizeof(SudokuProblem)};
        fwrite(&header, sizeof(header), 1, fp);
    }
    typedef struct {
        int id;
        int data[9][9];
    } SudokuProblem;
    SudokuProblem problem;
    problem.id = problem_id;
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            problem.data[i][j] = board[i][j];
    fwrite(&problem, sizeof(problem), 1, fp);
    fclose(fp);
    printf("已成功儲存到二進位檔案 %s\n", filename);
}
