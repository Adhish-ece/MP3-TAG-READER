#include "utility.h"
int main(int argc, char *argv[])
{
    OperationType flag=e_invalid;//random number greater that 8
    OperationType OpType = GetOperatioType(argc,argv,&flag);
    
    if(OpType == e_view)
    {
        printf("INFO: View option is selected\n");
        if(View()==e_success)
        {
            printf("\n=====================================\n");
            printf("          MP3 Metadata Info           \n");
            printf("=====================================\n");
            printf("%-10s : %s\n", "Title", FrameContent[i_title]);
            printf("%-10s : %s\n", "Album", FrameContent[i_album]);
            printf("%-10s : %s\n", "Artist", FrameContent[i_artist]);
            printf("%-10s : %s\n", "Year", FrameContent[i_year]);
            printf("%-10s : %s\n", "Comment", FrameContent[i_comments]);
            printf("%-10s : %s\n", "Content", FrameContent[i_content]);
            printf("=====================================\n\n");

        }
       
    }
    else if(flag==e_edit)
    {
        printf("edit Option is selected\n");
        if(Edit(OpType)==e_success)
        {
            printf(":)\n");
        }
    }
    else if(OpType==e_help)
    {
        Printing_Help();
    }
    else
    {
        printf("Invalid operation, use -h for view help menu\n");
    }

}
