#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

#define STACK_SIZE 1024
#define BUFFER_SIZE (4 * 1024 * 1024)

uint32_t stack[STACK_SIZE];
int top_index = -1;
char buffer[BUFFER_SIZE];
uint32_t callStack[STACK_SIZE];
int call_top = -1; // 调用栈顶
uint32_t debug_num = 0; // 用于DT_SEEK

int32_t ip = -1; // Instruction pointer
float to_float(uint32_t val) {
    union { uint32_t i; float f; } u;
    u.i = val; return u.f;
}
uint32_t from_float(float f) {
    union { uint32_t i; float f; } u;
    u.f = f; return u.i;
}

#define NEXT goto *labels[++ip]

static inline void do_add() {
    uint32_t a = stack[top_index--];
    uint32_t b = stack[top_index--];
    stack[++top_index] = a + b;
}

static inline void do_sub() {
    uint32_t a = stack[top_index--];
    uint32_t b = stack[top_index--];
    stack[++top_index] = b - a;
}

static inline void do_mul() {
    uint32_t a = stack[top_index--];
    uint32_t b = stack[top_index--];
    stack[++top_index] = a * b;
}

static inline void do_div() {
    uint32_t a = stack[top_index--];
    uint32_t b = stack[top_index--];
    if(a == 0) { fprintf(stderr, "Error: Division by zero\n"); exit(1); }
    stack[++top_index] = b / a;
}

static inline void do_shl() {
    uint32_t shift = stack[top_index--];
    uint32_t value = stack[top_index--];
    stack[++top_index] = value << shift;
}

static inline void do_shr() {
    uint32_t shift = stack[top_index--];
    uint32_t value = stack[top_index--];
    stack[++top_index] = value >> shift;
}

static inline void do_fp_add() {
    float a = to_float(stack[top_index--]);
    float b = to_float(stack[top_index--]);
    stack[++top_index] = from_float(a + b);
}

static inline void do_fp_sub() {
    float a = to_float(stack[top_index--]);
    float b = to_float(stack[top_index--]);
    stack[++top_index] = from_float(b - a);
}

static inline void do_fp_mul() {
    float a = to_float(stack[top_index--]);
    float b = to_float(stack[top_index--]);
    stack[++top_index] = from_float(a * b);
}

static inline void do_fp_div() {
    float a = to_float(stack[top_index--]);
    float b = to_float(stack[top_index--]);
    if(a == 0.0f) { fprintf(stderr, "Error: Division by zero\n"); exit(1); }
    stack[++top_index] = from_float(b / a);
}

static inline void do_end() {
    top_index = -1;
    memset(buffer, 0, BUFFER_SIZE);
}

static inline void do_lod(uint32_t offset) {
    uint32_t value;
    memcpy(&value, buffer + offset, sizeof(uint32_t));
    stack[++top_index] = value;
}

static inline void do_sto(uint32_t offset) {
    uint32_t value = stack[top_index--];
    memcpy(buffer + offset, &value, sizeof(uint32_t));
}

static inline void do_immi(uint32_t value) {
    stack[++top_index] = value;
}

static inline void do_inc() {
    uint32_t value = stack[top_index--];
    stack[++top_index] = value + 1;
}

static inline void do_dec() {
    uint32_t value = stack[top_index--];
    stack[++top_index] = value - 1;
}

static inline void do_sto_immi(uint32_t offset, uint32_t number) {
    memcpy(buffer + offset, &number, sizeof(uint32_t));
}

static inline void do_memcpy(uint32_t dest, uint32_t src, uint32_t len) {
    memcpy(buffer + dest, buffer + src, len);
}

static inline void do_memset(uint32_t dest, uint32_t val, uint32_t len) {
    memset(buffer + dest, val, len);
}

static inline void do_gt() {
    uint32_t a = stack[top_index--];
    uint32_t b = stack[top_index--];
    stack[++top_index] = (b > a) ? 1 : 0;
}

static inline void do_lt() {
    uint32_t a = stack[top_index--];
    uint32_t b = stack[top_index--];
    stack[++top_index] = (b < a) ? 1 : 0;
}

static inline void do_eq() {
    uint32_t a = stack[top_index--];
    uint32_t b = stack[top_index--];
    stack[++top_index] = (b == a) ? 1 : 0;
}

static inline void do_gt_eq() {
    uint32_t a = stack[top_index--];
    uint32_t b = stack[top_index--];
    stack[++top_index] = (b >= a) ? 1 : 0;
}

static inline void do_lt_eq() {
    uint32_t a = stack[top_index--];
    uint32_t b = stack[top_index--];
    stack[++top_index] = (b <= a) ? 1 : 0;
}

static inline void do_print() {
    if (top_index >= 0) {
        printf("%u\n", stack[top_index]);
    } else { fprintf(stderr, "Stack is empty.\n"); }
}

static inline void do_read_int(uint32_t offset) {
    uint32_t val;
    scanf("%u", &val);
    memcpy(buffer + offset, &val, sizeof(uint32_t));
}

static inline void do_fp_print() {
    if (top_index >= 0) {
        float f = to_float(stack[top_index]);
        printf("%f\n", f);
    } else { fprintf(stderr, "Stack is empty.\n"); }
}

static inline void do_fp_read(uint32_t offset) {
    float val;
    scanf("%f", &val);
    uint32_t ival = from_float(val);
    memcpy(buffer + offset, &ival, sizeof(uint32_t));
}

static inline void do_call(int target_inst) {
    callStack[++call_top] = ip;
}

static inline int do_ret() {
    if (call_top >= 0) {
        return callStack[call_top--];
    } else {
        fprintf(stderr, "Error: Return without call\n");
        exit(1);
    }
    return -1;
}

static inline void do_tik() { printf("tik\n"); }

static inline void do_seek(uint32_t value) {
    debug_num = value;
}

static inline void do_rnd() {
    uint32_t max = stack[top_index--];
    if (max == 0) {
        stack[++top_index] = 0;
        return;
    }
    stack[++top_index] = rand() % max;
}

int main() {
    // 初始化随机数生成器
    srand((unsigned int)time(NULL));

    // Label pointer array for computed goto
    void* labels[] = {
        &&L0, // Opcode: 19
        &&L1, // Opcode: 85131264
        &&L2, // Opcode: 301989888
        &&L3, // Opcode: 1
        &&L4, // Opcode: 275
        &&L5, // Opcode: 24717568
        &&L6, // Opcode: 303038464
        &&L7, // Opcode: 2
        &&L8, // Opcode: 2579
        &&L9, // Opcode: 4352
        &&L10, // Opcode: 285278208
        &&L11, // Opcode: 2
        &&L12, // Opcode: 4370
        &&L13, // Opcode: 302317568
        &&L14, // Opcode: 0
        &&L15, // Opcode: 69930
        &&L16, // Opcode: 556007424
        &&L17, // Opcode: 93
        &&L18, // Opcode: 19
        &&L19, // Opcode: 687865856
        &&L20, // Opcode: 31
        &&L21, // Opcode: 4906
        &&L22, // Opcode: 318832640
        &&L23, // Opcode: 3
        &&L24, // Opcode: 201270
        &&L25, // Opcode: 51445760
        &&L26, // Opcode: 318767104
        &&L27, // Opcode: 0
        &&L28, // Opcode: 9970470
        &&L29, // Opcode: 51511296
        &&L30, // Opcode: 285212672
        &&L31, // Opcode: 3
        &&L32, // Opcode: 275
        &&L33, // Opcode: 2904761856
        &&L34, // Opcode: 536870912
        &&L35, // Opcode: 194
        &&L36, // Opcode: 201221
        &&L37, // Opcode: 51445760
        &&L38, // Opcode: 687865856
        &&L39, // Opcode: 448
        &&L40, // Opcode: 53792
        &&L41, // Opcode: 51512576
        &&L42, // Opcode: 285212672
        &&L43, // Opcode: 3
        &&L44, // Opcode: 135721
        &&L45, // Opcode: 13770752
        &&L46, // Opcode: 302317568
        &&L47, // Opcode: 3
        &&L48, // Opcode: 785
        &&L49, // Opcode: 37038336
        &&L50, // Opcode: 287965184
        &&L51, // Opcode: 1
        &&L52, // Opcode: 16982308
        &&L53, // Opcode: 1245184
        &&L54, // Opcode: 16777216
        &&L55, // Opcode: 12819
        &&L56, // Opcode: 2042112
        &&L57, // Opcode: 705888256
        &&L58, // Opcode: 687865856
        &&L59, // Opcode: 31
        &&L60, // Opcode: 29715
        &&L61, // Opcode: 2042112
        &&L62, // Opcode: 321519616
        &&L63, // Opcode: 0
        &&L64, // Opcode: 201473
        &&L65, // Opcode: 305528832
        &&L66, // Opcode: 3
        &&L67, // Opcode: 785
        &&L68, // Opcode: 4864
        &&L69, // Opcode: 589692928
        &&L70, // Opcode: 318
        &&L71, // Opcode: 786
        &&L72, // Opcode: 200960
        &&L73, // Opcode: 18022400
        &&L74, // Opcode: 637534208
        &&L75, // Opcode: 86819
        &&L76, // Opcode: 23601152
        &&L77, // Opcode: 302317568
        &&L78, // Opcode: 3
        &&L79, // Opcode: 785
        &&L80, // Opcode: 39332096
        &&L81, // Opcode: 2015363072
        &&L82, // Opcode: 83886081
        &&L83, // Opcode: 786
        &&L84, // Opcode: 200960
        &&L85, // Opcode: 807993344
        &&L86, // Opcode: 536870915
        &&L87, // Opcode: 376
        &&L88, // Opcode: 201221
        &&L89, // Opcode: 51445760
        &&L90, // Opcode: 687865856
        &&L91, // Opcode: 977
        &&L92, // Opcode: 69930
        &&L93, // Opcode: 556007424
        &&L94, // Opcode: 425
        &&L95, // Opcode: 19
        &&L96, // Opcode: 687865856
        &&L97, // Opcode: 31
        &&L98, // Opcode: 10771
        &&L99, // Opcode: 2042112
        &&L100, // Opcode: 1947402240
        &&L101, // Opcode: 687865856
        &&L102, // Opcode: 31
        &&L103, // Opcode: 4906
        &&L104, // Opcode: 83951616
        &&L105, // Opcode: 786
        &&L106, // Opcode: 200960
        &&L107, // Opcode: 3542679552
        &&L108, // Opcode: 704643072
        &&L109, // Opcode: 273
        &&L110, // Opcode: 3525387264
        &&L111, // Opcode: 318767105
        &&L112, // Opcode: 0
        &&L113, // Opcode: 1255937
        &&L114, // Opcode: 16777216
        &&L115, // Opcode: 531
        &&L116, // Opcode: 51525120
        &&L117, // Opcode: 285212672
        &&L118, // Opcode: 3
        &&L119, // Opcode: 19
        &&L120, // Opcode: 536870913
        &&L121, // Opcode: 519
        &&L122, // Opcode: 12307
        &&L123, // Opcode: 2042112
        &&L124, // Opcode: 287309824
        &&L125, // Opcode: 318767106
        &&L126, // Opcode: 100
        &&L127, // Opcode: 7977
        &&L128, // Opcode: 17902080
        &&L129, // Opcode: 603979776
        &&L130, // Opcode: 140321
        &&L131, // Opcode: 4864
        &&L132, // Opcode: 704708608
        &&L133, // Opcode: 19
        &&L134, // Opcode: 687865856
        &&L135, // Opcode: 31
        &&L136, // Opcode: 69930
        &&L137, // Opcode: 556007424
        &&L138, // Opcode: 583
        &&L139, // Opcode: 19
        &&L140, // Opcode: 0
        &&L141, // Opcode: 3281665
        &&L142, // Opcode: 522780672
        &&L143, // Opcode: 704643072
        &&L144, // Opcode: 273
        &&L145, // Opcode: 2451645440
        &&L146, // Opcode: 318767106
        &&L147, // Opcode: 0
        &&L148, // Opcode: 3281665
        &&L149, // Opcode: 522780672
        &&L150, // Opcode: 318767104
        &&L151, // Opcode: 51
        &&L152, // Opcode: 7977
        &&L153, // Opcode: 3412736
        &&L154, // Opcode: 522780672
        &&L155, // Opcode: 318767104
        &&L156, // Opcode: 53
        &&L157, // Opcode: 7977
        &&L158, // Opcode: 1255936
        &&L159, // Opcode: 16777216
        &&L160, // Opcode: 201221
        &&L161, // Opcode: 51445760
        &&L162, // Opcode: 687865856
        &&L163, // Opcode: 65
        &&L164, // Opcode: 785
        &&L165, // Opcode: 46606592
        &&L166, // Opcode: 51445760
        &&L167, // Opcode: 687865856
        &&L168, // Opcode: 746
        &&L169, // Opcode: 785
        &&L170, // Opcode: 51194112
        &&L171, // Opcode: 287965184
        &&L172, // Opcode: 1
        &&L173, // Opcode: 47784228
        &&L174, // Opcode: 1245184
        &&L175, // Opcode: 16777216
        &&L176, // Opcode: 4906
        &&L177, // Opcode: 318832640
        &&L178, // Opcode: 51
        &&L179, // Opcode: 7977
        &&L180, // Opcode: 17902080
        &&L181, // Opcode: 603979776
        &&L182, // Opcode: 195617
        &&L183, // Opcode: 4864
        &&L184, // Opcode: 704708608
        &&L185, // Opcode: 19
        &&L186, // Opcode: 687865856
        &&L187, // Opcode: 31
        &&L188, // Opcode: 69930
        &&L189, // Opcode: 556007424
        &&L190, // Opcode: 799
        &&L191, // Opcode: 19
        &&L192, // Opcode: 0
        &&L193, // Opcode: 3478273
        &&L194, // Opcode: 522780672
        &&L195, // Opcode: 704643072
        &&L196, // Opcode: 273
        &&L197, // Opcode: 1612784640
        &&L198, // Opcode: 318767107
        &&L199, // Opcode: 0
        &&L200, // Opcode: 3281665
        &&L201, // Opcode: 522780672
        &&L202, // Opcode: 318767104
        &&L203, // Opcode: 42
        &&L204, // Opcode: 7977
        &&L205, // Opcode: 7607040
        &&L206, // Opcode: 522780672
        &&L207, // Opcode: 704643072
        &&L208, // Opcode: 19
        &&L209, // Opcode: 3
        &&L210, // Opcode: 785
        &&L211, // Opcode: 4270336
        &&L212, // Opcode: 51445760
        &&L213, // Opcode: 687865856
        &&L214, // Opcode: 907
        &&L215, // Opcode: 785
        &&L216, // Opcode: 61745408
        &&L217, // Opcode: 287965184
        &&L218, // Opcode: 1
        &&L219, // Opcode: 60629284
        &&L220, // Opcode: 1245184
        &&L221, // Opcode: 16777216
        &&L222, // Opcode: 4906
        &&L223, // Opcode: 318832640
        &&L224, // Opcode: 42
        &&L225, // Opcode: 7977
        &&L226, // Opcode: 17902080
        &&L227, // Opcode: 603979776
        &&L228, // Opcode: 245793
        &&L229, // Opcode: 4864
        &&L230, // Opcode: 704708608
        &&L231, // Opcode: 19
        &&L232, // Opcode: 687865856
        &&L233, // Opcode: 31
        &&L234, // Opcode: 69930
        &&L235, // Opcode: 556007424
        &&L236, // Opcode: 1045
        &&L237, // Opcode: 19
        &&L238, // Opcode: 687865856
        &&L239, // Opcode: 31
        &&L240, // Opcode: 11027
        &&L241, // Opcode: 2042112
        &&L242, // Opcode: 840105984
        &&L243, // Opcode: 687865856
        &&L244, // Opcode: 31
        &&L245, // Opcode: 10771
        &&L246, // Opcode: 2042112
        &&L247, // Opcode: 1947402240
        &&L248, // Opcode: 687865856
        &&L249, // Opcode: 31
        &&L250, // Opcode: 4906
        &&L251, // Opcode: 83951616
        &&L252, // Opcode: 786
        &&L253, // Opcode: 200960
        &&L254, // Opcode: 1093206016
        &&L255, // Opcode: 285212672
        &&L256, // Opcode: 3
        &&L257, // Opcode: 278569
        &&L258, // Opcode: 200960
        &&L259, // Opcode: 3542679552
        &&L260, // Opcode: 704643072
        &&L261, // Opcode: 273
        &&L262, // Opcode: 1377903616
        &&L263, // Opcode: 318767108
        &&L264, // Opcode: 0
        &&L265, // Opcode: 1255937
        &&L266, // Opcode: 16777216
        &&L267, // Opcode: 11027
        &&L268 // Opcode: 2042112
    };

    // Immediate values array
    uint32_t immediates[] = {
        4608,
        840106240,
        840106240,
        4163053056,
        823329024,
        321519872,
        873660672,
        321519872,
        302317824,
        1947402496,
        840106240
    };

    // 每个指令的立即数起始索引
    int opToImmIndices[] = {
        0, // L0 = 0
        1, // L1 - 85131264 (没有立即数)
        1, // L2 - 301989888 (没有立即数)
        1, // L3 - 1 (没有立即数)
        1, // L4 - 275 (没有立即数)
        1, // L5 - 24717568 (没有立即数)
        1, // L6 - 303038464 (没有立即数)
        1, // L7 - 2 (没有立即数)
        1, // L8 - 2579 (没有立即数)
        1, // L9 - 4352 (没有立即数)
        1, // L10 - 285278208 (没有立即数)
        1, // L11 - 2 (没有立即数)
        1, // L12 - 4370 (没有立即数)
        1, // L13 - 302317568 (没有立即数)
        1, // L14 - 0 (没有立即数)
        1, // L15 - 69930 (没有立即数)
        1, // L16 - 556007424 (没有立即数)
        1, // L17 - 93 (没有立即数)
        1, // L18 = 1
        2, // L19 - 687865856 (没有立即数)
        2, // L20 - 31 (没有立即数)
        2, // L21 - 4906 (没有立即数)
        2, // L22 - 318832640 (没有立即数)
        2, // L23 - 3 (没有立即数)
        2, // L24 - 201270 (没有立即数)
        2, // L25 - 51445760 (没有立即数)
        2, // L26 - 318767104 (没有立即数)
        2, // L27 - 0 (没有立即数)
        2, // L28 - 9970470 (没有立即数)
        2, // L29 - 51511296 (没有立即数)
        2, // L30 - 285212672 (没有立即数)
        2, // L31 - 3 (没有立即数)
        2, // L32 - 275 (没有立即数)
        2, // L33 - 2904761856 (没有立即数)
        2, // L34 - 536870912 (没有立即数)
        2, // L35 - 194 (没有立即数)
        2, // L36 - 201221 (没有立即数)
        2, // L37 - 51445760 (没有立即数)
        2, // L38 - 687865856 (没有立即数)
        2, // L39 - 448 (没有立即数)
        2, // L40 - 53792 (没有立即数)
        2, // L41 - 51512576 (没有立即数)
        2, // L42 - 285212672 (没有立即数)
        2, // L43 - 3 (没有立即数)
        2, // L44 - 135721 (没有立即数)
        2, // L45 - 13770752 (没有立即数)
        2, // L46 - 302317568 (没有立即数)
        2, // L47 - 3 (没有立即数)
        2, // L48 - 785 (没有立即数)
        2, // L49 - 37038336 (没有立即数)
        2, // L50 - 287965184 (没有立即数)
        2, // L51 - 1 (没有立即数)
        2, // L52 - 16982308 (没有立即数)
        2, // L53 - 1245184 (没有立即数)
        2, // L54 - 16777216 (没有立即数)
        2, // L55 - 12819 (没有立即数)
        2, // L56 - 2042112 (没有立即数)
        2, // L57 - 705888256 (没有立即数)
        2, // L58 - 687865856 (没有立即数)
        2, // L59 - 31 (没有立即数)
        2, // L60 - 29715 (没有立即数)
        2, // L61 - 2042112 (没有立即数)
        2, // L62 - 321519616 (没有立即数)
        2, // L63 - 0 (没有立即数)
        2, // L64 - 201473 (没有立即数)
        2, // L65 - 305528832 (没有立即数)
        2, // L66 - 3 (没有立即数)
        2, // L67 - 785 (没有立即数)
        2, // L68 - 4864 (没有立即数)
        2, // L69 - 589692928 (没有立即数)
        2, // L70 - 318 (没有立即数)
        2, // L71 - 786 (没有立即数)
        2, // L72 - 200960 (没有立即数)
        2, // L73 - 18022400 (没有立即数)
        2, // L74 - 637534208 (没有立即数)
        2, // L75 - 86819 (没有立即数)
        2, // L76 - 23601152 (没有立即数)
        2, // L77 - 302317568 (没有立即数)
        2, // L78 - 3 (没有立即数)
        2, // L79 - 785 (没有立即数)
        2, // L80 - 39332096 (没有立即数)
        2, // L81 - 2015363072 (没有立即数)
        2, // L82 - 83886081 (没有立即数)
        2, // L83 - 786 (没有立即数)
        2, // L84 - 200960 (没有立即数)
        2, // L85 - 807993344 (没有立即数)
        2, // L86 - 536870915 (没有立即数)
        2, // L87 - 376 (没有立即数)
        2, // L88 - 201221 (没有立即数)
        2, // L89 - 51445760 (没有立即数)
        2, // L90 - 687865856 (没有立即数)
        2, // L91 - 977 (没有立即数)
        2, // L92 - 69930 (没有立即数)
        2, // L93 - 556007424 (没有立即数)
        2, // L94 - 425 (没有立即数)
        2, // L95 = 2
        3, // L96 - 687865856 (没有立即数)
        3, // L97 - 31 (没有立即数)
        3, // L98 - 10771 (没有立即数)
        3, // L99 - 2042112 (没有立即数)
        3, // L100 - 1947402240 (没有立即数)
        3, // L101 - 687865856 (没有立即数)
        3, // L102 - 31 (没有立即数)
        3, // L103 - 4906 (没有立即数)
        3, // L104 - 83951616 (没有立即数)
        3, // L105 - 786 (没有立即数)
        3, // L106 - 200960 (没有立即数)
        3, // L107 - 3542679552 (没有立即数)
        3, // L108 - 704643072 (没有立即数)
        3, // L109 - 273 (没有立即数)
        3, // L110 - 3525387264 (没有立即数)
        3, // L111 - 318767105 (没有立即数)
        3, // L112 - 0 (没有立即数)
        3, // L113 - 1255937 (没有立即数)
        3, // L114 - 16777216 (没有立即数)
        3, // L115 - 531 (没有立即数)
        3, // L116 - 51525120 (没有立即数)
        3, // L117 - 285212672 (没有立即数)
        3, // L118 - 3 (没有立即数)
        3, // L119 = 3
        4, // L120 - 536870913 (没有立即数)
        4, // L121 - 519 (没有立即数)
        4, // L122 - 12307 (没有立即数)
        4, // L123 - 2042112 (没有立即数)
        4, // L124 - 287309824 (没有立即数)
        4, // L125 - 318767106 (没有立即数)
        4, // L126 - 100 (没有立即数)
        4, // L127 - 7977 (没有立即数)
        4, // L128 - 17902080 (没有立即数)
        4, // L129 - 603979776 (没有立即数)
        4, // L130 - 140321 (没有立即数)
        4, // L131 - 4864 (没有立即数)
        4, // L132 - 704708608 (没有立即数)
        4, // L133 = 4
        5, // L134 - 687865856 (没有立即数)
        5, // L135 - 31 (没有立即数)
        5, // L136 - 69930 (没有立即数)
        5, // L137 - 556007424 (没有立即数)
        5, // L138 - 583 (没有立即数)
        5, // L139 = 5
        6, // L140 - 0 (没有立即数)
        6, // L141 - 3281665 (没有立即数)
        6, // L142 - 522780672 (没有立即数)
        6, // L143 - 704643072 (没有立即数)
        6, // L144 - 273 (没有立即数)
        6, // L145 - 2451645440 (没有立即数)
        6, // L146 - 318767106 (没有立即数)
        6, // L147 - 0 (没有立即数)
        6, // L148 - 3281665 (没有立即数)
        6, // L149 - 522780672 (没有立即数)
        6, // L150 - 318767104 (没有立即数)
        6, // L151 - 51 (没有立即数)
        6, // L152 - 7977 (没有立即数)
        6, // L153 - 3412736 (没有立即数)
        6, // L154 - 522780672 (没有立即数)
        6, // L155 - 318767104 (没有立即数)
        6, // L156 - 53 (没有立即数)
        6, // L157 - 7977 (没有立即数)
        6, // L158 - 1255936 (没有立即数)
        6, // L159 - 16777216 (没有立即数)
        6, // L160 - 201221 (没有立即数)
        6, // L161 - 51445760 (没有立即数)
        6, // L162 - 687865856 (没有立即数)
        6, // L163 - 65 (没有立即数)
        6, // L164 - 785 (没有立即数)
        6, // L165 - 46606592 (没有立即数)
        6, // L166 - 51445760 (没有立即数)
        6, // L167 - 687865856 (没有立即数)
        6, // L168 - 746 (没有立即数)
        6, // L169 - 785 (没有立即数)
        6, // L170 - 51194112 (没有立即数)
        6, // L171 - 287965184 (没有立即数)
        6, // L172 - 1 (没有立即数)
        6, // L173 - 47784228 (没有立即数)
        6, // L174 - 1245184 (没有立即数)
        6, // L175 - 16777216 (没有立即数)
        6, // L176 - 4906 (没有立即数)
        6, // L177 - 318832640 (没有立即数)
        6, // L178 - 51 (没有立即数)
        6, // L179 - 7977 (没有立即数)
        6, // L180 - 17902080 (没有立即数)
        6, // L181 - 603979776 (没有立即数)
        6, // L182 - 195617 (没有立即数)
        6, // L183 - 4864 (没有立即数)
        6, // L184 - 704708608 (没有立即数)
        6, // L185 = 6
        7, // L186 - 687865856 (没有立即数)
        7, // L187 - 31 (没有立即数)
        7, // L188 - 69930 (没有立即数)
        7, // L189 - 556007424 (没有立即数)
        7, // L190 - 799 (没有立即数)
        7, // L191 = 7
        8, // L192 - 0 (没有立即数)
        8, // L193 - 3478273 (没有立即数)
        8, // L194 - 522780672 (没有立即数)
        8, // L195 - 704643072 (没有立即数)
        8, // L196 - 273 (没有立即数)
        8, // L197 - 1612784640 (没有立即数)
        8, // L198 - 318767107 (没有立即数)
        8, // L199 - 0 (没有立即数)
        8, // L200 - 3281665 (没有立即数)
        8, // L201 - 522780672 (没有立即数)
        8, // L202 - 318767104 (没有立即数)
        8, // L203 - 42 (没有立即数)
        8, // L204 - 7977 (没有立即数)
        8, // L205 - 7607040 (没有立即数)
        8, // L206 - 522780672 (没有立即数)
        8, // L207 - 704643072 (没有立即数)
        8, // L208 = 8
        9, // L209 - 3 (没有立即数)
        9, // L210 - 785 (没有立即数)
        9, // L211 - 4270336 (没有立即数)
        9, // L212 - 51445760 (没有立即数)
        9, // L213 - 687865856 (没有立即数)
        9, // L214 - 907 (没有立即数)
        9, // L215 - 785 (没有立即数)
        9, // L216 - 61745408 (没有立即数)
        9, // L217 - 287965184 (没有立即数)
        9, // L218 - 1 (没有立即数)
        9, // L219 - 60629284 (没有立即数)
        9, // L220 - 1245184 (没有立即数)
        9, // L221 - 16777216 (没有立即数)
        9, // L222 - 4906 (没有立即数)
        9, // L223 - 318832640 (没有立即数)
        9, // L224 - 42 (没有立即数)
        9, // L225 - 7977 (没有立即数)
        9, // L226 - 17902080 (没有立即数)
        9, // L227 - 603979776 (没有立即数)
        9, // L228 - 245793 (没有立即数)
        9, // L229 - 4864 (没有立即数)
        9, // L230 - 704708608 (没有立即数)
        9, // L231 = 9
        10, // L232 - 687865856 (没有立即数)
        10, // L233 - 31 (没有立即数)
        10, // L234 - 69930 (没有立即数)
        10, // L235 - 556007424 (没有立即数)
        10, // L236 - 1045 (没有立即数)
        10, // L237 = 10
        11, // L238 - 687865856 (没有立即数)
        11, // L239 - 31 (没有立即数)
        11, // L240 - 11027 (没有立即数)
        11, // L241 - 2042112 (没有立即数)
        11, // L242 - 840105984 (没有立即数)
        11, // L243 - 687865856 (没有立即数)
        11, // L244 - 31 (没有立即数)
        11, // L245 - 10771 (没有立即数)
        11, // L246 - 2042112 (没有立即数)
        11, // L247 - 1947402240 (没有立即数)
        11, // L248 - 687865856 (没有立即数)
        11, // L249 - 31 (没有立即数)
        11, // L250 - 4906 (没有立即数)
        11, // L251 - 83951616 (没有立即数)
        11, // L252 - 786 (没有立即数)
        11, // L253 - 200960 (没有立即数)
        11, // L254 - 1093206016 (没有立即数)
        11, // L255 - 285212672 (没有立即数)
        11, // L256 - 3 (没有立即数)
        11, // L257 - 278569 (没有立即数)
        11, // L258 - 200960 (没有立即数)
        11, // L259 - 3542679552 (没有立即数)
        11, // L260 - 704643072 (没有立即数)
        11, // L261 - 273 (没有立即数)
        11, // L262 - 1377903616 (没有立即数)
        11, // L263 - 318767108 (没有立即数)
        11, // L264 - 0 (没有立即数)
        11, // L265 - 1255937 (没有立即数)
        11, // L266 - 16777216 (没有立即数)
        11, // L267 - 11027 (没有立即数)
        11 // L268 - 2042112 (没有立即数)
    };

    int imm_index = -1; // Immediate value index

    // Start execution
    NEXT;

L0: // Opcode 19
    {
        imm_index++;
        int jump_offset = immediates[imm_index];
        // Calculate jump target
        int current_inst = ip;
        int target_inst = current_inst + jump_offset;
        if (target_inst >= 0 && target_inst < 269) {
            ip = target_inst;
            // Set immediate index for target instruction
            imm_index = opToImmIndices[target_inst] - 1;
            goto *labels[ip];
        } else {
            fprintf(stderr, "Error: Invalid jump target\n");
            exit(1);
        }
    }

L1: // Opcode 85131264
    fprintf(stderr, "Unknown opcode encountered: 85131264\n");
    exit(1);
    NEXT;

L2: // Opcode 301989888
    fprintf(stderr, "Unknown opcode encountered: 301989888\n");
    exit(1);
    NEXT;

L3: // Opcode 1
    do_sub();
    NEXT;

L4: // Opcode 275
    fprintf(stderr, "Unknown opcode encountered: 275\n");
    exit(1);
    NEXT;

L5: // Opcode 24717568
    fprintf(stderr, "Unknown opcode encountered: 24717568\n");
    exit(1);
    NEXT;

L6: // Opcode 303038464
    fprintf(stderr, "Unknown opcode encountered: 303038464\n");
    exit(1);
    NEXT;

L7: // Opcode 2
    do_mul();
    NEXT;

L8: // Opcode 2579
    fprintf(stderr, "Unknown opcode encountered: 2579\n");
    exit(1);
    NEXT;

L9: // Opcode 4352
    fprintf(stderr, "Unknown opcode encountered: 4352\n");
    exit(1);
    NEXT;

L10: // Opcode 285278208
    fprintf(stderr, "Unknown opcode encountered: 285278208\n");
    exit(1);
    NEXT;

L11: // Opcode 2
    do_mul();
    NEXT;

L12: // Opcode 4370
    fprintf(stderr, "Unknown opcode encountered: 4370\n");
    exit(1);
    NEXT;

L13: // Opcode 302317568
    fprintf(stderr, "Unknown opcode encountered: 302317568\n");
    exit(1);
    NEXT;

L14: // Opcode 0
    do_add();
    NEXT;

L15: // Opcode 69930
    fprintf(stderr, "Unknown opcode encountered: 69930\n");
    exit(1);
    NEXT;

L16: // Opcode 556007424
    fprintf(stderr, "Unknown opcode encountered: 556007424\n");
    exit(1);
    NEXT;

L17: // Opcode 93
    fprintf(stderr, "Unknown opcode encountered: 93\n");
    exit(1);
    NEXT;

L18: // Opcode 19
    {
        imm_index++;
        int jump_offset = immediates[imm_index];
        // Calculate jump target
        int current_inst = ip;
        int target_inst = current_inst + jump_offset;
        if (target_inst >= 0 && target_inst < 269) {
            ip = target_inst;
            // Set immediate index for target instruction
            imm_index = opToImmIndices[target_inst] - 1;
            goto *labels[ip];
        } else {
            fprintf(stderr, "Error: Invalid jump target\n");
            exit(1);
        }
    }

L19: // Opcode 687865856
    fprintf(stderr, "Unknown opcode encountered: 687865856\n");
    exit(1);
    NEXT;

L20: // Opcode 31
    do_print();
    NEXT;

L21: // Opcode 4906
    fprintf(stderr, "Unknown opcode encountered: 4906\n");
    exit(1);
    NEXT;

L22: // Opcode 318832640
    fprintf(stderr, "Unknown opcode encountered: 318832640\n");
    exit(1);
    NEXT;

L23: // Opcode 3
    do_div();
    NEXT;

L24: // Opcode 201270
    fprintf(stderr, "Unknown opcode encountered: 201270\n");
    exit(1);
    NEXT;

L25: // Opcode 51445760
    fprintf(stderr, "Unknown opcode encountered: 51445760\n");
    exit(1);
    NEXT;

L26: // Opcode 318767104
    fprintf(stderr, "Unknown opcode encountered: 318767104\n");
    exit(1);
    NEXT;

L27: // Opcode 0
    do_add();
    NEXT;

L28: // Opcode 9970470
    fprintf(stderr, "Unknown opcode encountered: 9970470\n");
    exit(1);
    NEXT;

L29: // Opcode 51511296
    fprintf(stderr, "Unknown opcode encountered: 51511296\n");
    exit(1);
    NEXT;

L30: // Opcode 285212672
    fprintf(stderr, "Unknown opcode encountered: 285212672\n");
    exit(1);
    NEXT;

L31: // Opcode 3
    do_div();
    NEXT;

L32: // Opcode 275
    fprintf(stderr, "Unknown opcode encountered: 275\n");
    exit(1);
    NEXT;

L33: // Opcode 2904761856
    fprintf(stderr, "Unknown opcode encountered: 2904761856\n");
    exit(1);
    NEXT;

L34: // Opcode 536870912
    fprintf(stderr, "Unknown opcode encountered: 536870912\n");
    exit(1);
    NEXT;

L35: // Opcode 194
    fprintf(stderr, "Unknown opcode encountered: 194\n");
    exit(1);
    NEXT;

L36: // Opcode 201221
    fprintf(stderr, "Unknown opcode encountered: 201221\n");
    exit(1);
    NEXT;

L37: // Opcode 51445760
    fprintf(stderr, "Unknown opcode encountered: 51445760\n");
    exit(1);
    NEXT;

L38: // Opcode 687865856
    fprintf(stderr, "Unknown opcode encountered: 687865856\n");
    exit(1);
    NEXT;

L39: // Opcode 448
    fprintf(stderr, "Unknown opcode encountered: 448\n");
    exit(1);
    NEXT;

L40: // Opcode 53792
    fprintf(stderr, "Unknown opcode encountered: 53792\n");
    exit(1);
    NEXT;

L41: // Opcode 51512576
    fprintf(stderr, "Unknown opcode encountered: 51512576\n");
    exit(1);
    NEXT;

L42: // Opcode 285212672
    fprintf(stderr, "Unknown opcode encountered: 285212672\n");
    exit(1);
    NEXT;

L43: // Opcode 3
    do_div();
    NEXT;

L44: // Opcode 135721
    fprintf(stderr, "Unknown opcode encountered: 135721\n");
    exit(1);
    NEXT;

L45: // Opcode 13770752
    fprintf(stderr, "Unknown opcode encountered: 13770752\n");
    exit(1);
    NEXT;

L46: // Opcode 302317568
    fprintf(stderr, "Unknown opcode encountered: 302317568\n");
    exit(1);
    NEXT;

L47: // Opcode 3
    do_div();
    NEXT;

L48: // Opcode 785
    fprintf(stderr, "Unknown opcode encountered: 785\n");
    exit(1);
    NEXT;

L49: // Opcode 37038336
    fprintf(stderr, "Unknown opcode encountered: 37038336\n");
    exit(1);
    NEXT;

L50: // Opcode 287965184
    fprintf(stderr, "Unknown opcode encountered: 287965184\n");
    exit(1);
    NEXT;

L51: // Opcode 1
    do_sub();
    NEXT;

L52: // Opcode 16982308
    fprintf(stderr, "Unknown opcode encountered: 16982308\n");
    exit(1);
    NEXT;

L53: // Opcode 1245184
    fprintf(stderr, "Unknown opcode encountered: 1245184\n");
    exit(1);
    NEXT;

L54: // Opcode 16777216
    fprintf(stderr, "Unknown opcode encountered: 16777216\n");
    exit(1);
    NEXT;

L55: // Opcode 12819
    fprintf(stderr, "Unknown opcode encountered: 12819\n");
    exit(1);
    NEXT;

L56: // Opcode 2042112
    fprintf(stderr, "Unknown opcode encountered: 2042112\n");
    exit(1);
    NEXT;

L57: // Opcode 705888256
    fprintf(stderr, "Unknown opcode encountered: 705888256\n");
    exit(1);
    NEXT;

L58: // Opcode 687865856
    fprintf(stderr, "Unknown opcode encountered: 687865856\n");
    exit(1);
    NEXT;

L59: // Opcode 31
    do_print();
    NEXT;

L60: // Opcode 29715
    fprintf(stderr, "Unknown opcode encountered: 29715\n");
    exit(1);
    NEXT;

L61: // Opcode 2042112
    fprintf(stderr, "Unknown opcode encountered: 2042112\n");
    exit(1);
    NEXT;

L62: // Opcode 321519616
    fprintf(stderr, "Unknown opcode encountered: 321519616\n");
    exit(1);
    NEXT;

L63: // Opcode 0
    do_add();
    NEXT;

L64: // Opcode 201473
    fprintf(stderr, "Unknown opcode encountered: 201473\n");
    exit(1);
    NEXT;

L65: // Opcode 305528832
    fprintf(stderr, "Unknown opcode encountered: 305528832\n");
    exit(1);
    NEXT;

L66: // Opcode 3
    do_div();
    NEXT;

L67: // Opcode 785
    fprintf(stderr, "Unknown opcode encountered: 785\n");
    exit(1);
    NEXT;

L68: // Opcode 4864
    fprintf(stderr, "Unknown opcode encountered: 4864\n");
    exit(1);
    NEXT;

L69: // Opcode 589692928
    fprintf(stderr, "Unknown opcode encountered: 589692928\n");
    exit(1);
    NEXT;

L70: // Opcode 318
    fprintf(stderr, "Unknown opcode encountered: 318\n");
    exit(1);
    NEXT;

L71: // Opcode 786
    fprintf(stderr, "Unknown opcode encountered: 786\n");
    exit(1);
    NEXT;

L72: // Opcode 200960
    fprintf(stderr, "Unknown opcode encountered: 200960\n");
    exit(1);
    NEXT;

L73: // Opcode 18022400
    fprintf(stderr, "Unknown opcode encountered: 18022400\n");
    exit(1);
    NEXT;

L74: // Opcode 637534208
    fprintf(stderr, "Unknown opcode encountered: 637534208\n");
    exit(1);
    NEXT;

L75: // Opcode 86819
    fprintf(stderr, "Unknown opcode encountered: 86819\n");
    exit(1);
    NEXT;

L76: // Opcode 23601152
    fprintf(stderr, "Unknown opcode encountered: 23601152\n");
    exit(1);
    NEXT;

L77: // Opcode 302317568
    fprintf(stderr, "Unknown opcode encountered: 302317568\n");
    exit(1);
    NEXT;

L78: // Opcode 3
    do_div();
    NEXT;

L79: // Opcode 785
    fprintf(stderr, "Unknown opcode encountered: 785\n");
    exit(1);
    NEXT;

L80: // Opcode 39332096
    fprintf(stderr, "Unknown opcode encountered: 39332096\n");
    exit(1);
    NEXT;

L81: // Opcode 2015363072
    fprintf(stderr, "Unknown opcode encountered: 2015363072\n");
    exit(1);
    NEXT;

L82: // Opcode 83886081
    fprintf(stderr, "Unknown opcode encountered: 83886081\n");
    exit(1);
    NEXT;

L83: // Opcode 786
    fprintf(stderr, "Unknown opcode encountered: 786\n");
    exit(1);
    NEXT;

L84: // Opcode 200960
    fprintf(stderr, "Unknown opcode encountered: 200960\n");
    exit(1);
    NEXT;

L85: // Opcode 807993344
    fprintf(stderr, "Unknown opcode encountered: 807993344\n");
    exit(1);
    NEXT;

L86: // Opcode 536870915
    fprintf(stderr, "Unknown opcode encountered: 536870915\n");
    exit(1);
    NEXT;

L87: // Opcode 376
    fprintf(stderr, "Unknown opcode encountered: 376\n");
    exit(1);
    NEXT;

L88: // Opcode 201221
    fprintf(stderr, "Unknown opcode encountered: 201221\n");
    exit(1);
    NEXT;

L89: // Opcode 51445760
    fprintf(stderr, "Unknown opcode encountered: 51445760\n");
    exit(1);
    NEXT;

L90: // Opcode 687865856
    fprintf(stderr, "Unknown opcode encountered: 687865856\n");
    exit(1);
    NEXT;

L91: // Opcode 977
    fprintf(stderr, "Unknown opcode encountered: 977\n");
    exit(1);
    NEXT;

L92: // Opcode 69930
    fprintf(stderr, "Unknown opcode encountered: 69930\n");
    exit(1);
    NEXT;

L93: // Opcode 556007424
    fprintf(stderr, "Unknown opcode encountered: 556007424\n");
    exit(1);
    NEXT;

L94: // Opcode 425
    fprintf(stderr, "Unknown opcode encountered: 425\n");
    exit(1);
    NEXT;

L95: // Opcode 19
    {
        imm_index++;
        int jump_offset = immediates[imm_index];
        // Calculate jump target
        int current_inst = ip;
        int target_inst = current_inst + jump_offset;
        if (target_inst >= 0 && target_inst < 269) {
            ip = target_inst;
            // Set immediate index for target instruction
            imm_index = opToImmIndices[target_inst] - 1;
            goto *labels[ip];
        } else {
            fprintf(stderr, "Error: Invalid jump target\n");
            exit(1);
        }
    }

L96: // Opcode 687865856
    fprintf(stderr, "Unknown opcode encountered: 687865856\n");
    exit(1);
    NEXT;

L97: // Opcode 31
    do_print();
    NEXT;

L98: // Opcode 10771
    fprintf(stderr, "Unknown opcode encountered: 10771\n");
    exit(1);
    NEXT;

L99: // Opcode 2042112
    fprintf(stderr, "Unknown opcode encountered: 2042112\n");
    exit(1);
    NEXT;

L100: // Opcode 1947402240
    fprintf(stderr, "Unknown opcode encountered: 1947402240\n");
    exit(1);
    NEXT;

L101: // Opcode 687865856
    fprintf(stderr, "Unknown opcode encountered: 687865856\n");
    exit(1);
    NEXT;

L102: // Opcode 31
    do_print();
    NEXT;

L103: // Opcode 4906
    fprintf(stderr, "Unknown opcode encountered: 4906\n");
    exit(1);
    NEXT;

L104: // Opcode 83951616
    fprintf(stderr, "Unknown opcode encountered: 83951616\n");
    exit(1);
    NEXT;

L105: // Opcode 786
    fprintf(stderr, "Unknown opcode encountered: 786\n");
    exit(1);
    NEXT;

L106: // Opcode 200960
    fprintf(stderr, "Unknown opcode encountered: 200960\n");
    exit(1);
    NEXT;

L107: // Opcode 3542679552
    fprintf(stderr, "Unknown opcode encountered: 3542679552\n");
    exit(1);
    NEXT;

L108: // Opcode 704643072
    fprintf(stderr, "Unknown opcode encountered: 704643072\n");
    exit(1);
    NEXT;

L109: // Opcode 273
    fprintf(stderr, "Unknown opcode encountered: 273\n");
    exit(1);
    NEXT;

L110: // Opcode 3525387264
    fprintf(stderr, "Unknown opcode encountered: 3525387264\n");
    exit(1);
    NEXT;

L111: // Opcode 318767105
    fprintf(stderr, "Unknown opcode encountered: 318767105\n");
    exit(1);
    NEXT;

L112: // Opcode 0
    do_add();
    NEXT;

L113: // Opcode 1255937
    fprintf(stderr, "Unknown opcode encountered: 1255937\n");
    exit(1);
    NEXT;

L114: // Opcode 16777216
    fprintf(stderr, "Unknown opcode encountered: 16777216\n");
    exit(1);
    NEXT;

L115: // Opcode 531
    fprintf(stderr, "Unknown opcode encountered: 531\n");
    exit(1);
    NEXT;

L116: // Opcode 51525120
    fprintf(stderr, "Unknown opcode encountered: 51525120\n");
    exit(1);
    NEXT;

L117: // Opcode 285212672
    fprintf(stderr, "Unknown opcode encountered: 285212672\n");
    exit(1);
    NEXT;

L118: // Opcode 3
    do_div();
    NEXT;

L119: // Opcode 19
    {
        imm_index++;
        int jump_offset = immediates[imm_index];
        // Calculate jump target
        int current_inst = ip;
        int target_inst = current_inst + jump_offset;
        if (target_inst >= 0 && target_inst < 269) {
            ip = target_inst;
            // Set immediate index for target instruction
            imm_index = opToImmIndices[target_inst] - 1;
            goto *labels[ip];
        } else {
            fprintf(stderr, "Error: Invalid jump target\n");
            exit(1);
        }
    }

L120: // Opcode 536870913
    fprintf(stderr, "Unknown opcode encountered: 536870913\n");
    exit(1);
    NEXT;

L121: // Opcode 519
    fprintf(stderr, "Unknown opcode encountered: 519\n");
    exit(1);
    NEXT;

L122: // Opcode 12307
    fprintf(stderr, "Unknown opcode encountered: 12307\n");
    exit(1);
    NEXT;

L123: // Opcode 2042112
    fprintf(stderr, "Unknown opcode encountered: 2042112\n");
    exit(1);
    NEXT;

L124: // Opcode 287309824
    fprintf(stderr, "Unknown opcode encountered: 287309824\n");
    exit(1);
    NEXT;

L125: // Opcode 318767106
    fprintf(stderr, "Unknown opcode encountered: 318767106\n");
    exit(1);
    NEXT;

L126: // Opcode 100
    fprintf(stderr, "Unknown opcode encountered: 100\n");
    exit(1);
    NEXT;

L127: // Opcode 7977
    fprintf(stderr, "Unknown opcode encountered: 7977\n");
    exit(1);
    NEXT;

L128: // Opcode 17902080
    fprintf(stderr, "Unknown opcode encountered: 17902080\n");
    exit(1);
    NEXT;

L129: // Opcode 603979776
    fprintf(stderr, "Unknown opcode encountered: 603979776\n");
    exit(1);
    NEXT;

L130: // Opcode 140321
    fprintf(stderr, "Unknown opcode encountered: 140321\n");
    exit(1);
    NEXT;

L131: // Opcode 4864
    fprintf(stderr, "Unknown opcode encountered: 4864\n");
    exit(1);
    NEXT;

L132: // Opcode 704708608
    fprintf(stderr, "Unknown opcode encountered: 704708608\n");
    exit(1);
    NEXT;

L133: // Opcode 19
    {
        imm_index++;
        int jump_offset = immediates[imm_index];
        // Calculate jump target
        int current_inst = ip;
        int target_inst = current_inst + jump_offset;
        if (target_inst >= 0 && target_inst < 269) {
            ip = target_inst;
            // Set immediate index for target instruction
            imm_index = opToImmIndices[target_inst] - 1;
            goto *labels[ip];
        } else {
            fprintf(stderr, "Error: Invalid jump target\n");
            exit(1);
        }
    }

L134: // Opcode 687865856
    fprintf(stderr, "Unknown opcode encountered: 687865856\n");
    exit(1);
    NEXT;

L135: // Opcode 31
    do_print();
    NEXT;

L136: // Opcode 69930
    fprintf(stderr, "Unknown opcode encountered: 69930\n");
    exit(1);
    NEXT;

L137: // Opcode 556007424
    fprintf(stderr, "Unknown opcode encountered: 556007424\n");
    exit(1);
    NEXT;

L138: // Opcode 583
    fprintf(stderr, "Unknown opcode encountered: 583\n");
    exit(1);
    NEXT;

L139: // Opcode 19
    {
        imm_index++;
        int jump_offset = immediates[imm_index];
        // Calculate jump target
        int current_inst = ip;
        int target_inst = current_inst + jump_offset;
        if (target_inst >= 0 && target_inst < 269) {
            ip = target_inst;
            // Set immediate index for target instruction
            imm_index = opToImmIndices[target_inst] - 1;
            goto *labels[ip];
        } else {
            fprintf(stderr, "Error: Invalid jump target\n");
            exit(1);
        }
    }

L140: // Opcode 0
    do_add();
    NEXT;

L141: // Opcode 3281665
    fprintf(stderr, "Unknown opcode encountered: 3281665\n");
    exit(1);
    NEXT;

L142: // Opcode 522780672
    fprintf(stderr, "Unknown opcode encountered: 522780672\n");
    exit(1);
    NEXT;

L143: // Opcode 704643072
    fprintf(stderr, "Unknown opcode encountered: 704643072\n");
    exit(1);
    NEXT;

L144: // Opcode 273
    fprintf(stderr, "Unknown opcode encountered: 273\n");
    exit(1);
    NEXT;

L145: // Opcode 2451645440
    fprintf(stderr, "Unknown opcode encountered: 2451645440\n");
    exit(1);
    NEXT;

L146: // Opcode 318767106
    fprintf(stderr, "Unknown opcode encountered: 318767106\n");
    exit(1);
    NEXT;

L147: // Opcode 0
    do_add();
    NEXT;

L148: // Opcode 3281665
    fprintf(stderr, "Unknown opcode encountered: 3281665\n");
    exit(1);
    NEXT;

L149: // Opcode 522780672
    fprintf(stderr, "Unknown opcode encountered: 522780672\n");
    exit(1);
    NEXT;

L150: // Opcode 318767104
    fprintf(stderr, "Unknown opcode encountered: 318767104\n");
    exit(1);
    NEXT;

L151: // Opcode 51
    fprintf(stderr, "Unknown opcode encountered: 51\n");
    exit(1);
    NEXT;

L152: // Opcode 7977
    fprintf(stderr, "Unknown opcode encountered: 7977\n");
    exit(1);
    NEXT;

L153: // Opcode 3412736
    fprintf(stderr, "Unknown opcode encountered: 3412736\n");
    exit(1);
    NEXT;

L154: // Opcode 522780672
    fprintf(stderr, "Unknown opcode encountered: 522780672\n");
    exit(1);
    NEXT;

L155: // Opcode 318767104
    fprintf(stderr, "Unknown opcode encountered: 318767104\n");
    exit(1);
    NEXT;

L156: // Opcode 53
    fprintf(stderr, "Unknown opcode encountered: 53\n");
    exit(1);
    NEXT;

L157: // Opcode 7977
    fprintf(stderr, "Unknown opcode encountered: 7977\n");
    exit(1);
    NEXT;

L158: // Opcode 1255936
    fprintf(stderr, "Unknown opcode encountered: 1255936\n");
    exit(1);
    NEXT;

L159: // Opcode 16777216
    fprintf(stderr, "Unknown opcode encountered: 16777216\n");
    exit(1);
    NEXT;

L160: // Opcode 201221
    fprintf(stderr, "Unknown opcode encountered: 201221\n");
    exit(1);
    NEXT;

L161: // Opcode 51445760
    fprintf(stderr, "Unknown opcode encountered: 51445760\n");
    exit(1);
    NEXT;

L162: // Opcode 687865856
    fprintf(stderr, "Unknown opcode encountered: 687865856\n");
    exit(1);
    NEXT;

L163: // Opcode 65
    fprintf(stderr, "Unknown opcode encountered: 65\n");
    exit(1);
    NEXT;

L164: // Opcode 785
    fprintf(stderr, "Unknown opcode encountered: 785\n");
    exit(1);
    NEXT;

L165: // Opcode 46606592
    fprintf(stderr, "Unknown opcode encountered: 46606592\n");
    exit(1);
    NEXT;

L166: // Opcode 51445760
    fprintf(stderr, "Unknown opcode encountered: 51445760\n");
    exit(1);
    NEXT;

L167: // Opcode 687865856
    fprintf(stderr, "Unknown opcode encountered: 687865856\n");
    exit(1);
    NEXT;

L168: // Opcode 746
    fprintf(stderr, "Unknown opcode encountered: 746\n");
    exit(1);
    NEXT;

L169: // Opcode 785
    fprintf(stderr, "Unknown opcode encountered: 785\n");
    exit(1);
    NEXT;

L170: // Opcode 51194112
    fprintf(stderr, "Unknown opcode encountered: 51194112\n");
    exit(1);
    NEXT;

L171: // Opcode 287965184
    fprintf(stderr, "Unknown opcode encountered: 287965184\n");
    exit(1);
    NEXT;

L172: // Opcode 1
    do_sub();
    NEXT;

L173: // Opcode 47784228
    fprintf(stderr, "Unknown opcode encountered: 47784228\n");
    exit(1);
    NEXT;

L174: // Opcode 1245184
    fprintf(stderr, "Unknown opcode encountered: 1245184\n");
    exit(1);
    NEXT;

L175: // Opcode 16777216
    fprintf(stderr, "Unknown opcode encountered: 16777216\n");
    exit(1);
    NEXT;

L176: // Opcode 4906
    fprintf(stderr, "Unknown opcode encountered: 4906\n");
    exit(1);
    NEXT;

L177: // Opcode 318832640
    fprintf(stderr, "Unknown opcode encountered: 318832640\n");
    exit(1);
    NEXT;

L178: // Opcode 51
    fprintf(stderr, "Unknown opcode encountered: 51\n");
    exit(1);
    NEXT;

L179: // Opcode 7977
    fprintf(stderr, "Unknown opcode encountered: 7977\n");
    exit(1);
    NEXT;

L180: // Opcode 17902080
    fprintf(stderr, "Unknown opcode encountered: 17902080\n");
    exit(1);
    NEXT;

L181: // Opcode 603979776
    fprintf(stderr, "Unknown opcode encountered: 603979776\n");
    exit(1);
    NEXT;

L182: // Opcode 195617
    fprintf(stderr, "Unknown opcode encountered: 195617\n");
    exit(1);
    NEXT;

L183: // Opcode 4864
    fprintf(stderr, "Unknown opcode encountered: 4864\n");
    exit(1);
    NEXT;

L184: // Opcode 704708608
    fprintf(stderr, "Unknown opcode encountered: 704708608\n");
    exit(1);
    NEXT;

L185: // Opcode 19
    {
        imm_index++;
        int jump_offset = immediates[imm_index];
        // Calculate jump target
        int current_inst = ip;
        int target_inst = current_inst + jump_offset;
        if (target_inst >= 0 && target_inst < 269) {
            ip = target_inst;
            // Set immediate index for target instruction
            imm_index = opToImmIndices[target_inst] - 1;
            goto *labels[ip];
        } else {
            fprintf(stderr, "Error: Invalid jump target\n");
            exit(1);
        }
    }

L186: // Opcode 687865856
    fprintf(stderr, "Unknown opcode encountered: 687865856\n");
    exit(1);
    NEXT;

L187: // Opcode 31
    do_print();
    NEXT;

L188: // Opcode 69930
    fprintf(stderr, "Unknown opcode encountered: 69930\n");
    exit(1);
    NEXT;

L189: // Opcode 556007424
    fprintf(stderr, "Unknown opcode encountered: 556007424\n");
    exit(1);
    NEXT;

L190: // Opcode 799
    fprintf(stderr, "Unknown opcode encountered: 799\n");
    exit(1);
    NEXT;

L191: // Opcode 19
    {
        imm_index++;
        int jump_offset = immediates[imm_index];
        // Calculate jump target
        int current_inst = ip;
        int target_inst = current_inst + jump_offset;
        if (target_inst >= 0 && target_inst < 269) {
            ip = target_inst;
            // Set immediate index for target instruction
            imm_index = opToImmIndices[target_inst] - 1;
            goto *labels[ip];
        } else {
            fprintf(stderr, "Error: Invalid jump target\n");
            exit(1);
        }
    }

L192: // Opcode 0
    do_add();
    NEXT;

L193: // Opcode 3478273
    fprintf(stderr, "Unknown opcode encountered: 3478273\n");
    exit(1);
    NEXT;

L194: // Opcode 522780672
    fprintf(stderr, "Unknown opcode encountered: 522780672\n");
    exit(1);
    NEXT;

L195: // Opcode 704643072
    fprintf(stderr, "Unknown opcode encountered: 704643072\n");
    exit(1);
    NEXT;

L196: // Opcode 273
    fprintf(stderr, "Unknown opcode encountered: 273\n");
    exit(1);
    NEXT;

L197: // Opcode 1612784640
    fprintf(stderr, "Unknown opcode encountered: 1612784640\n");
    exit(1);
    NEXT;

L198: // Opcode 318767107
    fprintf(stderr, "Unknown opcode encountered: 318767107\n");
    exit(1);
    NEXT;

L199: // Opcode 0
    do_add();
    NEXT;

L200: // Opcode 3281665
    fprintf(stderr, "Unknown opcode encountered: 3281665\n");
    exit(1);
    NEXT;

L201: // Opcode 522780672
    fprintf(stderr, "Unknown opcode encountered: 522780672\n");
    exit(1);
    NEXT;

L202: // Opcode 318767104
    fprintf(stderr, "Unknown opcode encountered: 318767104\n");
    exit(1);
    NEXT;

L203: // Opcode 42
    fprintf(stderr, "Unknown opcode encountered: 42\n");
    exit(1);
    NEXT;

L204: // Opcode 7977
    fprintf(stderr, "Unknown opcode encountered: 7977\n");
    exit(1);
    NEXT;

L205: // Opcode 7607040
    fprintf(stderr, "Unknown opcode encountered: 7607040\n");
    exit(1);
    NEXT;

L206: // Opcode 522780672
    fprintf(stderr, "Unknown opcode encountered: 522780672\n");
    exit(1);
    NEXT;

L207: // Opcode 704643072
    fprintf(stderr, "Unknown opcode encountered: 704643072\n");
    exit(1);
    NEXT;

L208: // Opcode 19
    {
        imm_index++;
        int jump_offset = immediates[imm_index];
        // Calculate jump target
        int current_inst = ip;
        int target_inst = current_inst + jump_offset;
        if (target_inst >= 0 && target_inst < 269) {
            ip = target_inst;
            // Set immediate index for target instruction
            imm_index = opToImmIndices[target_inst] - 1;
            goto *labels[ip];
        } else {
            fprintf(stderr, "Error: Invalid jump target\n");
            exit(1);
        }
    }

L209: // Opcode 3
    do_div();
    NEXT;

L210: // Opcode 785
    fprintf(stderr, "Unknown opcode encountered: 785\n");
    exit(1);
    NEXT;

L211: // Opcode 4270336
    fprintf(stderr, "Unknown opcode encountered: 4270336\n");
    exit(1);
    NEXT;

L212: // Opcode 51445760
    fprintf(stderr, "Unknown opcode encountered: 51445760\n");
    exit(1);
    NEXT;

L213: // Opcode 687865856
    fprintf(stderr, "Unknown opcode encountered: 687865856\n");
    exit(1);
    NEXT;

L214: // Opcode 907
    fprintf(stderr, "Unknown opcode encountered: 907\n");
    exit(1);
    NEXT;

L215: // Opcode 785
    fprintf(stderr, "Unknown opcode encountered: 785\n");
    exit(1);
    NEXT;

L216: // Opcode 61745408
    fprintf(stderr, "Unknown opcode encountered: 61745408\n");
    exit(1);
    NEXT;

L217: // Opcode 287965184
    fprintf(stderr, "Unknown opcode encountered: 287965184\n");
    exit(1);
    NEXT;

L218: // Opcode 1
    do_sub();
    NEXT;

L219: // Opcode 60629284
    fprintf(stderr, "Unknown opcode encountered: 60629284\n");
    exit(1);
    NEXT;

L220: // Opcode 1245184
    fprintf(stderr, "Unknown opcode encountered: 1245184\n");
    exit(1);
    NEXT;

L221: // Opcode 16777216
    fprintf(stderr, "Unknown opcode encountered: 16777216\n");
    exit(1);
    NEXT;

L222: // Opcode 4906
    fprintf(stderr, "Unknown opcode encountered: 4906\n");
    exit(1);
    NEXT;

L223: // Opcode 318832640
    fprintf(stderr, "Unknown opcode encountered: 318832640\n");
    exit(1);
    NEXT;

L224: // Opcode 42
    fprintf(stderr, "Unknown opcode encountered: 42\n");
    exit(1);
    NEXT;

L225: // Opcode 7977
    fprintf(stderr, "Unknown opcode encountered: 7977\n");
    exit(1);
    NEXT;

L226: // Opcode 17902080
    fprintf(stderr, "Unknown opcode encountered: 17902080\n");
    exit(1);
    NEXT;

L227: // Opcode 603979776
    fprintf(stderr, "Unknown opcode encountered: 603979776\n");
    exit(1);
    NEXT;

L228: // Opcode 245793
    fprintf(stderr, "Unknown opcode encountered: 245793\n");
    exit(1);
    NEXT;

L229: // Opcode 4864
    fprintf(stderr, "Unknown opcode encountered: 4864\n");
    exit(1);
    NEXT;

L230: // Opcode 704708608
    fprintf(stderr, "Unknown opcode encountered: 704708608\n");
    exit(1);
    NEXT;

L231: // Opcode 19
    {
        imm_index++;
        int jump_offset = immediates[imm_index];
        // Calculate jump target
        int current_inst = ip;
        int target_inst = current_inst + jump_offset;
        if (target_inst >= 0 && target_inst < 269) {
            ip = target_inst;
            // Set immediate index for target instruction
            imm_index = opToImmIndices[target_inst] - 1;
            goto *labels[ip];
        } else {
            fprintf(stderr, "Error: Invalid jump target\n");
            exit(1);
        }
    }

L232: // Opcode 687865856
    fprintf(stderr, "Unknown opcode encountered: 687865856\n");
    exit(1);
    NEXT;

L233: // Opcode 31
    do_print();
    NEXT;

L234: // Opcode 69930
    fprintf(stderr, "Unknown opcode encountered: 69930\n");
    exit(1);
    NEXT;

L235: // Opcode 556007424
    fprintf(stderr, "Unknown opcode encountered: 556007424\n");
    exit(1);
    NEXT;

L236: // Opcode 1045
    fprintf(stderr, "Unknown opcode encountered: 1045\n");
    exit(1);
    NEXT;

L237: // Opcode 19
    {
        imm_index++;
        int jump_offset = immediates[imm_index];
        // Calculate jump target
        int current_inst = ip;
        int target_inst = current_inst + jump_offset;
        if (target_inst >= 0 && target_inst < 269) {
            ip = target_inst;
            // Set immediate index for target instruction
            imm_index = opToImmIndices[target_inst] - 1;
            goto *labels[ip];
        } else {
            fprintf(stderr, "Error: Invalid jump target\n");
            exit(1);
        }
    }

L238: // Opcode 687865856
    fprintf(stderr, "Unknown opcode encountered: 687865856\n");
    exit(1);
    NEXT;

L239: // Opcode 31
    do_print();
    NEXT;

L240: // Opcode 11027
    fprintf(stderr, "Unknown opcode encountered: 11027\n");
    exit(1);
    NEXT;

L241: // Opcode 2042112
    fprintf(stderr, "Unknown opcode encountered: 2042112\n");
    exit(1);
    NEXT;

L242: // Opcode 840105984
    fprintf(stderr, "Unknown opcode encountered: 840105984\n");
    exit(1);
    NEXT;

L243: // Opcode 687865856
    fprintf(stderr, "Unknown opcode encountered: 687865856\n");
    exit(1);
    NEXT;

L244: // Opcode 31
    do_print();
    NEXT;

L245: // Opcode 10771
    fprintf(stderr, "Unknown opcode encountered: 10771\n");
    exit(1);
    NEXT;

L246: // Opcode 2042112
    fprintf(stderr, "Unknown opcode encountered: 2042112\n");
    exit(1);
    NEXT;

L247: // Opcode 1947402240
    fprintf(stderr, "Unknown opcode encountered: 1947402240\n");
    exit(1);
    NEXT;

L248: // Opcode 687865856
    fprintf(stderr, "Unknown opcode encountered: 687865856\n");
    exit(1);
    NEXT;

L249: // Opcode 31
    do_print();
    NEXT;

L250: // Opcode 4906
    fprintf(stderr, "Unknown opcode encountered: 4906\n");
    exit(1);
    NEXT;

L251: // Opcode 83951616
    fprintf(stderr, "Unknown opcode encountered: 83951616\n");
    exit(1);
    NEXT;

L252: // Opcode 786
    fprintf(stderr, "Unknown opcode encountered: 786\n");
    exit(1);
    NEXT;

L253: // Opcode 200960
    fprintf(stderr, "Unknown opcode encountered: 200960\n");
    exit(1);
    NEXT;

L254: // Opcode 1093206016
    fprintf(stderr, "Unknown opcode encountered: 1093206016\n");
    exit(1);
    NEXT;

L255: // Opcode 285212672
    fprintf(stderr, "Unknown opcode encountered: 285212672\n");
    exit(1);
    NEXT;

L256: // Opcode 3
    do_div();
    NEXT;

L257: // Opcode 278569
    fprintf(stderr, "Unknown opcode encountered: 278569\n");
    exit(1);
    NEXT;

L258: // Opcode 200960
    fprintf(stderr, "Unknown opcode encountered: 200960\n");
    exit(1);
    NEXT;

L259: // Opcode 3542679552
    fprintf(stderr, "Unknown opcode encountered: 3542679552\n");
    exit(1);
    NEXT;

L260: // Opcode 704643072
    fprintf(stderr, "Unknown opcode encountered: 704643072\n");
    exit(1);
    NEXT;

L261: // Opcode 273
    fprintf(stderr, "Unknown opcode encountered: 273\n");
    exit(1);
    NEXT;

L262: // Opcode 1377903616
    fprintf(stderr, "Unknown opcode encountered: 1377903616\n");
    exit(1);
    NEXT;

L263: // Opcode 318767108
    fprintf(stderr, "Unknown opcode encountered: 318767108\n");
    exit(1);
    NEXT;

L264: // Opcode 0
    do_add();
    NEXT;

L265: // Opcode 1255937
    fprintf(stderr, "Unknown opcode encountered: 1255937\n");
    exit(1);
    NEXT;

L266: // Opcode 16777216
    fprintf(stderr, "Unknown opcode encountered: 16777216\n");
    exit(1);
    NEXT;

L267: // Opcode 11027
    fprintf(stderr, "Unknown opcode encountered: 11027\n");
    exit(1);
    NEXT;

L268: // Opcode 2042112
    fprintf(stderr, "Unknown opcode encountered: 2042112\n");
    exit(1);
    NEXT;

    return 0;
}
