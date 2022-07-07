


typedef unsigned char pixel_t;

void AM_Drawer(pixel_t* fb) {
    for (int i = 0; i < 100; ++i) {
        fb[(i + 1) * 320] = 56;
    }
}