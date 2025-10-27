#include <stdint.h>
#include <stdbool.h>
#include "arch/i686/asm_wrappers.h"
#include "arch/i686/irq.h"
#include "stdio.h"
#include "memory.h"
#include "keyboard.h"
#include "shell/shell.h"

#define KEYBOARD_PORT 0x60


const uint32_t UNKNOWN = 0xFFFFFFFF;
const uint32_t ESC = 0xFFFFFFFF - 1;
const uint32_t CTRL = 0xFFFFFFFF - 2;
const uint32_t LSHFT = 0xFFFFFFFF - 3;
const uint32_t RSHFT = 0xFFFFFFFF - 4;
const uint32_t ALT = 0xFFFFFFFF - 5;
const uint32_t F1 = 0xFFFFFFFF - 6;
const uint32_t F2 = 0xFFFFFFFF - 7;
const uint32_t F3 = 0xFFFFFFFF - 8;
const uint32_t F4 = 0xFFFFFFFF - 9;
const uint32_t F5 = 0xFFFFFFFF - 10;
const uint32_t F6 = 0xFFFFFFFF - 11;
const uint32_t F7 = 0xFFFFFFFF - 12;
const uint32_t F8 = 0xFFFFFFFF - 13;
const uint32_t F9 = 0xFFFFFFFF - 14;
const uint32_t F10 = 0xFFFFFFFF - 15;
const uint32_t F11 = 0xFFFFFFFF - 16;
const uint32_t F12 = 0xFFFFFFFF - 17;
const uint32_t SCRLCK = 0xFFFFFFFF - 18;
const uint32_t HOME = 0xFFFFFFFF - 19;
const uint32_t UP = 0xFFFFFFFF - 20;
const uint32_t LEFT = 0xFFFFFFFF - 21;
const uint32_t RIGHT = 0xFFFFFFFF - 22;
const uint32_t DOWN = 0xFFFFFFFF - 23;
const uint32_t PGUP = 0xFFFFFFFF - 24;
const uint32_t PGDOWN = 0xFFFFFFFF - 25;
const uint32_t END = 0xFFFFFFFF - 26;
const uint32_t INS = 0xFFFFFFFF - 27;
const uint32_t DEL = 0xFFFFFFFF - 28;
const uint32_t CAPS = 0xFFFFFFFF - 29;
const uint32_t NONE = 0xFFFFFFFF - 30;
const uint32_t ALTGR = 0xFFFFFFFF - 31;
const uint32_t NUMLCK = 0xFFFFFFFF - 32;

const uint32_t lowercase[128] = {
UNKNOWN,ESC,'1','2','3','4','5','6','7','8',
'9','0','-','=','\b','\t','q','w','e','r',
't','y','u','i','o','p','[',']','\n',CTRL,
'a','s','d','f','g','h','j','k','l',';',
'\'','`',LSHFT,'\\','z','x','c','v','b','n','m',',',
'.','/',RSHFT,'*',ALT,' ',CAPS,F1,F2,F3,F4,F5,F6,F7,F8,F9,F10,NUMLCK,SCRLCK,HOME,UP,PGUP,'-',LEFT,UNKNOWN,RIGHT,
'+',END,DOWN,PGDOWN,INS,DEL,UNKNOWN,UNKNOWN,UNKNOWN,F11,F12,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN
};

const uint32_t uppercase[128] = {
UNKNOWN,ESC,'!','@','#','$','%','^','&','*','(',')','_','+','\b','\t','Q','W','E','R',
'T','Y','U','I','O','P','{','}','\n',CTRL,'A','S','D','F','G','H','J','K','L',':','"','~',LSHFT,'|','Z','X','C',
'V','B','N','M','<','>','?',RSHFT,'*',ALT,' ',CAPS,F1,F2,F3,F4,F5,F6,F7,F8,F9,F10,NUMLCK,SCRLCK,HOME,UP,PGUP,'-',
LEFT,UNKNOWN,RIGHT,'+',END,DOWN,PGDOWN,INS,DEL,UNKNOWN,UNKNOWN,UNKNOWN,F11,F12,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,
UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN,UNKNOWN
};

#define MAX_SIZE 128

bool g_capsOn = false;
bool g_capsLock = false;
static char kb_buffer[MAX_SIZE];
static int kb_head = 0;
static int kb_tail = 0;

void kb_buffer_push(char c) {
    int next = (kb_head + 1) % MAX_SIZE;
    if (next != kb_tail) {
        kb_buffer[kb_head] = c;
        kb_head = next;
    }
}

bool keyboard_haschar() {
    return kb_head != kb_tail;
}

char keyboard_getchar() {
    if (kb_head == kb_tail) return 0;
    char c = kb_buffer[kb_tail];
    kb_tail = (kb_tail + 1) % MAX_SIZE;
    return c;
}


void Keyboard_Init() {
    i686_IRQ_RegisterHandler(1, &keyboardHandler);
}
void keyboardHandler(Registers* regs) {
    int Y = getY();
    bool extended = false;
    // retrieve data from keyboard
    uint8_t code = i686_inb(KEYBOARD_PORT);
    i686_io_wait();

    if (code == 0xE0) {
        extended = true;
        return;  // wait for the next byte, this only happens for a few keys like the arrows
    }

    bool press = code & 0x80;       // was the button pressed or released?
    uint8_t scancode = code & 0x7F; // what button was it?

    switch(scancode) {
        case 1:             // escape
        case 14:            // backspace
            if (press == 0 && Y > 0) {
                kb_buffer_push('\b');
            }
            break;
        case 28:            // enter
            if (press == 0 && Y > 0) {
                kb_buffer_push('\n');
            }
            break;
        case 29: 
        case 54:            // right shift
            if (press == 0) g_capsOn = true;
            else g_capsOn = false;
            break;
        case 56:
        case 59:
        case 60:            // function keys
        case 61:
        case 62:
        case 63:
        case 64:
        case 65:
        case 66:
        case 67:
        case 68:            // functions keys end
        // case 72:
        //     if (press == 0 && Y > 1) movecursor(scancode);
        //     break;
        case 75:
            if (press == 0) movecursor(scancode);
            break;
        case 77:
            if (press == 0) movecursor(scancode);
            break;
        // case 80:
        //     if (press == 0 && Y > 0) movecursor(scancode);
        //     break;
        case 87:
        case 88:
            break;
        
        case 42:            // left shift
            if (press == 0) g_capsOn = true;
            else g_capsOn = false;
            break;
        case 58:            // caps lock
            if (!g_capsLock && press == 0) g_capsLock = true;
            else if (g_capsLock && press == 0) g_capsLock = false;
            break;
        default:
            if (press == 0) {
                if ((g_capsOn || g_capsLock) && Y > 0) {
                    kb_buffer_push(uppercase[scancode]);
                }
                else if (Y > 0) {
                    kb_buffer_push(lowercase[scancode]);
                }
                else {
                    break;
                }
            }
    }
    
    // debugging purposes and seeing what each key's scancode is
    // printk("Scan code: %d, 0x%x, Press: %d\n", scancode, scancode, press);
}