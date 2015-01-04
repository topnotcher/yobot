#ifndef SSR_H
#define SSR_H
void ssr_init(void);
void ssr_level(int16_t);
void ssr_off(void);
uint16_t ssr_max_level(void);

#define SSR_PORT_CONCAT3(a,b,c) a##b##c
#define SSR_PIN(id) SSR_PORT_CONCAT3(PIN,id,_bm)

#define SSR_MAX_LEVEL 31249
#endif
