/**
    The state machine that separates the arguments for the command by spaces and quotes
    @author ak8072
**/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//The current state of the state machine
enum state {
    WHITESPACE,
    CHAR,
    QUOTE
};

/**
    Returns a string that starts at the given index at the length given
    @param str - The source string you want to get a part of
    @param start - The starting index of the substring
    @param length - The length of the substring
    @return - The specified substring
**/
char* strsub(char* str, int start, int length) {
    char* output = calloc(length + 1, sizeof(char));
    for (int i = start; i < (start + length) && i < strlen(str); i++) {
        output[i - start] = str[i];
    }
    return output;
}

/**
    Separates a string by spaces or quotes
    @param str - The source string you want split
**/
void split_string(char* str) {
    int len = strlen(str);
    int count = 0;
    enum state current_state = WHITESPACE;
    int start = 0;
    int length = 1;
    
    for (int i = 0; i <= len; i++) {
        if (current_state == WHITESPACE) {
            if (str[i] == '"' && i < len - 1) {
                current_state = QUOTE;
                start = i + 1;
                length = 1;
            } else if (str[i] != ' ' && str[i] != '\t') {
                current_state = CHAR;
                start = i;
                length = 1;
            }
        } else if (current_state == CHAR) {
            length++;
            if (str[i] == ' ' || str[i] == '\t' || str[i] == '\0') {
                current_state = WHITESPACE;
                count++;
                printf("[%d] %s\n", count, strsub(str, start, length));
            }
        } else {
            if (str[i] != '"' && str[i] != '\0') {
                length++;
            } else {
                current_state = WHITESPACE;
                count++;
                printf("[%d] %s\n", count, strsub(str, start, --length));
            }
        }
    }
}