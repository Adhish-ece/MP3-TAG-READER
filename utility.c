#include "utility.h"

char cache[MAX_WORD];
char fname[50];

FILE *fp;

char FrameContent[6][MAX_WORD] = {{"\0"},{"\0"},{"\0"},{"\0"},{"\0"},{"\0"}};
const char *ID3TagMap[] = {
    "TIT2",     // title
    "TPE1",     // artist
    "TALB",     // album
    "TYER",     // year
    "TCON",     // content
    "COMM"    // comments
    
};

Status Edit(OperationType op)
{
    char new[MAX_WORD];
    fp = fopen(fname,"rb");
    if(!fp)
    {
        printf("can't open file\n");
        return e_failure;
    }
    FILE *dst = fopen(DST,"wb");
    if(!dst)
    {
        printf("can't open file\n");
        return e_failure;
    }
    char head[10];
    fread(head,1,10,fp);
    fwrite(head,1,10,dst); 
    int version  = head[3];
    int is_v4 = (version==4);
    const char *targetTag=NULL;
    switch(op)
    {
        case e_edit_title:    
        targetTag = ID3TagMap[i_title]; 
        break;

        case e_edit_artist:   
        targetTag = ID3TagMap[i_artist];
        break;

        case e_edit_album:    
        targetTag = ID3TagMap[i_album]; 
        break;

        case e_edit_content:  
        targetTag = ID3TagMap[i_content]; 
        break;

        case e_edit_comments: 
        targetTag = ID3TagMap[i_comments]; 
        break;

        case e_edit_year:     
        targetTag = is_v4 ? "TDRC" : "TYER"; 
        break;

        default:
            fclose(fp);
            fclose(dst);
            remove(DST);
            return e_failure;
    }
    printf("INFO: Editing tag %s (v2.%d)\n", targetTag, version);
    printf("Enter the New Value for %s : ",targetTag);
    //getchar();
    scanf("%[^\n]",new);
    printf("%s",new);
    int found=0;
    uint new_size;
    char NewSizeBytes[4];
    while(1)
    {
        char tag[5];
        if(fread(tag,1,4,fp)!=4)
        {
            break;
        }
        if(tag[0]==0)
        {
            if(!found)
            {             
                printf("INFO: Not found frame %s, creating...\n",tag);
                found = 1;
                if(strcmp(ID3TagMap[i_comments],targetTag)==0)
                {
                    char lang[3] ="eng";//default you can change it later
                    new_size = strlen(new)+5;
                    is_v4?IntToSynsafe(new_size,NewSizeBytes):IntToBigEndian(new_size,NewSizeBytes);
                    fwrite(targetTag,1,4,dst);
                    fwrite(NewSizeBytes,1,4,dst);
                    fputc(0,dst);//flag
                    fputc(0,dst);//flag
                    fputc(0,dst);//encoding
                    fwrite(lang,1,3,dst);
                    fputc(0,dst);//null afer language
                    fwrite(new,strlen(new),1,dst);
                }
                else
                {
                    new_size = strlen(new)+1;
                    is_v4?IntToSynsafe(new_size,NewSizeBytes):IntToBigEndian(new_size,NewSizeBytes);
                    fwrite(targetTag,1,4,dst);
                    fwrite(NewSizeBytes,1,4,dst);
                    fputc(0,dst);//flag
                    fputc(0,dst);//flag
                    fputc(0,dst);//encodeing
                    fwrite(new,strlen(new),1,dst);   
                }
                
                printf("SUCCESS: Frame updated with new value : %s\n",new);
                 found = 1;
            }     
            fwrite(tag,1,4,dst);
            break;
        }
        tag[4]='\0';
        char sizeBytes[4];
        fread(sizeBytes,1,4,fp);
        uint frame_size = is_v4?SynsafeToInt(sizeBytes):BigEndianToInt(sizeBytes);
        
        char flag[2];
        fread(flag,1,2,fp);
        long frameStart = ftell(fp);
        fseek(fp,frame_size,SEEK_CUR);
        
        if(strcmp(tag,targetTag)==0)
        {
            found = 1;
            printf("INFO: Found frame %s, rewriting...\n",tag);
            found = 1;
            if(strcmp(ID3TagMap[i_comments],targetTag)==0)
            {
                char lang[3] ="eng";//default you can change it later
                new_size = strlen(new)+5;
                is_v4?IntToSynsafe(new_size,NewSizeBytes):IntToBigEndian(new_size,NewSizeBytes);
                fwrite(tag,1,4,dst);
                fwrite(NewSizeBytes,1,4,dst);
                fwrite(flag,1,2,dst);
                fputc(0,dst);//text encoding type
                fwrite(lang,1,3,dst);
                fputc(0,dst);
                fwrite(new,strlen(new),1,dst);
            }
            else
            {
                new_size = strlen(new)+1;
                is_v4?IntToSynsafe(new_size,NewSizeBytes):IntToBigEndian(new_size,NewSizeBytes);
                fwrite(tag,1,4,dst);
                fwrite(NewSizeBytes,1,4,dst);
                fwrite(flag,1,2,dst);
                fputc(0,dst);
                fwrite(new,strlen(new),1,dst);
            }
          
            printf("SUCCESS: Frame updated with new value : %s\n",new);
        }
        else
        {
            char *buf = malloc(frame_size);
            fseek(fp,frameStart,SEEK_SET);
            fread(buf,1,frame_size,fp);
            fwrite(tag,1,4,dst);
            fwrite(sizeBytes,1,4,dst);
            fwrite(flag,1,2,dst);
            fwrite(buf,frame_size,1,dst);
            free(buf);
        } 
    }
    char buffer[4096];
    uint bytes;
    while((bytes=fread(buffer,1,sizeof(buffer),fp))>0)
    {
        fwrite(buffer,1,bytes,dst);
    }
    fclose(fp);
    fclose(dst);
    char choise;
    printf("Do you to create a copy of original (y/n): ");
    scanf(" %c",&choise);
    if(choise == 'y' || choise == 'n')
    {
        if(choise == 'y')
        {
            rename(DST,"Edited.mp3");
        }
        else
        {
            remove(fname);
            rename(DST,fname);
            return e_success;
        }
    }
    else
    {
        printf("Invalid choise \n");
    }
    return e_success;
}

Status  View()
{
    char *temp = cache;
    char tag[5];
    fp = fopen(fname,"rb");
    if(!fp)
    {
        printf("Can't open the file %s\n",fname);
        return e_failure;
    }
    fseek(fp,3,SEEK_SET);
    int version,flag=0;
    fread(&version,1,1,fp);
    //printf("%d\n",version);
    fseek(fp,10,SEEK_SET);
    if(version == 4)
    {
        flag = 1;
    }
    
    while(1)
    {
        if(fread(tag,4,1,fp)!=1)
        {
            printf("Error reading the tag\n");
            return e_failure;
        }
        if(tag[0] == 0 /*|| (tag[0] != 'T' && tag[0] != 'C' && tag[0] != 'A')*/)
        {
            printf("Reached end\n");
            return e_success;
        }
        
        tag[4]='\0';
        //printf("%s",tag);

        char size[4];
        if(fread(size,4,1,fp)!=1)
        {
            printf("Error reading the size\n");
            return e_failure;
        }
        uint frame_size;
        if(flag)
        {
            frame_size = SynsafeToInt(size);
            //printf("%u\n",frame_size);
        }
        else
        {
            frame_size = BigEndianToInt(size);
            //printf("%u\n",frame_size);
        }
        

        if(frame_size>=MAX_WORD)
        {
            frame_size =MAX_WORD-1;

        }
        //printf("%u",frame_size); 
        fseek(fp,2,SEEK_CUR);
        //printf("%s %c: ",tag,tag[0]);
        //printf("%s\n",temp);
        if(fread(temp,frame_size,1,fp)!=1)
        {
            //printf("%s\n",temp);
            printf("Error reading the frame content\n");
            return e_failure;
        }
        char *contentptr = temp+1;
        if(temp[0]==0)
        {
            temp[frame_size]='\0';
            if(strcmp(ID3TagMap[i_comments],tag)==0)
            contentptr = temp+5;
        }
        else if(temp[0]==1)
        {
            char temp2[MAX_WORD];
            if(strcmp(ID3TagMap[i_comments],tag)==0)
            {
                UTFtoASCII(temp+10,temp2);
            }
            else
            {
                UTFtoASCII(temp+3,temp2);
                
            }
            contentptr = temp2;      
        }
        for(int i = 0;i<6;i++)
        {
            if(strcmp(tag,"TDRC")==0)
            {
                strcpy(FrameContent[i_year],contentptr);
            }
            if(strcmp(tag,ID3TagMap[i])==0)
            {
                strcpy(FrameContent[i],contentptr);
                //printf("done");
                break;
            }
        }
        
    }
    fclose(fp);
    
}

void UTFtoASCII(const char *utf16_data, char *ascii_outuput) 
{
    int ut16_offset = 0;
    int ascii_index = 0;

    while (ascii_index < MAX_WORD - 1)
    {
        uchar byte1 = utf16_data[ut16_offset];
        uchar byte2 = utf16_data[ut16_offset + 1];

        if (byte1 == 0x00 && byte2 == 0x00)
        {
            break;
        }
        else if (byte2 == 0x00 && byte1 != 0x00)
        {
            ascii_outuput[ascii_index++] = byte1;
        }

        else if (byte2 != 0x00 && byte1 == 0x00)
        {
            ascii_outuput[ascii_index++] = byte2;
        }

        else
        {
            // Non-ASCII character (byte2 != 0x00) or unexpected byte sequence.
            ascii_outuput[ascii_index++] = '?';
        }

        ut16_offset += 2;
    }

    ascii_outuput[ascii_index] = '\0';
}

uint SynsafeToInt(char size[4])
{
    return (size[0] << 21) + (size[1] << 14) + (size[2] << 7) + size[3];//the order of index is assending because it is stored in big edian format
}

uint BigEndianToInt(char size[4])
{
    return ((unsigned char)size[0] << 24) |
           ((unsigned char)size[1] << 16) |
           ((unsigned char)size[2] << 8)  |
           ((unsigned char)size[3]);
}

OperationType GetOperatioType(int argc,char *argv[],OperationType *flag)
{
    if(argc < 2)
        return e_invalid;

    if(strcmp(argv[1], "-h") == 0)
        return e_help;

    if(strcmp(argv[1], "-v") == 0)
    {
        if(argc < 3 || argv[2] == NULL)
            return e_invalid;
        strcpy(fname, argv[2]);
        *flag = e_view;
        return e_view;
    }

    if(strcmp(argv[1], "-e") == 0)
    {
        if(argc < 4 || argv[2] == NULL || argv[3] == NULL)
            return e_invalid;

        strcpy(fname, argv[2]);
        *flag = e_edit;

        if(strcmp(argv[3], "-t") == 0) 
        return e_edit_title;
        if(strcmp(argv[3], "-a") == 0) 
        return e_edit_artist;
        if(strcmp(argv[3], "-A") == 0) 
        return e_edit_album;
        if(strcmp(argv[3], "-y") == 0) 
        return e_edit_year;
        if(strcmp(argv[3], "-m") == 0) 
        return e_edit_content;
        if(strcmp(argv[3], "-c") == 0) 
        return e_edit_comments;

        return e_invalid;
    }

    return e_invalid;
}

void Printing_Help()
{
    printf("\n\n---------------------HELP MENU---------------------\n\n");
    printf("%-8s to view mp3 file contents\n","1. -v ->");
    printf("%-8s to edit mp3 file contents\n","2. -e ->");
    printf("%-8s 2.1. -t -> to edit song title\n"," ");
    printf("%-8s 2.2. -a -> to edit artist name\n"," ");
    printf("%-8s 2.3. -A -> to edit album name\n"," ");
    printf("%-8s 2.4. -y -> to edit year\n"," ");
    printf("%-8s 2.5. -m -> to edit content\n"," ");
    printf("%-8s 2.6. -c -> to edit comments\n"," ");
    printf("\n\n---------------------------------------------------\n\n");
}

void IntToSynsafe(uint value, char out[4])
{
    out[0] = (value >> 21) & 0x7F;
    out[1] = (value >> 14) & 0x7F;
    out[2] = (value >> 7)  & 0x7F;
    out[3] = value & 0x7F;
}

void IntToBigEndian(uint value, char out[4])
{
    out[0] = (value >> 24) & 0xFF;
    out[1] = (value >> 16) & 0xFF;
    out[2] = (value >> 8)  & 0xFF;
    out[3] = value & 0xFF;
}
