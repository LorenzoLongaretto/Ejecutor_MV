#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "disassembler.h"

void create_mnemonics(char v_mnemonics[255][5]){
int i;
    for (i=0;i<255;i++)
        strcpy(v_mnemonics[i],"");
    strcpy(v_mnemonics[0],"MOV");//2 operandos
    strcpy(v_mnemonics[1],"ADD");
    strcpy(v_mnemonics[2],"SUB");
    strcpy(v_mnemonics[3],"SWAP");
    strcpy(v_mnemonics[4],"MUL");
    strcpy(v_mnemonics[5],"DIV");
    strcpy(v_mnemonics[6],"CMP");
    strcpy(v_mnemonics[7],"SHL");
    strcpy(v_mnemonics[8],"SHR");
    strcpy(v_mnemonics[9],"AND");
    strcpy(v_mnemonics[10],"OR");
    strcpy(v_mnemonics[11],"XOR");
    strcpy(v_mnemonics[12],"SLEN");
    strcpy(v_mnemonics[13],"SMOV");
    strcpy(v_mnemonics[14],"SCMP");
    strcpy(v_mnemonics[240],"SYS");//1 operando
    strcpy(v_mnemonics[241],"JMP");
    strcpy(v_mnemonics[242],"JZ");
    strcpy(v_mnemonics[243],"JP");
    strcpy(v_mnemonics[244],"JN");
    strcpy(v_mnemonics[245],"JNZ");
    strcpy(v_mnemonics[246],"JNP");
    strcpy(v_mnemonics[247],"JNN");
    strcpy(v_mnemonics[248],"LDL");
    strcpy(v_mnemonics[249],"LDH");
    strcpy(v_mnemonics[250],"RND");
    strcpy(v_mnemonics[251],"NOT");
    strcpy(v_mnemonics[252],"PUSH");
    strcpy(v_mnemonics[253],"POP");
    strcpy(v_mnemonics[254],"CALL");
//strcpy(v_mnemonics[4080],"RET"); // 0 operando
    //strcpy(v_mnemonics[4081],"STOP");//0 operando PROBLEMA
}

void create_registers(char vec[16][3]){
int i;
for (i=0;i<16;i++)
    strcpy(vec[i],"");
strcpy(vec[0],"DS");
strcpy(vec[1],"SS");
strcpy(vec[2],"ES");
strcpy(vec[3],"CS");
strcpy(vec[4],"HP");
strcpy(vec[5],"IP");
strcpy(vec[6],"SP");
strcpy(vec[7],"BP");
strcpy(vec[8],"CC");
strcpy(vec[9],"AC");
strcpy(vec[10],"AX");
strcpy(vec[11],"BX");
strcpy(vec[12],"CX");
strcpy(vec[13],"DX");
strcpy(vec[14],"EX");
strcpy(vec[15],"FX");
}


void disassembler(int memory[],int i,char v_mnemonics[][5],char vec[][3]){

//printf("[%04d]:\t%s\t",i,v_mnemonics[mnem]);
int t1,t2,mnem,valor1,valor2,offset1,offset2;
int32_t aux;
printf("  [%04d]: %02X %02X %02X %02X \t",i,(memory[i]& 0xFF000000)>>24,(memory[i]& 0x00FF0000)>>16,(memory[i]& 0x0000FF00)>>8,memory[i]& 0x000000FF);
aux=memory[i]& 0xF0000000;
if(aux==0xF0000000){
    aux=memory[i]& 0xFF000000;
    if(aux==0xFF000000){//0 op
    //nada xd
    mnem=(memory[i]& 0xFFF00000)>>20;
    if(mnem==0xFF1)
        printf("STOP \n");
    else
        printf("RET \n");
    }
    else{//1 op
        mnem=(memory[i]& 0xFF000000)>>24;
        printf("%s  ",v_mnemonics[mnem]);
        t1=(memory[i]&0x00C00000)>>22;
        valor1=memory[i]&0x0000FFFF;
        valor1<<=16;valor1>>=16;
        offset1=valor1 & 0x00000FF0;
        offset1<<=20;offset1>>=24;
        switch(t1) {
        case 0: // inmediato
            printf("%d\n",valor1);
            break;
        case 1: // registro
            printf("%s\n",vec[valor1]);
            break;
        case 2: // directo
            printf("[%d]\n",valor1);
        break;
        case 3:
        if(offset1>=0)
            printf("[%s+%d] \n",vec[valor1 & 0x0000000F],offset1);
        else
            printf("[%s-%d] \n",vec[valor1 & 0x0000000F],offset1);
    break;
    }
    }
}
else{//2 op
   mnem=(memory[i]& 0xF0000000)>>28;
    printf("%s  ",v_mnemonics[mnem]);
    t1=(memory[i]&0x0C000000)>>26;
    valor1=(memory[i]&0x00FFF000);
    valor1<<=8;valor1>>=20;
    valor2=memory[i]&0x00000FFF;
    valor2<<=20;valor2>>=20;
    offset1=valor1 & 0x00000FF0;
    offset1<<=20;offset1>>=24;
    offset2=valor2 & 0x00000FF0;
    offset2<<=20;offset2>>=24;
    switch(t1) {
    case 0: // inmediato
        printf("%d  ",(valor1));
        break;
    case 1: // registro
        printf("%s  ",vec[valor1]);
        break;
    case 2: // directo
        printf("[%d]  ",(valor1));
    break;
    case 3:
        if(offset1>=0)
            printf("[%s+%d] ",vec[valor1 & 0x0000000F],offset1);
        else
            printf("[%s-%d] ",vec[valor1 & 0x0000000F],offset1);
    break;
    }
    t2=(memory[i]&0x03000000)>>24;
 switch(t2) {
    case 0: // inmediato
        printf("%d\n",(valor2));
        break;
    case 1: // registro
        printf("%s\n",vec[valor2]);
        break;
    case 2: // directo
        printf("[%d]\n",(valor2));
    break;
    case 3:
        if(offset2>=0)
            printf("[%s+%d] \n",vec[valor2 & 0x0000000F],offset2);
        else
            printf("[%s-%d] \n",vec[valor2 & 0x0000000F],offset2);
    break;
    }
}
}


void debug(int32_t memory[],int records[]){
char v_mnemonics[255][5];
char vec[16][3];
int IP=records[5],DS=high(records[3]);
int aux,aux2=0,i;
create_mnemonics(v_mnemonics);//los creo cada vez que llamo, hay que ver esto, pero creo que se libera el espacio de mem cuando sale de este void
create_registers(vec);
//disassembler(memory,i,v_mnemonics,vec);
system("cls");
if(IP+7>DS)
    aux=IP-(7+IP+7-DS);
else
    aux=IP-5;
if(aux<0){
    aux2=-aux;
    aux=0;
}
for(i=aux;i<IP;i++){
   disassembler(memory,i,v_mnemonics,vec);// llamo al disassembler
}
printf(">");
disassembler(memory,IP,v_mnemonics,vec);
aux=IP+1;
for(i=aux;i<IP+7+aux2 && i<DS;i++){
    disassembler(memory,i,v_mnemonics,vec);// llamo al disassembler
}

for(i=0;i<16;i++){
    printf("%s\t%d |",vec[i],records[i]);
    if(i==5 || i==10){
        printf("\n");
    }
}
//getchar();

}



