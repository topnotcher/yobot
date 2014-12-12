#ifndef SSR_H
#define SSR_H
void ssr_init(void);
void ssr_on(void);
void ssr_off(void);

#define SSR_PORT_CONCAT3(a,b,c) a##b##c
#define SSR_PIN(id) SSR_PORT_CONCAT3(PIN,id,_bm)

#endif
