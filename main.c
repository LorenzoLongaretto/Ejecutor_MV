#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "disassembler.h"

#define DS 0
#define SS 1
#define ES 2
#define CS 3
#define HP 4
#define IP 5
#define SP 6
#define BP 7
#define CC 8
#define AC 9
#define AX 10
#define BX 11
#define CX 12
#define DX 13
#define EX 14
#define FX 15
typedef struct nodoC{
int header;
struct nodoC *sig;
//int pos;
}nodoC;

typedef nodoC *TListaC;

void (*functions[4082])(int, int);   // puntero a funciones (se definen todas con 2 parametros)
void load_functions(void (*functions[])(int, int));  // carga las funciones
void load_file(char *filename,int *error);
void load_record();
void muestraLC(TListaC LC);
void obtenervalores(int tipo,int32_t valor,int **aux);
void load_Reg(int32_t *reg,int tamanio,int direccion);
int high(int32_t reg);
int low(int32_t reg);
int segmentation(int segmento,int offset,int valor,int bloque);

int32_t memory[8192]; // memoria
int32_t records[16];  // registros del procesador
int b,c,d,i;

TListaC Disp,Ocup;

int main(int argc, char *argv[])
{
char *filename=argv[1];
int error  =0;
Disp=NULL,Ocup=NULL;
if(argc>1){
    load_record();   // Inicalizo en 0 todos los registros
    load_file(filename,&error);
    records[SP] = low(records[ES]-1);
    load_functions(functions);
    b=seekFlag("-b",argv,argc);
    c=seekFlag("-c",argv,argc);
    d=seekFlag("-d",argv,argc);

    if(c==1)
        system("cls");

    if(d==1){
        debug(memory,records);
    }
      if(error==0){
         while (records[IP]>=0 && records[IP]<low(records[DS])){
          execute();
        }
      }
      else
        printf("No se ejecuto ya que hay errores en la Carga\n");
/*
        printf("Disponibles: \n");
        muestraLC(Disp);
        printf("Ocupados: \n");
        muestraLC(Ocup);
*/
}
return 0;
}

void execute(){

int32_t aux,mnem,tipo1,tipo2,valor1 ,valor2;
int32_t *p1;
int32_t *p2;
tipo1=-1; tipo2=-1;
    aux=(memory[records[IP]]&0xF0000000);
    if(aux==0xF0000000){//1 o 0 operandos
        aux=memory[records[IP]]&0xFF000000;
        if (aux==0xFF000000){//0 operandos
            mnem=(memory[records[IP]]&0xFFF00000)>>20;
        }
        else{//1 operando
            mnem=(memory[records[IP]]&0xFF000000)>>24;
            tipo1=(memory[records[IP]]&0x00C00000)>>22;
            valor1=memory[records[IP]]&0x0000FFFF;
            valor1<<=16;valor1>>=16;
        }
    }
    else{//2 operandos
        mnem=(memory[records[IP]]&0xF0000000)>>28;
        tipo1=(memory[records[IP]]&0x0C000000)>>26;
        tipo2=(memory[records[IP]]&0x03000000)>>24;

        valor1=(memory[records[IP]]&0x00FFF000);
        valor1<<=8;valor1>>=20;//para negativos
        valor2=(memory[records[IP]]&0x00000FFF);
        valor2<<=20;valor2>>=20;
    }
    records[IP]++;
    p1=&valor1;
    p2=&valor2;
    obtenervalores(tipo1,valor1,&p1);
    obtenervalores(tipo2,valor2,&p2);
    functions[mnem](p1,p2);

}
void obtenervalores(int tipo,int32_t valor,int **aux){
    int memoria=0,i;
    int offset=0,segmento=0;
        if(tipo==1)
            *aux=&records[valor];
        else{
                //(low(records[DS])+valor)
            if(tipo==2)
                *aux=&memory[(low(records[DS])+valor)];
            else{
                if(tipo==3){

                 offset= valor & 0x00000FF0;
                 offset<<=20;offset>>=24;
                 switch(high(records[valor & 0x0000000F])){
                       case 0:
                              segmento = low(records[DS]);
                        break;
                       case 1:
                             segmento = low(records[SS]);
                        break;

                       case 2:
                             segmento = low(records[ES]);
                        break;
                 }

                 if( ((valor & 0x0000000F) == 6) || ((valor & 0x0000000F) ==7 ))
                   *aux= &memory[(offset + low(records[valor & 0x0000000F]))];
                else
                    *aux= &(memory[(offset + segmento + low(records[valor & 0x0000000F]))]);

                   if( ((valor & 0x0000000F) != 6) && ((valor & 0x0000000F) !=7 )){
                        if(segmentation(segmento,offset,low(records[valor & 0x0000000F]),high(records[valor & 0x0000000F]))){
                                          printf("Segmentation Fault\n");
                                          records[IP] = -1;
                        }
                    }
          }
       }
   }
}
int segmentation(int segmento,int offset,int valor,int bloque){
int memoria = segmento+offset+valor;
 if(bloque==3){  // CS
           if(memoria<0 || memoria>high(records[CS]))
            return 1;
 }
 else{
    if(bloque==2){ // ES (ulimo bloque)
        if(memoria<low(records[ES]) || memoria>(low(records[ES])+high(records[ES])))
            return 1;
    }
    else{  // DS o SS
           if(memoria<low(records[bloque]) || memoria>low(records[bloque+1]))
            return 1;
    }
 }
return 0;
}
void load_file(char *filename,int *error){  // Este metodo debe lanzar error si no esta el MV21 o si se pasa de memoria, y cargar los registros CS,DS,SS,ES con parte alta y baja
int i=0,suma=0,j;
int32_t linea;
FILE *arch;
arch =fopen(filename,"rb");
    while(fread(&linea,sizeof(linea),1,arch)==1){  // leo los primeros 5 bloques
            if(i<=4){
                        if(i==0 && linea!=1297494577)   // no esta "MV21" en la primer linea
                            *error=1;
                        else{
                            if(i!=0)
                                records[i-1]  |= (linea<<16); // cargo parte alta de los registros los registros
                        }

                        if(i==4)
                            suma=linea;     // valor de CS;
            }
            else{
                memory[i-5] = linea;
            }
             i++;
        }
                   load_Reg(&records[CS],suma,0); // cargo CS
                    for(j=0;j<3;j++){
                        records[j]|=(suma);
                        suma+=high(records[j]);
                    }
                    if(suma>8192){   // si me paso de memoria
                       *error=1;
                       printf("Exceso de Memoria\n");
                    }

    fclose(arch);
}

void load_record(){
int i=0;
for (i=0;i<15;i++)
    records[i]=0;
records[HP] = -1; //inicializo en -1 (NULL) , para la memoria dinamica
}
int seekFlag(char* flag,char* aux[],int argc){
    int i;
    for(i=0;i<argc;i++){
        if(strcmp(flag,aux[i])==0)
            return 1;
    }
    return 0;
}
void load_Reg(int32_t *reg,int tamanio,int direccion){

     *reg=0; // meti cambio aca porq sino te sumaba siempre por el OR
     *reg |= tamanio << 16;
     *reg |= direccion;
}
int high(int32_t reg){         // Parte alta
    return (reg & 0xFFFF0000)>>16;
}


int low(int32_t reg){         //    Parte Baja
    return (reg & 0x0000FFFF);
}
int memoria_indireccion(int32_t reg){
int segmento = 0,i;
    switch(high(reg)){
          case 0:
                segmento = low(records[DS]);
        break;
           case 1:
                segmento = low(records[SS]);
        break;
           case 2:
                segmento = low(records[ES]);
        break;

    }
    i=high(reg);
    if(segmentation(segmento,0,low(reg),high(reg)))
    {
        printf("Segmentation Fault\n");
        records[IP] = -1; // corto la ejecucion;
    }
return (segmento + low(reg));
}

void cargaListas(TListaC Disp,TListaC Ocup){
    TListaC ant,act;
    ant=NULL;
    act = Disp;
    do{
       ant=act;
       act=act->sig;
     memory[low(ant->header) + low(records[ES])] = act->header;

    }while(act!=Disp);

    ant=NULL;
    act = Ocup;
    do{
        ant=act;
        act=act->sig;
        memory[low(ant->header) + low(records[ES])] = act->header;

    }while(act!=Ocup);
}
// es la lista de ocupados,
void addOcupado(TListaC *LC,int header,int pos){
TListaC nuevo,ant,act;
nuevo = (TListaC)malloc(sizeof(nodoC));
 if(*LC==NULL){  // Lista Vacia
         nuevo->header = header;
        *LC = nuevo;
        (*LC)->sig=*LC;
    }
    else{
            ant=*LC;
            act=(*LC)->sig;
            if(act==ant){    // unico de la lista

                if(low(act->header)<pos)
                       *LC=nuevo;
                load_Reg(&nuevo->header,high(header),low(ant->header));
                    nuevo->sig=act;
                    act->sig =nuevo;
                    load_Reg(&act->header,high(act->header),pos);
            }
            else{  // Busco
                   ant=*LC;
                   act = (*LC)->sig;
                do{
                         ant=act;
                        act=act->sig;

                }while(low(ant->header)<pos && ant!=*LC);

                load_Reg(&nuevo->header,high(header),low(ant->header));
                load_Reg(&ant->header,high(ant->header),pos);
                ant->sig=nuevo;
                nuevo->sig=act;

                if(*LC == ant)
                    *LC=nuevo;
      }

    }
    load_Reg(&records[HP],high(records[HP]),pos); // cambio el valor de HPL
}
void muestraLC(TListaC LC){

if(LC!=NULL){
        TListaC act = LC->sig;
 do{
      printf("Parte Alta: %d -- Parte Baja: %d\n",high(act->header),low(act->header));
    act=act->sig;
}while(act!=LC->sig);

}
else
    printf("La lista esta vacia\n");


}
void eliminaNodoLibre(TListaC *LC,int header){
TListaC act,ant=NULL;
int encontrado=0;
    if(*LC!=NULL)
    {
        act=*LC;
        if(ant==act){
            *LC=NULL;
            free(act);
        }
        else{

            do{
            ant=act;
            act=act->sig;
            if(act->header == header) // encontro el header
            {

                encontrado=1;
                if(high(records[HP] == low(ant->header))){  // cambio el HP
                    load_Reg(&records[HP],low(act->header),low(records[HP]));
                }
                if(act==*LC)     // cabecera
                {
                    if(act==ant)  // unico de la lista
                        *LC=NULL;
                    else
                    {
                        ant->sig=act->sig;
                        *LC=ant;
                    }
                }
                else
                {
                    ant->sig=act->sig; //aislas actual para hacer free
                }
                load_Reg(&(ant->header),high(ant->header),low(act->header));   // cambio el anterior
                free(act);
            }
        }while(act!=*LC && !encontrado);
    }
}
}
void eliminaNodoOcupado(TListaC *LC,int header){
TListaC act=(*LC)->sig,ant=*LC;
int encontrado=0;
    if(*LC!=NULL)
    {
        if(act == ant){
            *LC=NULL;
            free(act);
        }else{
                 do{
                ant=act;
                act=act->sig;
                if(act->header == header) // encontro el header
                {

                    encontrado=1;
                    if(low(records[HP]) == low(ant->header)){  // cambio el HPL
                        load_Reg(&records[HP],high(records[HP]),low(act->header));
                    }
                    if(act==*LC)     // cabecera
                    {
                        if(act==ant)  // unico de la lista
                            *LC=NULL;
                        else
                        {
                            ant->sig=act->sig;
                            *LC=ant;
                        }
                    }
                    else
                    {
                        ant->sig=act->sig; //aislas actual para hacer free
                    }
                    load_Reg(&(ant->header),high(ant->header),low(act->header));   // cambio el anterior
                    free(act);
                }
            }while(act!=(*LC)->sig && !encontrado);
        }

}
}

void addLibre(TListaC *LC,int header,int pos){

TListaC nuevo,ant,act;
nuevo = (TListaC)malloc(sizeof(nodoC));

 if(*LC==NULL){  // Lista Vacia
        nuevo->header=header;
        *LC = nuevo;
        (*LC)->sig=*LC;
    }
    else{
            ant=*LC;
            act=(*LC)->sig;
            if(act==ant){    // unico de la lista

                if(low(act->header)<pos)
                       *LC=nuevo;
                load_Reg(&nuevo->header,high(header),low(ant->header));
                    nuevo->sig=act;
                    act->sig =nuevo;
                    load_Reg(&act->header,high(act->header),pos);
            }
            else{            // Busco
                   ant=NULL;
                   act = (*LC);
                do{
                         ant=act;
                        act=act->sig;

                }while(low(ant->header)<pos && act!=*LC);

                load_Reg(&nuevo->header,high(header),low(ant->header));
                load_Reg(&ant->header,high(ant->header),pos);
                ant->sig=nuevo;
                nuevo->sig=act;

                //if(*LC == ant)
                  //  *LC=nuevo;
      }
      if((*LC)->sig == nuevo)
        load_Reg(&records[HP],high(records[HP]),low(ant->header));  // cambio parte baja
     }
}
void changeCC(int *valor){
records[CC]= 0;
if (*valor==0)
        records[CC] += 1; // pongo 1 en el bit menos significativo

    if(*valor<0)
        records[CC] |= (1<<31);// pongo 1 en el bit mas significativo

}


void SMOV(int *valor1,int *valor2){

    while(*valor2!='\0'){
            *valor1 = *valor2;

            valor1++;
            valor2++;
        }
     *valor1 = *valor2; // paso el /0
}

void SLEN(int *valor1,int *valor2){
    int length=0,i;

    while(*valor2 != '\0'){
        length++;
        valor2++;
    }
    *valor1=length;
}

void SCMP(int *valor1,int *valor2){
int aux;

   do
    {
        aux=*valor1-*valor2;
        valor1++;
        valor2++;
    }
    while(aux==0 && ((*valor1!='\0')||(*valor2!='\0')));
    changeCC(&aux);
}

void PUSH(int *valor1,int *valor2){
    if(records[SP]==0){
        printf("Stack OverFlow\n");
        records[IP]=-1;
    }

    else{
        records[SP]-=1;
        memory[records[SP]] = *valor1;
    }
}
void POP(int *valor1,int *valor2){
  if(records[SP]==low(records[ES]+1)){
     printf("Stack UnderFlow\n");
     records[IP] = -1;
  }
  else{
    *valor1 = memory[records[SP]];
    records[SP]+=1;
  }

}
void CALL(int *valor1,int *valor2){

   records[SP]-=1;
   memory[records[SP]]  = records[IP]; // guarda el retorno
   records[IP] = *valor1;//se salta a la posicion
}

void RET(int *valor1,int *valor2){

    records[IP] = memory[records[SP]];
    records[SP] +=1;
}
void MOV(int *valor1,int *valor2){
    *valor1= *valor2;
}
void ADD(int *valor1,int *valor2){
    *valor1+=*valor2;
    changeCC(valor1);
}
void SUB(int *valor1,int *valor2){
    *valor1-=*valor2;
    changeCC(valor1);
}

void MUL(int *valor1,int *valor2){
    *valor1*= *valor2;
    changeCC(valor1);
}
void DIV(int *valor1,int *valor2){
int resto;
    resto= *valor1 % *valor2;
    *valor1 /=*valor2;
    changeCC(valor1);
    records[AC] = resto; // acumulo el resto de la division en AC
}

void SWAP(int *valor1,int *valor2){
int32_t aux;

    aux=*valor1;
    *valor1= *valor2;
    *valor2 =aux;
}

void CMP(int *valor1,int *valor2){
int aux;
    aux= (*valor1)-(*valor2);
    changeCC(&aux);

}

void AND(int *valor1,int *valor2){
    *valor1&=*valor2;
    changeCC(valor1);

}

void OR(int *valor1,int *valor2){
    *valor1|=*valor2;
    changeCC(valor1);
}

void XOR(int *valor1,int *valor2){
    *valor1^=*valor2;
    changeCC(valor1);

}

void NOT(int *valor1,int *valor2){
    *valor1 =~ *valor1;
}

void SYS(int *v1,int *v2){//leo v1
    switch(*v1){
    case 1://Read
       SYS_Read();
        break;
    case 2://Write
        SYS_Write();
        break;
    case 3:
        String_Read();
        break;
    case 4:
        String_Write();
        break;
    case 5:
        New();
        break;
    case 6:
        Free();
        break;
    case 7:
        system("cls");
        break;
    case 15://Breakpoint
        SYS_BP();
        break;
    }
}
///////////SYS///////////////////////////////////////////////////////////////////////////////
void SYS_Read(){

    int inicio=0,cant=low(records[CX]),i=0,aux,completo=0;
    char palabra[50];
// printf("-----------------\n");
 inicio = memoria_indireccion(records[DX]);
  while(i<cant && completo!=1){
        if((records[AX]&0x800)==0){ // prompt
            printf("[%04d]: ",inicio+i);
        }
        if((records[AX]&0x100)!=0){
                // lee caracter a caracter y guarda uno por celda
            if(i==0)
            scanf("%s",palabra);

           memory[inicio+i]=palabra[i];
           if(i==strlen(palabra))
            completo=1;
        }
        else{

               if((records[AX]&0x008)!=0){ // hexa
                scanf("%x",&(memory[inicio+i]));
               }
               if((records[AX]&0x004)!=0){ // octal
                scanf("%o",&(memory[inicio+i]));
               }
               if((records[AX]&0x001)!=0){ // decimal
                scanf("%d",&(memory[inicio+i]));
               }
           }
           i++;
     }
}

void SYS_Write(){
    int inicio=0,cant=low(records[CX]),i,aux,prompt=0;
// printf("-----------------\n");
 inicio = memoria_indireccion(records[DX]);
    for(i=0;i<cant;i++){

        if((records[AX]&0x800)==0 /*&& prompt==0*/){ // prompt
            printf("[%04d]: ",inicio+i);
            prompt=1;
        }
         if((records[AX]&0x010)!=0){// imprime el menos significativo como caracter
            aux=memory[inicio+i]/*& 0x0000000F*/;
            printf(" %c",aux);
        }
         if((records[AX]&0x008)!=0){ // hexa, pusimos la x para que sea mas legible
            printf(" x%X",memory[inicio+i]);
        }
         if((records[AX]&0x004)!=0){ // octal
            printf(" @ %o",memory[inicio+i]);
        }
         if((records[AX]&0x001)!=0){ // decimal
            printf(" %d",memory[inicio+i]);
        }
        if((records[AX]&0x100)==0){
            printf("\n");
        }
    }
}

void String_Read(){
    int inicio=0,cant=low(records[CX]),i=0,completo=0,segmento=0;
    char cadena[20];
   // printf("-----------------\n");
    inicio = memoria_indireccion(records[DX]);

            if((records[AX]&0x800)==0 ){ // prompt
            printf("[%04d]: ",inicio+i);
        }
        gets(cadena);
        while(i<cant && completo!=1){
        memory[inicio+i] = cadena[i];
        if(i==strlen(cadena)) // +1 asi paso el fin de cadena
            completo=1;
        i++;
    }
    if(i<strlen(cadena))  // paso el \0 si es que cant era menor que la cantidad de caracteres
        memory[i+1] = '\0';
}
void String_Write(){

int inicio=0,segmento=0,i=0,bandera=0;
 //printf("-----------------\n");
inicio = memoria_indireccion(records[DX]); //records[DX];

    while(memory[inicio+i]!='\0'){

        if((records[AX]&0x800)==0 && bandera==0){ // prompt
            printf("[%04d]: ",inicio+i);
            bandera=1;
        }

            printf("%c",memory[inicio+i]);


        i++;
    }
    if((records[AX]&0x100)==0){  // endline
            printf("\n");
        }
}
// HPH  = cantidad de celdas del ES y 0.
void New(){

int cant = low(records[CX]),bandera=0,i,pos,header=0,memoria;

TListaC aux,ant;


     if(Disp == NULL){ // inicializo lista y registro parte  alta de HP en 0
         load_Reg(&header,high(records[ES]),0);
         addLibre(&Disp,header,0); // meter el primer header
         records[HP] = 0x0000FFFF;
     }
        ant=Disp;
        aux=Disp->sig; // primero de la lista
        i=high(records[HP]);// indica la celda de memoria, del Extra Segment.

        do{
           if(cant<= high(aux->header)){ //reservo memoria
                pos=low(ant->header);
                bandera=1;
                if(cant==high(aux->header)){ // elimino nodo de disponibles
                  eliminaNodoLibre(&Disp,aux->header);
                }
                else{    // cambio header del disponible

                   if(ant!=aux){
                     load_Reg(&aux->header,high(aux->header)-cant-1,low(aux->header));// cambio alta del actual
                     load_Reg(&ant->header,high(ant->header),low(ant->header)+cant+1);
                   }
                   else
                      load_Reg(&aux->header,high(aux->header)-cant-1,low(aux->header)+cant+1);

                   if(high(records[HP]) == i)    // cambio HP
                     load_Reg(&records[HP],low(ant->header),low(records[HP]));
                }
                load_Reg(&header,cant,pos);
                addOcupado(&Ocup,header,pos);   // agrego a la lista de ocupados
                load_Reg(&records[DX],ES,i+1); // le asigno a DX la celda de memoria+1, ya que esta el header.

           }
           else{
                ant=aux;
                aux=aux->sig;
           }
         // cargo en memoria los nodos

         i=low(ant->header);

      }while(aux!=Disp->sig && bandera!=1);

 if(bandera==0){ //no se pudo hacer new
      printf("No hay memoria Disponible\n");
      records[IP] = -1;   // Corto la ejecucion
 }
cargaListas(Disp,Ocup);
}

void Free(){
int celda = low(records[DX]),bandera=0;
TListaC act,ant;
ant=Ocup;
act=Ocup->sig;
//celda =  memoria_indireccion(records[DX]);
    do{
         if(celda <= low(ant->header) + high(act->header)){ // elimino ese nodo
          bandera=1;
          addLibre(&Disp,act->header,low(ant->header));
          eliminaNodoOcupado(&Ocup,act->header);
         }
         else{
             ant=act;
         act=act->sig;
         }

    }while(Ocup!=NULL && act!=Ocup->sig && bandera!=1);

Acoplamiento(&Disp);
Acoplamiento(&Disp);
cargaListas(Disp,Ocup);
}
void Acoplamiento(TListaC *LC){
    int header;
    TListaC act,ant;
    ant = *LC;
    act=(*LC)->sig;
    do{
        if(low(ant->header) + high(act->header) + 1 == low(act->header)){
            header = act->sig->header;
            load_Reg(&act->header,high(act->header)+high(header)+1,low(header));
            eliminaNodoLibre(LC,header); // elimina el siguiente
        }
        else{
            ant=act;
            act=act->sig;
        }

    }while(act!=(*LC)->sig);
}
void SYS_BP(){ // es la pocion -b nada mas

char direcciones[20],inicio[5],fin[5];
int i,j,k;

//flag -c
if(records[IP]<low(records[DS]) && records[IP]>0){
        if(c)
            system("cls");
//flag -d
        if(d)
            debug(memory,records);
//flag -b
        if(b){
            printf("[%04d]:cmd:",records[IP]);

            gets(direcciones);
            if(direcciones[0]!='\0'){
                if(direcciones[0]!='p'){

                    strcpy(fin,"NULL");
                    sscanf(direcciones,"%s %s",inicio,fin);

                     j=atoi(inicio);
                    if(strcmp(fin,"NULL")!=0){// hay dos direcciones
                        k=atoi(fin);
                        for(i=j;i<=k;i++){
                                printf("[%04d]: %d\n",i,memory[i]);
                        }
                    }
                    else
                          printf("[%04d]: %d\n",j,memory[j]);
                }else{
                   // if(records[IP]<records[DS] && records[IP]>0){
                        execute();
                        SYS_BP();
                  //  }
                }
            }
        }
}
}

///////////////////////////////////////////////////////////////////////////

void JMP(int *valor1,int *valor2){

    records[IP]=*valor1;

}

void JP(int *valor1,int *valor2){

if((records[CC]&0x80000000)==0)//el bit de signo de CC es cero
        records[IP]=*valor1;
}

void JNP(int *valor1,int *valor2){ //jump not positive

if(!((records[CC]&0x80000000)==0) || records[CC]&00000001==1)
        records[IP]=*valor1;
}

void JN(int *valor1,int *valor2){

if((records[CC] & 0x80000000)!=0)//bit de signo de CC está en 1
        records[IP]=*valor1;

}

void JNN(int *valor1,int *valor2){

if(!((records[CC] & 0x80000000)!=0))//bit de signo de CC está en 1
        records[IP]=*valor1;

}

void JZ(int *valor1,int *valor2){
if((records[CC] & 0x00000001)!=0) // bit zero esta prendido
    records[IP]=*valor1;
}

void JNZ(int *valor1,int *valor2){
if(!((records[CC] & 0x00000001)!=0)) // bit zero esta prendido
    records[IP]=*valor1;
}


void SHL(int *valor1,int *valor2){

    *valor1<<=(*valor2);
    changeCC(valor1);
}


void SHR(int *valor1,int *valor2){
    *valor1>>=(*valor2);
    changeCC(valor1);
}
void LDH(int *valor1,int *valor2){

     records[AC] &= 0x0000FFFF;
    *valor1&=0x0000FFFF;
    *valor1<<=16;
    records[AC] |=*valor1;

}

void LDL(int *valor1,int *valor2){
    records[AC] &= 0xFFFF0000;
    *valor1&=0x0000FFFF;
    records[AC] |=*valor1;
}

void RND(int *valor1,int *valor2){
   srand(time(0));
   records[AC]  = (rand() % (*valor1+1));
}

void STOP(int *valor1,int *valor2){
    records[IP]=-1;
}

//functions[mnem](tipo1,tipo2,valor1,valor2);
void load_functions(void (*functions[])(int, int)){
    functions[0]=MOV;//2 operandos
    functions[1]=ADD;
    functions[2]=SUB;
    functions[3]=SWAP;
    functions[4]=MUL;
    functions[5]=DIV;
    functions[6]=CMP;
    functions[9]=AND;
    functions[10]=OR;
    functions[11]=XOR;
    functions[7]=SHL;
    functions[8]=SHR;
    functions[13]=SMOV;
    functions[12]=SLEN;
    functions[14]=SCMP;
    functions[241]=JMP;//1 operando
    functions[242]=JZ;
    functions[243]=JP;
    functions[244]=JN;
    functions[245]=JNZ;
    functions[251]=NOT;
    functions[240]=SYS;
    functions[246]=JNP;
    functions[247]=JNN;
    functions[248]=LDL;
    functions[249]=LDH;
    functions[250]=RND;
    functions[252]=PUSH;
    functions[253]=POP;
    functions[254]=CALL;
    functions[4080]=RET;
    functions[4081]=STOP;//0 operando
}
