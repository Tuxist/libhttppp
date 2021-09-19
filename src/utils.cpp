/*******************************************************************************
Copyright (c) 2018, Jan Koester jan.koester@gmx.net
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include "utils.h"

const  char *libhttppp::scopy(const char* first, const char* last, char* des){
    while (first != last) {
        *des++ = *first++;
    }
    return des;
}

unsigned long libhttppp::getlen(const char *str) {
    if(!str)
        return 0;
    unsigned int len = 0;
    while ((*str++) != '\0') {
        ++len;
    }
    return len;
}

void libhttppp::rscopy(const char* first, const char* last, char** des){
    size_t rs=last-first;
    *des=new char[rs+1];
    scopy(first,last,*des);
    *des[rs]='\0';
}

void libhttppp::reverse(char s[]){
    int i, j;
    char c;
    
    for (i = 0, j = getlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}    

void libhttppp::itoa(int n, char s[]){
        int i, sign;
        if ((sign = n) < 0) 
            n = -n;
        i = 0;
        do {       /* generate digits in reverse order */
            s[i++] = n % 10 + '0';   /* get next digit */
        } while ((n /= 10) > 0);     /* delete it */
        if (sign < 0)
            s[i++] = '-';
        s[i] = '\0';
        reverse(s);
}

void libhttppp::ultoa(unsigned long n, char s[]){
        unsigned int i;
        i = 0;
        do {       /* generate digits in reverse order */
            s[i++] = n % 10 + '0';   /* get next digit */
        } while ((n /= 10) > 0);     /* delete it */
        s[i] = '\0';
        reverse(s);
}

unsigned long libhttppp::append(char** src, const char* append){
    if(!append)
        return 0;
    char *srcptr=*src;
    unsigned long srcsize=getlen((*src));
    unsigned long asize=getlen(append);
    unsigned long nsize=srcsize+asize;    
    char *buf=new char [nsize+1];
    scopy(srcptr,srcptr+srcsize,buf);
    scopy(append,append+asize,buf+srcsize);
    delete[] srcptr;
    buf[nsize]='\0';
    *src=buf;
    return nsize;
}

int libhttppp::ncompare(const char *src,size_t ssize,const char *comp,size_t csize){
        if(ssize>csize)
            return -1;
        for(int i=0; i<csize; ++i){
            if(src[i]!=comp[i])
                return i;
        }
        return 0;
}

int libhttppp::atoi(char* str){
    int sign = 1, base = 0, i = 0;
    while (str[i] == ' '){
        i++;
    }
    if (str[i] == '-' || str[i] == '+'){
        sign = 1 - 2 * (str[i++] == '-');
    }
    while (str[i] >= '0' && str[i] <= '9'){
        if (base > 255 / 10
            || (base == 255 / 10
            && str[i] - '0' > 7)){
            if (sign == 1)
                return +2147483647;
            else
                return -2147483648;
        }
        base = 10 * base + (str[i++] - '0');
    }
    return base * sign;
}
