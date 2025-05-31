#include <stdio.h>
#include <stdlib.h>
#include <time.h>

//--- 函式原型宣告（prototype） -----------------------------------
void print_board(int board[][9]);
void print_board_beauty(int board[][9]);
void play_game(void);
int solve(int puzzle[][9], int pos);
int handle_input(void);
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

//=== 第一段：檔案讀取、遊戲初始化、主程式 =================================

int main() {
    // 嘗試從 sudoku.dat 讀第 18 題 (index=17)
    if (read_from_binary_file(board, "sudoku.dat", 17)) {
        play_game();
    } else {
        printf("\n讀取失敗！\n");
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
    typedef struct {
        int id;
        int data[9][9];
    } SudokuProblem;

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
void play_game() {
    printf("=== 數獨遊戲 ===\n");
    printf("規則：輸入 行 列 數字 來填數字\n");
    printf("錯誤5次遊戲結束\n\n");

    // 初始化遊戲：「複製盤面」+「求解答案」+「錯誤歸零」+「動作歷史歸零」
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++) {
            original_board[i][j] = board[i][j];
            player_board[i][j] = board[i][j];
            answer_board[i][j] = board[i][j];
        }
    solve(answer_board, 0);
    error_count = 0;
    move_count = 0;
    printf("遊戲初始化完成！\n");

    printf("初始盤面：\n");
    print_board(player_board);

    // 互動迴圈
    while (error_count < 5) {
        int result = handle_input();
        if (result == -1) {
            printf("遊戲結束！\n");
            break;
        }
        if (result == 1) {
            printf("\n當前盤面：\n");
            print_board(player_board);
            if (is_complete()) {
                if (is_valid_solution(player_board)) {
                    printf("🎉 恭喜！你完成了數獨！\n");
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
        print_board(answer_board);
    }
}

// 處理玩家輸入
int handle_input(void) {
    int row, col, num;
    printf("請輸入 行 列 數字 (1-9)，或輸入 0 0 0 結束遊戲: ");
    scanf("%d %d %d", &row, &col, &num);
    if (row == 0 && col == 0 && num == 0) {
        return -1; // 結束
    }
    if (row < 1 || row > 9 || col < 1 || col > 9 || num < 1 || num > 9) {
        printf("輸入超出範圍！請輸入 1-9 之間的數字。\n");
        return 0;
    }
    row--; col--;
    if (original_board[row][col] != 0) {
        printf("該位置是原始數字，不能修改！\n");
        return 0;
    }
    if (player_board[row][col] != 0) {
        printf("該位置已經填過數字了！\n");
        return 0;
    }
    if (answer_board[row][col] == num) {
        player_board[row][col] = num;
        printf("正確！\n");
    } else {
        error_count++;
        printf("錯誤！錯誤次數：%d\n", error_count);
    }
    return 1;
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
            if (board[i][j] == 0) printf("_");
            else printf("%d", board[i][j]);
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
            typedef struct {
                int id;
                int data[9][9];
            } SudokuProblem;
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
