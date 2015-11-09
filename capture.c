 /*
 *  V4L2 video capture example 
 * 
 *  This program can be used and distributed without restrictions. 
 */  
  
#include <stdio.h>  
#include <stdlib.h>  
#include <string.h>  
#include <assert.h>  
  
#include <getopt.h>             /* getopt_long() */  
  
#include <fcntl.h>              /* low-level i/o */  
#include <unistd.h>  
#include <errno.h>  
#include <malloc.h>  
#include <sys/stat.h>  
#include <sys/types.h>  
#include <sys/time.h>  
#include <sys/mman.h>  
#include <sys/ioctl.h>  
  
#include <asm/types.h>          /* for videodev2.h */  
  
#include <linux/videodev2.h>  


#define IMG_HEIGHT          480
#define IMG_WIDTH           640 

#define CLEAR(x)    memset (&(x), 0, sizeof (x))  


typedef enum {  
    IO_METHOD_READ, IO_METHOD_MMAP, IO_METHOD_USERPTR,  
} io_method;  

static io_method io = IO_METHOD_MMAP;  

struct buffer {  
    void * start;  
    size_t length;  
};  

struct buffer * buffers = NULL;  
static unsigned int n_buffers = 0;  

static char * dev_name = NULL;  
static int fd = -1;  

  
FILE *fp; 
FILE *fp_gray;
FILE *fp_yuv;
FILE *fp_two;
FILE *fp_back;

const char *filename = "test.yuv";  
const char *filename_gray = "gray.yuv"; 
const char *filename_420 = "test420.yuv";


const char *filename_two = "two_diff.yuv";  
const char *filename_back = "back_diff.yuv";

unsigned char* fbuf = NULL;



void copy(unsigned char* src, unsigned char* dest, int num)
{
    int i;
    for (i = 0; i < num; ++i)
    {
        dest[i] = src[i];
    }
}


static int two_diff(unsigned int width, unsigned int height){

    int ynum = width * height;

    fp_two = fopen(filename_two, "wa+");

    if (fp_two == NULL)
    {
        printf("file open error\n");
        return -1;
    }

    unsigned char* buf1 = (unsigned char*)malloc(ynum);
    unsigned char* buf2 = (unsigned char*)malloc(ynum);

    unsigned char* result_buf = (unsigned char*)malloc(ynum);

    memset(buf1, 0, ynum);
    memset(buf2, 0, ynum);
    memset(result_buf, 0, ynum);

    
    int i, read_num, cnt = ynum;
    int flag = -1;
    while((read_num = fread(buf1, sizeof(unsigned char), 
        ynum, fp_gray)) > 0){

        // printf("read_frame, %d\n", read_num);

        if (flag == -1)
        {
            copy(buf1, buf2, ynum);
            memset(buf1, 0, ynum);
            cnt = fread(buf1, sizeof(unsigned char), ynum, fp_gray);

            if (cnt < read_num)
            {
                read_num = cnt;
            }

            flag = 0;
        }

        

        for (i = 0; i < read_num; ++i)
        {
            result_buf[i] = 
                buf1[i] > buf2[i]? buf1[i] - buf2[i] : buf2[i] - buf1[i]; 
        }

        fwrite(result_buf, read_num, 1, fp_two);

        copy(buf1, buf2, ynum);

        memset(buf1, 0, ynum);
        memset(result_buf, 0, ynum);
    }

    free(buf1);
    free(buf2);
    free(result_buf);

    fclose(fp_two);

    return 0;
}


//
static int background_diff(unsigned int width, unsigned int height)
{
    int ynum = width * height;

    fp_back = fopen(filename_back, "wa+");

    if (fp_back == NULL)
    {
        printf("file open error\n");
        return -1;
    }

    
    unsigned char* background = (unsigned char*)malloc(ynum);

    memset(background, 0, ynum);

    int read_num, cnt;
    cnt = fread(background, sizeof(unsigned char), 
        ynum, fp_gray);

    if (read_num <= 0)
    {
        printf("fread error or file eof\n");
        return -1;
    }

    unsigned char* before = (unsigned char*)malloc(ynum);
    unsigned char* result_buf = (unsigned char*)malloc(ynum);

    memset(before, 0, ynum);
    memset(result_buf, 0, ynum);

    int i;

    while((read_num = fread(before, sizeof(unsigned char), 
        ynum, fp_gray)) > 0){

        if (read_num > cnt)
        {
           read_num = cnt;
        }

        for(i = 0; i < read_num; i++)
        {
            result_buf[i] = 
                before[i] > background[i]? before[i] - background[i] : background[i] - before[i];
        }

        fwrite(result_buf, read_num, 1, fp_back);

        memset(before, 0, ynum);
        memset(result_buf, 0, ynum);
    }

    free(before);
    free(background);
    free(result_buf);

    fclose(fp_back);

    return 0;
}

static int background_diff_n(unsigned int width, unsigned int height)
{
    int ynum = width * height;

    fp_back = fopen(filename_back, "wa+");

    if (fp_back == NULL)
    {
        printf("file open error\n");
        return -1;
    }

    
    unsigned char* background = (unsigned char*)malloc(ynum);

    memset(background, 0, ynum);

    int read_num, cnt;
    cnt = fread(background, sizeof(unsigned char), 
        ynum, fp_gray);

    if (read_num <= 0)
    {
        printf("fread error or file eof\n");
        return -1;
    }

    unsigned char* before = (unsigned char*)malloc(ynum);
    unsigned char* result_buf = (unsigned char*)malloc(ynum);

    memset(before, 0, ynum);
    memset(result_buf, 0, ynum);

    int i;

    while((read_num = fread(before, sizeof(unsigned char), 
        ynum, fp_gray)) > 0){

        if (read_num > cnt)
        {
           read_num = cnt;
        }

        for(i = 0; i < read_num; i++)
        {
            result_buf[i] = 
                before[i] > background[i]? before[i] - background[i] : background[i] - before[i];
        }

        fwrite(result_buf, read_num, 1, fp_back);

        memset(before, 0, ynum);
        memset(result_buf, 0, ynum);
    }

    free(before);
    free(background);
    free(result_buf);

    fclose(fp_back);

    return 0;
}

int main(int argc, char ** argv) 
{  

    fp_gray = fopen(filename_gray, "r"); 

    printf("the fp_gray   %p\n", fp_gray);

    if (fp_gray == NULL)
    {
        printf("file open error\n");
        return -1;
    }

    // printf("%d\n", );


    background_diff(IMG_WIDTH, IMG_HEIGHT);

    fclose(fp_gray);  
  
    exit(EXIT_SUCCESS);  
  
    return 0;  
}  


  
static void errno_exit(const char * s) 
{  
    fprintf(stderr, "%s error %d, %s/n", s, errno, strerror(errno));  
  
    exit(EXIT_FAILURE);  
}  
  
static int xioctl(int fd, int request, void * arg) 
{  
    int r;  
  
    do {  
        r = ioctl(fd, request, arg);  
    } while (-1 == r && EINTR == errno);  
  
    return r;  
}  


static int yuv422_to_yuv420(unsigned char* yuv422, unsigned char *yuv420, 
    unsigned int width, unsigned int height)
{

    //printf("yuv422_to_yuv420 start\n");

    unsigned int ynum = width * height;
    int i, j, k = 0;

    //得到Y分量
   // printf("yuv422_to_yuv420  Y start\n");
    if (yuv420 == NULL)
    {
        printf("yuv420 NULL\n");
    }

   for(i = 0; i < ynum; i++)
   {
       //printf("yuv422_to_yuv420  Y start, %d\n", i);
       yuv420[i] = yuv422[i * 2];
   }

    //得到U分量
   // printf("yuv422_to_yuv420 U start\n");
   for(i = 0; i < height; i++)
   {
       if((i % 2) != 0)
            continue;
       for(j = 0; j < (width / 2); j++)
       {
           if((4 * j + 1) > (2 * width))
                break;
           yuv420[ynum + j * 2 + k * width] = 
                    yuv422[i * 2 * width + 4 * j + 1];
        }

        k++;
   }

   // printf("yuv422_to_yuv420 V start\n");
    
    k=0;
    //得到V分量
    for(i = 0; i < height; i++)
    {
       if((i % 2) == 0)
            continue;

       for(j = 0; j < (width / 2); j++)
       {
           if((4 * j + 3) > (2 * width))
                break;
           yuv420[ynum + j * 2 + 1 + k * width] =
                    yuv422[i * 2 * width + 4 * j + 3];
          
       }
           
        k++;
    }

    return 0;
}


static int yuv422_to_gray(unsigned char* yuv422, unsigned char *gray, 
    unsigned int width, unsigned int height)
{

    unsigned int ynum = width * height;
    int i;

   for(i = 0; i < ynum; i++)
   {
       //printf("yuv422_to_yuv420  Y start, %d\n", i);
       gray[i] = yuv422[i * 2];
   }

   return 0;
}

/*
static int yuv422_to_rgb(unsigned char* yuv422, unsigned char* rgb, 
    unsigned int width, unsigned int height){

    unsigned int img_size = (width * height) << 1;

    unsigned int width_step_422 = width << 1;

    const unsigned char* p422 = yuv422;
    unsigned char* prgb = rgb;

    int R, G, B;
    int Y, U, V;

    int i, j;
    for (i = 0; i < height; ++i)
    {
        p422 = yuv422 + i * width_step_422;

        for (j = 0; j < width_step_422; j += 4)
        {
            Y = p422[j];
            U = p422[j + 1];
            V = p422[j + 3];

            R = (int)(Y + 1.4075 * (V - 128));
            G = (int)(Y - 0.3455 * (U - 128) - (0.7169 * (V - 128)));
            B = (int)(Y + 1.7790 * (U - 128));

            R = R < 0? 0 : R;
            G = G < 0? 0 : G;
            B = B < 0? 0 : B;

            R = R > 255? 255 : R;
            G = G > 255? 255 : G;
            B = B > 255? 255 : B;

            *(prgb++) = R;
            *(prgb++) = G;
            *(prgb++) = B;

            Y = p422[j + 2];

            R = (int)(Y + 1.4075 * (V - 128));
            G = (int)(Y - 0.3455 * (U - 128) - (0.7169 * (V - 128)));
            B = (int)(Y + 1.7790 * (U - 128));

            R = R < 0? 0 : R;
            G = G < 0? 0 : G;
            B = B < 0? 0 : B;

            R = R > 255? 255 : R;
            G = G > 255? 255 : G;
            B = B > 255? 255 : B;

            *(prgb++) = R;
            *(prgb++) = G;
            *(prgb++) = B;
        }
    }

    return 0;
}
*/

static void process_image(const void * p, int size) { 
    
    // printf("process_image start\n");

    int outLength = size >> 1;

    unsigned char *out = (unsigned char*)malloc(outLength);

    memset(out, 0, outLength);

    yuv422_to_gray((unsigned char*)p, out, IMG_WIDTH, IMG_HEIGHT);

    fwrite(p, size, 1, fp); 

    fwrite(out, outLength, 1, fp_gray);

    free(out);

}  
  
static int read_frame(void) {

    struct v4l2_buffer buf;  
    unsigned int i;  
  
    switch (io) {  
    case IO_METHOD_READ:  
        if (-1 == read(fd, buffers[0].start, buffers[0].length)) {  
            switch (errno) {  
            case EAGAIN:  
                return 0;  
  
            case EIO:  
                /* Could ignore EIO, see spec. */  
                /* fall through */  
            default:  
                errno_exit("read");  
            }  
        }  
  
        process_image(buffers[0].start, buffers[0].length);  
  
        break;  
  
    case IO_METHOD_MMAP:  

        CLEAR(buf);  
  
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
        buf.memory = V4L2_MEMORY_MMAP;  
  
        if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {  
            switch (errno) {  
            case EAGAIN:  
                return 0;  
  
            case EIO:  
                /* Could ignore EIO, see spec. */  
  
                /* fall through */  
  
            default:  
                errno_exit("VIDIOC_DQBUF");  
            }  
        }  
  
        assert(buf.index < n_buffers);  
  
        process_image(buffers[buf.index].start, buffers[buf.index].length);  
  
        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))  
            errno_exit("VIDIOC_QBUF");  
  
        break;  
  
    case IO_METHOD_USERPTR:  
        CLEAR(buf);  
  
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
        buf.memory = V4L2_MEMORY_USERPTR;  
  
        if (-1 == xioctl(fd, VIDIOC_DQBUF, &buf)) {  
            switch (errno) {  
            case EAGAIN:  
                return 0;  
  
            case EIO:  
                /* Could ignore EIO, see spec. */  
  
                /* fall through */  
  
            default:  
                errno_exit("VIDIOC_DQBUF");  
            }  
        }  
  
        for (i = 0; i < n_buffers; ++i)  
            if (buf.m.userptr == (unsigned long) buffers[i].start  
                    && buf.length == buffers[i].length)  
                break;  
  
        assert(i < n_buffers);  
  
        process_image((void *) buf.m.userptr, buf.length);  
  
        if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))  
            errno_exit("VIDIOC_QBUF");  
  
        break;  
    }  
  
    return 1;  
}  

  
static void mainloop(void) {  

    unsigned int count;  
  
    count = 100;  
  
    while (count-- > 0) {  
        for (;;) {  

            fd_set fds;  
            struct timeval tv;  
            int r;  
  
            FD_ZERO(&fds);  
            FD_SET(fd, &fds);  
  
            /* Timeout. */  
            tv.tv_sec = 3;  
            tv.tv_usec = 0;  
  
            r = select(fd + 1, &fds, NULL, NULL, &tv);  
  
            if (-1 == r) {  
                if (EINTR == errno)  
                    continue;  
  
                errno_exit("select");  
            }  
  
            if (0 == r) {  
                fprintf(stderr, "select timeout/n");  
                exit(EXIT_FAILURE);  
            }  
  
            if (read_frame())  
                break;  
  
            /* EAGAIN - continue select loop. */  
        }  
    }  
}  

  
static void start_capturing(void) {  

    unsigned int i;  
    enum v4l2_buf_type type;  
  
    switch (io) {  
    case IO_METHOD_READ:  
        /* Nothing to do. */  
        break;  
  
    case IO_METHOD_MMAP:  

        for (i = 0; i < n_buffers; ++i) {  

            struct v4l2_buffer buf;  
  
            CLEAR(buf);  
  
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
            buf.memory = V4L2_MEMORY_MMAP;  
            buf.index = i;  
  
            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf)) {//将拍摄的图像数据放入缓存队列
                errno_exit("VIDIOC_QBUF"); 
            } 
                 
        }  
  
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
  
        if (-1 == xioctl(fd, VIDIOC_STREAMON, &type)) {

            errno_exit("VIDIOC_STREAMON");  
        } 
            
  
        break;  
  
    case IO_METHOD_USERPTR:  
        for (i = 0; i < n_buffers; ++i) {  
            struct v4l2_buffer buf;  
  
            CLEAR(buf);  
  
            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
            buf.memory = V4L2_MEMORY_USERPTR;  
            buf.index = i;  
            buf.m.userptr = (unsigned long) buffers[i].start;  
            buf.length = buffers[i].length;  
  
            if (-1 == xioctl(fd, VIDIOC_QBUF, &buf))  
                errno_exit("VIDIOC_QBUF");  
        }  
  
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
  
        if (-1 == xioctl(fd, VIDIOC_STREAMON, &type))  
            errno_exit("VIDIOC_STREAMON");  
  
        break;  
    } 

    printf("start capture finish\n"); 
}  
  

  
static void init_read(unsigned int buffer_size) {  
    buffers = (struct buffer *)calloc(1, sizeof(struct buffer ));  
  
    if (!buffers) {  
        fprintf(stderr, "Out of memory/n");  
        exit(EXIT_FAILURE);  
    }  
  
    buffers[0].length = buffer_size;  
    buffers[0].start = malloc(buffer_size);  
  
    if (!buffers[0].start) {  
        fprintf(stderr, "Out of memory/n");  
        exit(EXIT_FAILURE);  
    }  
} 

  
static void init_mmap(void) {  

    struct v4l2_requestbuffers req;  //缓冲区结构体
  
    CLEAR(req);  
  
    req.count = 4;  //缓存数量，设置缓存区队列里保持多少张照片
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
    req.memory = V4L2_MEMORY_MMAP;  //方法为内存映射方法
  
    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {  
        if (EINVAL == errno) {  
            fprintf(stderr, "%s does not support "  
                    "memory mapping/n", dev_name);  
            exit(EXIT_FAILURE);  
        } else {  
            errno_exit("VIDIOC_REQBUFS");  
        }  
    }  
  
    if (req.count < 2) {  
        fprintf(stderr, "Insufficient buffer memory on %s/n", dev_name);  
        exit(EXIT_FAILURE);  
    }  
  
    buffers = (struct buffer *)calloc(req.count, sizeof(struct buffer ));  
  
    if (!buffers) {

        fprintf(stderr, "Out of memory/n");  
        exit(EXIT_FAILURE);  
    }  
  
    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {  
        struct v4l2_buffer buf;  
  
        CLEAR(buf);  
  
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
        buf.memory = V4L2_MEMORY_MMAP;  
        buf.index = n_buffers;  
  
        if (-1 == xioctl(fd, VIDIOC_QUERYBUF, &buf)) {
            errno_exit("VIDIOC_QUERYBUF");  
        } 
            
  
        buffers[n_buffers].length = buf.length;  
        buffers[n_buffers].start = mmap(NULL /* start anywhere */, buf.length,  
                PROT_READ | PROT_WRITE /* required */,  
                MAP_SHARED /* recommended */, fd, buf.m.offset);  //缓冲区起始存储位置
  
        if (MAP_FAILED == buffers[n_buffers].start){
            errno_exit("mmap");  
        }  
            
    }  
}  
  
static void init_userp(unsigned int buffer_size) {  
    struct v4l2_requestbuffers req;  
    unsigned int page_size;  
  
    page_size = getpagesize();  
    buffer_size = (buffer_size + page_size - 1) & ~(page_size - 1);  
  
    CLEAR(req);  
  
    req.count = 4;  
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
    req.memory = V4L2_MEMORY_USERPTR;  
  
    if (-1 == xioctl(fd, VIDIOC_REQBUFS, &req)) {  
        if (EINVAL == errno) {  
            fprintf(stderr, "%s does not support "  
                    "user pointer i/o/n", dev_name);  
            exit(EXIT_FAILURE);  
        } else {  
            errno_exit("VIDIOC_REQBUFS");  
        }  
    }  
  
    buffers = (struct buffer *)calloc(4, sizeof(struct buffer ));  
  
    if (!buffers) {  
        fprintf(stderr, "Out of memory/n");  
        exit(EXIT_FAILURE);  
    }  
  
    for (n_buffers = 0; n_buffers < 4; ++n_buffers) {  
        buffers[n_buffers].length = buffer_size;  
        buffers[n_buffers].start = memalign(/* boundary */page_size,  
                buffer_size);  
  
        if (!buffers[n_buffers].start) {  
            fprintf(stderr, "Out of memory/n");  
            exit(EXIT_FAILURE);  
        }  
    }  
}  
  
static void init_device(void) { 

    struct v4l2_capability cap;  
    
    //query device capability
    if (-1 == xioctl(fd, VIDIOC_QUERYCAP, &cap)) {  
        if (EINVAL == errno) {  
            fprintf(stderr, "%s is no V4L2 device/n", dev_name);  
            exit(EXIT_FAILURE);  
        } else {  
            errno_exit("VIDIOC_QUERYCAP");  
        }  
    }  
  
    if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) {  
        fprintf(stderr, "%s is no video capture device/n", dev_name);  
        exit(EXIT_FAILURE);  
    }  
  

    switch (io) {  
        case IO_METHOD_READ:  
            if (!(cap.capabilities & V4L2_CAP_READWRITE)) {  
                fprintf(stderr, "%s does not support read i/o/n", dev_name);  
                exit(EXIT_FAILURE);  
            }  
      
            break;  
      
        case IO_METHOD_MMAP:  
        case IO_METHOD_USERPTR:  
            if (!(cap.capabilities & V4L2_CAP_STREAMING)) { //是否支持流操作，支持数据流控制 
                fprintf(stderr, "%s does not support streaming i/o/n", dev_name);  
                exit(EXIT_FAILURE);  
            }  
      
            break;  
    }  
  
    /* Select video input, video standard and tune here. */  
    struct v4l2_cropcap cropcap;
    struct v4l2_crop crop;

    CLEAR(cropcap);  
  
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  //表示支持图像获取 
  
    if (0 == xioctl (fd, VIDIOC_CROPCAP, &cropcap)) {
        crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        crop.c = cropcap.defrect; /* reset to default */

        if (-1 == xioctl (fd, VIDIOC_S_CROP, &crop)) {
                switch (errno) {
                case EINVAL:
                        /* Cropping not supported. */
                        break;
                default:
                        /* Errors ignored. */
                        break;
                }
        }
    } else {    
        /* Errors ignored. */
    }

    struct v4l2_format fmt;  
    unsigned int min;  

    CLEAR(fmt);  
  
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
    fmt.fmt.pix.width = IMG_WIDTH;  
    fmt.fmt.pix.height = IMG_HEIGHT;  
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;  
    fmt.fmt.pix.field = V4L2_FIELD_NONE;  
  
    if (-1 == xioctl(fd, VIDIOC_S_FMT, &fmt))
    {
        errno_exit("VIDIOC_S_FMT"); 
    }  
         
  
    /* Note VIDIOC_S_FMT may change width and height. */  
  
    /* Buggy driver paranoia. */  
    min = fmt.fmt.pix.width * 2;  
    if (fmt.fmt.pix.bytesperline < min)
    {
        fmt.fmt.pix.bytesperline = min;
    }  
          
    min = fmt.fmt.pix.bytesperline * fmt.fmt.pix.height;  
    if (fmt.fmt.pix.sizeimage < min)
    {
        fmt.fmt.pix.sizeimage = min;
    }  
         
  
    switch (io) 
    {  
        case IO_METHOD_READ:  
            init_read(fmt.fmt.pix.sizeimage);  
            break;  
      
        case IO_METHOD_MMAP:  
            init_mmap();  
            break;  
      
        case IO_METHOD_USERPTR:  
            init_userp(fmt.fmt.pix.sizeimage);  
            break;  
    }  

    printf("init device finish\n");
}  
  

  
static void open_device(void) {  

    struct stat st;  
  
    if (-1 == stat(dev_name, &st)) {  
        fprintf(stderr, "Cannot identify '%s': %d, %s/n", dev_name, errno,  
                strerror(errno));  
        exit(EXIT_FAILURE);  
    }  
  
    if (!S_ISCHR(st.st_mode)) {  
        fprintf(stderr, "%s is no device/n", dev_name);  
        exit(EXIT_FAILURE);  
    }  
  
    fd = open(dev_name, O_RDWR /* required */| O_NONBLOCK, 0);  
  
    if (-1 == fd) {  
        fprintf(stderr, "Cannot open '%s': %d, %s/n", dev_name, errno,  
                strerror(errno));  
        exit(EXIT_FAILURE);  
    } 

    printf("open device finish\n"); 

}  

static void stop_capturing(void) {  
    enum v4l2_buf_type type;  
  
    switch (io) {  
    case IO_METHOD_READ:  
        /* Nothing to do. */  
        break;  
  
    case IO_METHOD_MMAP:  
    case IO_METHOD_USERPTR:  
        type = V4L2_BUF_TYPE_VIDEO_CAPTURE;  
  
        if (-1 == xioctl(fd, VIDIOC_STREAMOFF, &type))  
            errno_exit("VIDIOC_STREAMOFF");  
  
        break;  
    }  
}  


static void uninit_device(void) {  
    unsigned int i;  
  
    switch (io) {  
    case IO_METHOD_READ:  
        free(buffers[0].start);  
        break;  
  
    case IO_METHOD_MMAP:  
        for (i = 0; i < n_buffers; ++i)  
            if (-1 == munmap(buffers[i].start, buffers[i].length))  
                errno_exit("munmap");  
        break;  
  
    case IO_METHOD_USERPTR:  
        for (i = 0; i < n_buffers; ++i)  
            free(buffers[i].start);  
        break;  
    }  
  
    free(buffers);  
}  

static void close_device(void) {  

    if (-1 == close(fd))  
        errno_exit("close");  
  
    fd = -1;  
}  
  
static void usage(FILE * fp, int argc, char ** argv) {  
    fprintf(fp, "Usage: %s [options]/n/n"  
            "Options:/n"  
            "-d | --device name   Video device name [/dev/video]/n"  
            "-h | --help          Print this message/n"  
            "-m | --mmap          Use memory mapped buffers/n"  
            "-r | --read          Use read() calls/n"  
            "-u | --userp         Use application allocated buffers/n"  
            "", argv[0]);  
}  
  
static const char short_options[] = "d:hmru";  
  
static const struct option long_options[] = { { "device", required_argument,  
        NULL, 'd' }, { "help", no_argument, NULL, 'h' }, { "mmap", no_argument,  
        NULL, 'm' }, { "read", no_argument, NULL, 'r' }, { "userp", no_argument,  
        NULL, 'u' }, { 0, 0, 0, 0 } };  
 
/*
int main(int argc, char ** argv) {  

    dev_name = "/dev/video0";  
  
    for (;;) {  
        int index;  
        int c;  
  
        c = getopt_long(argc, argv, short_options, long_options, &index);  
  
        if (-1 == c)  
            break;  
  
        switch (c) {  
        case 0: 
            break;  
  
        case 'd':  
            dev_name = optarg;  
            break;  
  
        case 'h':  
            usage(stdout, argc, argv);  
            exit(EXIT_SUCCESS);  
  
        case 'm':  
            io = IO_METHOD_MMAP;  
            break;  
  
        case 'r':  
            io = IO_METHOD_READ;  
            break;  
  
        case 'u':  
            io = IO_METHOD_USERPTR;  
            break;  
  
        default:  
            usage(stderr, argc, argv);  
            exit(EXIT_FAILURE);  
        }  
    }  

    fp = fopen(filename, "wa+");
    if (fp == NULL)
    {
        printf("file open error\n");
        return -1;
    }

    fp_gray = fopen(filename_gray, "wa+"); 
    if (fp_dest == NULL)
    {
        printf("file open error\n");
        return -1;
    }

    open_device();  
  
    init_device();  
  
    start_capturing();  
  
    

    mainloop();

    fclose(fp);  
    fclose(fp_dest);
    //fclose(fp_yuv);

    stop_capturing();  
  
    uninit_device();  
  
    close_device();  
  
    exit(EXIT_SUCCESS);  
  
    return 0;  
}  
*/