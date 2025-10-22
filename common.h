#ifndef COMMON_H
#define COMMON_H
#include<stdio.h>
#include<string.h>
#include<stdlib.h>


#define MAX_WORD 256
#define DST "temp.mp3"


typedef unsigned int uint;
typedef unsigned char uchar;


extern char cache[MAX_WORD];
extern char fname[50];

extern FILE *fp;

extern char FrameContent[6][MAX_WORD];
extern const char *ID3TagMap[];




typedef enum
{
    i_title,
    i_artist,
    i_album,
    i_year,
    i_content,
    i_comments
}IDX;


typedef enum
{
    e_view,
    e_edit,
    e_edit_title,
    e_edit_artist,
    e_edit_album,
    e_edit_year,
    e_edit_content,
    e_edit_comments,
    e_invalid,
    e_help
}OperationType;

typedef enum
{
    e_success,
    e_failure
}Status;

#endif