#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "../includes/nstrUtils.h"


#define ALL_NUM "0123456789"

#define MAXSTR_INPUT 50
#define MAXINT_INPUT 10

//> Compare 2 strings.
bool nstr_compare(char s1[], char s2[]){
    size_t mn = 0;
    while(s1[mn]){
        if(!(s2[mn]) || s1[mn] != s2[mn] || (!(s1[mn+1]) & s2[mn+1])){
            return false;
        }
        
        mn += 1;
    }
    
    return true;
}

//> Duplicate the string.
char *nstrcreate(char *str){
    char *mstr = malloc(MAXSTR_INPUT * sizeof(char));
    int i = 0;
    while(str[i] || str[i] != '\0'){
        mstr[i] = str[i];
        i++;
    }
    mstr[i] = '\0';
    return mstr;
}

//> INT to STR
char digit2nstr(int num){
    return ALL_NUM[num];
}


//> Input for integer with (10 digits maximum).
void nIntInput(int *num){
    char *tem = malloc(sizeof(char) * MAXINT_INPUT);
    fgets(tem, MAXINT_INPUT, stdin);

    if(tem[0] == '\n'){
        // printf("Please enter the number");
        *num = -1;
        
        goto end;
    }

    if(sscanf(tem, "%d", num) != 1){
        printf("Invaild type (enter the number)");
        *num = 0;

        goto end;
    }

end:
    printf("\n");
    free(tem);
    return;
}

void nCharInput(char *cstr){
    fgets(cstr, MAXSTR_INPUT, stdin);
    if(cstr[0] == '\n'){
        cstr = NULL;
    } else if(cstr[1]){
        cstr = NULL;
    }
    printf("%c\n", cstr);

    return;
}


int nstrlen(char *str){
    int len = 0;
    while(str[len]){
        len++;
    }
    
    return len; 
}