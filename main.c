#include<stdio.h>
#include<string.h>
#define MAX_FRAMES 1000
#define MAX_FRAME_CONTENT 128

typedef unsigned char uchar;
typedef unsigned int uint;

//////////////////////////////////////////////////////////////////////////////////////////////////////////


typedef enum
{
    view,
    edit,
    edit_title,
    edit_album,
    edit_artist,
    edit_year,
    edit_content,
    edit_comment,
    invalid
}OperationType;

typedef enum
{
    success,
    failure,
    end_of_frame
}Status;


typedef enum
{
    IDEV1,
    IDEV2_2,
    IDEV2_3,
    IDEV2_4
}VersionType;


////////////////////////////////////////////////////////////////////////////////////////////////////////
typedef struct
{
    char song_fname[20];
    char identifier[4];
    uchar version;
    uchar revision;
    uchar flag;
    uchar size[4];
    uint tag_size;
    FILE *fp;
    
}Header;


typedef struct
{
    char frame_id[5];
    uint frame_size;
    uchar frame_content[MAX_FRAME_CONTENT]; 
}Frame;


///////////////////////////////////////////////////////////////////////////////////////////////////////////

OperationType get_operation_Type(char **);
void Printing_Help();
Status Get_header_From_mp3(char *argv[],Header *head);
uint safesync(uchar size[4]);
VersionType get_version_type(Header *header);
Status Get_Frame_From_ID3V2_3(Frame frame[],int *frame_count, FILE *fp);
uint Big_Edian_To_Int(char size[]);
Status Decode_Text_From_Content(Frame *f);
uint Decode_UTF16_to_ASCII(const char *utf16_data,uint utf16_len,char *ascii_outuput,uint max_ascii_len);



///////////////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
   OperationType OpType = get_operation_Type(argv);
   Header head;
   Frame frame[MAX_FRAMES];
   int frame_count;

   if(OpType==view)
   {
    printf("View option is selected\n");
    if (Get_header_From_mp3(argv, &head) == success)
    {
        printf("Song header file is collected \n");

        switch (get_version_type(&head))
        {
            case IDEV1:
                printf("IDE V1\n");
                break;

            case IDEV2_2:
                printf("IDE V2_2\n");
                break;

            case IDEV2_3:
                printf("IDE V2_3\n");
                if(Get_Frame_From_ID3V2_3(frame,&frame_count,head.fp) == end_of_frame)
                {
                    printf("Frame readed from IDEV2_3\n");
                    for(int i =0;i<frame_count;i++)
                    {
                        printf("%s : %s\n",frame[i].frame_id,frame[i].frame_content);
                    }
                }
                break;

            case IDEV2_4:
                printf("IDE V2_4\n");
                break;

            default:
                printf("Unknown version type\n");
                break;
        }
    }

   }
   else
   printf("sorry");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

Status Get_Frame_From_ID3V2_3(Frame frame[],int *frame_count, FILE *fp)
{
    fseek(fp,10,SEEK_SET);
    *frame_count = 0;
    
    while(1)
    {
        if(*frame_count>=MAX_FRAMES)
        {
            printf("Maximum frame limit Error!\n");
            return failure;
        }
        Frame *f = &frame[*frame_count];
        
        if(fread(f->frame_id,4,1,fp)!=1)
        {
            printf("Error reading frame ID\n");
            return failure;
        }
        
        if(!(f->frame_id[0]=='T' || f->frame_id[0]=='C'))
        return end_of_frame;
        uchar size[4];
        if(fread(size,4,1,fp)!=1)
        {
            printf("Error While reading the size\n");
            return failure;
        }
        f->frame_size = Big_Edian_To_Int(size);
        
        fseek(fp, 2, SEEK_CUR); 

        uint to_read = f->frame_size<MAX_FRAME_CONTENT?f->frame_size:MAX_FRAME_CONTENT-1;
        if(fread(f->frame_content,to_read,1,fp)!=1)
        {
            printf("Error Getting frame contents");
            return failure;
        }
        //f->frame_content[to_read]='\0';
        Decode_Text_From_Content(f);
        (*frame_count)++;
    }
}

Status Decode_Text_From_Content(Frame *f)
{
    uchar encoding = f->frame_content[0];
    uchar *content_start = f->frame_content+1;
    uint content_len = f->frame_size-1;
    uint len = 0;

    if(strncmp(f->frame_id,"COMM",4)==0)
    {
        uint offset = 1+3;
        uint i;
        uchar *desc_end =NULL;

        if(encoding == 0x01)//utf 16
        {
            for(int i = offset;i<content_len;i+=2)
            {
                if(f->frame_content[i]==0x00 && f->frame_content[i+1]==0x00)
                {
                    desc_end = f->frame_content+i+2;
                    break;
                }
            }
        }
        else//iso 8859-1 0x00 1 byte null terminator
        {
            for(i = offset;i<content_len;i++)
            {
                if(f->frame_content[i] == 0x00)
                {
                    desc_end = f->frame_content+i+1;
                    break;
                }
            }
        }

        if(desc_end)
        {
            content_start = desc_end;
            content_len = f->frame_size-(content_start-f->frame_content);
        }
        else
        return failure;

    }
   
    
    if(encoding ==0x01)
    {
        len = Decode_UTF16_to_ASCII(content_start,content_len,(char *)content_start,MAX_FRAME_CONTENT-(content_start-f->frame_content));
    }
    int i;
    for(i =0;i<len;i++)
    {
         f->frame_content[i]=f->frame_content[i+1];
        
    }
    if(f->frame_content[i+1]!='\0')
    f->frame_content[i+1]='\0';

    return success;
}

uint Decode_UTF16_to_ASCII(const char *utf16_data,uint utf16_len,char *ascii_outuput,uint max_ascii_len)
{
    uint ut16_offset = 0;
    uint ascii_index = 0;
    if(utf16_len>2 && (utf16_data[0]==0xFF || utf16_data[0]==0xFE))
    {
        ut16_offset =2;
    }
    while(ut16_offset<utf16_len-1 && ascii_index<max_ascii_len)
    {
        uchar byte1 =utf16_data[ut16_offset];
        uchar byte2 = utf16_data[ut16_offset+1];
        if (byte1==0x00 && byte2 == 0x00)
        {
            break;
        }
        else if(byte1==0x00 && byte2 !=0x00)
        {
            ascii_outuput[ascii_index++]=byte2;
        }
        else if(byte2==0x00 && byte1 !=0x00)
        {
            ascii_outuput[ascii_index++]=byte1;
        }
        ut16_offset+=2;
    }
    ascii_outuput[ascii_index]='\0';
    return ascii_index;
}

VersionType get_version_type(Header *head)
{
    if(strcmp(head->identifier,"ID3")==0)
    {
        //printf("%u",head->version);
        if(head->version==2)
        return IDEV2_2;
        if(head->version==3)
        return IDEV2_3;
        if(head->version==4)
        return IDEV2_4;    
    }
    return IDEV1;
}

uint Big_Edian_To_Int(char buffer[])
{
    return ((buffer[0] & 0x7F) << 21) |
       ((buffer[1] & 0x7F) << 14) |
       ((buffer[2] & 0x7F) << 7)  |
       (buffer[3] & 0x7F);
}

Status Get_header_From_mp3(char *argv[],Header *head)
{
    if(strstr(argv[2],".mp3")!=NULL)
    {
        strcpy(head->song_fname,argv[2]);
        head->fp =fopen(head->song_fname,"rb");
        if(!head->fp)
        {
            printf("Error opening file\n");
            return failure;
        }
        fseek(head->fp,0,SEEK_SET);
        
        if(fread(head->identifier,3,1,head->fp)!=1)
        {
            printf("Error Reading Identifier\n");
            return failure;
        }
        head->identifier[3]='\0';
        if(fread(&head->version,1,1,head->fp)!=1)
        {
            printf("Error Reading version\n");
            return failure;
        }
         if(fread(&head->revision,1,1,head->fp)!=1)
        {
            printf("Error Reading Revesion\n");
            return failure;
        }
        if(fread(&head->flag,1,1,head->fp)!=1)
        {
            printf("Error reading flag\n");
            return failure;
        }
        if(fread(head->size,4,1,head->fp)!=1)
        {
            printf("Error reading Frame size\n");
            return failure;
        }
        head->tag_size = safesync(head->size);
        return success;
    }
    return failure;

}

uint safesync(uchar size[4])
{
    return (((size[0] & 0x7F)<<21) | ((size[1] & 0x7F)<<14) | ((size[2] & 0x7F)<<7)  | ((size[3] & 0x7F)));
}

OperationType get_operation_Type(char *argv[])
{
    if(argv[1]!=NULL)
    {
        if(strcmp(argv[1],"-v")==0)
        return view;
        else if(strcmp(argv[1],"-e")==0)
        {
            if(argv[3]!=NULL)
            {
                if(strcmp(argv[3],"-t")==0)
                return edit_title;
                else if(strcmp(argv[3],"-a")==0)
                return edit_artist;
                else if(strcmp(argv[3],"-A")==0)
                return edit_album;
                else if(strcmp(argv[3],"-y")==0)
                return edit_year;
                else if(strcmp(argv[3],"-m")==0)
                return edit_content;
                else if(strcmp(argv[3],"-c")==0)
                return edit_comment;
            }
        }   
    }
    return invalid;
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
