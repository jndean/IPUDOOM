# ifndef __IPU_PRINT_H__
#define __IPU_PRINT_H__
#ifdef __cplusplus
extern "C" {
#endif


void reset_ipuprint();
void ipuprint(const char* str);
void ipuprintnum(int x);
void get_ipuprint_data(char* dst, int dst_size);


#ifdef __cplusplus
}
#endif 
#endif // __IPU_PRINT_H__