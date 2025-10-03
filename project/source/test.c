/*
 * id3_viewer.c
 *
 * Safe ID3v2.3 tag viewer (single-file).
 *
 * Compile:
 *   gcc -o id3view id3_viewer.c
 *
 * Run:
 *   ./id3view -v song.mp3
 *
 * Notes:
 *  - ID3 header size is synchsafe (handled by safesync()).
 *  - ID3v2.3 frame sizes are big-endian 32-bit (be32_to_uint()).
 *  - Program caps reads to MAX_FRAME_CONTENT and skips remainder of larger frames.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define MAX_FRAMES 1000
#define MAX_FRAME_CONTENT 1024

typedef uint8_t uchar;
typedef uint32_t uint;

typedef enum { view, invalid } OperationType;
typedef enum { success, failure, end_of_frame } Status;
typedef enum { IDEV1, IDEV2_2, IDEV2_3, IDEV2_4 } VersionType;

typedef struct {
    char song_fname[512];
    char identifier[4];
    uchar version;
    uchar revision;
    uchar flag;
    uchar size_bytes[4];
    uint tag_size;   // synchsafe decoded
    FILE *fp;
} Header;

typedef struct {
    char frame_id[5];
    uint frame_size;                // declared frame size
    uint frame_read;                // bytes actually read into frame_content
    uchar frame_content[MAX_FRAME_CONTENT];
} Frame;

/* prototypes */
OperationType get_operation_Type(int argc, char *argv[]);
void print_help(void);
Status get_header_from_mp3(int argc, char *argv[], Header *head);
uint safesync(const uchar size[4]);
VersionType get_version_type(const Header *header);
uint be32_to_uint(const uchar buf[4]);
Status get_frames_from_id3v23(Frame frames[], int *frame_count, Header *head);
Status decode_text_from_content(Frame *f);
int is_text_frame(const char *frame_id);
void print_frames(Frame frames[], int frame_count);

/* ---------------- main ---------------- */
int main(int argc, char *argv[])
{
    OperationType op = get_operation_Type(argc, argv);
    if (op == invalid) { print_help(); return 1; }

    Header head;
    Frame frames[MAX_FRAMES];
    int frame_count = 0;

    if (get_header_from_mp3(argc, argv, &head) != success) {
        fprintf(stderr, "Failed to read MP3/ID3 header.\n");
        return 1;
    }

    VersionType vt = get_version_type(&head);
    if (vt != IDEV2_3) {
        printf("Warning: ID3 version appears to be 2.%u (this tool focuses on v2.3). Proceeding.\n", head.version);
    } else {
        printf("ID3 version: 2.%u.%u\n", head.version, head.revision);
    }

    Status s = get_frames_from_id3v23(frames, &frame_count, &head);
    if (s == end_of_frame || (s == success && frame_count > 0)) {
        printf("Frames read from ID3v2.3 (count = %d)\n", frame_count);
        print_frames(frames, frame_count);
    } else {
        fprintf(stderr, "Error reading frames.\n");
        if (head.fp) fclose(head.fp);
        return 1;
    }

    if (head.fp) fclose(head.fp);
    return 0;
}

/* ---------------- helpers ---------------- */

OperationType get_operation_Type(int argc, char *argv[])
{
    if (argc >= 3 && strcmp(argv[1], "-v") == 0) return view;
    return invalid;
}

void print_help(void)
{
    printf("Usage: id3view -v file.mp3\n");
}

/* Read header: 'ID3' + version + revision + flags + 4 synchsafe size bytes */
Status get_header_from_mp3(int argc, char *argv[], Header *head)
{
    if (argc < 3) return failure;
    const char *fname = argv[2];
    strncpy(head->song_fname, fname, sizeof(head->song_fname)-1);
    head->song_fname[sizeof(head->song_fname)-1] = '\0';

    FILE *fp = fopen(head->song_fname, "rb");
    if (!fp) { perror("fopen"); return failure; }
    head->fp = fp;

    if (fread(head->identifier, 1, 3, fp) != 3) return failure;
    head->identifier[3] = '\0';
    if (strncmp(head->identifier, "ID3", 3) != 0) {
        // No ID3v2 tag found; set tag_size to 0
        head->version = 0; head->revision = 0; head->flag = 0; head->tag_size = 0;
        return success;
    }
    if (fread(&head->version, 1, 1, fp) != 1) return failure;
    if (fread(&head->revision, 1, 1, fp) != 1) return failure;
    if (fread(&head->flag, 1, 1, fp) != 1) return failure;
    if (fread(head->size_bytes, 1, 4, fp) != 4) return failure;

    head->tag_size = safesync(head->size_bytes);
    return success;
}

/* synchsafe decode: 7 bits per byte */
uint safesync(const uchar size[4])
{
    return (((size[0] & 0x7F) << 21) |
            ((size[1] & 0x7F) << 14) |
            ((size[2] & 0x7F) << 7)  |
            (size[3] & 0x7F));
}

VersionType get_version_type(const Header *header)
{
    if (!header) return IDEV1;
    if (strncmp(header->identifier, "ID3", 3) != 0) return IDEV1;
    if (header->version == 2) return IDEV2_2;
    if (header->version == 3) return IDEV2_3;
    if (header->version == 4) return IDEV2_4;
    return IDEV1;
}

/* big-endian 32-bit (normal) used for ID3v2.3 frame sizes */
uint be32_to_uint(const uchar buf[4])
{
    return ((uint)buf[0] << 24) | ((uint)buf[1] << 16) | ((uint)buf[2] << 8) | (uint)buf[3];
}

/* read frames safely using header->tag_size; stop at tag_end; skip leftover bytes of large frames */
Status get_frames_from_id3v23(Frame frames[], int *frame_count, Header *head)
{
    if (!head || !head->fp) return failure;
    FILE *fp = head->fp;

    long tag_start = 10; // ID3v2 header length
    long tag_end = tag_start + (long)head->tag_size;
    if (head->tag_size == 0) { *frame_count = 0; return end_of_frame; }

    if (fseek(fp, tag_start, SEEK_SET) != 0) return failure;
    *frame_count = 0;

    while (ftell(fp) < tag_end) {
        if (*frame_count >= MAX_FRAMES) { fprintf(stderr, "Max frames reached\n"); return failure; }

        Frame *f = &frames[*frame_count];
        memset(f, 0, sizeof(Frame));
        f->frame_read = 0;
        f->frame_size = 0;

        long pos = ftell(fp);
        if (pos + 10 > tag_end) return end_of_frame; // not enough for next header

        // read frame id (4 bytes)
        if (fread(f->frame_id, 1, 4, fp) != 4) return failure;
        f->frame_id[4] = '\0';

        // padding check
        if ((uchar)f->frame_id[0] == 0x00) return end_of_frame;

        // read size (4 bytes) - ID3v2.3 uses normal big-endian 32-bit sizes
        uchar size_bytes[4];
        if (fread(size_bytes, 1, 4, fp) != 4) return failure;
        f->frame_size = be32_to_uint(size_bytes);

        // skip flags (2 bytes)
        if (fseek(fp, 2, SEEK_CUR) != 0) return failure;

        // determine how many bytes we can safely read (cap at buffer and not beyond tag_end)
        long after_header = ftell(fp);
        long remaining_tag = tag_end - after_header;
        uint to_read = 0;
        if (remaining_tag <= 0) {
            to_read = 0;
        } else {
            uint max_can_read = (remaining_tag > 0) ? (uint)remaining_tag : 0;
            to_read = (f->frame_size < (MAX_FRAME_CONTENT - 1)) ? f->frame_size : (MAX_FRAME_CONTENT - 1);
            if (to_read > max_can_read) to_read = max_can_read < (MAX_FRAME_CONTENT - 1) ? max_can_read : (MAX_FRAME_CONTENT - 1);
        }

        if (to_read > 0) {
            size_t got = fread(f->frame_content, 1, to_read, fp);
            if (got != to_read) { fprintf(stderr, "Error reading content for %s\n", f->frame_id); return failure; }
            f->frame_read = (uint)got;
            if (f->frame_read < MAX_FRAME_CONTENT) f->frame_content[f->frame_read] = '\0';
        } else {
            f->frame_read = 0;
            f->frame_content[0] = '\0';
        }

        // skip remainder of frame body if larger than buffer
        long consumed = (long)f->frame_read;
        long total_frame = (long)f->frame_size;
        if (total_frame > consumed) {
            long to_skip = total_frame - consumed;
            if (ftell(fp) + to_skip > tag_end) to_skip = tag_end - ftell(fp);
            if (to_skip > 0) {
                if (fseek(fp, to_skip, SEEK_CUR) != 0) { fprintf(stderr, "Skipping failed\n"); return failure; }
            }
        }

        // decode (using exact read length)
        if (decode_text_from_content(f) != success) {
            // we treat decode failures as non-fatal; content placeholder may exist
        }

        (*frame_count)++;
    }

    return end_of_frame;
}

/* crude check whether frame is textual */
int is_text_frame(const char *frame_id)
{
    if (!frame_id) return 0;
    if (strncmp(frame_id, "COMM", 4) == 0) return 1;
    if (frame_id[0] == 'T') return 1;
    return 0;
}

/* decode textual payloads using frame_read length (no strlen). Replace non-printables with '?'
 * supports:
 *  0x00 - ISO-8859-1 (single-byte)
 *  0x01 - UTF-16 (with optional BOM)
 *  0x03 - UTF-8
 *
 * For APIC/GEOB/PIC we place a short placeholder.
 */
Status decode_text_from_content(Frame *f)
{
    if (!f) return failure;
    if (f->frame_read == 0) { f->frame_content[0] = '\0'; return success; }

    // binary frames -> placeholder
    if (strncmp(f->frame_id, "APIC", 4) == 0 || strncmp(f->frame_id, "GEOB", 4) == 0 || strncmp(f->frame_id, "PIC", 3) == 0) {
        snprintf((char*)f->frame_content, MAX_FRAME_CONTENT, "<binary frame %s, %u bytes>", f->frame_id, f->frame_size);
        return success;
    }

    uchar *body = f->frame_content;         // raw buffer (we know frame_read bytes are valid)
    uint body_len = f->frame_read;          // exactly how many bytes we read
    if (body_len == 0) { f->frame_content[0] = '\0'; return success; }

    uchar encoding = body[0];
    uchar *payload = body + 1;
    uint payload_len = (body_len >= 1) ? (body_len - 1) : 0;

    // COMM frame: skip language (3 bytes) + description (terminator varies by encoding)
    if (strncmp(f->frame_id, "COMM", 4) == 0) {
        if (payload_len < 3) { f->frame_content[0] = '\0'; return failure; }
        payload += 3;
        payload_len = (payload_len > 3) ? (payload_len - 3) : 0;

        uchar *desc_end = NULL;
        if (encoding == 0x01) {
            // UTF-16 description terminator = 0x00 0x00
            for (uint i = 0; i + 1 < payload_len; i += 2) {
                if (payload[i] == 0x00 && payload[i+1] == 0x00) { desc_end = payload + i + 2; break; }
            }
        } else {
            for (uint i = 0; i < payload_len; ++i) {
                if (payload[i] == 0x00) { desc_end = payload + i + 1; break; }
            }
        }
        if (!desc_end) { f->frame_content[0] = '\0'; return failure; }

        // move remaining text to start of buffer for simpler decoding
        uint remaining = (uint)((body + body_len) - desc_end);
        if (remaining >= MAX_FRAME_CONTENT - 1) remaining = MAX_FRAME_CONTENT - 1;
        memmove(f->frame_content + 1, desc_end, remaining);
        f->frame_content[1 + remaining] = '\0';
        // update payload pointer and length
        payload = f->frame_content + 1;
        payload_len = remaining;
        encoding = body[0]; // same encoding byte we kept in position 0 earlier
    }

    char out[MAX_FRAME_CONTENT];
    uint out_i = 0;

    if (encoding == 0x01) {
        // UTF-16: detect BOM if present
        if (payload_len >= 2) {
            uchar b0 = payload[0], b1 = payload[1];
            int be = -1; // 0 = LE, 1 = BE
            uint start = 0;
            if (b0 == 0xFF && b1 == 0xFE) { be = 0; start = 2; }
            else if (b0 == 0xFE && b1 == 0xFF) { be = 1; start = 2; }
            else be = 0; // assume LE if unknown

            for (uint i = start; i + 1 < payload_len && out_i < MAX_FRAME_CONTENT - 1; i += 2) {
                uchar hi = payload[i];
                uchar lo = payload[i+1];
                if (hi == 0x00 && lo == 0x00) break;
                uchar ch = (be ? hi : lo); // crude: pick one byte to map to ASCII-ish
                if (ch >= 32 && ch < 127) out[out_i++] = (char)ch;
                else out[out_i++] = '?';
            }
        }
    } else if (encoding == 0x03) {
        // UTF-8: keep printable ASCII, other bytes -> '?'
        for (uint i = 0; i < payload_len && out_i < MAX_FRAME_CONTENT - 1; ++i) {
            uchar ch = payload[i];
            if (ch == 0x00) break;
            if (ch >= 32 && ch < 127) out[out_i++] = (char)ch;
            else out[out_i++] = '?';
        }
    } else {
        // ISO-8859-1 / default: keep printable ASCII, map others to '?'
        for (uint i = 0; i < payload_len && out_i < MAX_FRAME_CONTENT - 1; ++i) {
            uchar ch = payload[i];
            if (ch == 0x00) break;
            if (ch >= 32 && ch < 127) out[out_i++] = (char)ch;
            else out[out_i++] = '?';
        }
    }

    out[out_i] = '\0';
    // copy decoded output back into frame_content
    memset(f->frame_content, 0, MAX_FRAME_CONTENT);
    size_t copy = strlen(out) < MAX_FRAME_CONTENT - 1 ? strlen(out) : MAX_FRAME_CONTENT - 1;
    memcpy(f->frame_content, out, copy);
    f->frame_content[copy] = '\0';
    return success;
}

/* print frames: text frames shown, binary frames summarized */
void print_frames(Frame frames[], int frame_count)
{
    for (int i = 0; i < frame_count; ++i) {
        Frame *f = &frames[i];
        if (is_text_frame(f->frame_id)) {
            printf("%-6s : %s\n", f->frame_id, f->frame_content[0] ? (char*)f->frame_content : "<empty>");
        } else {
            if (f->frame_content[0] != '\0') {
                printf("%-6s : %s\n", f->frame_id, f->frame_content);
            } else {
                printf("%-6s : <non-text frame, declared %u bytes, read %u bytes>\n", f->frame_id, f->frame_size, f->frame_read);
            }
        }
    }
}
