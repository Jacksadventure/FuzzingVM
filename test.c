#include <cstdint>
extern int rnd();
uint32_t MAX_DEPTH = 5;
void b(int depth){
    if(depth > MAX_DEPTH) return;
    switch  (rnd(2)){
        case 1:
            j(depth+1);
            break;
        case 2:
            k(depth+1);
            break;
    }
    return ;
}
void d(int depth){return ;}
void c(int depth){return ;}
void g(int depth){return ;}
void j(int depth){return ;}
void k(int depth){return ;}
void a(int depth){
    if(depth > MAX_DEPTH) return;
    switch  (rnd(3)){
        case 1:
            b(depth+1);
            break;
        case 2:
            d(depth+1);
            c(depth+1);
            break;
    }
    return ;
}
a(0);