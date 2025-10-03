#define MAX_FRAMES 50
#define MAX_FRAME_CONTENT 512  // maximum size for each frame content

typedef struct
{
    char frame_id[5];
    uint frame_size;
    uchar frame_content[MAX_FRAME_CONTENT];  // fixed-size array instead of malloc
} Frame;

Status Get_Frames_From_ID3V2_3(Frame frames[], int *frame_count, FILE *fp)
{
    fseek(fp, 10, SEEK_SET);  // Skip ID3 header
    *frame_count = 0;

    while (1)
    {
        if (*frame_count >= MAX_FRAMES)
            return failure;

        Frame *f = &frames[*frame_count];

        // Read Frame ID
        if (fread(f->frame_id, 4, 1, fp) != 1)
            return failure;
        f->frame_id[4] = '\0';

        if (f->frame_id[0] == 0)  // Padding / end of frames
            return end_of_frame;

        // Read frame size
        uchar size_bytes[4];
        if (fread(size_bytes, 4, 1, fp) != 1)
            return failure;

        f->frame_size = (size_bytes[0] << 24) |
                        (size_bytes[1] << 16) |
                        (size_bytes[2] << 8) |
                         size_bytes[3];

        // Skip flags
        if (fseek(fp, 2, SEEK_CUR) != 0)
            return failure;

        // Read content (copy directly into fixed-size array)
        uint to_read = f->frame_size < MAX_FRAME_CONTENT ? f->frame_size : MAX_FRAME_CONTENT - 1;
        if (fread(f->frame_content, to_read, 1, fp) != 1)
            return failure;

        f->frame_content[to_read] = '\0';  // null-terminate

        (*frame_count)++;
    }

    return success;
}
