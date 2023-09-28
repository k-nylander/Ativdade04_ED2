#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#define INSERE_TODOS 0;

typedef struct beneficiario { //____001#Joao#Unimed#Plano de Saude
    int ID;
    char Nome[50];
    char Seguradora[50];
    char tipoSeg[30];
} Beneficiario;

typedef struct indexOffset {
    int ID;
    int offset;
    struct indexOffset * prox;
} IndexOffset;

char buffer[135];
char filename[30];

char* transformaEmChar(int value) {
    char *result = malloc(sizeof(char) * 4);
    if (value < 0 || value > 999) {
        strcpy(result, "ERR");
    } else {
        snprintf(result, 4, "%03d", value);
    }
    return result;
}

int transformaEmInt(char *value) {
    if (strlen(value) != 3) {
        return -1;
    }
    return atoi(value);
}

void stringToBeneficiario(char *input, Beneficiario *result) {
    char *token = strtok(input, "#");
    result->ID = transformaEmInt(token);
    token = strtok(NULL, "#");
    strcpy(result->Nome, token);
    token = strtok(NULL, "#");
    strcpy(result->Seguradora, token);
    token = strtok(NULL, "#");
    strcpy(result->tipoSeg, token);
}

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

char* concatBeneficiario(Beneficiario *input) {
    strcpy(buffer, concat(4, transformaEmChar(input->ID), input->Nome, input->Seguradora, input->tipoSeg));
    return buffer;
}

int busca_espaco_livre_na_lista(FILE *fp, long unsigned int tamanho) {
    int posicao_inicial = ftell(fp);

    fseek(fp,0,0);

    int atpos = 0;
    fread(&atpos, sizeof(int), 1, fp);

    while (atpos != -1) {
        fseek(fp, atpos, SEEK_SET);
        long unsigned int tam_buffer;
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

int busca_espaco_livre(FILE *fp, int size) {
    int byte_offset = busca_espaco_livre_na_lista(fp, size);

    if (byte_offset != -1) {
        return byte_offset;
    }

    fseek(fp, 0, SEEK_END);
    return ftell(fp);
}

int Insere(char *string, FILE *fp) {
    unsigned char string_len = strlen(string);
    int gapsize = 0, byteindex = busca_espaco_livre_na_lista(fp, string_len + 10);
    
    if(byteindex == -1){
        fseek(fp, 0, SEEK_END);
        int byteoffset = ftell(fp);
        fwrite(&string_len, sizeof(unsigned char), 1, fp);
        fwrite(string, string_len, 1, fp);
        return byteoffset;
    }

    fseek(fp, byteindex, SEEK_SET);
    fread(&gapsize, sizeof(int), 1, fp);
    
    if(gapsize == string_len){
        fseek(fp, byteindex + sizeof(int), SEEK_SET);
        fwrite(string, string_len, 1, fp);
    }else{
        fwrite(&string_len, sizeof(unsigned char), 1, fp);
        fwrite(string, string_len, 1, fp);
        gapsize -= string_len + sizeof(unsigned char);
        fwrite(&gapsize, sizeof(unsigned char), 1, fp);
        fwrite("*", sizeof(char), 1, fp);
        int indexAntigo, indexRemovido = ftell(fp) - (2 * sizeof(char));
        //Pulou para trás
        rewind(fp);
        fread(&indexAntigo, sizeof(int), 1, fp);
        rewind(fp);
        fwrite(&indexRemovido, sizeof(int), 1, fp);
        fseek(fp, indexRemovido + (2 * sizeof(char)), SEEK_SET);
        fwrite(&indexAntigo, sizeof(int), 1, fp);
    }

    return byteindex;  
}

Beneficiario *load(FILE *input_file, int *tam_destino){ // ## Lê e retorna os inputs como um array de beneficiários ##
    rewind(input_file); // Volta para o início do arquivo

    typedef struct segurado{ // Modelo dos dados inseridos.
        char codigo_cliente[4];
        char nome_cliente[50];
        char nome_seguradora[50];
        char tipo_seguro[30];
    } Segurado;

    Segurado buffer_segurado; // Variável temporária para armazenar os dados lidos do arquivo binário

    (*tam_destino) = 0; // Inicializa o número de beneficiários como zero

    while(fread(&buffer_segurado, sizeof(Segurado), 1, input_file) != 0) // Enquanto não chegar no fim do arquivo
        (*tam_destino)++; // Incrementa o número de beneficiários

    rewind(input_file); // Volta para o início do arquivo

    Beneficiario *buffer_beneficiarios = malloc((*tam_destino) * sizeof(Beneficiario)); // Aloca o vetor com o tamanho correto

    for(int i = 0; i < (*tam_destino); i++){ // Para cada beneficiário no arquivo binário
        fread(&buffer_segurado, sizeof(Segurado), 1, input_file); // Lê os dados do arquivo binário

        // Atribui os valores lidos do arquivo binário aos campos correspondentes do beneficiário no vetor
        buffer_beneficiarios[i].ID = atoi(buffer_segurado.codigo_cliente);
        strcpy(buffer_beneficiarios[i].Nome, buffer_segurado.nome_cliente);
        strcpy(buffer_beneficiarios[i].Seguradora, buffer_segurado.nome_seguradora);
        strcpy(buffer_beneficiarios[i].tipoSeg, buffer_segurado.tipo_seguro);
    }

    return buffer_beneficiarios;
}

// void insereNaLista(int id, int offset, IndexOffset **lista){
//     IndexOffset *novo = malloc(sizeof(IndexOffset));
//     novo->ID = id;
//     novo->offset = offset;
//     novo->prox = NULL;
//     if(*lista == NULL){
//         *lista = novo;
//     }else{
//         IndexOffset *aux = *lista, *anterior = NULL;
//         while(aux->prox != NULL && aux->prox->ID < novo->ID){
//             antigo = aux;
//             aux = aux->prox;
//         }
//         aux->prox = novo;
//     }
// }

IndexOffset *insereNaLista(int id, int offset, IndexOffset *lista){
    IndexOffset *novo = malloc(sizeof(IndexOffset));
    novo->ID = id;
    novo->offset = offset;
    novo->prox = NULL;

    if(lista == NULL || lista->ID > id){
        novo->prox = lista;
        return novo;
    }
    else if(lista->ID == id){
        free(novo);
        return lista;
    }
    else {
        IndexOffset *aux = lista;
        while(aux->prox != NULL && aux->prox->ID < id){
            if(aux->prox->ID == id){
                free(novo);
                return lista;
            }
            aux = aux->prox;
        }
        novo->prox = aux->prox;
        aux->prox = novo;
        return lista;
    }
}

IndexOffset *lista_indexOffset(FILE *fp){
    fseek(fp, sizeof(int), 0); // Pula o primeiro registro

    unsigned char tamanho;
    Beneficiario buffer_beneficiario; // Variável temporária para armazenar os dados lidos do arquivo binário
    IndexOffset *lista = NULL;

    while (fread(&tamanho, sizeof(unsigned char), 1, fp) == 1 && fread(&buffer, tamanho, 1, fp) == 1){
        stringToBeneficiario(buffer, &buffer_beneficiario);
        lista = insereNaLista(buffer_beneficiario.ID, ftell(fp) - tamanho - sizeof(unsigned char), lista);
    }

    return lista;
}

int existeNaLista(int id, IndexOffset *lista){
    IndexOffset *aux = lista;
    while(aux != NULL){
        if(aux->ID == id)
            return aux->offset;
        aux = aux->prox;
    }
    return -1;
}

Beneficiario *carregaNaMemoria(FILE *fp, int *tam_destino){
    fseek(fp, sizeof(int), 0); // Pula o primeiro registro

    unsigned char tamanho;
    Beneficiario buffer_beneficiario; // Variável temporária para armazenar os dados lidos do arquivo binário
    Beneficiario *resultado = NULL;
    int i = 0;

    while (fread(&tamanho, sizeof(unsigned char), 1, fp) == 1 && fread(&buffer, tamanho, 1, fp) == 1){
        buffer[tamanho] = '\0'; // Adiciona o terminador de string no final do buffer
        stringToBeneficiario(buffer, &buffer_beneficiario);
        resultado = realloc(resultado, sizeof(Beneficiario) * (i + 1));
        resultado[i++] = buffer_beneficiario;
    }

    (*tam_destino) = i;
    return resultado;
}

typedef struct indice_secundario{
    char nome_da_seguradora[50];
    int offset;
}INDICE_SECUNDARIO;

INDICE_SECUNDARIO *criar_indice_secundario(Beneficiario *fonte_de_dados, int tamanho, FILE * lista_de_indices_secundarios){
//Carrega
//Ordenar a lista de indices secundários pelo nome das seguradoras.
//Montar lista de indices secundários.
//Pré Atualizar os arquivos.
//E retornar

    INDICE_SECUNDARIO *indice_secundario = malloc(sizeof(INDICE_SECUNDARIO) * tamanho);
    for(int i = 0; i < tamanho; i++){
        strcpy(indice_secundario[i].nome_da_seguradora,"");
        indice_secundario[i].offset = -1;
    }
    
    for(int i = 0; i < tamanho; i++){
        bool existe = false;
        int offset = -1;

        for (int j = 0; j < tamanho; j++){
            if(strlen(indice_secundario[i].nome_da_seguradora) == 0) break;

            if(strcmp(indice_secundario[i].nome_da_seguradora, fonte_de_dados[i].Seguradora) == 0){
                existe = true;
                offset = indice_secundario[i].offset;
                break;
            }
        }

        fseek(lista_de_indices_secundarios, 0, SEEK_END);
        int offset_atual = ftell(lista_de_indices_secundarios);

        fwrite(transformaEmChar(fonte_de_dados[i].ID), sizeof(char), 3, lista_de_indices_secundarios);
        fwrite(&offset_atual, sizeof(int), 1, lista_de_indices_secundarios);
        fflush(lista_de_indices_secundarios);

        if(existe){
            for(int k = 0; k < tamanho; k++){
                if(strcmp(indice_secundario[k].nome_da_seguradora, fonte_de_dados[i].Seguradora) == 0){
                    indice_secundario[k].offset = offset_atual;
                    break;
                }
            }
            continue;
        }

        int pos = -1;
        for(int h = 0; h < tamanho; h++){
            if(strcmp(fonte_de_dados[i].Seguradora, indice_secundario[h].nome_da_seguradora) < 0){
                pos = h;
                break;
            }
        }

        INDICE_SECUNDARIO aux = indice_secundario[pos];
        for(int l = pos; l < tamanho; l++){
            INDICE_SECUNDARIO aux2 = indice_secundario[l + 1];
            indice_secundario[l + 1] = aux;
            aux = aux2;
        }

        strcpy(indice_secundario[pos].nome_da_seguradora, fonte_de_dados[i].Seguradora);
        indice_secundario[pos].offset = offset_atual;
    }
    return indice_secundario;
}


void insere_no_arquivo_IS(FILE *fp, INDICE_SECUNDARIO *indice_secundario, int tamanho){
    FILE *fp2 = fopen("temp", "w+b");
    for(int i = 0; i < tamanho; i++){
        if(strlen(indice_secundario[i].nome_da_seguradora) == 0) break;

        unsigned char tamanho = strlen(indice_secundario[i].nome_da_seguradora);

        fwrite(&indice_secundario[i].offset, sizeof(int), 1, fp2);
        fwrite(&tamanho, sizeof(unsigned char), 1, fp2);        
        fwrite(indice_secundario[i].nome_da_seguradora, sizeof(char), 50, fp2);
    }
    remove("chaves_secundarias_LK.bin");
    rename("temp", "chaves_secundarias_LK.bin");
    fclose(fp2);
    fp2 = fopen("chaves_secundarias_LK.bin", "r+b");
}

/* 💀💀 Código antigo 💀💀

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
*/

int main() {

    // printf("Arquivo dos dados:");
    // scanf("%s",filename);

    char filename1[30] = "dados_principal_LK.bin";
    FILE *data_file;
    if((data_file = fopen(filename1, "r+b")) == NULL){
        data_file = fopen(filename1, "w+b");
        printf("Arquivo criado com sucesso!\n"); 
        int inicializador = -1;
        fwrite(&inicializador, sizeof(int), 1, data_file);
    }else{
        printf("Arquivo aberto com sucesso!\n"); 
    }

    FILE *input_file;
    char input_filename[30] = "insere.bin";
    
    // printf("Arquivo de inputs(Para pular digite 0):");
    // scanf("%s",input_filename);

    IndexOffset *lista = NULL;
    Beneficiario *inputs = NULL;
    int tamanho;

    if(strcmp(input_filename, "0") != 0 && (input_file = fopen(input_filename, "r+b")) != NULL){

        inputs = load(input_file, &tamanho); // retorna Beneficiário inputs[tamanho] = {{ID1, Nome1, Seguradora1, tipoSeg1}, ... {IDn, Nomen, Seguradoran, tipoSegn}};
        lista = lista_indexOffset(data_file); // Lista no momento  

        for(int i = 0; i < tamanho; i++){
            if(existeNaLista(inputs[i].ID , lista) != -1)
                continue;
            Insere(concatBeneficiario(&inputs[i]), data_file);
            printf("Inserido com sucesso!\n\n\n");
            insereNaLista(inputs[i].ID, ftell(data_file) - sizeof(concatBeneficiario(&inputs[i])) - sizeof(unsigned char), &lista);
        }
    }


    FILE * index_file = fopen("indexOffset_buscaP_LK.bin", "w+b");
    IndexOffset *aux = lista;
    while(aux != NULL){            
        printf("%d %d\n", aux->ID, aux->offset);
        fwrite(&aux->ID, sizeof(int), 1, index_file);
        fwrite(&aux->offset, sizeof(int), 1, index_file);
        aux = aux->prox;
    }

    FILE * input_busca_file = fopen("busca_p.bin", "r+b");

    char array_de_indices[20][4]; //maximo de 20 indices
    int count_array_de_indices = fread(array_de_indices, 4 * sizeof(char), 20, input_busca_file);

    fclose(input_busca_file);
    
    
    for (int i = 0; i < count_array_de_indices; i++){
        char *id_buscado = &array_de_indices[i][0];
        int numero = transformaEmInt(id_buscado);
        int offset_buscado = existeNaLista(numero, lista);

        Beneficiario buffer_beneficiario;
        fseek(data_file, offset_buscado, SEEK_SET);
        unsigned char tamanho;
        fread(&tamanho, sizeof(unsigned char), 1, data_file);


        char strinbuff[139];
        fread(&strinbuff, tamanho, 1, data_file);
        stringToBeneficiario(strinbuff, &buffer_beneficiario);

        printf("\n\nOFFSET DO ID %s: %d\n", &array_de_indices[i][0] , offset_buscado);
        printf("ID: %d\nNome: %s\nSeguradora: %s\nTipo: %s\n----------------\n", buffer_beneficiario.ID, buffer_beneficiario.Nome, buffer_beneficiario.Seguradora, buffer_beneficiario.tipoSeg);
    }



    Beneficiario *dados_principal = carregaNaMemoria(data_file, &tamanho);

    for(int j = 0; j < tamanho; j++){
        printf("ID: %d\nNome: %s\nSeguradora: %s\nTipo: %s\n----------------\n", dados_principal[j].ID, dados_principal[j].Nome, dados_principal[j].Seguradora, dados_principal[j].tipoSeg);
    }

    FILE *chaves_secundarias = fopen("chaves_secundarias_LK.bin", "w+b"), *chave_secundarias_lista = fopen("chaves_secundarias_lista_LK.bin", "w+b");
    INDICE_SECUNDARIO *indice_secundario = criar_indice_secundario(dados_principal, tamanho, chave_secundarias_lista);
    insere_no_arquivo_IS(chaves_secundarias, indice_secundario, tamanho);

    for(int i = 0; i < tamanho; i++){
        printf("Nome da seguradora: %s\nOffset: %d\n", indice_secundario[i].nome_da_seguradora, indice_secundario[i].offset);
    }

    
    /* ⬇️⬇️⬇️ Código antigo ⬇️⬇️⬇️
    printf("Selecione o arquivo:");
    char filename[30];
    scanf("%s",filename);
    FILE *fp;

    if((fp = fopen(filename, "r+b")) == NULL){
        fp = fopen(filename, "w+b");
        printf("Arquivo criado com sucesso!\n"); 
        int inicializador = -1;
        fwrite(&inicializador, sizeof(int), 1, fp);
    }else{
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
    */


    fclose(input_file);
    fclose(data_file); 

}