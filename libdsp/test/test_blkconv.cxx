#include "blkconv.h"
#include <stdio.h>

int main()
{
    float taps[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    blkconv conv(taps, sizeof(taps)/sizeof(float), 32);
    float *in = conv.get_process_buf();
    int len = conv.get_blksize();
    int i;
    printf("blksize = %d\n", len);
    for (i=0; i<len; i++)
    {
        in[i] = 1.0;
    }

    conv.process();
    for (i=0; i<len;i++)
    {
        printf("%.2f\n", in[i]);
    }
    

    for (i=0; i<len; i++)
    {
        in[i] = 0.0f;
    }
    
    conv.process();
    for (i=0; i<len;i++)
    {
        printf("%.2f\n", in[i]);
    }
}
