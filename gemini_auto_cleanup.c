#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>

#define MAX_PATH 1024
#define MAX_OUTPUT_SIZE 16384
#define TEMP_OUTPUT_FILE "/tmp/gemini_output.txt"

char* get_directory_contents(const char* path) {
    DIR *dir;
    struct dirent *entry;
    char *output = (char*)malloc(MAX_OUTPUT_SIZE);
    if (output == NULL) {
        perror("malloc");
        return NULL;
    }
    output[0] = '\0';

    dir = opendir(path);
    if (dir == NULL) {
        perror("opendir failed");
        free(output);
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        char suffix = '\0';
        if (entry->d_type == DT_DIR) {
            suffix = '/';
        }
        
        size_t name_len = strlen(entry->d_name);
        if (strlen(output) + name_len + 3 > MAX_OUTPUT_SIZE) {
            fprintf(stderr, "Error: Directory content buffer overflowed.\n");
            break;
        }

        strcat(output, entry->d_name);
        if (suffix != '\0') {
            size_t current_len = strlen(output);
            output[current_len] = suffix;
            output[current_len + 1] = '\0';
        }
        strcat(output, "  ");
    }

    closedir(dir);
    
    return output;
}

int main() {
    char current_dir[MAX_PATH];
    char *dir_contents = NULL;
    char gemini_prompt[4096]; 
    char gemini_command[8192];
    
    if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
        perror("getcwd failed");
        return 1;
    }
    
    printf("Current directory: %s\n", current_dir);

    printf("Fetching directory contents using readdir()...\n");
    dir_contents = get_directory_contents(current_dir); 
    
    if (dir_contents == NULL) {
        fprintf(stderr, "Failed to get directory contents.\n");
        return 1;
    }

    snprintf(gemini_prompt, sizeof(gemini_prompt), 
             "あなたはディレクトリ '%s' にいます。内容は:\n---\n%s---\nこの内容に基づき、まず実行すべき整理とクリーンアップのコマンドリストを以下の形式で出力してください。gemini_auto_cleanup.cとその実行ファイル、demo_setup.bash、demo_clean.sh、README.md, .gitディレクトリは移動させたり削除したりしないでください。**コマンドリストの出力後**に、整理の全体像やその他の提案を自由形式で記述してください。\n\nコマンド形式:\n'MKDIR: [新しいディレクトリ名]'\n'MOVE: [移動元ファイル/ディレクトリ名] [移動先ディレクトリ名]'\n'DELETE: [ファイル名/空ディレクトリ名]'\n\n",
             current_dir, dir_contents);
             
    snprintf(gemini_command, sizeof(gemini_command), 
             "gemini --prompt \"%s\" > %s", gemini_prompt, TEMP_OUTPUT_FILE); 

    printf("\n--- Gemini Command (to be executed) ---\n%s\n----------------------------------------\n", gemini_command);
    
    printf("Executing Gemini CLI and saving output to %s...\n", TEMP_OUTPUT_FILE);
    int status = system(gemini_command); 

    if (status == -1) {
        perror("system failed");
        free(dir_contents);
        return 1;
    } else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Gemini CLI exited with status %d\n", WEXITSTATUS(status));
        free(dir_contents);
        return 1;
    } 

    printf("\n--- Processing Gemini's output ---\n");
    
    FILE *cmd_file = fopen(TEMP_OUTPUT_FILE, "r");
    if (cmd_file == NULL) {
        perror("Failed to open Gemini output file");
        free(dir_contents);
        return 1;
    }

    char line[MAX_PATH * 2];
    int in_proposal_section = 0;
    
    while (fgets(line, sizeof(line), cmd_file) != NULL) {
        line[strcspn(line, "\n")] = 0; 
        
        if (strncmp(line, "MKDIR:", 6) == 0 || 
            strncmp(line, "MOVE:", 5) == 0 || 
            strncmp(line, "DELETE:", 7) == 0) {
            
            if (!in_proposal_section) {
                printf("\n--- Executing Commands ---\n");
            }
            in_proposal_section = 0;

            char temp_line[MAX_PATH * 2];
            strncpy(temp_line, line, sizeof(temp_line) - 1);
            temp_line[sizeof(temp_line) - 1] = '\0';
            
            char *token = strtok(temp_line, ": "); 
            char *command_type = token;
            
            if (command_type == NULL) continue; 

            char *arg1 = strtok(NULL, " "); 
            char *arg2 = (arg1 != NULL) ? strtok(NULL, " ") : NULL; 
            
            if (strcmp(command_type, "MKDIR") == 0 && arg1 != NULL) {
                printf("Attempting MKDIR: %s\n", arg1);
                if (mkdir(arg1, 0755) == -1) {
                    if (errno != EEXIST) perror("  -> mkdir failed");
                    else printf("  -> Directory already exists.\n");
                } else {
                    printf("  -> Success.\n");
                }
            } else if (strcmp(command_type, "MOVE") == 0 && arg1 != NULL && arg2 != NULL) {
                
                char new_path[MAX_PATH];
                
                const char *filename = strrchr(arg1, '/');
                if (filename == NULL) {
                    filename = arg1; 
                } else {
                    filename++; 
                }

                if (snprintf(new_path, sizeof(new_path), "%s/%s", arg2, filename) >= sizeof(new_path)) {
                    fprintf(stderr, "Error: New path too long for buffer. Skipping MOVE.\n");
                    continue;
                }
                
                printf("Attempting MOVE: %s to %s\n", arg1, new_path);

                if (rename(arg1, new_path) == -1) { 
                    perror("  -> rename failed (MOVE)");
                } else {
                    printf("  -> Success.\n");
                }
            } else if (strcmp(command_type, "DELETE") == 0 && arg1 != NULL) {
                printf("Attempting DELETE: %s\n", arg1);
                
                if (rmdir(arg1) == 0) {
                    printf("  -> Directory removed: %s\n", arg1);
                } 
                else if (errno == ENOTEMPTY || errno == EINVAL || errno == ENOTDIR) {
                     if (unlink(arg1) == 0) {
                        printf("  -> File removed: %s\n", arg1);
                    } else {
                        perror("  -> unlink failed (DELETE file)");
                    }
                }
                else {
                    perror("  -> rmdir failed (DELETE directory)");
                }
            } else {
                fprintf(stderr, "Unrecognized command format: %s\n", line);
            }
        } else {
            if (!in_proposal_section) {
                printf("\n--- Gemini's Proposals/Summary ---\n");
                in_proposal_section = 1;
            }
            printf("%s\n", line);
        }
    }

    if (in_proposal_section) {
        printf("--------------------------------------\n");
    }

    fclose(cmd_file);
    unlink(TEMP_OUTPUT_FILE);
    free(dir_contents);
    return 0;
}