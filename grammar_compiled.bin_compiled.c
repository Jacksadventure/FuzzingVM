#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define STACK_SIZE 1024
#define BUFFER_SIZE (4 * 1024 * 1024)

uint32_t stack[STACK_SIZE];
int top_index = -1;
char buffer[BUFFER_SIZE];

float to_float(uint32_t val) {
    union { uint32_t i; float f; } u;
    u.i = val; return u.f;
}
uint32_t from_float(float f) {
    union { uint32_t i; float f; } u;
    u.f = f; return u.i;
}

#define guard(n) asm("#" #n)

void loop_func() {
    static int count = 100000000;
    if(count <= 0) exit(0);
    count--; 
}

void do_add() {
    uint32_t a = stack[top_index--];
    uint32_t b = stack[top_index--];
    stack[++top_index] = a + b;
}

void do_sub() {
    uint32_t a = stack[top_index--];
    uint32_t b = stack[top_index--];
    stack[++top_index] = b - a;
}

void do_mul() {
    uint32_t a = stack[top_index--];
    uint32_t b = stack[top_index--];
    stack[++top_index] = a * b;
}

void do_div() {
    uint32_t a = stack[top_index--];
    uint32_t b = stack[top_index--];
    if(a == 0) { fprintf(stderr, "Error: Division by zero\n"); exit(1); }
    stack[++top_index] = b / a;
}

void do_shl() {
    uint32_t shift = stack[top_index--];
    uint32_t value = stack[top_index--];
    stack[++top_index] = value << shift;
}

void do_shr() {
    uint32_t shift = stack[top_index--];
    uint32_t value = stack[top_index--];
    stack[++top_index] = value >> shift;
}

void do_fp_add() {
    float a = to_float(stack[top_index--]);
    float b = to_float(stack[top_index--]);
    stack[++top_index] = from_float(a + b);
}

void do_fp_sub() {
    float a = to_float(stack[top_index--]);
    float b = to_float(stack[top_index--]);
    stack[++top_index] = from_float(b - a);
}

void do_fp_mul() {
    float a = to_float(stack[top_index--]);
    float b = to_float(stack[top_index--]);
    stack[++top_index] = from_float(a * b);
}

void do_fp_div() {
    float a = to_float(stack[top_index--]);
    float b = to_float(stack[top_index--]);
    if(a == 0.0f) { fprintf(stderr, "Error: Division by zero\n"); exit(1); }
    stack[++top_index] = from_float(b / a);
}

void do_end() {
    top_index = -1;
    memset(buffer, 0, BUFFER_SIZE);
}

void do_lod(uint32_t offset) {
    uint32_t value;
    memcpy(&value, buffer + offset, sizeof(uint32_t));
    stack[++top_index] = value;
}

void do_sto(uint32_t offset) {
    uint32_t value = stack[top_index--];
    memcpy(buffer + offset, &value, sizeof(uint32_t));
}

void do_immi(uint32_t value) {
    stack[++top_index] = value;
}

void do_inc() {
    uint32_t value = stack[top_index--];
    stack[++top_index] = value + 1;
}

void do_dec() {
    uint32_t value = stack[top_index--];
    stack[++top_index] = value - 1;
}

void do_sto_immi(uint32_t offset, uint32_t number) {
    memcpy(buffer + offset, &number, sizeof(uint32_t));
}

void do_memcpy(uint32_t dest, uint32_t src, uint32_t len) {
    memcpy(buffer + dest, buffer + src, len);
}

void do_memset(uint32_t dest, uint32_t val, uint32_t len) {
    memset(buffer + dest, val, len);
}

void do_gt() {
    uint32_t a = stack[top_index--];
    uint32_t b = stack[top_index--];
    stack[++top_index] = (b > a) ? 1 : 0;
}

void do_lt() {
    uint32_t a = stack[top_index--];
    uint32_t b = stack[top_index--];
    stack[++top_index] = (b < a) ? 1 : 0;
}

void do_eq() {
    uint32_t a = stack[top_index--];
    uint32_t b = stack[top_index--];
    stack[++top_index] = (b == a) ? 1 : 0;
}

void do_gt_eq() {
    uint32_t a = stack[top_index--];
    uint32_t b = stack[top_index--];
    stack[++top_index] = (b >= a) ? 1 : 0;
}

void do_lt_eq() {
    uint32_t a = stack[top_index--];
    uint32_t b = stack[top_index--];
    stack[++top_index] = (b <= a) ? 1 : 0;
}

void do_print() {
    if (top_index >= 0) {
        printf("%u\n", stack[top_index]);
    } else { fprintf(stderr, "Stack is empty.\n"); }
}

void do_read_int(uint32_t offset) {
    uint32_t val;
    scanf("%u", &val);
    memcpy(buffer + offset, &val, sizeof(uint32_t));
}

void do_fp_print() {
    if (top_index >= 0) {
        float f = to_float(stack[top_index]);
        printf("%f\n", f);
    } else { fprintf(stderr, "Stack is empty.\n"); }
}

void do_fp_read(uint32_t offset) {
    float val;
    scanf("%f", &val);
    uint32_t ival = from_float(val);
    memcpy(buffer + offset, &ival, sizeof(uint32_t));
}

void do_tik() { printf("tik\n"); }

int main() {
L0:
    goto L4608;
L2:
    goto L3;
L3:
    goto L4;
L4:
    do_sub();
    goto L5;
L5:
    goto L6;
L6:
    goto L7;
L7:
    goto L8;
L8:
    do_mul();
    goto L9;
L9:
    goto L10;
L10:
    goto L11;
L11:
    goto L12;
L12:
    do_mul();
    goto L13;
L13:
    goto L14;
L14:
    goto L15;
L15:
    do_add();
    goto L16;
L16:
    goto L17;
L17:
    goto L18;
L18:
    goto L19;
L19:
    goto L840106240;
L21:
    goto L22;
L22:
    do_print();
    goto L23;
L23:
    goto L24;
L24:
    goto L25;
L25:
    do_div();
    goto L26;
L26:
    goto L27;
L27:
    goto L28;
L28:
    goto L29;
L29:
    do_add();
    goto L30;
L30:
    goto L31;
L31:
    goto L32;
L32:
    goto L33;
L33:
    do_div();
    goto L34;
L34:
    goto L35;
L35:
    goto L36;
L36:
    goto L37;
L37:
    goto L38;
L38:
    goto L39;
L39:
    goto L40;
L40:
    goto L41;
L41:
    goto L42;
L42:
    goto L43;
L43:
    goto L44;
L44:
    goto L45;
L45:
    do_div();
    goto L46;
L46:
    goto L47;
L47:
    goto L48;
L48:
    goto L49;
L49:
    do_div();
    goto L50;
L50:
    goto L51;
L51:
    goto L52;
L52:
    goto L53;
L53:
    do_sub();
    goto L54;
L54:
    goto L55;
L55:
    goto L56;
L56:
    goto L57;
L57:
    goto L58;
L58:
    goto L59;
L59:
    goto L60;
L60:
    goto L61;
L61:
    do_print();
    goto L62;
L62:
    goto L63;
L63:
    goto L64;
L64:
    goto L65;
L65:
    do_add();
    goto L66;
L66:
    goto L67;
L67:
    goto L68;
L68:
    do_div();
    goto L69;
L69:
    goto L70;
L70:
    goto L71;
L71:
    goto L72;
L72:
    goto L73;
L73:
    goto L74;
L74:
    goto L75;
L75:
    goto L76;
L76:
    goto L77;
L77:
    goto L78;
L78:
    goto L79;
L79:
    goto L80;
L80:
    do_div();
    goto L81;
L81:
    goto L82;
L82:
    goto L83;
L83:
    goto L84;
L84:
    goto L85;
L85:
    goto L86;
L86:
    goto L87;
L87:
    goto L88;
L88:
    goto L89;
L89:
    goto L90;
L90:
    goto L91;
L91:
    goto L92;
L92:
    goto L93;
L93:
    goto L94;
L94:
    goto L95;
L95:
    goto L96;
L96:
    goto L97;
L97:
    goto L840106240;
L99:
    goto L100;
L100:
    do_print();
    goto L101;
L101:
    goto L102;
L102:
    goto L103;
L103:
    goto L104;
L104:
    goto L105;
L105:
    do_print();
    goto L106;
L106:
    goto L107;
L107:
    goto L108;
L108:
    goto L109;
L109:
    goto L110;
L110:
    goto L111;
L111:
    goto L112;
L112:
    goto L113;
L113:
    goto L114;
L114:
    goto L115;
L115:
    do_add();
    goto L116;
L116:
    goto L117;
L117:
    goto L118;
L118:
    goto L119;
L119:
    goto L120;
L120:
    goto L121;
L121:
    do_div();
    goto L122;
L122:
    goto L4163053056;
L124:
    goto L125;
L125:
    goto L126;
L126:
    goto L127;
L127:
    goto L128;
L128:
    goto L129;
L129:
    goto L130;
L130:
    goto L131;
L131:
    goto L132;
L132:
    goto L133;
L133:
    goto L134;
L134:
    goto L135;
L135:
    goto L136;
L136:
    goto L137;
L137:
    goto L823329024;
L139:
    goto L140;
L140:
    do_print();
    goto L141;
L141:
    goto L142;
L142:
    goto L143;
L143:
    goto L144;
L144:
    goto L321519872;
L146:
    do_add();
    goto L147;
L147:
    goto L148;
L148:
    goto L149;
L149:
    goto L150;
L150:
    goto L151;
L151:
    goto L152;
L152:
    goto L153;
L153:
    do_add();
    goto L154;
L154:
    goto L155;
L155:
    goto L156;
L156:
    goto L157;
L157:
    goto L158;
L158:
    goto L159;
L159:
    goto L160;
L160:
    goto L161;
L161:
    goto L162;
L162:
    goto L163;
L163:
    goto L164;
L164:
    goto L165;
L165:
    goto L166;
L166:
    goto L167;
L167:
    goto L168;
L168:
    goto L169;
L169:
    goto L170;
L170:
    goto L171;
L171:
    goto L172;
L172:
    goto L173;
L173:
    goto L174;
L174:
    goto L175;
L175:
    goto L176;
L176:
    goto L177;
L177:
    goto L178;
L178:
    do_sub();
    goto L179;
L179:
    goto L180;
L180:
    goto L181;
L181:
    goto L182;
L182:
    goto L183;
L183:
    goto L184;
L184:
    goto L185;
L185:
    goto L186;
L186:
    goto L187;
L187:
    goto L188;
L188:
    goto L189;
L189:
    goto L190;
L190:
    goto L191;
L191:
    goto L873660672;
L193:
    goto L194;
L194:
    do_print();
    goto L195;
L195:
    goto L196;
L196:
    goto L197;
L197:
    goto L198;
L198:
    goto L321519872;
L200:
    do_add();
    goto L201;
L201:
    goto L202;
L202:
    goto L203;
L203:
    goto L204;
L204:
    goto L205;
L205:
    goto L206;
L206:
    goto L207;
L207:
    do_add();
    goto L208;
L208:
    goto L209;
L209:
    goto L210;
L210:
    goto L211;
L211:
    goto L212;
L212:
    goto L213;
L213:
    goto L214;
L214:
    goto L215;
L215:
    goto L216;
L216:
    goto L302317824;
L218:
    do_div();
    goto L219;
L219:
    goto L220;
L220:
    goto L221;
L221:
    goto L222;
L222:
    goto L223;
L223:
    goto L224;
L224:
    goto L225;
L225:
    goto L226;
L226:
    goto L227;
L227:
    do_sub();
    goto L228;
L228:
    goto L229;
L229:
    goto L230;
L230:
    goto L231;
L231:
    goto L232;
L232:
    goto L233;
L233:
    goto L234;
L234:
    goto L235;
L235:
    goto L236;
L236:
    goto L237;
L237:
    goto L238;
L238:
    goto L239;
L239:
    goto L240;
L240:
    goto L1947402496;
L242:
    goto L243;
L243:
    do_print();
    goto L244;
L244:
    goto L245;
L245:
    goto L246;
L246:
    goto L247;
L247:
    goto L840106240;
L249:
    goto L250;
L250:
    do_print();
    goto L251;
L251:
    goto L252;
L252:
    goto L253;
L253:
    goto L254;
L254:
    goto L255;
L255:
    do_print();
    goto L256;
L256:
    goto L257;
L257:
    goto L258;
L258:
    goto L259;
L259:
    goto L260;
L260:
    do_print();
    goto L261;
L261:
    goto L262;
L262:
    goto L263;
L263:
    goto L264;
L264:
    goto L265;
L265:
    goto L266;
L266:
    goto L267;
L267:
    do_div();
    goto L268;
L268:
    goto L269;
L269:
    goto L270;
L270:
    goto L271;
L271:
    goto L272;
L272:
    goto L273;
L273:
    goto L274;
L274:
    goto L275;
L275:
    do_add();
    goto L276;
L276:
    goto L277;
L277:
    goto L278;
L278:
    goto L279;
L279:
    return 0;
}
