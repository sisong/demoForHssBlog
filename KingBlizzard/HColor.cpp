//HColor.cpp
#include "HColor.h"

static unsigned char _color_table[256*3];
const unsigned char* color_table=&_color_table[256];
class _CAuto_inti_color_table
{
private:
    static inline long border_color(long x)
    {
        if (x<0) return 0;
        else if (x>255) return 255;
        else return x;
    }
public:
    _CAuto_inti_color_table() {
        for (int i=0;i<256*3;++i)
            _color_table[i]=(unsigned char)border_color(i-256);
    }
};
static _CAuto_inti_color_table _Auto_inti_color_table;