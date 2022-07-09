


typedef unsigned char pixel_t;


void AM_LevelInit(void) {

}

void AM_Drawer(pixel_t* fb) {
    for (int i = 0; i < 100; ++i) {
        fb[i * 321] = 56;
    }
}