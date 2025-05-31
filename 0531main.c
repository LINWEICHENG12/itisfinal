#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    int id;
    int data[9][9];
} SudokuProblem;

typedef struct {
    int numbers;
    int datasize;
} SudokuDataHeader;

int board[9][9] = {
    {0, 0, 0, 0, 0, 0, 0, 9, 0},
    {1, 9, 0, 4, 7, 0, 6, 0, 8},
    {0, 5, 2, 8, 1, 9, 4, 0, 7},
    {2, 0, 0, 0, 4, 8, 0, 0, 0},
    {0, 0, 9, 0, 0, 0, 5, 0, 0},
    {0, 0, 0, 7, 5, 0, 0, 0, 9},
    {9, 0, 7, 3, 6, 4, 1, 8, 0},
    {5, 0, 6, 0, 8, 1, 0, 7, 4},
    {0, 8, 0, 0, 0, 0, 0, 0, 0}
};

int read_from_text_file(int board[][9], const char* filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("無法開啟檔案 %s 進行讀取\n", filename);
        return 0;
    }

    char line[20];  // 足夠容納一行的緩衝區
    int row = 0;

    while (row < 9 && fgets(line, sizeof(line), fp) != NULL) {
        for (int col = 0; col < 9; col++) {
            if (line[col] == '.') {
                board[row][col] = 0;  // 空格
            } else if (line[col] >= '1' && line[col] <= '9') {
                board[row][col] = line[col] - '0';  // 轉換字元到數字
            } else {
                continue;  // 忽略其他字元（如換行）
            }
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

int read_from_binary_file(int board[][9], const char* filename, int problem_index) {
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("無法開啟檔案 %s 進行讀取\n", filename);
        return 0;
    }

    // 讀取標頭
    SudokuDataHeader header;
    fread(&header, sizeof(header), 1, fp);

    printf("檔案中有 %d 個數獨問題\n", header.numbers);

    if (problem_index < 0 || problem_index >= header.numbers) {
        printf("問題編號 %d 超出範圍 (0-%d)\n", problem_index, header.numbers - 1);
        fclose(fp);
        return 0;
    }

    // 跳到指定的問題位置
    fseek(fp, sizeof(header) + problem_index * header.datasize, SEEK_SET);

    // 讀取問題
    SudokuProblem problem;
    fread(&problem, sizeof(problem), 1, fp);

    // 將問題資料複製到提供的板盤中
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            board[i][j] = problem.data[i][j];
        }
    }

    printf("已讀取問題 ID: %d\n", problem.id);
    fclose(fp);

    return 1;
}

void print_board(int board[][9]);

void save_to_binary_file(int board[][9], int problem_id, const char* filename, int is_append);

int solve(int puzzle[][9], int pos);

int isValid(int number, int puzzle[][9], int row, int col);
int isValid(int number, int puzzle[][9], int row, int col) {
    int rowStart = (row / 3) * 3;
    int colStart = (col / 3) * 3;
    
    for (int i = 0; i < 9; i++) {
        // 檢查同一行
        if (puzzle[row][i] == number) return 0;
        
        // 檢查同一列
        if (puzzle[i][col] == number) return 0;
        
        // 檢查 3x3 小方格
        if (puzzle[rowStart + (i / 3)][colStart + (i % 3)] == number) return 0;
    }
    
    return 1;
}

int solve(int puzzle[][9], int pos) {
    // 終止條件：所有位置都填完了
    if (pos == 81) {
        return 1;  // 成功解出
    }
    
    // 將位置編號轉換為行列座標
    int row = pos / 9;
    int col = pos % 9;
    
    // 如果該位置已有數字，跳到下一個位置
    if (puzzle[row][col] != 0) {
        return solve(puzzle, pos + 1);
    }
    
    // 嘗試填入數字 1-9
    for (int num = 1; num <= 9; num++) {
        // 檢查這個數字是否可以放在這個位置
        if (isValid(num, puzzle, row, col)) {
            // 暫時填入這個數字
            puzzle[row][col] = num;
            
            // 遞迴處理下一個位置
            if (solve(puzzle, pos + 1)) {
                return 1;  // 成功找到解答
            }
            
            // 如果遞迴失敗，回溯：清空該格
            puzzle[row][col] = 0;
        }
    }
    
    // 所有數字都試過，仍無法解出
    return 0;
}
void print_board(int board[][9]) {
    printf("\n +-------+-------+-------+\n");
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (j % 3 == 0) printf(" | ");
            else printf(" ");

            if (board[i][j] == 0) {
                printf("_");  // 用底線表示空格
            } else {
                printf("%d", board[i][j]);
            }
        }
        printf(" |\n");
        if (i % 3 == 2) printf(" +-------+-------+-------+\n");
    }
}

int main() {
    // 讀取 sudoku.dat 的第 18 題（索引從 0 開始）
    if (read_from_binary_file(board, "sudoku.dat", 17)) {
        printf("\n原始盤面：\n");
        print_board(board);
        if (solve(board, 0)) {
            printf("\n解答：\n");
            print_board(board);
        } else {
            printf("\n此盤無解！\n");
        }
        save_to_text_file(board, "output.txt"); // 儲存解答到 output.txt
    } else {
        printf("\n讀取失敗！\n");
    }
    return 0;
}

void save_to_text_file(int board[][9], const char* filename) {
    FILE *fp = fopen(filename, "w");
    if (fp == NULL) {
        printf("無法開啟檔案 %s 進行寫入\n", filename);
        return;
    }

    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            if (board[i][j] == 0) {
                fprintf(fp, ".");  // 使用點表示空格
            } else {
                fprintf(fp, "%d", board[i][j]);
            }
        }
        fprintf(fp, "\n");  // 每行結束換行
    }

    fclose(fp);
    printf("已成功儲存到 %s\n", filename);
}

void save_to_binary_file(int board[][9], int problem_id, const char* filename, int is_append) {
    FILE *fp;
    if (is_append) {
        // 檢查檔案是否存在
        fp = fopen(filename, "rb+");
        if (fp == NULL) {
            // 檔案不存在，建立新檔案
            fp = fopen(filename, "wb+");
            if (fp == NULL) {
                printf("無法建立檔案 %s\n", filename);
                return;
            }

            // 寫入新的標頭
            SudokuDataHeader header;
            header.numbers = 1;
            header.datasize = sizeof(SudokuProblem);
            fwrite(&header, sizeof(header), 1, fp);
        } else {
            // 檔案存在，更新標頭中的問題數量
            SudokuDataHeader header;
            fread(&header, sizeof(header), 1, fp);
            header.numbers++;

            // 回到檔案開頭更新標頭
            fseek(fp, 0, SEEK_SET);
            fwrite(&header, sizeof(header), 1, fp);

            // 移動到檔案末尾以添加新問題
            fseek(fp, 0, SEEK_END);
        }
    } else {
        // 建立新檔案
        fp = fopen(filename, "wb");
        if (fp == NULL) {
            printf("無法開啟檔案 %s 進行寫入\n", filename);
            return;
        }

        // 寫入標頭
        SudokuDataHeader header;
        header.numbers = 1;
        header.datasize = sizeof(SudokuProblem);
        fwrite(&header, sizeof(header), 1, fp);
    }

    // 建立並寫入問題
    SudokuProblem problem;
    problem.id = problem_id;

    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            problem.data[i][j] = board[i][j];
        }
    }

    fwrite(&problem, sizeof(problem), 1, fp);
    fclose(fp);

    printf("已成功儲存到二進位檔案 %s\n", filename);
}

// 檢查數字在某一行是否已存在
int exists_in_row(int board[][9], int row, int num) {
    for (int col = 0; col < 9; col++) {
        if (board[row][col] == num) {
            return 1;
        }
    }
    return 0;
}

// 檢查數字在某一列是否已存在
int exists_in_col(int board[][9], int col, int num) {
    for (int row = 0; row < 9; row++) {
        if (board[row][col] == num) {
            return 1;
        }
    }
    return 0;
}

// 檢查數字在 3x3 的方格內是否已存在
int exists_in_box(int board[][9], int start_row, int start_col, int num) {
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            if (board[row + start_row][col + start_col] == num) {
                return 1;
            }
        }
    }
    return 0;
}

// 檢查在某一位置放置數字是否合法
int is_safe_place(int board[][9], int row, int col, int num) {
    return !exists_in_row(board, row, num) &&
           !exists_in_col(board, col, num) &&
           !exists_in_box(board, row - row % 3, col - col % 3, num);
}

// 使用回溯法解決數獨
int solve_sudoku(int board[][9]) {
    int row, col;
    int is_empty = 0;
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
    int num_array[9] = {1,2,3,4,5,6,7,8,9};
    for (int i = 0; i < 9; i++) {
        int j = i + rand() % (9 - i);
        int temp = num_array[i];
        num_array[i] = num_array[j];
        num_array[j] = temp;
    }
    for (int i = 0; i < 9; i++) {
        int num = num_array[i];
        if (is_safe_place(board, row, col, num)) {
            board[row][col] = num;
            if (solve_sudoku(board)) return 1;
            board[row][col] = 0;
        }
    }
    return 0;
}

// 生成一個隨機的數獨盤面
void generate_random_sudoku(int board[][9], int difficulty) {
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            board[i][j] = 0;
    srand(time(NULL));
    for (int i = 0; i < 5; i++) {
        int row = rand() % 9;
        int col = rand() % 9;
        int num = 1 + rand() % 9;
        if (is_safe_place(board, row, col, num))
            board[row][col] = num;
    }
    solve_sudoku(board);
    int cells_to_remove;
    switch(difficulty) {
        case 1: cells_to_remove = 30 + rand() % 10; break;
        case 2: cells_to_remove = 40 + rand() % 10; break;
        case 3: cells_to_remove = 50 + rand() % 10; break;
        default: cells_to_remove = 40;
    }
    while (cells_to_remove > 0) {
        int row = rand() % 9;
        int col = rand() % 9;
        if (board[row][col] != 0) {
            board[row][col] = 0;
            cells_to_remove--;
        }
    }
}

// 美化輸出格式
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

// 落子與回復功能
int original_board[9][9];
int move_history[81][3];
int move_count = 0;

void init_game(int board[][9]) {
    for (int i = 0; i < 9; i++)
        for (int j = 0; j < 9; j++)
            original_board[i][j] = board[i][j];
    move_count = 0;
}

int place_number(int board[][9], int row, int col, int num) {
    if (row < 0 || row >= 9 || col < 0 || col >= 9) {
        printf("位置超出範圍！\n");
        return 0;
    }
    if (original_board[row][col] != 0) {
        printf("原始題目的格子不能修改！\n");
        return 0;
    }
    if (num < 1 || num > 9) {
        printf("數字必須在 1-9 之間！\n");
        return 0;
    }
    move_history[move_count][0] = row;
    move_history[move_count][1] = col;
    move_history[move_count][2] = board[row][col];
    move_count++;
    board[row][col] = num;
    return 1;
}

int remove_number(int board[][9], int row, int col) {
    return place_number(board, row, col, 0);
}

int undo(int board[][9]) {
    if (move_count <= 0) {
        printf("沒有可以回復的動作！\n");
        return 0;
    }
    move_count--;
    int row = move_history[move_count][0];
    int col = move_history[move_count][1];
    int old_value = move_history[move_count][2];
    board[row][col] = old_value;
    printf("已回復到上一步。\n");
    return 1;
}

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