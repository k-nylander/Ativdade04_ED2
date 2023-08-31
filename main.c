#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

// Pronto
typedef struct beneficiario { //____001#Joao#Unimed#Plano de Saude
    int ID;
    char Nome[50];
    char Seguradora[50];
    char tipoSeg[30];
} Beneficiario;

char buffer[139];
char filename[30];

// Pronto
char* transformaEmChar(int value) {
    char *result = malloc(sizeof(char) * 4);
    if (value < 0 || value > 999) {
        strcpy(result, "ERR");
    } else {
        snprintf(result, 4, "%03d", value);
    }
    return result;
}

// Pronto
int transformaEmInt(char *value) {
    if (strlen(value) != 3) {
        return -1;
    }
    return atoi(value);
}

//Pronto
char* concat(int numInputs, ...) {
    va_list args;
    va_start(args, numInputs);

    int totalLength = 0;
    for (int i = 0; i < numInputs; i++) {
        totalLength += strlen(va_arg(args, const char*));
    }
    totalLength += numInputs - 1;
    va_end(args);

    char* result = (char*)malloc(totalLength + 1);
    if (result == NULL) {
        return NULL;
    }

    va_start(args, numInputs);
    strcpy(result, va_arg(args, const char*));
    for (int i = 1; i < numInputs; i++) {
        strcat(result, "#");
        strcat(result, va_arg(args, const char*));
    }
    strcat(result, "\0");
    va_end(args);

    return result;
}

// Pronto
char* concatBeneficiario(Beneficiario *input) {
    strcpy(buffer, concat(4, transformaEmChar(input->ID), input->Nome, input->Seguradora, input->tipoSeg));
    return buffer;
}

// Pronto
int busca_espaco_livre_na_lista(FILE *fp, int tamanho) {
    int posicao_inicial = ftell(fp);

    fseek(fp,0,0);

    int atpos = 0;
    fread(&atpos, sizeof(int), 1, fp);

    while (atpos != -1) {
        fseek(fp, atpos, SEEK_SET);
        int tam_buffer;
        fread(&tam_buffer, sizeof(int), 1, fp);

        int next_pos = -1;
        fseek(fp, sizeof(char), SEEK_CUR);
        fread(&next_pos, sizeof(int), 1, fp);

        if (tam_buffer >= tamanho + 2 * sizeof(int) + sizeof(char)) {
            fseek(fp, posicao_inicial, SEEK_SET);
            return atpos;
        } else if(tam_buffer == tamanho) {
            return atpos;
        }else{
            atpos = next_pos;
        }
    }
    fseek(fp, posicao_inicial, SEEK_SET);
    return -1;
}

// Pronto
int busca_espaco_livre(FILE *fp, int size) {
    int byte_offset = busca_espaco_livre_na_lista(fp, size);

    if (byte_offset != -1) {
        return byte_offset;
    }

    fseek(fp, 0, SEEK_END);
    return ftell(fp);
}

// Pronto
void Insere(char *string, FILE *fp) {   
    int string_len = strlen(string), gapsize = 0, byteindex = busca_espaco_livre(fp, string_len);
    fseek(fp, byteindex, SEEK_SET);
    fread(&gapsize, sizeof(int), 1, fp);
    
    //Gambiarra
    int curPos = ftell(fp);
    fseek(fp, 0, SEEK_END);
    int endPos = ftell(fp);
    fseek(fp, curPos, 0);

    if(gapsize == string_len){
        fseek(fp, byteindex + sizeof(int), SEEK_SET);
        fwrite(string, string_len, 1, fp);
    }else if(byteindex == endPos){
        fseek(fp, 0, SEEK_END);
        fwrite(&string_len, sizeof(int), 1, fp);
        fwrite(string, string_len, 1, fp);
    }else{
        fseek(fp, byteindex, SEEK_SET);
        fwrite(&string_len, sizeof(int), 1, fp);
        fwrite(string, string_len, 1, fp);
        gapsize -= string_len + sizeof(int);
        fwrite(&gapsize, sizeof(int), 1, fp);
        fwrite("*", sizeof(char), 1, fp);
        int indexAntigo, indexRemovido = ftell(fp) - sizeof(char) - sizeof(int);
        //Pulou para trás
        rewind(fp);
        fread(&indexAntigo, sizeof(int), 1, fp);
        rewind(fp);
        fwrite(&indexRemovido, sizeof(int), 1, fp);
        fseek(fp, indexRemovido + sizeof(int) + sizeof(char), SEEK_SET);
        fwrite(&indexAntigo, sizeof(int), 1, fp);
    }
}

// Pronto
void remover (FILE * fp, int remocao){
    char codigoRemocao[4];
    strcpy(codigoRemocao, transformaEmChar(remocao));
    fseek(fp, 2 * sizeof(int), SEEK_SET); // Pula para o primeiro registro
    while(!feof(fp)){
        char codigo[4];
        fread(codigo, sizeof(char), 3, fp);
        codigo[3] = '\0'; //Cod na mao
    //  04 00 00 00 1E 00 00 00 |001|#Joao#Unimed#Plano de Saude
        if(strcmp(codigo, codigoRemocao) == 0){
            fseek(fp, -3 * sizeof(char), SEEK_CUR);
            fwrite("*", sizeof(char), 1, fp);
            int indexAntigo, indexRemovido = ftell(fp) - sizeof(char) - sizeof(int);
            //Pulou para trás
            rewind(fp);
            fread(&indexAntigo, sizeof(int), 1, fp);
            rewind(fp);
            fwrite(&indexRemovido, sizeof(int), 1, fp);
            fseek(fp, indexRemovido + sizeof(int) + sizeof(char), SEEK_SET);
            fwrite(&indexAntigo, sizeof(int), 1, fp);
            break;
        }
        fseek(fp, -3 * sizeof(char) - sizeof(int), 1);
        int tam_registro;
        fread(&tam_registro, sizeof(int), 1, fp);
        if(codigo[0] == '*')
            fseek(fp, tam_registro, 1);
        else
            fseek(fp, tam_registro + sizeof(int), 1);
    }
}

// Pronto
void compactar(FILE* fp){
    FILE *temp = fopen("compact.bin", "w+b"); // Cria um arquivo temporario
    int inicializador = -1; // Inicializa o primeiro registro
    fwrite(&inicializador, sizeof(int), 1, temp); // Escreve o primeiro registro no arquivo temporario
    fseek(fp, sizeof(int), SEEK_SET); // Pula o primeiro registro
    
    int tam_registro;
    char verificador;

    while (!feof(fp))
    {
        fread(&tam_registro, sizeof(int), 1, fp); //Le o tamanho do registro
        fread(&verificador, sizeof(char), 1, fp); //Le o verificador
        if(verificador == '*'){
            if(feof(fp))
                break;
            fseek(fp, tam_registro - sizeof(char), 1);
        }
        else{
            fseek(fp, - sizeof(char), 1);
            fread(buffer, tam_registro, 1, fp);
            fwrite(&tam_registro, sizeof(int), 1, temp);
            fwrite(buffer, tam_registro, 1, temp);
        }
    }
    remove(filename);
    rename("compact.bin", filename);

    fclose(fp);
    fclose(temp);
}

//Pronto
int main() {
    printf("Selecione o arquivo:");
    char filename[30];
    scanf("%s",filename);
    FILE *fp;

    if(fopen(filename, "r+b") == NULL){
        fp = fopen(filename, "w+b");
        printf("Arquivo criado com sucesso!\n"); 
        int inicializador = -1;
        fwrite(&inicializador, sizeof(int), 1, fp);
    }else{
        fp = fopen(filename, "r+b");
        printf("Arquivo aberto com sucesso!\n"); 
    }

    int choice;
    while(1){
        printf("Menu:\n1. Inserir\n2. Remover\n3. Compactar\n4. Sair\nEscolha uma opcao: ");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                Beneficiario bufferbn;
                printf("Insira o ID do beneficiario: ");
                scanf("%d", &bufferbn.ID);
                printf("Insira o Nome do beneficiario: ");
                scanf("%s", &bufferbn.Nome);
                printf("Insira o nome da Seguradora: ");
                scanf("%s", &bufferbn.Seguradora);
                printf("Insira tipo de Seguro: ");
                scanf("%s", &bufferbn.tipoSeg);
                Insere(concatBeneficiario(&bufferbn), fp);
                printf("Inserido com sucesso!\n\n\n");
                break;
            case 2:
                printf("ID a ser removido:");
                int IdRemover;
                scanf("%d", &IdRemover);
                remover(fp, IdRemover);
                printf("Removido com sucesso!\n");
                break;
            case 3:
                compactar(fp);
                printf("Compactado com sucesso!\n");
                break;
            case 4:
                printf("Encerrando o programa...\n");
                exit(0);
                break;
            default:
                printf("Opcao invalida. Tente novamente.\n");
        }
    }
}